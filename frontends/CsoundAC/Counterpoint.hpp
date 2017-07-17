#ifndef COUNTERPOINT_HPP
#define COUNTERPOINT_HPP
/*
  Status: RO
  Return-Path: <bil@ccrma.Stanford.EDU>
  Received: from smtp3.Stanford.EDU ([171.67.16.138])
  by aaron.mail.atl.earthlink.net (EarthLink SMTP Server) with ESMTP id 1ctFSm3Lh3Nl3qa0
  for <gogins@pipeline.com>; Mon, 15 Nov 2004 07:22:30 -0500 (EST)
  Received: from cm-mail.stanford.edu (cm-mail.Stanford.EDU [171.64.197.135])
  by smtp3.Stanford.EDU (8.12.11/8.12.11) with ESMTP id iAFCMSeh031083
  for <gogins@pipeline.com>; Mon, 15 Nov 2004 04:22:29 -0800
  Received: from ccrma (cmn13.stanford.edu [171.64.197.162])
  by cm-mail.stanford.edu (8.11.6/8.11.6) with ESMTP id iAFCMSH28453
  for <gogins@pipeline.com>; Mon, 15 Nov 2004 04:22:28 -0800
  Message-ID: <41989F84.7000906@ccrma>
  Date: Mon, 15 Nov 2004 04:22:28 -0800
  From: Bill Schottstaedt <bil@ccrma.Stanford.EDU>
  User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.4.2) Gecko/20040308
  X-Accept-Language: en-us, en
  MIME-Version: 1.0
  To: Michael Gogins <gogins@pipeline.com>
  Subject: Re: Permission
  References: <000301c4cab5$48db27f0$6501a8c0@Generator>
  In-Reply-To: <000301c4cab5$48db27f0$6501a8c0@Generator>
  Content-Type: text/plain; charset=us-ascii; format=flowed
  Content-Transfer-Encoding: 7bit
  X-ELNK-AV: 0

  You're most welcome to use my Fux code and article in any
  way you like.  I did a C translation of it about 10 years
  ago -- I think the original SAIL article is at ccrma-ftp
  as fux.txt, and I'll append what I think is the C version --
  if it doesn't work, let me know, and I'll scrounge around
  some old disks at home.  (This project was a ton of fun --
  I've always wanted to go back and carry it further, but
  have never had time):
*/

/* AUTOMATIC COUNTERPOINT
 *
 * from "Automatic Species Counterpoint", Bill Schottstaedt, CCRMA STAN-M-19, May 1984
 * translated from SAIL to C in March 1995
 *
 * This file can be treated as a stand-alone C program or as a foreign function module for Lisp.
 *
 * make fux creates the C program
 * cc fux.c -c -O -DCM creates the module
 *
 * See the "main" function for examples, or fux.lisp (which ties fux.c into CMN/CM).
 */

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include <string>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <eigen3/Eigen/Dense>
#include <random>
%}
#else
#include <string>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <eigen3/Eigen/Dense>
#include <random>
#include "Random.hpp"
#endif

