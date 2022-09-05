// Array of out pins for the Arduino receiving data from sensors 1-16
int pins[16] = {2,3,4,5,6,7,8,9,18,17,16,15,14,12,11,10};
// The main data structure updated by the sensors: two arrays of 16 byte values between 0 and 254 
// (one for each sensor) corresponding to the more slow (integrated) response -> ocup
// and to the more fast (differential) response -> mov
byte ocup[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
byte mov[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//byte 255 is reserved as a closing byte
byte closing_byte=255;
long tlast=0;
//an ad-hoc array for the time between updates for the mov decay depending on the number of activated sensors 
long tupdate[17]={100,60,40,30,20,15,12,10,9,8,7,6,5,4,3,2,1};
int npins = 16;
int nocup = 0;

void setup()
{
  Serial.begin(115200);
  for (int i=0; i<npins; i++)
    pinMode(pins[i],INPUT);
  // actual initial state  
  for (int i=0; i<npins; i++) {
    mov[i]=0;
    if(digitalRead(pins[i])==HIGH) {
      ocup[i]=1;  
    }
    else {
      ocup[i]=0;
    }
  }  
}

void loop() {
 for (int i=0; i<npins; i++) {
    // 1. SENSORS STATE UPDATE
    if(ocup[i]==0 && digitalRead(pins[i])==HIGH) {
      // ith sensor turns on: sets ocup[i] to 1 and add 1 to mov[i] 
      ocup[i]=1;
      mov[i]+=1;
      if(mov[i]>254) {
        // maximum value allowed for mov
        mov[i]=254;
      }
    }
    if(ocup[i]==1 && digitalRead(pins[i])==LOW) {
      // ith sensor turns off: sets ocup[i] to 0 and add 1 to mov[i]
      // mov keeps track of the changes of the state, without regard to the sign
      ocup[i]=0;
      mov[i]+=1;
      if(mov[i]>254) {
        mov[i]=254;
      }
      //write_pars();
    }    
  }
  // 2. MOV DECAY UPDATE
  nocup = 0;
  for (int i=0; i<npins; i++){
    nocup += ocup[i]; 
  }
  // the mov array decays by -1 with an update interval that depends on the number of
  // activated sensors (nocup). The more activated the faster the decay 
  if (tlast>tupdate[nocup]) {
    for (int i=0; i<npins; i++) {
      if (mov[i]>0) mov[i]-=1;
    }
    tlast = 0;
    //write_pars();
  }
  // time update, output to serial and loop delay 
  tlast++; 
  write_pars();           
  delay(500);
}

void write_pars() {
  for (int i=0; i<npins; i++){
    Serial.write(ocup[i]);
    //Serial.print(ocup[i]);Serial.print(","); 
  }
  //Serial.print("            ");
  for (int i=0; i<npins; i++){
    Serial.write(mov[i]);
    //Serial.print(mov[i]);Serial.print(",");
  }
  // closing byte
  Serial.write(closing_byte);
  //Serial.println();
}
