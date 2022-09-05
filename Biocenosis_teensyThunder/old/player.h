#ifndef player_h_
#define player_h_

#include <Audio.h>

class Player 
{
    public:
    
       Player();
      ~Player();

      AudioAmplifier amp;
      AudioAmplifier gainL;
      AudioAmplifier gainR;      
      AudioAmplifier gainL2;
      AudioAmplifier gainR2;

      AudioPlaySdWav playsdwav;
      //AudioFilterBiquad        biquad;

      AudioConnection *player_amp_patchCord0;
      AudioConnection *amp_filter_patchCord0;
      
      AudioConnection *player_mix_patchCord0;
      AudioConnection *player_mix_patchCord1;
      AudioConnection *player_mix_patchCord2;
      AudioConnection *player_mix_patchCord3;
  
      void play();
      void set_file(String n);
      void set_gain(double g);
      void set_location(double x, double y);
      void set_biquad(double freq, double Q);
      bool isPlaying();

      String filename;
}; 

#endif
