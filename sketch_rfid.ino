#include <SoftwareSerial.h>

SoftwareSerial RFID(2,3);
#define RFID_SIZE 14
int c = 0;
char currentValue[RFID_SIZE];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  RFID.begin(9600);
}

// Returns true if the specified value is one of the codes in the EEPROM
bool IsValidCode(char* v) {
  
}

void checkCode(char* v) {
  char expected[RFID_SIZE] = {57,49,66,67,49,66,3,2,49,48,48,48,50,54};
  int numberOfMatches = 0;
  for(int i=0; i<=RFID_SIZE;i++)
    if(v[i]==expected[i])
      numberOfMatches++;

  if(numberOfMatches<RFID_SIZE) {
    Serial.print("Number of matches: ");
    Serial.print(numberOfMatches);
  }
  else
    Serial.print("Full Match!!!!");
}

void loop() {
  if(RFID.available() >0)
  {
    int i = RFID.read();
    currentValue[c] = i;
    c++;
    Serial.print(i, DEC);
    Serial.print(" ");
    if(c==RFID_SIZE) {
      checkCode(currentValue);
      Serial.print("\n");
      c=0;
      delay(150);
    }
    
  }
}
