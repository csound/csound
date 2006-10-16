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
%template(ChordVector) std::vector< std::vector<double> >;
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
   * Performs voice-leading between 
   * two sets of unordered pitches (MIDI key numbers).
   * The voice-leading is first the closest by
   * taxicab norm, and then the simplest in motion,
   * optionally avoiding parallel fifths.
   *
   * See: http://ruccas.org/pub/Gogins/music_atoms.pdf
   */
  class Voicelead
  {
  public:
    /**
     * Returns the pitch-class of the pitch.
     */
    static double pc(double pitch, size_t tonesPerOctave = 12);


    /**
     * Returns the voice-leading vector between
     * chord1 and chord2.
     */   
    static std::vector<double> voiceleading(const std::vector<double> &chord1, 
					    const std::vector<double> &chord2);

    /**
     * Returns the simpler (fewer motions) of the voiceleadings
     * between source chord and either destination1 or destination2,
     * optionally avoiding parallel fifths.
     */
    static const std::vector<double> &simpler(const std::vector<double> &source, 
					      const std::vector<double> &destination1, 
					      const std::vector<double> &destination2, 
					      bool avoidParallels);

    /**
     * Returns the smoothness (distance by taxicab or L1 norm)
     * of the voiceleading between chord1 and chord2.
     */
    static double smoothness(const std::vector<double> &chord1, 
			     const std::vector<double> &chord1);
                
    /*
     * Returns whether the progression between chord1 and chord2
     * contains a parallel fifth.
     */
    static bool areParallel(const std::vector<double> &chord1, 
			    const std::vector<double> &chord2);

    /**
     * Returns the closer, first by smoothness then by simplicity.,
     * of the voiceleadings between source and either 
     * destination1 or destination2, optionally avoiding
     * parallel fifths.
     */
    static const std::vector<double> &closer(const std::vector<double> &source, 
					     const std::vector<double> &destination1, 
					     const std::vector<double> &destination2, 
					     bool avoidParallels);

    /**
     * Returns the chord with the first note rotated to the last note.
     */
    static std::vector<double> rotate(const std::vector<double> &chord);

    /**
     * Returns the set of all rotations of the chord.
     */
    static void rotations(std::vector<double> chord, 
			  std::vector< std::vector<double> > &rotations_);
  
    /**
     * Returns the chord as the list of its pitch-classes.
     */
    static std::vector<double> pcs(const std::vector<double> &chord, size_t tonesPerOctave = 12);

    /**
     * Converts a chord to a pitch-class set number 
     * N = sum (2 ^ pc).
     */
    static double numberFromChord(const std::vector<double> &chord, size_t tonesPerOctave = 12);

    /**
     * Converts a pitch-class set number to a pitch-class set chord.
     */
    static std::vector<double> pcsFromNumber(double pcn, size_t tonesPerOctave = 12);

    /**
     * Returns all voicings of the chord
     * within the specified range.
     */
    static std::vector< std::vector<double> > voicings(const std::vector<double> &chord, 
						       double lowest, 
						       double range);
 
    /**
     * Returns the closest voiceleading within the specified range, 
     * first by smoothness then by simplicity, 
     * between the source chord any of the destination chords,
     * optionally avoiding parallel fifths.
     */
    static const std::vector<double> &closest(const std::vector<double> &source, 
					      const std::set< std::vector<double> > &destinations, 
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
    static std::vector<double> voicelead(const std::vector<double> &source, 
					 const std::vector<double> &targetPitchClassSet, 
					 double lowest, 
					 double range, 
					 bool avoidParallels);

    /**
     * Return the pitch in pitches that is closest to the specified pitch.
     */
    static double closestPitch(double pitch, const std::vector<double> &pitches);

    /**
     * Return the pitch that results from making the minimum adjustment
     * to the pitch-class of the pitch argument that is required to make
     * its pitch-class the same as one of the pitch-classes in the
     * pitch-class set argument.
     */
    static double conformToPitchClassSet(double pitch, const std::vector<double> &pcs, size_t tonesPerOctave = 12);

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
    static std::vector<Event> voiceleadEvents(const std::vector<Event> &source,
					      const std::vector<Event> &target,
					      double lowest,
					      double range,
					      bool avoidParallels);
#endif
  };
  
}
#endif

