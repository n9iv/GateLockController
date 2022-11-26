// CHANGE LOG:
// Added watchdog
// fixed array out of range warning
// added Internal board led
// Added 5 new keys
#define APPVERSION 2.04 // update every push to git 


/* Connections:
 * Yellow   = Digital4 to RelayIn1
 * Green    = Digital2 to RF control
 * Red      = 5v to RF 5v to Relay 5V
 * Blue     = GND to RF GND to Relay GND
 * 
 * Brown    = Relay Middle to 
 */

// turn DEBUGGING  to true to print keys - set monitor to 115K
bool DEBUGGING = false;

#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <Adafruit_SleepyDog.h>

SoftwareSerial RFID(2,3);

#define RFID_SIZE 14 // in bytes
#define GATE_CONTROL_PIN 4

struct RfidCode {
  char v[RFID_SIZE];
};


struct RfidCode expected[] = { 
                              {57,49,66,67,49,66,3,2,49,48,48,48,50,54}, // 0002527676
                              {69,52,69,68,56,52,3,2,48,68,48,48,56,48}, // 0008447213 // Revital
                              {49,53,48,56,53,56,3,2,48,70,48,48,52,65}, // 0004855048
                              {48,49,54,56,53,70,3,2,49,48,48,48,50,54}, // 0002490728
                              {48,68,70,67,67,55,3,2,49,48,48,48,50,54}, // 0002493948
                              {68,67,66,65,69,65,3,2,48,68,48,48,56,49}, // 0008510650
                              {65,48,57,56,55,68,3,2,48,70,48,48,52,65}, // 0004890776 // sharon
                              {52,49,49,52,54,51,3,2,49,48,48,48,50,54}, // 0002507028
                              {65,69,49,54,51,52,3,2,48,68,48,48,56,49}, // 0008498710
                              {51,52,51,51,51,49,3,2,49,48,48,48,50,54}, // 0002503731
                              {49,69,48,48,65,66,51,53,70,49,55,49,3,2}, // S1
                              {65,48,48,48,67,68,65,52,49,56,68,3,2,49}, // S2
                              {65,48,48,48,68,54,67,69,48,57,66,3,2,49}, // S3
                              {65,48,48,48,68,55,68,54,69,48,52,3,2,49}, // S4
                              {65,48,48,48,69,48,68,53,49,52,56,3,2,49}, // R1  
                              {48,65,65,55,65,52,69,56,48,3,2,49,69,48}, // K1 (under U5 Key with chip
                              {48,48,65,65,68,56,55,53,49,57,3,2,49,69}, // K2
                              {65,69,55,56,50,54,3,2,56,53,48,48,55,53}  // Ad's key
                             };

void printCode(struct RfidCode& code) {
  if(DEBUGGING==false)
    return; 
  
  Serial.print("{");
  for(int i=0; i<RFID_SIZE-1; i++) {
    Serial.print(code.v[i], DEC);
    Serial.print(",");
  }
  Serial.print(code.v[RFID_SIZE-1], DEC);
  Serial.print("}");
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // for debug
  RFID.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  
  pinMode(GATE_CONTROL_PIN, OUTPUT);  


  // Setup Watchdog
  int countdownMS = Watchdog.enable(8000);
  Serial.print("Enabled the watchdog with max countdown of ");
  Serial.print(countdownMS, DEC);
  Serial.println(" milliseconds!");
  Serial.println();
  
  Serial.print("Setup completed. Controller ver ");
  Serial.print(APPVERSION);
  Serial.print(" ready!\n");
}

// Reutnrs true if a and b are matched by byte comparison. 
bool DoesCodesMatch(struct RfidCode& a, struct RfidCode& b) {
  int numberOfMatches = 0;
  for(int i=0; i<RFID_SIZE;i++) {
    if(a.v[i]==b.v[i])
      numberOfMatches++;
  }
  return numberOfMatches==RFID_SIZE;
}
/*
// Returns true if the specified value is one of the codes in the EEPROM
bool IsValidCode(struct RfidCode& codeFromReader) {
  struct RfidCode codeFromEEPROM;
  for(int addr=0; addr<EEPROM.length(); addr+=RFID_SIZE) {
    EEPROM.get(addr, codeFromEEPROM);
    if(DoesCodesMatch(codeFromReader, codeFromEEPROM)) {
      Serial.print("Found valid code\n");
      return true;  
    }
  }
  return false;
}
*/
// Returns true if the specified code is the controlling code (used for updating the list of valid codes)
bool IsControlCode(struct RfidCode& codeFromReader) {
  int numOfTags = sizeof(expected)/sizeof(expected[0]);
  for(int i=0; i<numOfTags; i++) {
    if(DoesCodesMatch(codeFromReader, expected[i])==true) {
      Serial.print("\nIdentified Tag "); Serial.print(i);
      Serial.print("\n");
      return true;
    }
  }
  return false;
}


void openGate() {  
  Serial.print("Opening Gate for 2.5 sec...\n");

  digitalWrite(GATE_CONTROL_PIN, HIGH);
  delay(2500);
  digitalWrite(GATE_CONTROL_PIN, LOW);

  Serial.print("Closed Gate\n");
}

int shiftRight(RfidCode* code) {
  int first = code->v[0];
  for(int i=0; i<RFID_SIZE-1; i++)
    code->v[i]=code->v[i+1];
  return first;
}

struct RfidCode r;
int c = 0;
int ledState = HIGH;
/*
 * The loop function reads an RFID code (14 bytes) and then handles it according to its internal state machine.
 * The state machine has to following states:
*   Start = no known card was read. in this stage, the code is compared to one of the controlling cards and if its not, the program checks if its a valid code and opens the door
*   Register = The previous card was a Register control card which indicates that the next card will be registered as a valid card
*   Unregister = The previous card was an Unregister control card which indicates that the next valid card will be unregistered
*   
*   in the Start state every code should be read at least 3 consecuteve times to be considered as read 
*   In the Register and Unregister states, every code should be read at least 10 times
 */
void loop() {
  if(RFID.available() >0)
  {
    int i = RFID.read();
    r.v[c] = i;
    c++;
    if(c==RFID_SIZE) {
      if(DEBUGGING)
        printCode(r);
      // TODO: support state machine. add robustness by reading the same code several times for it to be considered as "read"
      if(IsControlCode(r)) {
        openGate();
        RFID.flush(); // Flush all serial bytes read, otherwise, the controller will keep openning the door according to correcrt RFID Codes that remains in his buffer
        c=0;
      }
      else {
        int discardedChar = shiftRight(&r);
        if(DEBUGGING) {
          Serial.print(discardedChar, DEC);
          Serial.print(" discarded\n");
        }
        c--;
      }

      delay(5);
    }    
  }
  /*
  delay(1);
  if(ledState==HIGH)
      ledState = LOW;
  else
    ledState = HIGH;
  digitalWrite(LED_BUILTIN, ledState);
*/
//  Serial.print(" Watchdog reset\n");
  Watchdog.reset();
}
