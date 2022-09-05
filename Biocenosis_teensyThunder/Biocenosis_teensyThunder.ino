#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14
#define MIN_DIST 50

#include <Audio.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "player2.h"

#define QUAD

#ifdef QUAD

  AudioOutputI2SQuad       out;
  AudioControlSGTL5000     sgtl5000_1;
  AudioControlSGTL5000     sgtl5000_2;

#else

  AudioOutputI2S out;
  AudioControlSGTL5000 audioShield;

#endif
AudioConnection **mixer_out_patchCord;

#define N_PLAYER 3
Player **players;

unsigned int time_interval = 5;
unsigned long previousMillis = 0; 

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

float cumstorm=0.0;
float threshold_storm=600.0;
float stormdecay=0.98;

AudioPlaySdWav background1;
AudioPlaySdWav background2;
String backgroundfile = "RUMBLE10.WAV";
int crossfade = 2000; //milliseconds

AudioMixer4 masterL;
AudioMixer4 masterR;
AudioMixer4 masterL2;
AudioMixer4 masterR2;

AudioMixer4 player_mixerL;
AudioMixer4 player_mixerR;
AudioMixer4 player_mixerL2;
AudioMixer4 player_mixerR2;

AudioConnection event_mixer_patchCord00(player_mixerL, 0, masterL, 0);
AudioConnection event_mixer_patchCord01(player_mixerR, 0, masterR, 0);
AudioConnection event_mixer_patchCord02(player_mixerL2, 0, masterL2, 0);
AudioConnection event_mixer_patchCord03(player_mixerR2, 0, masterR2, 0);

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

//AudioAnalyzePeak         peak1;
//AudioConnection peak(masterL, 0, peak1, 0);

void go()
{
  int evn = -1;
  for (int i=0;i<N_PLAYER; i++) {
    if (!players[i]->isPlaying()) {
      evn = i;
      break;
    }
  }
  String aux=String("GO ")+evn;
  Serial.println(aux);
  if (evn >= 0) {
    Serial.print("DALE!!!!!!!!! ---------------------------------->   ");
    players[evn]->set_gain(0.5);
        String filename = String("BOLT") + random(1,15) + ".WAV";
    players[evn]->set_file(filename);
    players[evn]->set_gains((double)incoming.vchan[0]/100.0,(double)incoming.vchan[1]/100.0,(double)incoming.vchan[2]/100.0,(double)incoming.vchan[3]/100.0);
    players[evn]->play();
    Serial.println(filename);
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

  players = (Player**)malloc(sizeof(Player*) * N_PLAYER);
  mixer_out_patchCord = (AudioConnection**)malloc(sizeof(AudioConnection*) * 4 * N_PLAYER);
  for (int i = 0; i < N_PLAYER; i++) {
      players[i] = new Player();
      mixer_out_patchCord[i*4+0] = new AudioConnection(players[i]->gainL, 0, player_mixerL, i);
      mixer_out_patchCord[i*4+1] = new AudioConnection(players[i]->gainR, 0, player_mixerR, i);
      mixer_out_patchCord[i*4+2] = new AudioConnection(players[i]->gainL2, 0, player_mixerL2, i);
      mixer_out_patchCord[i*4+3] = new AudioConnection(players[i]->gainR2, 0, player_mixerR2, i);
  }

  Serial.println(String("Arrancanding Players ") + N_PLAYER);
  background1.play(backgroundfile.c_str());
}

void loop() {
  //unsigned long currentMillis = millis();
  //if (currentMillis - previousMillis >= time_interval) {
    receiveStruct();
    if (newdata == true) {
          Serial.println("EVENTO");
          showData();
          cumstorm += (float) incoming.spduration*(incoming.intensity+1.0)/100.0;
          Serial.print("CUMSTORM ");
          Serial.println(cumstorm);
          if (incoming.distance<MIN_DIST) {
            go();
          }        
          newdata = false;
    }
    //previousMillis = currentMillis;
    cumstorm *= stormdecay;
  //}
  if (!background2.isPlaying() && (background1.lengthMillis()-background1.positionMillis())<crossfade)
  {
      background2.play(backgroundfile.c_str());
      Serial.println(backgroundfile.c_str());
  }
  if (!background1.isPlaying() && (background2.lengthMillis()-background2.positionMillis())<crossfade)
  {
      background1.play(backgroundfile.c_str());
      Serial.println(backgroundfile.c_str());
  }
  wait(time_interval);
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
