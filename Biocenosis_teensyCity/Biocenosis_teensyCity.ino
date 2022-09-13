#include "PCA9685.h"
#include "FastLED.h"
// Number of Channels and Modules
#define NCHAN 16
#define NMOD 10
// I2C Channels (Buildings)
TwoWire *wi0;
TwoWire *wi1;

// First Bus
PCA9685 ledArray0(0x40);
PCA9685 ledArray1(0x41);
PCA9685 ledArray2(0x42);
PCA9685 ledArray3(0x43);
PCA9685 ledArray4(0x44);

// Second Bus
PCA9685 ledArray5(0x40);
PCA9685 ledArray6(0x41);
PCA9685 ledArray7(0x42);
PCA9685 ledArray8(0x43);
PCA9685 ledArray9(0x44);

PCA9685 ledArray10(0x45); // zona industrial

//ARRAY of LED controllers
PCA9685 LEDARR[10]={ledArray0,ledArray1,ledArray2,ledArray3,ledArray4,ledArray5,ledArray6,ledArray7,ledArray8,ledArray9};
//Array of Train channels 
//ntracks channels for Tracks and Stations V1 S1 V2 V3 V4 V5 V6 V7 S2 V8 V9 S3 V10 V11 V12
int ntrack[15] = {123,124,125,126,127,48,49,50,51,52,53,156,47,54,147};
//nbridge channels for Tracks and Bridge V1 S1 V2 V3 V4 V5 V6 V7 S2 V13 V14
int nbridge[11] = {123,124,125,126,127,48,49,50,51,56,57};
//Train Stops
int nstop[3] = {123,51,156};

// MONITORING and SERIAL Communication
boolean Monitor_values = false; // print values to serial
boolean Monitor_train = false; // print train state to serial
boolean Monitor_parameters = false; // print parameters values to serial
boolean Monitor_message = false; // print Arduino serial message from sensors to serial
boolean debug = false; 
int argv=0;
int astep;
long i2s_speed = 100;
int test_delay = 500;

// COMMANDS AND MESSAGE
String command = String(4);
boolean commandComplete = false;
int NBYTES=32;
byte message[32];
//byte message2[2];
boolean messageComplete = false;
boolean messageComplete2 = false;
boolean Blackout;
int messN = 0;
//int messN2 = 0;
unsigned long previousMillis = 0; 

// COUPLED MAP PARAMETERS
int THRESH1=888;
int THRESH2=573;
float mu0=1.02;
float mu1=1.01;
float mu2=0.98;
float b = 0.035; 

// SIMPLEX NOISE PARAMETERS
int scale = 220; 
int time_step = 3; 
float slope = 1;
uint8_t mean = 128;
unsigned long TIME = 0;

// TIME SCALE
unsigned int time_interval = 400;
unsigned long COUNT = 0;

//TRAIN SPEED  PSEUDOPERIOD in seconds
float tspeed = 0.3;
int tperiod = 128; //mean time interval between trains
boolean istrain=false;
float lastrain=0;
int track=0;
int bridge=0;
int TRAINMIN = 100; //minimum dim value of train
int STOPMIN = 600; //minimum dim value of station
int YMIN = 30; // minimum dim intensity for target Can be lowered during blackouts

//SLUGISHNESS
int SLUG1 = 8;
int SLUG2 = 15;

//SENSITIVITY TO PUBLIC
float sensitivity_ocup = 2.0;
float sensitivity_mov = 2.0;
float sensitivity_inverse = 0.0;

const int N=160;  // Total number of channels NCHAN*NMOD
const int NumN=5; // esto tiene que ser consistente con como fue armado nn.h cantidad de vecinos
float x[N]; //state vector 0-999 linear > log amp 
float xn;
int y[N]; // Target LED value 0-4095
int z[N]; // Actual LED value 0-4095

// LOAD DATA FROM .h to PROGMEM
const int cal [] PROGMEM = {
#include "cal.h"
}; //calibration

const int ymin0 [] PROGMEM = {
#include "ymin.h"
}; //calibration

int ymin[NCHAN*NMOD];

const int nn [] PROGMEM = {
#include "nn.h"
}; //nearest neighbours network

const float coord [] PROGMEM = {
#include "coord.h"
}; //XY coordinates

