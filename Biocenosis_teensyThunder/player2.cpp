#include "player2.h"


void Player::set_file(String _filename)
{
  filename = _filename;
}


void Player::play()
{
  playsdwav.play(filename.c_str());
}

bool Player::isPlaying() {
  return playsdwav.isPlaying();  
}

Player::~Player()
{
  free(player_mix_patchCord0);
  free(player_mix_patchCord1);
  free(player_mix_patchCord2);
  free(player_mix_patchCord3);  
  free(player_amp_patchCord0);  
  free(amp_filter_patchCord0);  
}

void Player::set_location(double x, double y)
{
  double d = 1; //minimum gain is 1/(1+d)
  
  gainL.gain(1/((x+y)*d+1));
  gainR.gain(1/((1-x+y)*d + 1));
  gainL2.gain(1/( (x+(1-y))*d+1));
  gainR2.gain(1/((2-x-y)*d + 1));  
}

void Player::set_gains(double g1, double g2,double g3,double g4)
{
  gainL.gain(g1);
  gainR.gain(g2);
  gainL2.gain(g3);
  gainR2.gain(g4);  
}

void Player::set_gain(double g)
{
  amp.gain(g);
}


Player::Player()
{

 player_amp_patchCord0 = new AudioConnection(playsdwav,0, amp, 0);
 player_mix_patchCord0 = new AudioConnection(amp, 0, gainL, 0);
 player_mix_patchCord1 = new AudioConnection(amp, 0, gainR, 0);
 player_mix_patchCord2 = new AudioConnection(amp, 0, gainL2, 0);
 player_mix_patchCord3 = new AudioConnection(amp, 0, gainR2, 0);


}
