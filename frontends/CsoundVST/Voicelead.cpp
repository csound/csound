#include "Voicelead.hpp"

#include <iostream>
#include <ctime>
#include <set>
#include <algorithm>
#include <cmath>

std::ostream &operator << (std::ostream &stream, 
			   const std::vector<double> &chord)
{
  stream << "[";
  for (size_t i = 0, n = chord.size(); i < n; i++) {
    if (i > 0) {
      stream << ", ";
    }
    stream << chord[i];
  }
  stream << "]";
  return stream;
}

namespace csound 
{
  static int debug = 0;

  double round(double x)
  {
    return std::floor(x + 0.5);
  }

  std::vector<double> sort(const std::vector<double> &chord)
  {
    std::vector<double> sorted(chord);
    std::sort(sorted.begin(), sorted.end());
    return sorted;
  }

  double Voicelead::pc(double p, size_t divisionsPerOctave)
  {
    p = std::fabs(round(p));
    return double(int(round(p)) % divisionsPerOctave);
  }

  double Voicelead::numberFromChord(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::set<double> pcs_;
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      pcs_.insert(pc(chord[i], divisionsPerOctave));
    }
    double N = 0;
    for(std::set<double>::iterator it = pcs_.begin(); it != pcs_.end(); ++it) {
      N = N + std::pow(2.0, *it);
    }
    return N;
  }

  std::vector<double> Voicelead::pcsFromNumber(double pcn, size_t divisionsPerOctave)
  {
    size_t n = size_t(round(pcn));
    std::vector<double> pcs;
    for (double i = 0.0; i < double(divisionsPerOctave); i = i + 1.0) {
      size_t p2 = size_t(std::pow(2.0, i));
      if ((p2 & n) == p2) {
	pcs.push_back(i);
      }
    }
    return pcs;
  }

  std::vector<double> Voicelead::voiceleading(const std::vector<double> &a, 
					      const std::vector<double> &b)
  {
    std::vector<double> v(a.size());
    for (size_t i = 0, n = a.size(); i < n; i++) {
      v[i] = (b[i] - a[i]);
    }
    return v;
  }

  const std::vector<double> &Voicelead::simpler(const std::vector<double> &source, 
						const std::vector<double> &destination1, 
						const std::vector<double> &destination2, 
						bool avoidParallels)
  {
    std::vector<double> v1 = voiceleading(source, destination1);
    std::sort(v1.begin(), v1.end());
    std::vector<double> v2 = voiceleading(source, destination2);
    std::sort(v2.begin(), v2.end());    
    for (size_t i = v1.size() - 1; i >= 0; i--) {
      if(v1[i] < v2[i]) {
	return destination1;
      }
      if(v2[i] > v1[i]) {
	return destination2;
      }
    }
    return destination1;
  }                

  double Voicelead::smoothness(const std::vector<double> &a, 
			       const std::vector<double> &b) 
  {
    double L1 = 0.0;
    for (size_t i = 0, n = a.size(); i < n; i++) {
      L1 += std::fabs(b[i] - a[i]);
    }
    return L1;
  }
                
  bool Voicelead::areParallel(const std::vector<double> &a, 
			      const std::vector<double> &b)
  {
    for (size_t i = 0, n = a.size(); i < n; i++) {
      for (size_t j = 0, k = b.size(); j < k; j++) {
	if (i != j) {
	  if ( ((a[i] - a[j]) ==  7.0 && (b[i] - b[j]) ==  7.0) || 
	       ((a[i] - a[j]) == -7.0 && (b[i] - b[j]) == -7.0) ) {
	    if (debug > 1) {
	      std::cout << "Parallel fifth: " << std::endl;
	      std::cout << " chord 1: " << a << std::endl;
	      std::cout << " leading: " << voiceleading(a, b) << std::endl;
	      std::cout << " chord 2: " << b << std::endl;
	    }
	    return true;
	  }
	}
      }
    }
    return false;
  }                
  
  const std::vector<double> &Voicelead::closer(const std::vector<double> &source, 
					       const std::vector<double> &destination1, 
					       const std::vector<double> &destination2, 
					       bool avoidParallels)
  {     
    if (avoidParallels) {
      if (areParallel(source, destination1)) {
	return destination2;
      }
      if (areParallel(source, destination2)) {
	return destination1;
      }
    }
    double s1 = smoothness(source, destination1);
    double s2 = smoothness(source, destination2);
    if (s1 < s2) {
      return destination1;
    }
    if (s2 < s1) {
      return destination2;
    }
    return simpler(source, destination1, destination2, avoidParallels);
  }      

  std::vector<double> Voicelead::rotate(const std::vector<double> &chord)
  {
    std::vector<double> inversion;
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion.push_back(chord[i]);
    }
    inversion.push_back(chord[0]);
    return inversion;
  }

  std::vector< std::vector<double> > Voicelead::rotations(const std::vector<double> &chord)
  {
    std::vector< std::vector<double> > rotations_;
    std::vector<double> inversion = pcs(chord);
    if (debug > 1) {
      std::cout << "rotating:   " << chord << std::endl;
      std::cout << "rotation 1: " << inversion << std::endl;
    }
    rotations_.push_back(inversion);
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion = rotate(inversion);
      if (debug > 1) {
	std::cout << "rotation " << (i+1) << ": " << inversion << std::endl;
      }
      rotations_.push_back(inversion);
    }
    if (debug > 1) {
      std::cout << std::endl;
    }
    return rotations_;
  }
  
  std::vector<double> Voicelead::pcs(const std::vector<double> &chord, size_t divisionsPerOctave) 
  {
    std::vector<double> pcs_(chord.size());
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      pcs_[i] = pc(chord[i], divisionsPerOctave);
    }
    if (debug > 1) {
      std::cout << "chord: " << chord << std::endl;
      std::cout << "pcs: " << pcs_ << std::endl;
    }
    return pcs_;
  }

  const std::vector<double> Voicelead::closest(const std::vector<double> &source, 
						const std::vector< std::vector<double> > &targets,
						bool avoidParallels)
  {
    if (targets.size() == 0) {
      return source;
    } else if (targets.size() == 1) {
      return targets[0];
    }
    std::vector<double> t1 = targets[0];
    for (size_t i = 1, n = targets.size(); i < n; i++) {
      t1 = closer(source, t1, targets[i], avoidParallels);
    }
    return t1;
  }

  void inversions(const std::vector<double> &original, 
		  const std::vector<double> &iterator, 
		  size_t voice, 
		  double range, 
		  std::set< std::vector<double> > &chords, 
		  size_t divisionsPerOctave = 12)
  {
    if (voice >= original.size()) {
      return;
    }
    std::vector<double> iterator_ = iterator;
    for (double pitch = original[voice]; pitch < range; pitch = pitch + divisionsPerOctave) {
      iterator_[voice] = pitch;
      chords.insert(iterator_);
      inversions(original, iterator_, voice + 1, range, chords, divisionsPerOctave);
    }
  }
  
  std::vector< std::vector<double> > Voicelead::voicings(const std::vector<double> &chord, 
							 double lowest, 
							 double range)
  {
    std::vector< std::vector<double> > rotations_ = rotations(chord);
    std::set< std::vector<double> > inversions_;
    for (size_t i = 0, n = rotations_.size(); i < n; i++) {
      std::vector<double> iterator = rotations_[i];
      inversions(rotations_[i], iterator, 0, range, inversions_);
    }
    std::vector< std::vector<double> > inversions__;
    for(std::set< std::vector<double > >::iterator it = inversions_.begin(); it != inversions_.end(); ++it) {
      inversions__.push_back(*it);
    }
    return inversions__;
  }

  /**
   * Bijective voiceleading first by closeness, then by simplicity, 
   * with optional avoidance of parallel fifths.
   */
  std::vector<double> Voicelead::voicelead(const std::vector<double> &source_, 
					   const std::vector<double> &target_, 
					   double lowest, 
					   double range, 
					   bool avoidParallels)
  {
    std::vector<double> source = source_;
    std::vector<double> target = target_;
    std::vector< std::vector<double> > voicings_ = voicings(target, lowest, range);
    std::vector<double> voicing = closest(source, voicings_, avoidParallels);
    if (debug) {
      std::cout << "   From: " << source_ << std::endl;
      std::cout << "     To: " << target_ << std::endl;
      std::cout << "Leading: " << voiceleading(source_, voicing) << std::endl;
      std::cout << "     Is: " << voicing << std::endl << std::endl;
    }
    return voicing;
  } 

  double Voicelead::closestPitch(double pitch, const std::vector<double> &pitches)
  {
    std::vector<double>::const_iterator lowerI = std::lower_bound(pitches.begin(), pitches.end(), pitch);
    if (lowerI == pitches.end()) {
      return pitches.back();
    }
    std::vector<double>::const_iterator upperI = std::upper_bound(pitches.begin(), pitches.end(), pitch);
    if (upperI == pitches.begin()) {
      return pitches.front();
    }
    double lower = *lowerI;
    double upper = *upperI;
    double lowerDifference = std::fabs(pitch - lower);
    double upperDifference = std::fabs(pitch - upper);
    if (lowerDifference < upperDifference) {
      return lower;
    } else {
      return upper;
    }
  }

  double Voicelead::conformToPitchClassSet(double pitch, const std::vector<double> &pcs, size_t divisionsPerOctave)
  {
    double closestPc = closestPitch(Voicelead::pc(pitch, divisionsPerOctave), pcs);
    double octave = round(pitch / divisionsPerOctave) * divisionsPerOctave;
    if (closestPc <= (divisionsPerOctave / 2.0)) {
      return (octave - divisionsPerOctave) + closestPc;
    } else {
      return octave + closestPc;
    }
  }

}