const float amp [] PROGMEM = {
#include "amp.h"
}; //Log amplitude 0-1                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           

const byte sw [] PROGMEM = {
#include "sw.h"
}; //Sensor weight


void setup() {
  // USB Serial
  Serial.begin(115200);
  // Arduino Nano Serial
  Serial1.begin(115200);
  //Serial.println("Starting Wire0");
  //ESP32 Serial
  Serial2.begin(115200);
  
  Wire.begin();
  // Notar que invertimos
  wi1 = &Wire;
  //Serial.println("Starting Wire1");
  Wire1.begin();
  wi0 = &Wire1;
  delay(1500);
  ledArray0.set_wire(wi0);
  ledArray1.set_wire(wi0);
  ledArray2.set_wire(wi0);
  ledArray3.set_wire(wi0);
  ledArray4.set_wire(wi0);
  ledArray5.set_wire(wi1);
  ledArray6.set_wire(wi1);
  ledArray7.set_wire(wi1);
  ledArray8.set_wire(wi1);
  ledArray9.set_wire(wi1);
  ledArray10.set_wire(wi1);
  LEDARR[0] = ledArray0;
  LEDARR[1] = ledArray1;
  LEDARR[2] = ledArray2;
  LEDARR[3] = ledArray3;
  LEDARR[4] = ledArray4;
  LEDARR[5] = ledArray5;
  LEDARR[6] = ledArray6;
  LEDARR[7] = ledArray7;
  LEDARR[8] = ledArray8;
  LEDARR[9] = ledArray9;
  for (int imod=0; imod<NMOD; imod++) {
    is_connected(LEDARR[imod],imod);
  }
  is_connected(ledArray10,10);
  delay(3000);
  //Serial.println("ALL ZERO");
  all_value(0);
  delay(1000);
  state_random();
  // zona industrial
  for (int i=0; i<16; i++) {
    ledArray10.setPWM(i, 0, 40);
  }  
  for (int i=0; i<160; i++) {
    ymin[i]=ymin0[i];
  }  
  //Serial.println("READY to ROCK");
}

void loop() {
  
  // 1. USB COMMAND UPDATE
  //if(Serial.available() > 0) {
  //  getIncomingChars();
  //}
  //if (commandComplete == true) {
  //  if (debug) {
  //    Serial.println("COMMAND COMPLETE");
  //    Serial.println(command);   
  //  }
  //  if (command.length()>1) {
  //    processCommand();
  //  }
  //  command = "";
  //} 

  // 2. ARDUINO MESSAGE UPDATE
  if(Serial1.available() > 0) {
    getIncomingBytes();
  }
  if (messageComplete == true) {
    processMessage();
  }  
  if(Serial2.available() > 0) {
    getIncomingBytes2();
  }
  if (messageComplete2 == true) {
    processMessage2();
  }   
  unsigned long currentMillis = millis();
 
  // 3. UPDATE
  
  if (currentMillis - previousMillis >= time_interval) {
    //Serial.print(COUNT);
    //Serial.print(",");
    // 3.1 UPDATE BUILDING TARGETS
    previousMillis = currentMillis;  
    updateBuildings();
    COUNT++;
    // 3.2 UPDATE TRAIN 
    TIME += time_step;
    lastrain += tspeed;
    trainStep(); 
    // 3.3 WRITE LED AND DELAY
    write_led();
  }
}

void write_led(){
  for(int imod = 0; imod < NMOD; imod++) {
      for(int ichan = 0; ichan < NCHAN; ichan++) {
        int n = NCHAN*imod+ichan;
        // here z follow target y with assymetrical slugishness 5 and 20 are ad-hoc values
        if (z[n]>=y[n]) {
          z[n] -= (int)(z[n]-y[n])/SLUG1;
        }
        else {
          z[n] -= (int)(z[n]-y[n])/SLUG2;
        } 
        if (z[n]<0) z[n]=0;
        if (z[n]>4095) z[n]=4095;
        LEDARR[imod].setPWM(ichan, 0, z[n]); 
      }
  }
}

