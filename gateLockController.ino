#include <SoftwareSerial.h>
#include <EEPROM.h>

SoftwareSerial RFID(2,3);

#define RFID_SIZE 14

struct RfidCode {
  char v[RFID_SIZE];
};
void printCode(struct RfidCode& code) {
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
  Serial.begin(9600);
  RFID.begin(9600);
}

// Reutnrs true if a and b are matched by byte comparison. 
bool DoesCodesMatch(struct RfidCode& a, struct RfidCode& b) {
  int numberOfMatches = 0;
  for(int i=0; i<=RFID_SIZE;i++) {
    if(a.v[i]==b.v[i])
      numberOfMatches++;
  }

  if(numberOfMatches<RFID_SIZE) {
    Serial.print("Number of matches: ");
    Serial.print(numberOfMatches);
  }
  else
    Serial.print("Full Match!!!!");

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
  struct RfidCode expected = {57,49,66,67,49,66,3,2,49,48,48,48,50,54};
  return DoesCodesMatch(codeFromReader, expected);
}


struct RfidCode r;
int c = 0;

/*
 * The loop function reads an RFID code (14 bytes) and then handles it according to its internal state machine.
 * The state machine has to following states:
*   start = no known card was read. in this stage, the code is compared to one of the controlling cards and if its not, the program checks if its a valid code and opens the door
*   Register = The previous card was a Register control card which indicates that the next card will be registered as a valid card
*   Unregister = The previous card was an Unregister control card which indicates that the next valid card will be unregistered
 */
void loop() {
  if(RFID.available() >0)
  {
    int i = RFID.read();
    r.v[c] = i;
    c++;
    if(c==RFID_SIZE) {
      printCode(r);
      if(IsControlCode(r))
        Serial.print(" Found Controlling code, replace with new RFID Card" );
        
      Serial.print("\n");
      c=0;
      delay(150);
    }    
  }
}
