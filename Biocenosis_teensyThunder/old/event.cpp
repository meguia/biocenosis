#include "event.h"

void Event::set_filters(double freqs[], double qs[])
{
  for (int i=0;i<N_PLAYERS;i++)
    players[i].set_biquad(freqs[i], qs[i]);
}

bool Event::isplaying()
{
  bool isp= false;  
  for (int i=0;i<N_PLAYERS;i++) {
    if (players[i].isPlaying()) isp = true;
  }
  return isp;    
}


void Event::set_files(int ns[])
{
  for (int i=0;i<1;i++)
  {
    String filename = String("BOLT") + ns[i] + ".WAV";
    players[i].set_file(filename);
  }
//  for (int i=1;i<2;i++)
//  {
//    String filename = String("RUMBLE") + ns[i] + ".WAV";
//    players[i].set_file(filename);
//  }
}


void Event::set_location(double x, double y)
{
  players[0].set_location(x,y);
  //players[1].set_location(x,y);
  //players[1].set_location(0.5,0.5);
  //players[3].set_location(0.5,0.5);
}

void Event::set_delays(int _delays[])
{
  for (int i=0;i<N_PLAYERS;i++) delays[i]=_delays[i];  
}
void Event::set_gains(double gains[])
{
  for (int i=0;i<N_PLAYERS;i++) players[i].set_gain(gains[i]); 
}
void Event::play()
{
  Serial.print("Event "); Serial.print(id); Serial.println(" playing ");

  playing=true;
  for (int i=0;i<N_PLAYERS;i++) doned[i]=0;
  for (int i=0;i<N_PLAYERS;i++) delays_var[i]=delays[i];
}
void Event::update()
{
  for (int i=0;i<N_PLAYERS;i++)
  {
    if(delays_var[i]>0 && playing)
      {
        delays_var[i]--;
        //Serial.println(String("Event ") + id + String(" Player: ") + i + String(" del: ") + (int)delays_var[i]);
      }
    if(delays_var[i]==0 && playing)
    {
      Serial.println(String("Event ") + id + String(" Player: ") + i + String(" play"));
      players[i].play();
      delays_var[i] = -1;
      doned[i] = 1;
    }
  }
  
  int sumdoned = 0;
  for (int i=0;N_PLAYERS<1;i++)
    sumdoned+=doned[i];

  if (sumdoned==1){
    playing=false;
    for (int i=0;i<N_PLAYERS;i++) doned[i]=0;
    Serial.print("Event "); Serial.print(id); Serial.println(" Done ");
  }
}

Event::~Event()
{
  free(player_mix_patchCords);
}

Event::Event(size_t _id)
{
    id = _id;
    player_mix_patchCords = (AudioConnection**)malloc(sizeof(AudioConnection*) * 4*N_PLAYERS);
 
    for (int i = 0; i < N_PLAYERS; i++) {
        player_mix_patchCords[i*4+0] = new AudioConnection(players[i].gainL, 0, mixerL, i);
        player_mix_patchCords[i*4+1] = new AudioConnection(players[i].gainR, 0, mixerR, i);
        player_mix_patchCords[i*4+2] = new AudioConnection(players[i].gainL2, 0, mixerL2, i);
        player_mix_patchCords[i*4+3] = new AudioConnection(players[i].gainR2, 0, mixerR2, i);   
    }
}