class SILENCE_PUBLIC Counterpoint
{
public:
  void (*messageCallback)(CSOUND *csound, int attribute, const char *format, va_list valist);
  void message(const char *format,...);
  void message(const char *format, va_list valist);
  int MostNotes;
  int MostVoices;
  enum
    {
      MostNotes_ = 128,
      MostVoices_ = 12
    };
  long randx;
  Eigen::MatrixXi Ctrpt;
  Eigen::MatrixXi Onset;
  Eigen::MatrixXi Dur;
  Eigen::VectorXi TotalNotes;
  Eigen::MatrixXi BestFit;
  Eigen::MatrixXi BestFit1;
  Eigen::MatrixXi BestFit2;
  Eigen::VectorXi vbs;
  Eigen::MatrixXi RhyPat;
  Eigen::VectorXi RhyNotes;
  int Fits[3];
  virtual void initialize(int mostnotes, int mostvoices);
  virtual void clear();
  Counterpoint();
  virtual ~Counterpoint();
  int ABS(int i);
  int MIN(int a, int b);
  int MAX(int a, int b);
  void ARRBLT(int *dest, int *source, int num);
  enum
    {
      Unison = 0,
      MinorSecond = 1,
      MajorSecond = 2,
      MinorThird = 3,
      MajorThird = 4,
      Fourth = 5,
      Tritone = 6,
      Fifth = 7,
      MinorSixth = 8,
      MajorSixth = 9,
      MinorSeventh = 10,
      MajorSeventh = 11,
      Octave = 12
    };
#if !defined(SWIG)
  static int PerfectConsonance[13];
  static int ImperfectConsonance[13];
  static int Dissonance[13];
#endif
  enum
    {
      Aeolian = 1,
      Dorian = 2,
      Phrygian = 3,
      Lydian = 4,
      Mixolydian = 5,
      Ionian = 6,
      Locrian = 7
    };
#if !defined(SWIG)
  static int _Ionian[12];
  static int _Dorian[12];
  static int _Phrygian[12];
  static int _Lydian[12];
  static int _Mixolydian[12];
  static int _Aeolian[12];
  static int _Locrian[12];
#endif
  int InMode(int Pitch, int Mode);
#if !defined(SWIG)
  static int BadMelodyInterval[13];
#endif
  int BadMelody(int Intv);
  int ASkip(int Interval);
  int AStep(int Interval);
  int AThird(int Interval);
  int ASeventh(int Interval);
  int AnOctave(int Interval);
  int ATenth(int Interval);
  enum
    {
      DirectMotion = 1,
      ContraryMotion = 2,
      ObliqueMotion = 3,
      NoMotion = 4
    };
  int MotionType(int Pitch1, int Pitch2, int Pitch3, int Pitch4);
  int DirectMotionToPerfectConsonance(int Pitch1, int Pitch2, int Pitch3, int Pitch4);
  int ConsecutiveSkipsInSameDirection(int Pitch1, int Pitch2, int Pitch3);
  int LowestSemitone;
  int HighestSemitone;
  int OutOfRange(int Pitch);
  int ExtremeRange(int Pitch);
  int BasePitch,Mode,TotalTime;
  int Us(int n, int v);
  int LastNote(int n, int v);
  int FirstNote(int n, int v);
  int NextToLastNote(int n, int v);
  void SetUs(int n, int p, int v);
  int TotalRange(int Cn, int Cp, int v);
  int Cantus(int n, int v);
  int VIndex(int Time, int VNum);
  int Other(int Cn, int v, int v1);
  int Bass(int Cn, int v);
  enum {

