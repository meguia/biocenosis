#define PERIODMILIS 10
// SD card on Teensy 3.x Audio Shield (Rev C)
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14
#define MIN_DIST 150

#include <Audio.h>
#include "wind2.h"
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "effect_platervbstereo.h"
#include <CommandParser.h>
#include "event.h"

#define QUAD

#ifdef QUAD

  AudioOutputI2SQuad       out;
  AudioControlSGTL5000     sgtl5000_1;
  AudioControlSGTL5000     sgtl5000_2;

#else

  AudioOutputI2S out;
  AudioControlSGTL5000 audioShield;

#endif

Event **evs;
AudioConnection **mixer_out_patchCord;

#define N_EVENT 3

wind2 w;
//AudioEffectPlateReverb  reverb;

typedef struct struct_message {
    uint8_t intensity; // intensity value between 0 and 100
    uint8_t distance; // distance in 10 x meters < 200
    uint8_t vchan[4];  // amplitude one byte per channel 0-100 each
    uint8_t spwidth; // spatial width number of channels between 1 and 16
    uint8_t spduration; // duration in seconds of the sparks up to 255
} struct_message;
byte startmarker=255;

struct_message incoming;
byte* structstart = reinterpret_cast<byte*>(&incoming);
bool newdata = false;

AudioPlaySdWav background1;
AudioPlaySdWav background2;
String backgroundfile = "RUMBLE10.WAV";
int crossfade = 2000; //milliseconds

AudioMixer4 masterL;
AudioMixer4 masterR;
AudioMixer4 masterL2;
AudioMixer4 masterR2;

AudioMixer4 event_mixerL;
AudioMixer4 event_mixerR;
AudioMixer4 event_mixerL2;
AudioMixer4 event_mixerR2;

//AudioConnection reeverb_mixer_patchCord00(reverb, 0, masterL, 2);
//AudioConnection reeverb_mixer_patchCord11(reverb, 1, masterR, 2);
//AudioConnection reeverb_mixer_patchCord02(reverb, 0, masterL2, 2);
//AudioConnection reeverb_mixer_patchCord13(reverb, 1, masterR2, 2);
  
AudioConnection wind_mixer_patchCord00(w, 0, masterL, 3);
AudioConnection wind_mixer_patchCord01(w, 0, masterR, 3);
AudioConnection wind_mixer_patchCord02(w, 0, masterL2, 3);
AudioConnection wind_mixer_patchCord03(w, 0, masterR2, 3);

AudioConnection event_mixer_patchCord00(event_mixerL, 0, masterL, 0);
AudioConnection event_mixer_patchCord01(event_mixerR, 0, masterR, 0);
AudioConnection event_mixer_patchCord02(event_mixerL2, 0, masterL2, 0);
AudioConnection event_mixer_patchCord03(event_mixerR2, 0, masterR2, 0);

AudioConnection background_mixer_patchCord00(background1, 0, masterL, 1);
AudioConnection background_mixer_patchCord01(background1, 0, masterR, 1);
AudioConnection background_mixer_patchCord02(background1, 0, masterL2, 1);
AudioConnection background_mixer_patchCord03(background1, 0, masterR2, 1);

AudioConnection background_mixer_patchCord10(background2, 0, masterL, 2);
AudioConnection background_mixer_patchCord11(background2, 0, masterR, 2);
AudioConnection background_mixer_patchCord12(background2, 0, masterL2, 2);
AudioConnection background_mixer_patchCord13(background2, 0, masterR2, 2);

#ifdef QUAD

  AudioConnection master_out_patchCord00(masterL, 0, out, 0);
  AudioConnection master_out_patchCord11(masterR, 0, out, 1);
  AudioConnection master_out_patchCord22(masterL2, 0, out, 2);
  AudioConnection master_out_patchCord33(masterR2, 0, out, 3);

#else

  AudioConnection master_out_patchCord00(masterL, 0, out, 0);
  AudioConnection master_out_patchCord11(masterR, 0, out, 1);

#endif

AudioAnalyzePeak         peak1;
AudioConnection peak(masterL, 0, peak1, 0);

typedef CommandParser<16, 8, 10, 32, 64> MyCommandParser;

MyCommandParser parser;

void readCommand() {
  if (Serial.available()) {
    char line[512];
    size_t lineLength = Serial.readBytesUntil('\n', line, 511);
    line[lineLength] = '\0';
    Serial.println(line);
    char response[MyCommandParser::MAX_RESPONSE_SIZE];
    parser.processCommand(line, response);
    Serial.println(response);
  }
}