int main(int argc, const char **argv)
{
  clock_t began = std::clock();
  int voiceleadings = 0;
  for (size_t i = 0; i < 100; i++ ){
    std::vector<double> CM7;
    CM7.push_back(60.);
    CM7.push_back(64.);
    CM7.push_back(67.);
    CM7.push_back(71.);
    //CM7.push_back(74.);
    std::vector<double> Dm7;
    Dm7.push_back(62.);
    Dm7.push_back(65.);
    Dm7.push_back(69.);
    Dm7.push_back(72.);
    //Dm7.push_back(74.);
    std::vector<double> G7;
    G7.push_back(67.);
    G7.push_back(71.);
    G7.push_back(74.);
    G7.push_back(77.);
    //G7.push_back(79.);
    std::cout << "CM7:   " << CM7 << std::endl;
    std::cout << "Pcs: " << csound::Voicelead::pcs(CM7) << std::endl;
    std::cout << "Dm7:   " << Dm7 << std::endl;
    std::cout << "Pcs: " << csound::Voicelead::pcs(Dm7) << std::endl;
    std::cout << "G7:    " << G7 << std::endl;
    std::cout << "Pcs: " << csound::Voicelead::pcs(G7) << std::endl;
    std::vector<double> target = csound::Voicelead::voicelead(CM7, Dm7, 0.0, 84.0, true);
    voiceleadings++;
    target = csound::Voicelead::voicelead(target, G7, 36.0, 60.0, true);
    voiceleadings++;
  }
  clock_t ended = std::clock();
  double elapsed = double(ended - began) / double(CLOCKS_PER_SEC);
  double secondsPerVoiceleading = elapsed / double(voiceleadings);
  std::cout << elapsed << " seconds / " << voiceleadings << " voiceleadings = " << secondsPerVoiceleading << " seconds per voiceleading." << std::endl;
  std::vector<double> G7;
  G7.push_back(67.);
  G7.push_back(71.);
  G7.push_back(74.);
  G7.push_back(77.);
  std::vector< std::vector<double> > voicings_ = csound::Voicelead::voicings(csound::Voicelead::pcs(G7), 48.0, 48.0);
  for (size_t i = 0, n = voicings_.size(); i < n; i++) {
    std::cout << "voiding " << (i+1) << ": " << voicings_[i] << std::endl;
  }
  return 0;
}