void trainStep() {
  if (istrain==false && lastrain>tperiod) { 
    //Serial.println("TREN");
    long rand = random(1, 5);  // returns a value 1, 2, 3
    switch (rand) {
      case 1: 
        track = 1;
        break;
      case 2: 
        track = -1;
        break;
      case 3: 
        bridge = 1;
        break;
      case 4: 
        bridge = -1;
        break;                 
    }
    istrain=true;
    lastrain=0.0;    
  }
  for (int n=0;n<15;n++) y[ntrack[n]]=TRAINMIN;
  for (int n=0;n<11;n++) y[nbridge[n]]=TRAINMIN;
  for (int n=0;n<3;n++) y[nstop[n]]=STOPMIN;
  if (istrain) {
    int n=(int)lastrain; 
    if (track>0 && n<15) {
      y[ntrack[n]] = 4*4095;
    }
    if (track<0 && n<15) {
      y[ntrack[14-n]] = 4*4095;
    }
    if (bridge>0 && n<11) {
      y[nbridge[n]] = 4*4095;
    }
    if (bridge<0 && n<11) {
      y[nbridge[10-n]] = 4*4095;
    }
    if (lastrain>15) {
      istrain=false;
      bridge=0;
      track=0;
    }
  }
  if (Monitor_train) {
    for (int n=0; n<15; n++) {
      Serial.print(z[ntrack[n]]);
      Serial.print(",");
    }
    Serial.println(); 
    for (int n=0; n<11; n++) {
      Serial.print(z[nbridge[n]]);
      Serial.print(",");
    }
    Serial.println(); 
  }
}

void updateBuildings() {
  
  // MANUAL BLACKOUT!! -> change!!
  if(Blackout) {
    //inicio 
    if (COUNT>3000) {
      for (int n=0;n<N;n++) {
        x[n]=0.01;
        ymin[n]=5;
      }  
      COUNT = 0;
    }
    if (ymin[1]==5 && COUNT>100) {
      for (int n=0;n<N;n++) { 
        ymin[n] = ymin0[n];
        x[n]+=10.0;
        z[n]=y[n];
      }
      Blackout = false;
    }  
  }
 
  // MAIN LOOP OVER N CHANNELS
  // TODO this also sets the train channels that are overwritten by trainStep  
  for (int n=0;n<N;n++) {
    // 1. COUPLED MAP STEP
    if (x[n]<THRESH2) {
      xn = mu1*x[n]+0.5;
    }
    else if (x[n]<THRESH1) {
      xn = mu0*x[n];
    }
    else {
        float suma=0;
        for (int i=1;i<NumN;i++) {
          int idx=nn[n*NumN+i];
          suma += b*x[idx];
        }
      xn = mu2*x[n]+suma;
    }
    if (xn>999.0) {
      xn = 0.1;
    }
    if (xn<0.1) {
      xn = 0.1;
    }
    x[n]=xn;
    
    // 2. SIMPLEX NOISE
    uint8_t temp = inoise8(coord[n*2]*scale,coord[n*2+1]*scale,0.1*TIME);
    float sn = sigmoid(temp,slope,mean);
    astep = (int) sn*x[n];
    if (astep>999) {
      astep=999;
    }
    if (Monitor_values) {
      Serial.print(astep);
      Serial.print(",");
      //Serial.print(y[n]);
    }  
    // 3. CONVERT state x (modified by inoise > astep) into PWM target y 
    // amp is log scale. 15 is the LED strip PWM threshold
    y[n] = (int) (amp[astep]*cal[n]) + ymin[n];
    if (y[n]>4095) {
     y[n]=4095;
    }
  }
  if (Monitor_values) {
    Serial.println();   
  }
  if (Monitor_parameters) {
    Serial.print(mu0); 
    Serial.print(",");
    Serial.print(mu1); 
    Serial.print(",");
    Serial.print(mu2); 
    Serial.print(",");
    Serial.print(b); 
    Serial.print(",");
    Serial.print(scale); 
    Serial.print(",");
    Serial.print(time_step); 
    Serial.print(",");
    Serial.print(slope); 
    Serial.print(",");
    Serial.print(time_interval); 
    Serial.println();
  }
}  

void getIncomingChars() {
  char inChar = Serial.read();
  if(inChar == 10 || inChar == 13){
    commandComplete = true;
  } else {
    command += inChar;
  }
}

