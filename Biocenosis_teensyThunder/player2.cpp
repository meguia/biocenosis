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
}


void Player::set_gains(double g1, double g2,double g3,double g4)
{
  gainL.gain(g1);
  gainR.gain(g2);
  gainL2.gain(g3);
  gainR2.gain(g4);  
}



Player::Player()
{
 player_mix_patchCord0 = new AudioConnection(playsdwav, 0, gainL, 0);
 player_mix_patchCord1 = new AudioConnection(playsdwav, 0, gainR, 0);
 player_mix_patchCord2 = new AudioConnection(playsdwav, 1, gainL2, 0);
 player_mix_patchCord3 = new AudioConnection(playsdwav, 1, gainR2, 0);
}
