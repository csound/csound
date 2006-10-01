#ifndef CSOUND_VOICELEAD_HPP
#define CSOUND_VOICELEAD_HPP

#include "Platform.hpp"
#ifdef SWIG
%module CsoundVST
%{
#ifndef TEST
#include "Event.hpp"
#endif
#include <vector>
#include <set>
#include <algorithm>
#include <cmath>
  %}
%include "std_vector.i"
%template(EventVector) std::vector<csound::Event>;
%template(DoubleVector) std::vector<double>;
#else
#ifndef TEST
#include "Event.hpp"
#endif
#include <vector>
#include <set>
#include <algorithm>
#include <cmath>
#endif

namespace csound 
{
  /**
   * Returns the pitch-class of the pitch,
   * in 12-tone equal temperament.
   */
  double pc(double pitch);

  /**
   * Returns the voice-leading vector between
   * chord1 and chord2.
   */   
  std::vector<double> voiceleading(const std::vector<double> &chord1, 
				   const std::vector<double> &chord2);

  /**
   * Returns the simpler (fewer motions) of the voiceleadings
   * between source chord and either destination1 or destination2,
   * optionally avoiding parallel fifths.
   */
  const std::vector<double> &simpler(const std::vector<double> &source, 
				     const std::vector<double> &destination1, 
				     const std::vector<double> &destination2, 
				     bool avoidParallels);

  /**
   * Returns the smoothness (distance by taxicab or L1 norm)
   * of the voiceleading between chord1 and chord2.
   */
  double smoothness(const std::vector<double> &chord1, 
		    const std::vector<double> &chord1);
                
  /*
   * Returns whether the progression between chord1 and chord2
   * contains a parallel fifth.
   */
  bool areParallel(const std::vector<double> &chord1, 
		   const std::vector<double> &chord2);

  /**
   * Returns the closer, first by smoothness then by simplicity.,
   * of the voiceleadings between source and either 
   * destination1 or destination2, optionally avoiding
   * parallel fifths.
   */
  const std::vector<double> &closer(const std::vector<double> &source, 
				    const std::vector<double> &destination1, 
				    const std::vector<double> &destination2, 
				    bool avoidParallels);

  /**
   * Returns the chord with the first note rotated to the last note.
   */
  std::vector<double> rotate(const std::vector<double> &chord);

  /**
   * Returns the set of all rotations of the chord.
   */
  void rotations(std::vector<double> chord, 
		 std::vector< std::vector<double> > &rotations_);
  
  /**
   * Returns the chord as the list of its pitch-classes.
   */
  std::vector<double> tones(const std::vector<double> &chord);

  /**
   * Returns in the argument all inversions of the chord
   * within the specified range.
   */
  void inversions(const std::vector<double> &chord, 
		  std::set< std::vector<double> > &inversions__, 
		  double range);
  
  /**
   * Returns the closest voiceleading within the specified range, 
   * first by smoothness then by simplicity, 
   * between the source chord any of the destination chords,
   * optionally avoiding parallel fifths.
   */
  const std::vector<double> &closest(const std::vector<double> &source, 
				     std::set< std::vector<double> > &destinations,
				     double range, 
				     bool avoidParallels);
  
  /**
   * Returns the closest voiceleading within the specified range, 
   * first by smoothness then by simplicity, 
   * between the source chord and the target pitch-class set,
   * optionally avoiding parallel fifths.
   * The algorithm uses a brute-force search through all
   * unordered chords fitting the target pitch-class set within
   * the specified range. For up to 6 voices this is usable
   * for non-real-time computation.
   */
  std::vector<double> voicelead(const std::vector<double> &source, 
				const std::vector<double> &targetPitchClassSet, 
				double lowest, 
				double range, 
				bool avoidParallels);

#ifndef TEST
  /**
   * Returns the closest voiceleading within the specified range, 
   * first by smoothness then by simplicity, 
   * between the source chord and the target pitch-class set,
   * optionally avoiding parallel fifths.
   * The algorithm uses a brute-force search through all
   * unordered chords fitting the target pitch-class set within
   * the specified range. For up to 6 voices this is usable
   * for non-real-time computation.
   */
  std::vector<Event> voicelead(const std::vector<Event> &source,
			       const std::vector<Event> &target,
			       double lowest,
			       double range,
			       bool avoidParallels);
#endif
  
}

#endif

