/*
 * C S O U N D
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef CONVERSIONS_H
#define CONVERSIONS_H

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include <cmath>
#include <string>
#include <cstdio>
#include <map>
#include <vector>
%}
%include "std_string.i"
%include "std_vector.i"
%template(DoubleVector) std::vector<double>;
#else
#include <cmath>
#include <string>
#include <cstdio>
#include <map>
#include <vector>
#endif

namespace csound
{
/**
 * Conversions to and from various music and signal processing units.
 * Note that:
 * silence::Event represents loudness in MIDI units (0 to 127).
 * silence::Orchestra represents loudness in gain (0 to 1).
 * silence::WaveSoundfileOut represents loudness in amplitude (0 to 1 for float samples, 0 to 32767 for short samples).
 * Loudness can also be represented in positive decibels (0 to 84 for short samples, 0 to whatever for float samples).
 * For float samples, decibels are assumed to be equivalent to MIDI velocity;
 * otherwise, MIDI velocity is rescaled according to the maximum dynamic range supported by the sample size.
 * All loudness conversions are driven by sample word size, which must be set before use;
 * the default is 4 (float samples).
 */
class SILENCE_PUBLIC Conversions
{
    static bool initialized_;
    static std::map<std::string, double> pitchClassSetsForNames;
    static std::map<double, std::string> namesForPitchClassSets;
    static void subfill(std::string root, const char *cname, double pcs);
    static void fill(const char *cname, const char *cpitches);
    static std::string listPitchClassSets();
#if !defined(SWIG)
    static const double PI_;
    static const double TWO_PI_;
    static const double middleCHz;
    static const double log10d20;
    static const double log10scale;
    static const double NORM_7_;
    static const double floatMaximumAmplitude;
    static int sampleSize;
#endif
public:
    static double getPI(void);
    static double get2PI(void);
    static double getMiddleCHz(void);
    static double getNORM_7(void);
    static bool initialize();
    /**
     * Returns the maximum soundfile amplitude for the sample size.
     */
    static int getSampleSize(void);
    /**
     * Returns the maximum soundfile amplitude for the sample size,
     * assuming either float or twos' complement integer samples.
     */
    static double getMaximumAmplitude(int size);
    static double getMaximumDynamicRange();
    static double amplitudeToDecibels(double amplitude);
    static double amplitudeToGain(double Amplitude);
    static double decibelsToAmplitude(double decibels);
    static double decibelsToMidi(double decibels);
    static double gainToAmplitude(double Gain);
    static double midiToDecibels(double Midi);
    static double midiToAmplitude(double Midi);
    static double amplitudeToMidi(double Amplitude);
    static double midiToGain(double Midi);
    static double leftPan(double x);
    static double round(double value);
    static double temper(double octave, double tonesPerOctave);
    static double phaseToTableLengths(double Phase, double TableSampleCount);
    static double hzToMidi(double Hz, bool rounded);
    static double hzToOctave(double Hz);
    static double hzToSamplingIncrement(double Hz, double SR, double SamplesPerCycle);
    static double midiToHz(double Midi);
    static double midiToOctave(double Midi);
    static double midiToSamplingIncrement(double Midi, double SR, double SamplesPerCycle);
    static double octaveToHz(double Octave);
    static double octaveToMidi(double Octave, bool rounded);
    static double octaveToSamplingIncrement(double Octave, double SR, double SamplesPerCycle);
    static double rightPan(double x);
    static int swapInt(int Source);
    static short swapShort(short Source);
    /**
     * Translate the string value to a boolean value,
     * returning the default if the string value is empty.
     */
    static bool stringToBool(std::string value, bool default_=false);
    static std::string boolToString(bool value);
    /**
     * Translate the string value to an integer value,
     * returning the default if the string value is empty.
     */
    static int stringToInt(std::string value, int default_=0);
    static std::string intToString(int value);
    /**
     * Translate the string value to a double-precision value,
     * returning the default if the string value is empty.
     */
    static double stringToDouble(std::string value, double default_=0.0);
    static std::string doubleToString(double value);
    /**
     * Parses text in the format "n,..,n" to a vector of doubles.
     */
    static void stringToVector(const std::string &text, std::vector<double> &vector);
    static double midiToPitchClass(double midiKey);
    static double pitchClassSetToMidi(double pitchClassSet);
    static double midiToPitchClassSet(double midiKey);
    static double pitchClassToMidi(double pitchClass);
    /**
     * Given the pitch-class set number M = sum over pitch-classes of (2 ^ pitch-class),
     * return the pitch-class in the set that is closest to the argumen pitch-class.
     */
    static double findClosestPitchClass(double M, double pitchClass, double tones = 12.0);
    static double midiToRoundedOctave(double midiKey);
    static std::string &trim(std::string &value);
    static std::string &trimQuotes(std::string &value);
    /**
     * True modulus accounting for sign.
     */
    static double modulus(double a, double b);
    /**
     * Return the pitches for a chord name.
     */
    static std::vector<double> nameToPitches(std::string name);
    /**
     * Return the pitch-class set number M = sum over pitch-classes of (2 ^ pitch-class)
     * for the jazz-style scale or chord name. These numbers form a multiplicative
     * monoid for all pitch-class sets in a system of equal temperament.
     */
    static double nameToM(std::string name);
    /**
     * Return the jazz-style scale or chord name for
     * the pitch-class set number M = sum over pitch-classes of (2 ^ pitch-class)
     * These numbers form a multiplicative monoid
     * for all pitch-class sets in a system of equal temperament.
     */
    static std::string mToName(double pitchClassSet);
    /**
     * Return a new copy of a "C" string allocated on the heap.
     * The user is responsible for freeing the copy.
     */
    static char *dupstr(const char *string);
    /**
     * Return a new value in dB that represents
     * the input value in dB adjusted by the specified gain.
     * If odbfs is false (the default), then
     * 0 dB is the threshold of hearing;
     * otherwise, 0 dB is full scale.
     */
    static double gainToDb(double inputDb, double gain, bool odbfs = false);
    static double EPSILON();
    static double &epsilonFactor();
    static bool eq_epsilon(double a, double b);
    static bool gt_epsilon(double a, double b);
    static bool lt_epsilon(double a, double b);
    static bool ge_epsilon(double a, double b);
    static bool le_epsilon(double a, double b);
};
}

#endif
