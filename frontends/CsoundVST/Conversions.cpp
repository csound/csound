/**
* C S O U N D   V S T 
*
* A VST plugin version of Csound, with Python scripting.
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
#include "Conversions.hpp"
#include <strstream>

#ifdef WIN32
extern "C"
{
    extern __stdcall void OutputDebugStringA(const char *text);
};
#endif

namespace csound
{
	std::map<std::string, double> Conversions::pitchClassSetsForNames;
	std::map<double, std::string> Conversions::namesForPitchClassSets;
	const double Conversions::PI_ = atan(1.0) * 4.0;
	const double Conversions::TWO_PI_ = atan(1.0) * 8.0;
	const double Conversions::middleCHz = 261.6256;
	const double Conversions::log10d20 = log(10.0) / 20.0;
	const double Conversions::log10scale = 1.0 / log(10.0);
	const double Conversions::NORM_7_ = 1.0 / 127.0;
	const double Conversions::floatMaximumAmplitude = 32767.0; // 2353970.0;
	int Conversions::sampleSize = 4;	
	bool Conversions::initialized_ = false;
	static bool initialized__ = Conversions::initialize();

	void Conversions::subfill(std::string root, char *cname, double cpcs)
	{
			double pcs = pitchClassSetsForNames[root];
			root.append(cname);
			double fcpcs = pcs * cpcs;
			double mcpcs = std::fmod(fcpcs, 4095.0);
#ifdef WIN32
			char buffer[0xff];
			sprintf(buffer, "%s: pcs = %f cpcs = %f fcpcs = %f mcpcs = %f\n", root.c_str(), pcs, cpcs, fcpcs, mcpcs);
			OutputDebugStringA(buffer);
#endif
			pitchClassSetsForNames[root] = mcpcs;
	}

	void Conversions::fill(char *cname, char *cpitches_)
	{
		double cpcs = 0.0;
		char separators[] = " ";
		char *cpitches = strdup(cpitches_);
		char *token = strtok(cpitches, separators);
		while(token)
		{
			double pcs = pitchClassSetsForNames[token];
			cpcs = cpcs + pcs;
			token = strtok(0, separators);
		}
		cpcs = std::fmod(cpcs, 4095.0);
		subfill("C",  cname, cpcs);
		subfill("C#", cname, cpcs);
		subfill("Db", cname, cpcs);
		subfill("D",  cname, cpcs);
		subfill("D#", cname, cpcs);
		subfill("Eb", cname, cpcs);
		subfill("E",  cname, cpcs);
		subfill("F",  cname, cpcs);
		subfill("F#", cname, cpcs);
		subfill("Gb", cname, cpcs);
		subfill("G",  cname, cpcs);
		subfill("G#", cname, cpcs);
		subfill("Ab", cname, cpcs);
		subfill("A",  cname, cpcs);
		subfill("A#", cname, cpcs);
		subfill("Bb", cname, cpcs);
		subfill("B",  cname, cpcs);
		free(cpitches);
	}

	bool Conversions::initialize()
	{
		if(!initialized_)
		{
			pitchClassSetsForNames["C" ] = std::pow(2.0,  0.0);
			pitchClassSetsForNames["C#"] = std::pow(2.0,  1.0);
			pitchClassSetsForNames["Db"] = std::pow(2.0,  1.0);
			pitchClassSetsForNames["D" ] = std::pow(2.0,  2.0);
			pitchClassSetsForNames["D#"] = std::pow(2.0,  3.0);
			pitchClassSetsForNames["Eb"] = std::pow(2.0,  3.0);
			pitchClassSetsForNames["E" ] = std::pow(2.0,  4.0);
			pitchClassSetsForNames["F" ] = std::pow(2.0,  5.0);
			pitchClassSetsForNames["F#"] = std::pow(2.0,  6.0);
			pitchClassSetsForNames["Gb"] = std::pow(2.0,  6.0);
			pitchClassSetsForNames["G" ] = std::pow(2.0,  7.0);
			pitchClassSetsForNames["G#"] = std::pow(2.0,  8.0);
			pitchClassSetsForNames["Ab"] = std::pow(2.0,  8.0);
			pitchClassSetsForNames["A" ] = std::pow(2.0,  9.0);
			pitchClassSetsForNames["A#"] = std::pow(2.0, 10.0);
			pitchClassSetsForNames["Bb"] = std::pow(2.0, 10.0);
			pitchClassSetsForNames["B" ] = std::pow(2.0, 11.0);
			listPitchClassSets();
			for(std::map<std::string, double>::const_iterator it = pitchClassSetsForNames.begin(); it != pitchClassSetsForNames.end(); ++it)
			{
				namesForPitchClassSets[it->second] = it->first;
			}
			listPitchClassSets();
			// Scales.
			fill(" major",            "C     D     E  F     G     A     B");
			fill(" minor",            "C     D  Eb    F     G  Ab    Bb  ");
			fill(" natural minor",    "C     D  Eb    F     G  Ab    Bb  ");
			fill(" harmonic minor",   "C     D  Eb    F     G  Ab       B");
			fill(" chromatic",        "C  C# D  D# E  F  F# G  G# A  A# B");
			fill(" whole tone",       "C     D     E     F#    G#    A#  ");
			fill(" diminished",       "C     D  D#    F  F#    G# A     B");
			fill(" pentatonic",       "C     D     E        G     A      ");
			fill(" pentatonic major", "C     D     E        G     A      ");
			fill(" pentatonic minor", "C        Eb    F     G        Bb  ");
			fill(" augmented",        "C        Eb E        G  Ab    Bb  ");
			fill(" Lydian dominant",  "C     D     E     Gb G     A  Bb  ");
			fill(" 3 semitone",       "C        D#       F#       A      ");
			fill(" 4 semitone",       "C           E           G#        ");
			fill(" blues",            "C     D  Eb    F  Gb G        Bb  ");
			fill(" bebop",            "C     D     E  F     G     A  Bb B");
			// Major chords.
			fill("M",                 "C           E        G            ");
			fill("6",                 "C           E        G     A      ");
			fill("69",                "C     D     E        G     A      ");
			fill("69b5",              "C     D     E     Gb       A      ");
			fill("M7",                "C           E        G           B");
			fill("M9",                "C     D     E        G           B");
			fill("M11",               "C     D     E  F     G           B");
			fill("M13",               "C     D     E  F     G     A     B");
			// Minor chords.
			fill("m",                 "C        Eb          G            ");
			fill("m6",                "C        Eb          G     A      ");
			fill("m69",               "C     D  Eb          G     A      ");
			fill("m7",                "C        Eb          G        Bb  ");
			fill("m#7",               "C        Eb          G           B");
			fill("m7b5",              "C        Eb       Gb          Bb  ");
			fill("m9",                "C     D  Eb          G        Bb  ");
			fill("m9#7",              "C     D  Eb          G           B");
			fill("m11",               "C     D  Eb    F     G        Bb  ");
			fill("m13",               "C     D  Eb    F     G     A  Bb  ");
			// Augmented chords.
			fill("+",                 "C            E         G#         ");
			fill("7#5",               "C            E         G#     Bb  ");
			fill("7b9#5",             "C  Db        E         G#     Bb  ");
			fill("9#5",               "C     D      E         G#     Bb  ");
			// Diminished chords.
			fill("o",                 "C        Eb       Gb              ");
			fill("o7",                "C        Eb       Gb       A      ");
			// Suspended chords.
			fill("6sus",              "C              F     G     A      ");
			fill("69sus",             "C     D        F     G     A      ");
			fill("7sus",              "C              F     G        Bb  ");
			fill("9sus",              "C     D        F     G        Bb  ");
			fill("M7sus",             "C              F     G           B");
			fill("M9sus",             "C     D        F     G           B");
			// Dominant chords.
			fill("7",                 "C            E       G        Bb  ");
			fill("7b5",               "C            E    Gb          Bb  ");
			fill("7b9",               "C  Db        E       G        Bb  ");
			fill("7b9b5",             "C  Db        E    Gb          Bb  ");
			fill("9",                 "C            E       G        Bb  ");
			fill("11",                "C     D      E F     G        Bb  ");
			fill("13",                "C     D      E F     G     A  Bb  ");
			for(std::map<std::string, double>::iterator it = pitchClassSetsForNames.begin(); it != pitchClassSetsForNames.end(); ++it)
			{
				namesForPitchClassSets[it->second] = it->first;
			}
			initialized_ = true;
		}
		return initialized_;
	}

	double Conversions::getPI(void) 
	{
		return PI_;
	}
	double Conversions::get2PI(void)
	{
		return TWO_PI_;
	}
	double Conversions::getMiddleCHz(void)
	{
		return middleCHz;
	}
	double Conversions::getNORM_7(void)
	{
		return NORM_7_;
	}
	/**
	* Returns the maximum soundfile amplitude for the sample size.
	*/
	int Conversions::getSampleSize(void)
	{
		return sampleSize;
	}
	/**
	* Returns the maximum soundfile amplitude for the sample size,
	* assuming either float or twos' complement integer samples.
	*/
	double Conversions::getMaximumAmplitude(int size)
	{
		double amplitude = 0;
		if(size == 4)
		{
			amplitude = 1.0;			
		}
		else
		{
			amplitude = pow(2.0, (size * 8) - 1) - 1;
		}
		return amplitude;
	}
	double Conversions::getMaximumDynamicRange()
	{
		if(sampleSize == 4)
		{
			return 127.0;
		}
		return log(getMaximumAmplitude(sampleSize) * 2.0) / log(2.0) * 6.0;
	}
	double Conversions::amplitudeToDecibels(double amplitude)
	{
		if(sampleSize == 4)
		{
			amplitude *= floatMaximumAmplitude;
		}
		return log(fabs(amplitude)) / log10d20;
	}
	double Conversions::amplitudeToGain(double Amplitude)
	{
		if(sampleSize == 4)
		{
			return fabs(Amplitude);
		}
		return fabs(Amplitude) / getMaximumAmplitude(sampleSize);
	}
	double Conversions::decibelsToAmplitude(double decibels)
	{
		if(sampleSize == 4)
		{
			return exp(decibels * log10d20) / floatMaximumAmplitude;
		}
		return exp(decibels * log10d20);
	}
	double Conversions::decibelsToMidi(double decibels)
	{
		if(sampleSize == 4)
		{
			return decibels;
		}
		return decibels / getMaximumDynamicRange() * 127.0;
	}
	double Conversions::gainToAmplitude(double Gain)
	{
		if(sampleSize == 4)
		{
			return Gain;
		}
		return Gain * getMaximumAmplitude(sampleSize);
	}
	double Conversions::midiToDecibels(double Midi)
	{
		if(sampleSize == 4)
		{
			return Midi;
		}
		return (Midi / 127.0) * getMaximumDynamicRange();
	}
	double Conversions::midiToAmplitude(double Midi)
	{
		return decibelsToAmplitude(midiToDecibels(Midi));
	}
	double Conversions::amplitudeToMidi(double Amplitude)
	{
		return decibelsToMidi(amplitudeToDecibels(Amplitude));
	}
	double Conversions::midiToGain(double Midi)
	{
		return Midi / 127.0;
	}
	double Conversions::leftPan(double x)
	{
		double theta = PI_ * (x / 4.0);
		return(sqrt(2.0) / 2.0) * (cos(theta) + sin(theta));
	}
	const double Conversions::round(double value)
	{
		return floor(value + 0.5);
	}
	double Conversions::temper(double octave, double tonesPerOctave)
	{
		if(tonesPerOctave == 0)
		{
			return octave;
		}
		return round(octave * tonesPerOctave) / tonesPerOctave;
	}
	double Conversions::phaseToTableLengths(double Phase, double TableSampleCount)
	{
		return TableSampleCount * Phase / TWO_PI_;
	}
	double Conversions::hzToMidi(double Hz, bool rounded)
	{
		return octaveToMidi(hzToOctave(Hz), rounded);
	}
	double Conversions::hzToOctave(double Hz)
	{
		return log(Hz / middleCHz) / log(2) + 8.0;
	}
	double Conversions::hzToSamplingIncrement(double Hz, double SR, double SamplesPerCycle)
	{
		return(SamplesPerCycle * Hz) / SR;
	}
	double Conversions::midiToHz(double Midi)
	{
		return octaveToHz(midiToOctave(Midi));
	}
	double Conversions::midiToOctave(double Midi)
	{
		return Midi / 12.0 + 3.0;
	}
	double Conversions::midiToSamplingIncrement(double Midi, double SR, double SamplesPerCycle)
	{
		return hzToSamplingIncrement(midiToHz(Midi), SR, SamplesPerCycle);
	}
	double Conversions::octaveToHz(double Octave)
	{
		return middleCHz * pow(2, Octave - 8.0);
	}
	double Conversions::octaveToMidi(double Octave, bool rounded)
	{
		if(rounded)
		{
			return round(Octave * 12.0 - 36.0);
		}
		return Octave * 12.0 - 36.0;
	}
	double Conversions::octaveToSamplingIncrement(double Octave, double SR, double SamplesPerCycle)
	{
		return hzToSamplingIncrement(octaveToHz(Octave), SR, SamplesPerCycle);
	}
	double Conversions::rightPan(double x)
	{
		double theta = PI_ * (x / 4.0);
		return(sqrt(2.0) / 2.0) * (cos(theta) - sin(theta));
	}
	int Conversions::swapInt(int Source)
	{
		int returnValue;
		returnValue = ((Source & 0x000000ff) << 24);
		returnValue +=((Source & 0x0000ff00) << 8);
		returnValue +=((Source & 0x00ff0000) >> 8);
		returnValue +=((Source & 0xff000000) >> 24);
		return returnValue;
	}
	short Conversions::swapShort(short Source)
	{
		int returnValue;
		returnValue |=((Source & 0x00ff) << 8);
		returnValue = ((Source & 0xff00) >> 8);
		return(short) returnValue;
	}
	bool Conversions::stringToBool(std::string value)
	{
		switch(value[0])
		{
		case '1':
		case 'T':
		case 't':
			return true;
		}
		return false;
	}
	std::string Conversions::boolToString(bool value)
	{
		if(value)
		{
			return "True";
		}
		else
		{
			return "False";
		}
	}
	int Conversions::stringToInt(std::string value)
	{
		return atoi(value.c_str());
	}
	std::string Conversions::intToString(int value)
	{
		char buffer[0xff];
		sprintf(buffer, "%d", value);
		return buffer;
	}
	double Conversions::stringToDouble(std::string value)
	{
		return atof(value.c_str());
	}
	std::string Conversions::doubleToString(double value)
	{
		char buffer[0xff];
		sprintf(buffer, "%f", value);
		return buffer;
	}
	double Conversions::midiToPitchClass(double midiKey)
	{
		return int(round(midiKey)) % 12;
	}
	double Conversions::pitchClassToMidi(double pitchClass)
	{
		return int(round(pitchClass));
	}
	double Conversions::pitchClassSetToMidi(double pitchClassSet)
	{
		if(pitchClassSet == 0.0)
		{
			return pitchClassSet;
		}
		else
		{
			return log(pitchClassSet) / log(2.0);
		}
	}
	double Conversions::midiToPitchClassSet(double midiKey)
	{
		double pitchClass = midiToPitchClass(midiKey);
		return pow(2.0, pitchClass);
	}
	double Conversions::findClosestPitchClass(double pitchClassSet, double midiKeyOrPitchClass, double divisionsPerOctave)
	{
		int roundedTargetPitchClassSet = (int) round(pitchClassSet);
		int roundedPitchClass = (int) midiToPitchClass(midiKeyOrPitchClass);
		int lowerPitchClass = 0;
		int upperPitchClass = 0;
		int temporaryPitchClass;
		int temporaryPitchClassSet;
		for(temporaryPitchClass = roundedPitchClass; temporaryPitchClass < divisionsPerOctave; temporaryPitchClass++)
		{
			temporaryPitchClassSet = (int) midiToPitchClassSet(temporaryPitchClass);
			if((roundedTargetPitchClassSet & temporaryPitchClassSet) == temporaryPitchClassSet)
			{
				upperPitchClass = temporaryPitchClass;
				break;
			}
		}
		for(temporaryPitchClass = roundedPitchClass; temporaryPitchClass >= 0; temporaryPitchClass--)
		{
			temporaryPitchClassSet = (int) midiToPitchClassSet(temporaryPitchClass);
			if((roundedTargetPitchClassSet & temporaryPitchClassSet) == temporaryPitchClassSet)
			{
				lowerPitchClass = temporaryPitchClass;
				break;
			}
		}
		int deltaLower = abs(temporaryPitchClass - lowerPitchClass);
		int deltaUpper = abs(upperPitchClass - temporaryPitchClass);
		if(deltaLower < deltaUpper)
		{
			return lowerPitchClass;
		}
		else
		{
			return upperPitchClass;
		}
	}
	double Conversions::midiToRoundedOctave(double midiKey)
	{
		return round(midiToOctave(midiKey));
	}
	std::string &Conversions::trim(std::string &value)
	{
		size_t i = value.find_first_not_of(" \n\r\t");
		if(i != value.npos)
		{
			value.erase(0, i);
		}
		i = value.find_last_not_of(" \n\r\t");
		if(i != value.npos)
		{
			value.erase(i + 1, value.npos);
		}
		return value;
	}
	std::string &Conversions::trimQuotes(std::string &value)
	{
		size_t i = value.find_first_not_of("\"");
		if(i != value.npos)
		{
			value.erase(0, i);
		}
		i = value.find_last_not_of("\"");
		if(i != value.npos)
		{
			value.erase(i + 1, value.npos);
		}
		return value;
	}
	double Conversions::modulus(double a, double b)
	{
		while (a >= b)
		{
			a -= b;
		}
		while (a < 0)
		{
			a += b;
		}
		return a;
	}

	double Conversions::nameToPitchClassSet(std::string name)
	{
		if(pitchClassSetsForNames.find(name) != pitchClassSetsForNames.end())
		{
			return pitchClassSetsForNames[name];
		}
		else
		{
			return -1.0;
		}
	}

	std::string Conversions::pitchClassSetToName(double pitchClassSet)
	{
		if(namesForPitchClassSets.find(pitchClassSet) != namesForPitchClassSets.end())
		{
			return namesForPitchClassSets[pitchClassSet];
		}
		else
		{
			return "Not found.";
		}
	}

	std::string Conversions::listPitchClassSets()
	{
		std::strstream stream;
		for(std::map<std::string, double>::const_iterator it = pitchClassSetsForNames.begin(); it != pitchClassSetsForNames.end(); ++it)
		{
			stream << it->first << " = " << it->second << "\r\n";
#ifdef WIN32
			{
				char buffer[0xff];
				sprintf(buffer, "%s = %f\n", it->first.c_str(), it->second);
				OutputDebugStringA(buffer);
			}
#endif
		}
		return stream.str();
	}

}