int evn = 0;

void cmd_filters(MyCommandParser::Argument *args, char *response) { 
  double freqs[] = {args[0].asDouble,args[1].asDouble,args[2].asDouble,args[3].asDouble};
  double qs[] = {0.707, 0.707, 0.707, 0.707};
  evs[evn]->set_filters(freqs, qs);
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void cmd_gains(MyCommandParser::Argument *args, char *response) { 
  double gains[] = {args[0].asDouble,args[1].asDouble,args[2].asDouble,args[3].asDouble};
  evs[evn]->set_gains(gains);  
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void cmd_files(MyCommandParser::Argument *args, char *response) { 
  int files[] = {args[0].asInt64,args[1].asInt64,args[2].asInt64,args[3].asInt64};
  evs[evn]->set_files(files);
  
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void cmd_delays(MyCommandParser::Argument *args, char *response) { 
  int delays[4];
  for (int i=0;i<N_PLAYERS;i++)
   { delays[i]=args[i].asDouble;}
  
  evs[evn]->set_delays(delays); 
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}


void cmd_location(MyCommandParser::Argument *args, char *response) { 
  evs[evn]->set_location(args[0].asDouble, args[1].asDouble);
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void cmd_play(MyCommandParser::Argument *args, char *response) { 
  evs[evn]->play();
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void cmd_playn(MyCommandParser::Argument *args, char *response) { 
  if (args[0].asInt64==0)
    evn=0;
  else
    evn=1;
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void cmd_wind(MyCommandParser::Argument *args, char *response) { 
  masterL.gain(3,args[0].asDouble);
  masterR.gain(3,args[0].asDouble);
  masterL2.gain(3,args[0].asDouble);
  masterR2.gain(3,args[0].asDouble);
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void cmd_reverb(MyCommandParser::Argument *args, char *response) { 
//  Serial.print("Reverb: "); Serial.print(args[0].asDouble); Serial.print(" "); Serial.print(args[1].asDouble); Serial.print(" "); 
//  Serial.print(args[2].asDouble); Serial.print(" "); Serial.print(args[3].asDouble); Serial.print(" ");  Serial.println(args[4].asDouble);
//
//  masterL.gain(2,args[0].asDouble);
//  masterR.gain(2,args[0].asDouble);
//  masterL2.gain(2,args[0].asDouble);
//  masterR2.gain(2,args[0].asDouble);
//  reverb.size(args[1].asDouble);     // max reverb length
//  reverb.lowpass(args[2].asDouble);  // sets the reverb master lowpass filter
//  reverb.lodamp(args[3].asDouble);   // amount of low end loss in the reverb tail
//  reverb.hidamp(args[4].asDouble);   // amount of treble loss in the reverb tail
//  reverb.diffusion(1.0);  // 1.0 is the detault setting, lower it to create more "echoey" reverb
//  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void cmd_test(MyCommandParser::Argument *args, char *response) { 
  go();
  strlcpy(response, "success", MyCommandParser::MAX_RESPONSE_SIZE);
}

void go()
{
  evn = -1;
  for (int i=0;i<N_EVENT; i++) {
    if (!evs[i]->isplaying()) {
      evn = i;
      break;
    }
  }
  String aux=String("GO ")+evn;
  Serial.println(aux);
  if (evn >= 0) {
    Serial.println("DALE");
    double tmp[4] = {0.8,1.2,0.4,0.4};
    evs[evn]->set_gains(tmp);
    //int tmp2[4] = {20,500,600,700};
    int tmp2[4] = {20,-1,-1,-1};
    evs[evn]->set_delays(tmp2);
    int tmp3[4] = {random(1,15),random(1,15),random(1,10),random(1,10)};
    evs[evn]->set_files(tmp3);
//    double tmp4[4] = {5000,3000,800,500};
//    double qs[] = {0.707, 0.707, 0.707, 0.707};
//    evs[evn]->set_filters(tmp4, qs);
//    evs[evn]->set_location(random(100)/100.0, random(100)/100.0);
    evs[evn]->play();
  
    aux=String("AudioMemory ")+AudioMemoryUsage() + String(" Max ") + AudioMemoryUsageMax();;
    Serial.println(aux);
  }  
}

void setup() { 
  AudioMemory(60);

  #ifdef QUAD

    sgtl5000_1.setAddress(LOW);
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.8);
    sgtl5000_2.setAddress(HIGH);
    sgtl5000_2.enable();
    sgtl5000_2.volume(0.8);

  #else
    audioShield.enable();
    audioShield.volume(0.8);
  
  #endif
  // mute reverb
  //masterL.gain(2,0);
  //masterR.gain(2,0);
  //masterL2.gain(2,0);
  //masterR2.gain(2,0);

//  Wind params

  w.setParamValue("freqmean", 600);
  w.setParamValue("desv", 20); // desvio de freqmean cuando fluctua
  w.setParamValue("modrate", 0.05); // rate de fluctuación menos más lento
  double wg = 0.5;
  masterL.gain(3,wg);
  masterR.gain(3,wg);
  masterL2.gain(3,wg);
  masterR2.gain(3,wg);
//  

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here if no SD card, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

  Serial.begin(9600); 
  while(!Serial);
  Serial1.begin(115200); // RTX2 TX2
  while(!Serial1);

  parser.registerCommand("PLAY", "", &cmd_play);
  parser.registerCommand("FILES", "iiii", &cmd_files);
  parser.registerCommand("GAINS", "dddd", &cmd_gains);
  parser.registerCommand("FILTERS", "dddd", &cmd_filters);
  parser.registerCommand("WIND", "dd", &cmd_wind);
  parser.registerCommand("DELAYS", "dddd", &cmd_delays);
  parser.registerCommand("PLAYN", "i", &cmd_playn);
  parser.registerCommand("REVERB", "ddddd", &cmd_reverb);
  parser.registerCommand("LOCATION", "dd", &cmd_location);
  parser.registerCommand("TEST", "i", &cmd_test);

  evs = (Event**)malloc(sizeof(Event*) * N_EVENT);
  mixer_out_patchCord = (AudioConnection**)malloc(sizeof(AudioConnection*) * 4 * N_EVENT);
  for (int i = 0; i < N_EVENT; i++) {
      evs[i] = new Event(i);
      mixer_out_patchCord[i*4+0] = new AudioConnection(evs[i]->mixerL, 0, event_mixerL, i);
      mixer_out_patchCord[i*4+1] = new AudioConnection(evs[i]->mixerR, 0, event_mixerR, i);
      mixer_out_patchCord[i*4+2] = new AudioConnection(evs[i]->mixerL2, 0, event_mixerL2, i);
      mixer_out_patchCord[i*4+3] = new AudioConnection(evs[i]->mixerR2, 0, event_mixerR2, i);
  }

  Serial.println(String("Arrancanding Events ") + N_EVENT);

  background1.play(backgroundfile.c_str());

}

void loop() {
  receiveStruct();
  readCommand();
  if (newdata == true) {
        Serial.println("EVENTO");
        showData();
        if (incoming.distance<MIN_DIST) {
          go();
        }        
        newdata = false;
  }
  wait(PERIODMILIS);
  for (int i = 0; i < N_EVENT; i++)
    evs[i]->update();

  if (!background2.isPlaying() && (background1.lengthMillis()-background1.positionMillis())<crossfade)
  {
      background2.play(backgroundfile.c_str());
  }
  if (!background1.isPlaying() && (background2.lengthMillis()-background2.positionMillis())<crossfade)
  {
      background1.play(backgroundfile.c_str());
  }

  if (peak1.available()) {
      Serial.print("Peak");  Serial.println(peak1.read());
  }
}

void wait(unsigned int milliseconds)
{
  elapsedMillis msec = 0;
  while (msec <= milliseconds) {   
  }
}

void receiveStruct() {    
  if (Serial1.available() >= sizeof(incoming) + 1 and newdata == false) {
    byte rb = Serial1.read();
    if (rb == startmarker) {
        for (byte n=0; n < sizeof(incoming); n++) {
            *(structstart + n) = Serial1.read();
        }
            // make sure there is no garbage left in the buffer
        while (Serial1.available() > 0) {
            byte dump = Serial1.read();
        }
        newdata = true;
    }
  }
}  

void showData() {    
    Serial.print(incoming.intensity);
    Serial.print(",");
    Serial.print(incoming.distance);
    Serial.print(",");
    for (int i=0;i<4;i++) {
      Serial.print(incoming.vchan[i]);
      Serial.print(",");
    }
    Serial.print(incoming.spwidth);
    Serial.print(",");
    Serial.println(incoming.spduration);
}
