#include "Voicelead.hpp"

#include <iostream>
#include <ctime>

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

  double pc(double p)
  {
    p = std::fabs(p);
    return int(round(p)) % 12;
  }
        
  std::vector<double> voiceleading(const std::vector<double> &a, 
				   const std::vector<double> &b)
  {
    std::vector<double> v;
    for (size_t i = 0, n = a.size(); i < n; i++) {
      v.push_back(b[i] - a[i]);
    }
    return v;
  }

  const std::vector<double> &simpler(const std::vector<double> &source, 
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

  double smoothness(const std::vector<double> &a, 
		    const std::vector<double> &b) 
  {
    double L1 = 0.0;
    for (size_t i = 0, n = a.size(); i < n; i++) {
      L1 += std::fabs(b[i] - a[i]);
    }
    return L1;
  }
                
  bool areParallel(const std::vector<double> &a, 
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
  
  const std::vector<double> &closer(const std::vector<double> &source, 
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

  std::vector<double> rotate(const std::vector<double> &chord)
  {
    std::vector<double> inversion;
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion.push_back(chord[i]);
    }
    inversion.push_back(chord[0]);
    return inversion;
  }

  void rotations(std::vector<double> chord, 
		 std::vector< std::vector<double> > &rotations_)
  {
    rotations_.clear();
    std::vector<double> inversion = tones(chord);
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
    std::cout << std::endl;
  }
  
  std::vector<double> tones(const std::vector<double> &chord) 
  {
    std::vector<double> tones_(chord.size());
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      tones_[i] = pc(chord[i]);
    }
    if (debug > 1) {
      std::cout << "chord: " << chord << std::endl;
      std::cout << "tones: " << tones_ << std::endl;
    }
    return tones_;
  }

  std::vector<double> sort(const std::vector<double> &chord)
  {
    std::vector<double> sorted(chord);
    std::sort(sorted.begin(), sorted.end());
    return sorted;
  }

  void inversions_(const std::vector<double> &tones, 
		   std::vector<double> &iterating_chord, 
		   size_t voice, 
		   std::set< std::vector<double> > &inversions__, 
		   double range)
  {
    if (voice >= tones.size()) {
      return;
    }
    for(double p = pc(iterating_chord[voice]); p <= range; p+= 12.0) {
      iterating_chord[voice] = p;
      std::vector<double> sorted = sort(iterating_chord);
      if (debug > 1) {
	std::cout << "iterating " << sorted << std::endl;
      }
      inversions__.insert(sorted);
      inversions_(tones, iterating_chord, voice + 1, inversions__, range);
    }
  }

  void inversions(const std::vector<double> &chord, 
		  std::set< std::vector<double> > &inversions__, 
		  double range)
  {
    std::vector<double> tones_ = tones(chord);
    std::vector< std::vector<double> > iterating_chords;
    rotations(tones_, iterating_chords);
    inversions__.clear();
    for(size_t i = 0, n = iterating_chords.size(); i < n; i++) {
      size_t voice = 0;
      std::vector<double> iterating_chord = iterating_chords[i];
      inversions_(tones_, iterating_chord, voice, inversions__, range);
    }
  }
  
  const std::vector<double> &closest(const std::vector<double> &source, 
				     const std::set< std::vector<double> > &destinations, 
				     bool avoidParallels)
  {
    std::set< std::vector<double> >::const_iterator it = destinations.begin();
    std::vector<double> &d1 = const_cast< std::vector<double> &>(*it);
    for (++it; it != destinations.end(); ++it) {
      const std::vector<double> &d2 = *it;
      d1 = closer(source, d1, d2, avoidParallels);
    }
    return d1;
  }
  
  std::vector< std::vector<double> > voicings(const std::vector<double> &chord, 
					      double lowest, 
					      double range)
  {
    std::vector<double> source = chord;
    for (size_t i = 0, n = source.size(); i < n; i++) {
      source[i] = source[i] - lowest;
    }
    std::set< std::vector<double> > inversions_;
    inversions(source, inversions_, range);
    std::vector< std::vector<double> > invs;
    for (std::set< std::vector<double> >::iterator it = inversions_.begin(); it != inversions_.end(); ++it) {
      invs.push_back(*it);
    }
    for (size_t i = 0, n = invs.size(); i < n; i++) {
      for (size_t j = 0, k = invs[i].size(); j < k; j++) {
	invs[i][k] += lowest;
      }
    }
    return invs;
  }

  /**
   * Bijective voiceleading first by closeness, then by simplicity, 
   * with optional avoidance of parallel fifths.
   */
  std::vector<double> voicelead(const std::vector<double> &source_, 
				const std::vector<double> &target_, 
				double lowest, 
				double range, 
				bool avoidParallels)
  {
    std::vector<double> source = source_;
    std::vector<double> target = target_;
    for (size_t i = 0, n = source.size(); i < n; i++) {
      source[i] = source[i] - lowest;
      target[i] = target[i] - lowest;
    }
    std::set< std::vector<double> > inversions_;
    inversions(target, inversions_, range);
    std::vector<double> targetChord = closest(source, inversions_, avoidParallels);
    for (size_t i = 0, n = source.size(); i < n; i++) {
      targetChord[i] = targetChord[i] + lowest;
      target[i] = target[i] + lowest;
    }
    if (debug) {
      std::cout << "   From: " << source_ << std::endl;
      std::cout << "     To: " << target_ << std::endl;
      std::cout << "Leading: " << voiceleading(source_, targetChord) << std::endl;
      std::cout << "     Is: " << targetChord << std::endl << std::endl;
    }
    return targetChord;
  } 

#ifndef TEST
  std::vector<Event> voiceleadEvents(const std::vector<Event> &source,
				     const std::vector<Event> &target,
				     double lowest,
				     double range,
				     bool avoidParallels)
  {
    std::vector<double> s;
    std::vector<double> t;
    for (size_t i = 0, n = source.size(); i < n; i++) {
      s.push_back(source[i].getKey());
      t.push_back(target[i].getKey());
    }
    std::vector<double> tc = voicelead(s, t, lowest, range, avoidParallels);
    std::vector<Event> tec;
    for (size_t i = 0, n = source.size(); i < n; i++) {
      tec.push_back(source[i]);
      tec[i].setKey(tc[i]);
    }
    return tec;
  }
#endif

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
    std::cout << "Tones: " << csound::tones(CM7) << std::endl;
    std::cout << "Dm7:   " << Dm7 << std::endl;
    std::cout << "Tones: " << csound::tones(Dm7) << std::endl;
    std::cout << "G7:    " << G7 << std::endl;
    std::cout << "Tones: " << csound::tones(G7) << std::endl;
    std::vector<double> target = csound::voicelead(CM7, Dm7, 0.0, 84.0, true);
    voiceleadings++;
    target = csound::voicelead(target, G7, 36.0, 60.0, true);
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
  std::vector< std::vector<double> > voicings_ = csound::voicings(csound::tones(G7), 48.0, 48.0);
  for (size_t i = 0, n = voicings_.size(); i < n; i++) {
    std::cout << "voiding " << (i+1) << ": " << voicings_[i] << std::endl;
  }
  return 0;
}
