#include "CounterpointNode.hpp"
#include "System.hpp"

namespace csound
{
  CounterpointNode::CounterpointNode() :  generationMode(GenerateCounterpoint),
					  musicMode(Counterpoint::Aeolian),
					  species(Counterpoint::Two),
					  voices(2),
					  secondsPerPulse(0.5)
  {
    FillRhyPat();
    Counterpoint::messageCallback = System::getMessageCallback();
  }

  CounterpointNode::~CounterpointNode()
  {
  }

  void CounterpointNode::produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &globalCoordinates)
  {
    // Make a local copy of the child notes.
    Score source;
    source.insert(source.begin(), score.begin() + beginAt, score.begin() + endAt);
    System::message("Original source notes: %d\n", source.size());
    // Remove the child notes from the target.
    score.erase(score.begin() + beginAt, score.begin() + endAt);
    // Select the cantus firmus.
    source.sort();
    std::vector<int> cantus;
    std::vector<int> voicebeginnings(voices);
    // Take notes in sequence, quantized on time, as the cantus.
    // If there are chords, pick the best fitting note in the chord and discard the others.    
    std::vector< std::vector<int> > chords;
    double time = -1;
    double oldTime = 0;
    for (size_t i = 0; i < source.size(); i++)
      {
	oldTime = time;
	time = std::floor((source[i].getTime() / secondsPerPulse) + 0.5) * secondsPerPulse;
	if (oldTime != time) 
	  {
	    std::vector<int> newchord;
	    chords.push_back(newchord);
	  }
	chords.back().push_back(int(source[i].getKey()));
      }
    for(size_t i = 0, n = chords.size(); i < n; i++)
      {
	int bestfit = chords[i].front();
	int oldDifference = 0;
	for(size_t j = 1; j < chords[i].size(); j++)
	  {
	    int difference = std::abs(bestfit - chords[i][j]);
	    oldDifference = difference;
	    if (difference > 0 && difference < oldDifference)
	      {
		bestfit = chords[i][j];
	      }
	  }
	cantus.push_back(bestfit);
      }
    System::message("Cantus firmus notes: %d\n", source.size());
    // For now, just space out the initial pitches.
    int range = HighestSemitone - LowestSemitone;
    int voicerange = range / (voices + 1);
    System::message("Cantus begins at key %d\n", cantus[0]);
    for (size_t i = 0; i < voices; i++)
      {
	voicebeginnings[i] = cantus[0] + ((i + 1) * voicerange);
	System::message("Voice %d begins at key %d\n", (i + 1), voicebeginnings[i]);
      }
    // Generate the counterpoint.
    counterpoint(musicMode, &voicebeginnings[0], voices, cantus.size(), species, &cantus[0]);
    // Translate the counterpoint back to a Score.
    double duration = 0;
    double key = 0;
    double velocity = 70;
    double phase = 0;
    double x = 0;
    double y = 0;
    double z = 0;
    double pcs = 4095.0;
    Score generated;
    for(size_t voice = 0; voice < Ctrpt.size2(); voice++)
      {
	double time = 0;
	for(int note = 1; note <= TotalNotes[voice]; note++)
	  {
	    time = Onset[note][voice] * secondsPerPulse;
	    duration = Dur[note][voice] * secondsPerPulse;
	    key = double(Ctrpt[note][voice]);
	    generated.append(time, duration, double(144), double(voice), key, velocity, phase, x, y, z, pcs);
	    System::message("%f %f %f %f %f %f %f %f %f %f %f\n", time, duration, double(144), double(voice), key, velocity, phase, x, y, z, pcs);
	  }
      }
    // Get the right coordinate system going.
    ublas::matrix<double> local = getLocalCoordinates();
    ublas::matrix<double> compositeCoordinates = ublas::prod(local, globalCoordinates);
    System::message("Total notes in generated counterpoint: %d\n", generated.size());
    for (size_t i = beginAt; i < endAt; i++)
      {
	generated[i] = ublas::prod(compositeCoordinates, generated[i]);
      }
    // Put the generated counterpoint (back?) into the target score.
    score.insert(score.end(), generated.begin(), generated.end());
  }
}
