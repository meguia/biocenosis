#ifndef event_h_
#define event_h_

#define N_PLAYERS 2

#include "player.h"


class Event 
{
    public:
    
       Event(size_t id);
      ~Event();

      Player players[N_PLAYERS];
      long long int delays[N_PLAYERS];
      long long int delays_var[N_PLAYERS];
      bool playing = false;
      int doned[N_PLAYERS] = {0,0};
      size_t id;

      AudioMixer4 mixerL;
      AudioMixer4 mixerR;
      AudioMixer4 mixerL2;
      AudioMixer4 mixerR2;

      AudioConnection **player_mix_patchCords;

      void play();
      void update();
      
      void set_filters(double freqs[], double qs[]);
      void set_files(int ns[]);
      void set_delays(int _delays[]);
      void set_gains(double gains[]);
      void set_location(double x, double y);
      bool isplaying();
}; 

#endif
