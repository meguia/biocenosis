#include "PCA9685.h"
#define NCHAN 16
#define PWM_MAX 4095


TwoWire *wi0;
TwoWire *wi1;


// First Channel
PCA9685 ledArray0(0x40);
PCA9685 ledArray1(0x41);
PCA9685 ledArray2(0x42);
PCA9685 ledArray3(0x43);
PCA9685 ledArray4(0x44);

// Second Channel
PCA9685 ledArray5(0x40);
PCA9685 ledArray6(0x41);
PCA9685 ledArray7(0x42);
PCA9685 ledArray8(0x43);
PCA9685 ledArray9(0x44);

PCA9685 LEDARR[10]={ledArray0,ledArray1,ledArray2,ledArray3,ledArray4,ledArray5,ledArray6,ledArray7,ledArray8,ledArray9};



int nmodules = 10;
int module = 0;
int chan = 0;
int argv=0;
long i2s_speed = 100;
int test_delay = 500;
String command = String(4);
boolean commandComplete = false;
boolean debug = false;

// LOAD DATA FROM .h to PROGMEM
const int cal [] PROGMEM = {
#include "cal.h"
}; //calibration


void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Wire0");
  Wire.begin();
  // Notar que invertimos
  wi1 = &Wire;
  Serial.println("Starting Wire1");
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
  for (int i=0; i<nmodules; i++) {
    is_connected(LEDARR[i],i);
  }
  delay(5000);
  Serial.println("ALL ZERO");
  all_value(0);
  delay(1000);
  Serial.println("READY to ROCK");
}


void loop()
{
  if(Serial.available() > 0) {
    getIncomingChars();
  }
  if (commandComplete == true) {
    if (debug) {
      Serial.println("COMMAND COMPLETE");
      Serial.println(command);
    
    }
    if (command.length()>1) {
      processCommand();
    }
    command = "";
    commandComplete = false;
  } 
  delay(1);
}

void processCommand() {
    module = String(command.charAt(0)).toInt();
    chan = hex2i(command.charAt(1));
    if (command.length()>2) {
      int nchars = command.length();
      String arg = command.substring(2,nchars);
      //Serial.println(arg);
      if (isNumeric(arg)) {
        argv = arg.toInt();
        if (argv>PWM_MAX) {
          argv = PWM_MAX;
        }
        //Serial.println(argv);
      }  
      else { 
        argv = 0;
      }
      set_channel(LEDARR[module],module,chan,argv);
    }
    else {
      test_channel(LEDARR[module],module,chan);  
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

void test_channel(PCA9685 ledArray, int module, int channel) {
  int nchan = module*16+channel;
  ledArray.setPWM(channel, 0, cal[nchan]);  
  Serial.print("TEST ");
  Serial.print(module);
  Serial.print(":");
  Serial.print(channel);
  Serial.print(":");
  Serial.print(nchan);
  Serial.print(">");
  Serial.print(cal[nchan]);
  Serial.println();
  delay(test_delay);
  ledArray.setPWM(channel, 0, 0);
}

void set_channel(PCA9685 ledArray, int module, int channel, int argv) {
  int nchan = module*16+channel;
  Serial.print("SET ");
  Serial.print(module);
  Serial.print(":");
  Serial.print(channel);
  Serial.print(":");
  Serial.print(nchan);
  Serial.print(">");
  Serial.print(argv);
  Serial.println();
  ledArray.setPWM(channel, 0, argv);  
}

void all_value(int val) {
  for (int i=0; i<10; i++) {
    all_value_module(LEDARR[i], val);
  }
}

void all_value_module(PCA9685 ledArray, int val) {
  for (int i=0; i<16; i++) {
    ledArray.setPWM(i, 0, val); 
  }
}

int hex2i(char c) 
{
  int x =0;
  if (c >= '0' && c <= '9') {
    x = c - '0'; 
  }
  else if (c >= 'A' && c <= 'F') {
    x = (c - 'A') + 10; 
  }
  else return 0;
  return x;
}