void getIncomingBytes() {
  byte inByte = Serial1.read();
  if(inByte == 255 || messN > NBYTES){
    messageComplete = true;
    if (Monitor_message) {
      for(int i=0; i<8; i++) {
        Serial.print(message[i]);
      }
      Serial.print(":");
      for(int i=8; i<16; i++) {
        Serial.print(message[i]);
      }
      Serial.print(" | ");
      for(int i=16; i<32; i++) {
        Serial.print(message[i]);
        Serial.print(" ");
      }
      Serial.println("");     
    }
    messN = 0;
  } else {
    message[messN] = inByte;
    messN++;
  }
}

void getIncomingBytes2() {
  byte inByte = Serial2.read();
  
    messageComplete2 = true;
    if (1) {
      Serial.print("BLACKOUT!!!!!! ");
      
        Serial.print(inByte);

      Serial.println();     
    }
    //messN2 = 0;
  //} else {
    //message2[messN2] = inByte;
    //messN2++;
  //}
}

void processMessage() {
  for (int n=0;n<N;n++) {
    for (int i=1;i<16;i++) {
      x[n] += message[i]*sw[n*16+i]*sensitivity_ocup;
      x[n] += message[16+i]*sw[n*16+i]*sensitivity_mov;
      x[n] += message[16+i]/sw[n*16+i]*sensitivity_inverse;
    }
    if (x[n]>999.0) {
          x[n] = 0.0;
    }
    if (x[n]<0.1) {
          x[n] = 0.1;
    }
  }  
  messageComplete = false;
}

void processMessage2() {
  messageComplete2 = false;
}

void processCommand() {
  // Commands 
  // command + int ;
  // t set THRESH1 parameter
  // T set THRESH2 parameter
  // m set mu0 (/500)
  // n set mu1 (/500)
  // M set mu2 (/500)
  // b set b (-500)/10000
  // s set scale for simplex noise
  // p set time_step for simplex noise
  // q set sigmoid slope
  // r set time_interval global
  
  int nchars = command.length();
  String arg = command.substring(1,nchars);
  //Serial.println(command);
  // Evalua el argumento numerico
  if (isNumeric(arg)) {
    argv = arg.toInt();
  }  
  else { 
    argv = 0;
  }

  switch (command.charAt(0)) {
    case 't':
      THRESH1=argv;
      break;
    case 'T':
      THRESH2=argv;
      break;
    case 'm':
      mu0 = (float) argv/500.0;
      break;
    case 'n':
      mu1 = (float) argv/500.0;
      break;
    case 'M':
      mu2 = (float) argv/500.0;
      break;
    case 'b':
      b = (float)(argv-500)/10000.0;
      break;
    case 's':
      scale = argv;
      break;
    case 'p':
      time_step = argv;
      break;
    case 'q':
      slope = (float)argv/10.0;
      break;
    case 'r':
      time_interval = (int) argv;
      break;                  
  }
  command = "";
  commandComplete = false;  
}

void is_connected(PCA9685 ledArray, int n) {
  Serial.print("Starting Ledarray ");
  Serial.print(n);
  bool resp1 = ledArray.begin();
  Serial.print(" | Returned ");
  Serial.print(resp1);
  Serial.print(" | Is Connected ? ");
  bool resp2 = ledArray.isConnected();
  Serial.println(resp2);
}

boolean isNumeric(String arg){
  boolean ret = true;
  int nchars = arg.length();
  for (int n=0; n < nchars; n++) {  
    char character = arg.charAt(n);
    if(character < 48 || character > 75){
      ret = false;
    }
  }  
  return ret;
}

void all_value(int val) {
  for (int imod=0; imod<NMOD; imod++) {
    for (int ichan=0; ichan<NCHAN; ichan++) {
      LEDARR[imod].setPWM(ichan, 0, val); 
    }  
  }
}

void state_random(){
  for (int n=0;n<N;n++) {
    x[n] = random(1000);
    astep = (int) x[n];
    y[n] = (int) (amp[astep]*cal[n]) + 30;
    z[n] = y[n];
  }
}

float sigmoid(uint8_t x, float slope, uint8_t mean)
{
  float temp = float((x-mean)/slope);
  return 1.0+temp/(1+abs(temp)); // between 0.0 and 2.0
}
