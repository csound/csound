#include "Counterpoint.hpp"
#include <vector>
#include <iostream>
#include <fstream>

int Counterpoint::PerfectConsonance[13] =   {1,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  1};
int Counterpoint::ImperfectConsonance[13] = {0,  0,  0,  1,  1,  0,  0,  0,  1,  1,  0,  0,  0};
int Counterpoint::Dissonance[13] =          {0,  1,  1,  0,  0,  1,  1,  0,  0,  0,  1,  1,  0};
int Counterpoint::_Ionian[12] =             {1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  0,  1};
int Counterpoint::_Dorian[12] =             {1,  0,  1,  1,  0,  1,  0,  1,  0,  1,  1,  0};
int Counterpoint::_Phrygian[12] =           {1,  1,  0,  1,  0,  1,  0,  1,  1,  0,  1,  0};
int Counterpoint::_Lydian[12] =             {1,  0,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1};
int Counterpoint::_Mixolydian[12] =         {1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  1,  0};
int Counterpoint::_Aeolian[12] =            {1,  0,  1,  1,  0,  1,  0,  1,  0,  0,  1,  0};
int Counterpoint::_Locrian[12] =            {1,  1,  0,  1,  0,  1,  1,  0,  1,  0,  1,  0};
int Counterpoint::BadMelodyInterval[13] =   {0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  1,  1,  0};
int Counterpoint::Indx[17] =                {0,  1, -1,  2, -2,  3, -3,  0,  4, -4,  5,  7, -5,  8, 12, -7,-12};
boost::mt19937 Counterpoint::mersenneTwister;

void Counterpoint::initialize(int mostnotes, int mostvoices)
{
  randx = 1;
  MostNotes = mostnotes;
  MostVoices = mostvoices;
  Ctrpt.resize(MostNotes, MostVoices, 0.0);
  Onset.resize(MostNotes, MostVoices, 0.0);
  Dur.resize(MostNotes, MostVoices, 0.0);
  TotalNotes.resize(MostVoices, 0.0);
  BestFit.resize(MostNotes, MostVoices, 0.0);
  BestFit1.resize(MostNotes, MostVoices, 0.0);
  BestFit2.resize(MostNotes, MostVoices, 0.0);
  RhyPat.resize(11, 9, 0.0);
  RhyNotes.resize(11, 0.0);
  vbs.resize(MostVoices, 0.0);
}

void Counterpoint::clear()
{
  Ctrpt.resize(0, 0, 0.0);
  Onset.resize(0, 0, 0.0);
  Dur.resize(0, 0, 0.0);
  TotalNotes.resize(0, 0.0);
  BestFit.resize(0, 0, 0.0);
  BestFit1.resize(0, 0, 0.0);
  BestFit2.resize(0, 0, 0.0);
  RhyPat.resize(0, 0, 0.0);
  RhyNotes.resize(0, 0.0);
  vbs.resize(MostVoices, 0.0);
}

void Counterpoint::counterpoint(int OurMode, int *StartPitches, int CurV, int cantuslen, int Species, int *cantus)
{
  initialize((cantuslen * 8) + 1, CurV + 1);
  if(StartPitches)
    {
      for (int i = 0; i < CurV; i++) 
	{
	  vbs[i] = StartPitches[i];
	}
    }
  int i;
  for (i=1;i<=cantuslen;i++) 
    {
      Ctrpt[i][0] = cantus[i-1];
    }
  for (i=0;i<3;i++)
    { 
      Fits[i]=0;
    }
  AnySpecies(OurMode,&vbs[0],CurV,cantuslen,Species);
}

void Counterpoint::toCsoundScore(std::string filename, double secondsPerPulse)
{
  size_t voice = 0;
  double time = 0;
  double duration = 0;
  double key = 0;
  double velocity = 70;
  double phase = 0;
  double x = 0;
  double y = 0;
  double z = 0;
  double pcs = 0;
  char buffer[0x100];
  std::fstream stream(filename.c_str(), std::ios::in | std::ios::out | std::ios::trunc);
  int totalnotes = 0;
  fprintf(stderr, "\n; %s\n", filename.c_str());
  for(voice = 0; voice < Ctrpt.size2(); voice++)
    {
      time = 0;
      for(size_t note = 1; note <= size_t(TotalNotes[voice]); note++)
	{
	  time = Onset[note][voice] * secondsPerPulse;
	  duration = Dur[note][voice] * secondsPerPulse;
	  key = double(Ctrpt[note][voice]);
	  sprintf(buffer, "i %d %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g %-1.7g\n", voice + 1, time, duration, key, velocity, phase, x, y, z, pcs);
	  fprintf(stderr, buffer);
	  stream << buffer;
	  totalnotes++;
	}
    }
  sprintf(buffer, "; Total notes = %d\n", totalnotes);
  fprintf(stderr, buffer);
  stream << buffer;
}