    WholeNote = 8,
    HalfNote = 4,
    DottedHalfNote = 6,
    QuarterNote = 2,
    DottedQuarterNote = 3,
    EighthNote = 1
  };
  int Beat8(int n);
  int DownBeat(int n, int v);
  int UpBeat(int n, int v);
  int PitchRepeats(int Cn, int Cp, int v);
  enum {
    One = 0,
    Two = 2,
    Three = 3,
    Four = 4,
    Five = 5,
    Six = 6,
    Eight = 8
  };
  int Size(int MelInt);
  int TooMuchOfInterval(int Cn, int Cp, int v);
  int ADissonance(int Interval, int Cn, int Cp, int v, int Species);
  int Doubled(int Pitch, int Cn, int v);
  enum
    {
      infinity = 1000000,
      Bad = 100,
      RealBad = 200
    };
  int UnisonPenalty;
  int DirectToFifthPenalty;
  int DirectToOctavePenalty;
  int ParallelFifthPenalty;
  int ParallelUnisonPenalty;
  int EndOnPerfectPenalty;
  int NoLeadingTonePenalty;
  int DissonancePenalty;
  int OutOfRangePenalty;
  int OutOfModePenalty;
  int TwoSkipsPenalty;
  int DirectMotionPenalty;
  int PerfectConsonancePenalty;
  int CompoundPenalty;
  int TenthToOctavePenalty;
  int SkipTo8vePenalty;
  int SkipFromUnisonPenalty;
  int SkipPrecededBySameDirectionPenalty;
  int FifthPrecededBySameDirectionPenalty;
  int SixthPrecededBySameDirectionPenalty;
  int SkipFollowedBySameDirectionPenalty;
  int FifthFollowedBySameDirectionPenalty;
  int SixthFollowedBySameDirectionPenalty;
  int TwoSkipsNotInTriadPenalty;
  int BadMelodyPenalty;
  int ExtremeRangePenalty;
  int LydianCadentialTritonePenalty;
  int LowerNeighborPenalty;
  int UpperNeighborPenalty;
  int OverTwelfthPenalty;
  int OverOctavePenalty;
  int SixthLeapPenalty;
  int OctaveLeapPenalty;
  int BadCadencePenalty;
  int DirectPerfectOnDownbeatPenalty;
  int RepetitionOnUpbeatPenalty;
  int DissonanceNotFillingThirdPenalty;
  int UnisonDownbeatPenalty;
  int TwoRepeatedNotesPenalty;
  int ThreeRepeatedNotesPenalty;
  int FourRepeatedNotesPenalty;
  int LeapAtCadencePenalty;
  int NotaCambiataPenalty;
  int NotBestCadencePenalty;
  int UnisonOnBeat4Penalty;
  int NotaLigaturePenalty;
  int LesserLigaturePenalty;
  int UnresolvedLigaturePenalty;
  int NoTimeForaLigaturePenalty;
  int EighthJumpPenalty;
  int HalfUntiedPenalty;
  int UnisonUpbeatPenalty;
  int MelodicBoredomPenalty;
  int SkipToDownBeatPenalty;
  int ThreeSkipsPenalty;
  int DownBeatUnisonPenalty;
  int VerticalTritonePenalty;
  int MelodicTritonePenalty;
  int AscendingSixthPenalty;
  int RepeatedPitchPenalty;
  int NotContraryToOthersPenalty;
  int NotTriadPenalty;
  int InnerVoicesInDirectToPerfectPenalty;
  int InnerVoicesInDirectToTritonePenalty;
  int SixFiveChordPenalty;
  int UnpreparedSixFivePenalty;
  int UnresolvedSixFivePenalty;
  int AugmentedIntervalPenalty;
  int ThirdDoubledPenalty;
  int DoubledLeadingTonePenalty;
  int DoubledSixthPenalty;
  int DoubledFifthPenalty;
  int TripledBassPenalty;
  int UpperVoicesTooFarApartPenalty;
  int UnresolvedLeadingTonePenalty;
  int AllVoicesSkipPenalty;
  int DirectToTritonePenalty;
  int CrossBelowBassPenalty;
  /* I added the following during the translation to C */
  int CrossAboveCantusPenalty;
  int NoMotionAgainstOctavePenalty;
  int SpecialSpeciesCheck(int Cn, int Cp, int v, int Other0, int Other1, int Other2, int NumParts,
                          int Species, int MelInt, int Interval, int ActInt, int LastIntClass, int Pitch, int LastMelInt, int CurLim);
  enum
    {
      INTERVALS_WITH_BASS_SIZE = 8
    };
  int IntervalsWithBass[INTERVALS_WITH_BASS_SIZE];
  /* 0 = octave, 2 = step, 3 = third, 4 = fourth, 5 = fifth, 6 = sixth, 7 = seventh */
  void AddInterval(int n);
  int OtherVoiceCheck(int Cn, int Cp, int v, int NumParts, int Species, int CurLim);
  int Check(int Cn, int Cp, int v, int NumParts, int Species, int CurLim);
  int BestFitPenalty,MaxPenalty,Branches,AllDone;
  float PenaltyRatio;
  enum
    {
      NumFields = 16,
      Field = (MostVoices_+1),
      EndF = (Field*NumFields)
    };
  int SaveIndx(int indx, int *Sp);
  void SaveResults(int CurrentPenalty, int Penalty, int v1, int Species);
#if !defined(SWIG)
  static int Indx[17];
#endif
  int Look(int CurPen, int CurVoice, int NumParts, int Species, int Lim, int *Pens, int *Is, int *CurNotes);
  void BestFitFirst(int CurTime, int CurrentPenalty, int NumParts, int Species, int BrLim);
  void FillRhyPat();
  float RANDOM(float amp);
  void UsedRhy(int n);
  int CurRhy(int n);
  void CleanRhy();
  int GoodRhy();
  void counterpoint(int OurMode, int *StartPitches, int CurV, int CantusFirmusLength, int Species, int *cantus);
  void AnySpecies(int OurMode, int *StartPitches, int CurV, int CantusFirmusLength, int Species);
  void fillCantus(int c0, int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8, int c9, int c10, int c11, int c12, int c13, int c14);
  void toCsoundScore(std::string filename, double secondsPerPulse);
  void winners(int v1, int *data, int *best, int *best1, int *best2, int *durs);
#if !defined(SWIG)
  static std::mt19937 mersenneTwister;
  std::normal_distribution<> uniform_real_generator;
#endif
};

#endif

