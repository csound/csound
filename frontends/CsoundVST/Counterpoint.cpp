#include "Counterpoint.hpp"
#include <vector>
#include <iostream>
#include <fstream>

int Counterpoint::PerfectConsonance[13] =   {1,0,0,0,0,0,0,1,0,0,0,0,1};
int Counterpoint::ImperfectConsonance[13] = {0,0,0,1,1,0,0,0,1,1,0,0,0};
int Counterpoint::Dissonance[13] =          {0,1,1,0,0,1,1,0,0,0,1,1,0};
int Counterpoint::_Ionian[12] =     {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
int Counterpoint::_Dorian[12] =     {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0};
int Counterpoint::_Phrygian[12] =   {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0};
int Counterpoint::_Lydian[12] =     {1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1};
int Counterpoint::_Mixolydian[12] = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0};
int Counterpoint::_Aeolian[12] =    {1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0};
int Counterpoint::_Locrian[12] =    {1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0};
int Counterpoint::BadMelodyInterval[13] = {0,0,0,0,0,0,1,0,0,1,1,1,0};
int Counterpoint::Indx[17] = {0,1,-1,2,-2,3,-3,0,4,-4,5,7,-5,8,12,-7,-12};
double Counterpoint::inverse_rscl = .000061035156;

int Counterpoint::UnisonPenalty		                  = Counterpoint::Bad;
int Counterpoint::DirectToFifthPenalty		          = Counterpoint::RealBad;
int Counterpoint::DirectToOctavePenalty		          = Counterpoint::RealBad;
int Counterpoint::ParallelFifthPenalty		          = Counterpoint::infinity;
int Counterpoint::ParallelUnisonPenalty		          = Counterpoint::infinity;
int Counterpoint::EndOnPerfectPenalty		          = Counterpoint::infinity;
int Counterpoint::NoLeadingTonePenalty		          = Counterpoint::infinity;
int Counterpoint::DissonancePenalty		          = Counterpoint::infinity;
int Counterpoint::OutOfRangePenalty		          = Counterpoint::RealBad;
int Counterpoint::OutOfModePenalty		          = Counterpoint::infinity;
int Counterpoint::TwoSkipsPenalty			  = 1;
int Counterpoint::DirectMotionPenalty		          = 1;
int Counterpoint::PerfectConsonancePenalty	          = 2;
int Counterpoint::CompoundPenalty			  = 1;
int Counterpoint::TenthToOctavePenalty		          = 8;
int Counterpoint::SkipTo8vePenalty		          = 8;
int Counterpoint::SkipFromUnisonPenalty		          = 4;
int Counterpoint::SkipPrecededBySameDirectionPenalty	  = 1;
int Counterpoint::FifthPrecededBySameDirectionPenalty	  = 3;
int Counterpoint::SixthPrecededBySameDirectionPenalty	  = 8;
int Counterpoint::SkipFollowedBySameDirectionPenalty	  = 3;
int Counterpoint::FifthFollowedBySameDirectionPenalty	  = 8;
int Counterpoint::SixthFollowedBySameDirectionPenalty	  = 34;
int Counterpoint::TwoSkipsNotInTriadPenalty	          = 3;
int Counterpoint::BadMelodyPenalty		          = Counterpoint::infinity;
int Counterpoint::ExtremeRangePenalty		          = 5;
int Counterpoint::LydianCadentialTritonePenalty	          = 13;
int Counterpoint::UpperNeighborPenalty		          = 1;
int Counterpoint::LowerNeighborPenalty		          = 1;
int Counterpoint::OverTwelfthPenalty		          = Counterpoint::infinity;
int Counterpoint::OverOctavePenalty		          = Counterpoint::Bad;
int Counterpoint::SixthLeapPenalty		          = 2;
int Counterpoint::OctaveLeapPenalty		          = 5;
int Counterpoint::BadCadencePenalty		          = Counterpoint::infinity;
int Counterpoint::DirectPerfectOnDownbeatPenalty	  = Counterpoint::infinity;
int Counterpoint::RepetitionOnUpbeatPenalty	          = Counterpoint::Bad;
int Counterpoint::DissonanceNotFillingThirdPenalty	  = Counterpoint::infinity;
int Counterpoint::UnisonDownbeatPenalty		          = 3;
int Counterpoint::TwoRepeatedNotesPenalty		  = 2;
int Counterpoint::ThreeRepeatedNotesPenalty	          = 4;
int Counterpoint::FourRepeatedNotesPenalty	          = 7;
int Counterpoint::LeapAtCadencePenalty		          = 13;
int Counterpoint::NotaCambiataPenalty		          = Counterpoint::infinity;
int Counterpoint::NotBestCadencePenalty		          = 8;
int Counterpoint::UnisonOnBeat4Penalty		          = 3;
int Counterpoint::NotaLigaturePenalty		          = 21;
int Counterpoint::LesserLigaturePenalty                   = 8;
int Counterpoint::UnresolvedLigaturePenalty	          = Counterpoint::infinity;
int Counterpoint::NoTimeForaLigaturePenalty	          = Counterpoint::infinity;
int Counterpoint::EighthJumpPenalty		          = Counterpoint::Bad;
int Counterpoint::HalfUntiedPenalty		          = 13;
int Counterpoint::UnisonUpbeatPenalty		          = 21;
int Counterpoint::MelodicBoredomPenalty		          = 1;
int Counterpoint::SkipToDownBeatPenalty		          = 1;
int Counterpoint::ThreeSkipsPenalty		          = 3;
int Counterpoint::DownBeatUnisonPenalty		          = Counterpoint::Bad;
int Counterpoint::VerticalTritonePenalty		  = 2;
int Counterpoint::MelodicTritonePenalty		          = 8;
int Counterpoint::AscendingSixthPenalty		          = 1;
int Counterpoint::RepeatedPitchPenalty		          = 1;
int Counterpoint::NotContraryToOthersPenalty	          = 1;
int Counterpoint::NotTriadPenalty			  = 34;
int Counterpoint::InnerVoicesInDirectToPerfectPenalty	  = 21;
int Counterpoint::InnerVoicesInDirectToTritonePenalty	  = 13;
int Counterpoint::SixFiveChordPenalty		          = Counterpoint::infinity;
int Counterpoint::UnpreparedSixFivePenalty	          = Counterpoint::Bad;
int Counterpoint::UnresolvedSixFivePenalty	          = Counterpoint::Bad;
int Counterpoint::AugmentedIntervalPenalty	          = Counterpoint::infinity;
int Counterpoint::ThirdDoubledPenalty		          = 5;
int Counterpoint::DoubledLeadingTonePenalty	          = Counterpoint::infinity;
int Counterpoint::DoubledSixthPenalty		          = 5;
int Counterpoint::DoubledFifthPenalty		          = 3;
int Counterpoint::TripledBassPenalty		          = 3;
int Counterpoint::UpperVoicesTooFarApartPenalty	          = 1;
int Counterpoint::UnresolvedLeadingTonePenalty	          = Counterpoint::infinity;
int Counterpoint::AllVoicesSkipPenalty		          = 8;
int Counterpoint::DirectToTritonePenalty		  = Counterpoint::Bad;
int Counterpoint::CrossBelowBassPenalty		          = Counterpoint::infinity;
int Counterpoint::CrossAboveCantusPenalty		  = Counterpoint::infinity;
int Counterpoint::NoMotionAgainstOctavePenalty            = 34; 

void Counterpoint::initialize(int mostnotes, int mostvoices)
{
  randx = 1;
  MostNotes = mostnotes;
  MostVoices = mostvoices + 1;
  Ctrpt.resize(MostNotes, MostVoices, 0);
  Onset.resize(MostNotes, MostVoices, 0);
  Dur.resize(MostNotes, MostVoices, 0);
  TotalNotes.resize(MostVoices, 0);
  BestFit.resize(MostNotes, MostVoices, 0);
  BestFit1.resize(MostNotes, MostVoices, 0);
  BestFit2.resize(MostNotes, MostVoices, 0);
  RhyPat.resize(11, 9, 0);
  RhyNotes.resize(11, 0);
  vbs.resize(MostVoices, 0);
}

void Counterpoint::counterpoint(int OurMode, int *StartPitches, int CurV, int cantuslen, int Species, int *cantus)
{
  initialize(cantuslen * 8, CurV);
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

