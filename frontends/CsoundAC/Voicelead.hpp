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
#ifndef CSOUND_VOICELEAD_HPP
#define CSOUND_VOICELEAD_HPP

#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include "Event.hpp"
#include "CppSound.hpp"
#include <vector>
#include <algorithm>
#include <cmath>
  %}
%include "std_vector.i"
%template(ChordVector) std::vector< std::vector<double> >;
#else
#include "Event.hpp"
#include <vector>
#endif

namespace csound
{
  /**
   * This class contains facilities for
   * voiceleading, harmonic progression,
   * and identifying chord types.
   *
   * See: http://ruccas.org/pub/Gogins/music_atoms.pdf
   */
  class SILENCE_PUBLIC Voicelead
  {
  public:
    /**
     * Return the pitch-class of the pitch.
     * Pitch is measured in semitones, and
     * the octave is always 12 semitones,
     * so the pitch-class is the pitch modulo 12.
     * If the pitch is an integral number of semitones,
     * and the number of divisions per octave is also 12,
     * then the pitch-class of a pitch is an integer.
     * If the pitch is not an integral number of semitones,
     * or the number of divisions per octave is not 12,
     * then the pitch-class is not necessarily an integer.
     */
    static double pc(double pitch, size_t divisionsPerOctave = 12);


    /**
     * Return the voice-leading vector (difference)
     * between chord1 and chord2.
     */
    static std::vector<double> voiceleading(const std::vector<double> &chord1,
                                            const std::vector<double> &chord2);

    /**
     * Return the simpler (fewer motions) of the voiceleadings
     * between source chord and either destination1 or destination2,
     * optionally avoiding parallel fifths.
     */
    static const std::vector<double> &simpler(const std::vector<double> &source,
                                              const std::vector<double> &destination1,
                                              const std::vector<double> &destination2,
                                              bool avoidParallels);

    /**
     * Return the smoothness (distance by taxicab or L1 norm)
     * of the voiceleading between chord1 and chord2.
     */
    static double smoothness(const std::vector<double> &chord1,
                             const std::vector<double> &chord2);

    /**
     * Return the Euclidean distance between two chords,
     * which must have the same number of voices.
     */
    static double euclideanDistance(const std::vector<double> &chord1,
                                    const std::vector<double> &chord2);

    /*
     * Return whether the progression between chord1 and chord2
     * contains a parallel fifth.
     */
    static bool areParallel(const std::vector<double> &chord1,
                            const std::vector<double> &chord2);

    /**
     * Return the closer, first by smoothness then by simplicity.,
     * of the voiceleadings between source and either
     * destination1 or destination2, optionally avoiding
     * parallel fifths.
     */
    static const std::vector<double> &closer(const std::vector<double> &source,
                                             const std::vector<double> &destination1,
                                             const std::vector<double> &destination2,
                                             bool avoidParallels);

    /**
     * Return the chord with the first note rotated to the last note.
     */
    static std::vector<double> rotate(const std::vector<double> &chord);

    /**
     * Return the set of all rotations of the chord.
     */
    static std::vector< std::vector<double> > rotations(const std::vector<double> &chord);

    /**
     * Return the chord as the list of its pitch-classes.
     * Although the list is nominally unordered, it is
     * returned sorted in ascending order. Note that pitch-classes
     * may be doubled.
     */
    static std::vector<double> pcs(const std::vector<double> &chord, size_t divisionsPerOctave = 12);

    /**
     * Return the chord as the list of its pitch-classes.
     * Although the list is nominally unordered, it is
     * returned sorted in ascending order. Note that pitch-classes
     * are NOT doubled.
     */
    static std::vector<double> uniquePcs(const std::vector<double> &chord, size_t divisionsPerOctave = 12);

    /**
     * Convert a chord to a pitch-class set number
     * M = sum over pitch-classes of (2 ^ pitch-class).
     * These numbers form a multiplicative monoid.
     * Arithmetic on this monoid can perform many
     * harmonic and other manipulations of pitch.
     */
    static double pitchClassSetToM(const std::vector<double> &chord, size_t divisionsPerOctave = 12);

    /**
     * Convert a pitch-class set number M = sum over pitch-classes of (2 ^ pitch-class)
     * to a pitch-class set chord.
     */
    static std::vector<double> mToPitchClassSet(double pcn, size_t divisionsPerOctave = 12);

    /**
     * Convert a pitch-class set to a prime chord number and a transposition.
     * Note that the prime chord numbers, and transpositions, each form an additive cyclic group.
     */
    static std::vector<double> pitchClassSetToPandT(const std::vector<double> &pcs,
                                                    size_t divisionsPerOctave = 12);

    /**
     * Convert a prime chord number and transposition to a pitch-class set.
     */
    static std::vector<double> pAndTtoPitchClassSet(double prime,
                                                    double transposition,
                                                    size_t divisionsPerOctave = 12);

    /**
     * Return the closest voiceleading within the specified range,
     * first by smoothness then by simplicity,
     * between the source chord any of the destination chords,
     * optionally avoiding parallel fifths.
     */
    static const std::vector<double> closest(const std::vector<double> &source,
                                             const std::vector< std::vector<double> > &destinations,
                                             bool avoidParallels);

    /**
     * Return the closest voiceleading within the specified range,
     * first by smoothness then by simplicity,
     * between the source chord and the target pitch-class set,
     * optionally avoiding parallel fifths.
     * The algorithm uses a brute-force search through all
     * unordered chords, which are stored in a cache,
     * fitting the target pitch-class set within
     * the specified range. Although the time complexity
     * is exponential, this is still usable for non-real-time
     * operations in most cases of musical interest.
     */
    static std::vector<double> voicelead(const std::vector<double> &source,
                                         const std::vector<double> &targetPitchClassSet,
                                         double lowest,
                                         double range,
                                         bool avoidParallels,
                                         size_t divisionsPerOctave = 12);

    /**
     * Return the closest voiceleading within the specified range,
     * first by smoothness then by simplicity,
     * between the source chord and the target pitch-class set,
     * optionally avoiding parallel fifths.
     * The algorithm uses a brute-force search through all
     * unordered chords, which are recursively enumerated,
     * fitting the target pitch-class set within
     * the specified range. Although the time complexity
     * is exponential, the algorithm is still usable
     * for non-real-time operations in most cases of musical interest.
     */
    static std::vector<double> recursiveVoicelead(const std::vector<double> &source,
                                                  const std::vector<double> &targetPitchClassSet,
                                                  double lowest,
                                                  double range,
                                                  bool avoidParallels,
                                                  size_t divisionsPerOctave = 12);

    /**
     * Return the pitch in pitches that is closest to the specified pitch.
     */
    static double closestPitch(double pitch, const std::vector<double> &pitches);

    /**
     * Return the pitch that results from making the minimum adjustment
     * to the pitch-class of the pitch argument that is required to make
     * its pitch-class the same as one of the pitch-classes in the
     * pitch-class set argument. I.e., "round up or down" to make
     * the pitch fit into a chord or scale.
     */
    static double conformToPitchClassSet(double pitch, const std::vector<double> &pcs, size_t divisionsPerOctave = 12);

    /**
     * Invert by rotating the chord and adding an octave to its last pitch.
     */
    static std::vector<double> invert(const std::vector<double> &chord);

    /**
     * Return as many inversions of the pitch-classes in the chord
     * as there are voices in the chord.
     */
    static std::vector< std::vector<double> > inversions(const std::vector<double> &chord);

    /**
     * Return the chord transposed so its lowest pitch is at the origin.
     */
    static std::vector<double> toOrigin(const std::vector<double> &chord);

    /**
     * Return the normal chord: that inversion of the pitch-classes in the chord
     * which is closest to the orthogonal axis of the Tonnetz for that chord.
     * Similar to, but not identical with, "normal form."
     */
    static std::vector<double> normalChord(const std::vector<double> &chord);

    /**
     * Return the prime chord: that inversion of the pitch-classes in the chord
     * which is closest to the orthogonal axis of the Tonnetz for that chord,
     * transposed so that its lowest pitch is at the origin.
     * Similar to, but not identical with, "prime form."
     */
    static std::vector<double> primeChord(const std::vector<double> &chord);

    /**
     * Return C = (sum over pitch-classes of (pitch-class ^ 2)) - 1
     * (additive cyclic group for pitch-class sets)
     * for the named pitch-class set.
     */
    static double nameToC(std::string name, size_t divisionsPerOctave_);

    /**
     * Return C = (sum over pitch-classes of (pitch-class ^ 2)) - 1
     * (additive cyclic group for non-empty pitch-class sets)
     * for M = sum over pitch-classes of (2 ^ pitch-class)
     * (multiplicative monoid for pitch-class sets).
     */
    static double mToC(double M, size_t divisionsPerOctave);

    /**
     * Return M = sum over pitch-classes of (2 ^ pitch-class)
     * (multiplicative monoid for pitch-class sets)
     * for C = (sum over pitch-classes of (pitch-class ^ 2)) - 1
     * (additive cyclic group for non-empty pitch-class sets).
     */
    static double cToM(double C, size_t divisionsPerOctaven = 12);

    /**
     * Return C = (sum over pitch-classes of (pitch-class ^ 2)) - 1
     * (additive cyclic group for non-empty pitch-class sets)
     * for P = index of prime chords.
     * If an exact match is not found the closest match is returned.
     */
    static double cToP(double C, size_t divisionsPerOctave = 12);

    /**
     * Return P = index of prime chords
     * for C = (sum over pitch-classes of (pitch-class ^ 2)) - 1
     * (additive cyclic group for non-empty pitch-class sets).
     * If an exact match is not found the closest match is returned.
     */
    static double pToC(double Z, size_t divisionsPerOctave = 12);

    /**
     * Return a copy of the chord where each pitch is replaced by its corresponding pitch-class.
     * The voices remain in their original order.
     */
    static std::vector<double> orderedPcs(const std::vector<double> &chord, size_t divisionsPerOctave = 12);

    /**
     * Return a copy of the chord sorted by ascending distance from its first pitch-class.
     */
    static std::vector<double> sortByAscendingDistance(const std::vector<double> &chord, size_t divisionsPerOctave = 12);

    /**
     * Return the closest crossing-free, non-bijective voiceleading
     * from the source chord to the pitch-classes in the target chord,
     * using Dimitri Tymoczko's linear programming algorithm.
     * Because voices can be doubled, the source chord
     * is returned along with result.
     * The algorithm does not avoid parallel motions,
     * and does not maintain the original order of the voices.
     * The return value contains the original chord, the voiceleading vector,
     * and the resulting chord, in that order.
     */
    static std::vector< std::vector<double> > nonBijectiveVoicelead(const std::vector<double> &sourceChord,
                                                                    const std::vector<double> &targetPitchClassSet,
                                                                    size_t divisionsPerOctave = 12);

    /**
     * Return the prime chord for the index P.
     */
    static std::vector<double> pToPrimeChord(double P, size_t divisionsPerOctave = 12);

    static void initializePrimeChordsForDivisionsPerOctave(size_t divisionsPerOctave);

    /**
     * Return the voiced chord for the prime chord index P, transposition T,
     * and voicing index V within the specified range for the indicated number of tones per octave.
     * The algorithm finds the zero voicing
     * (the lowest octave transposition
     * of the normal chord of the chord
     * that is no lower than the lowest pitch,
     * which has voicing index V = 0) and the zero
     * iterator (the lowest (in all voices)
     * unordered voicing of the chord that is no lower
     * (in all voices) than the lowest pitch,
     * which has enumeration index = 0). Thus,
     * V of a voicing equals the enumeration index of that
     * voicing minus the enumeration index of the zero voicing.
     * The algorithm enumerates the voicings, and thus V, until V is matched.
     * If V is greater than the maximum V, its modulus is used.
     */
    static std::vector<double> ptvToChord(size_t P, size_t T, size_t V_, size_t lowest, size_t range, size_t divisionsPerOctave = 12);

    /**
     * Return the voiced chord for the prime chord index P, transposition T,
     * and voicing index V within the specified range for the indicated number of tones per octave.
     * The algorithm finds the zero voicing
     * (the lowest octave transposition
     * of the normal chord of the chord
     * that is no lower than the lowest pitch,
     * which has voicing index V = 0) and the zero
     * iterator (the lowest (in all voices)
     * unordered voicing of the chord that is no lower
     * (in all voices) than the lowest pitch,
     * which has enumeration index = 0). Thus, the
     * V of a voicing equals the enumeration index of that
     * voicing minus the enumeration index of the zero voicing.
     * The algorithm enumerates the voicings until the chord is matched.
     */
    static std::vector<double> chordToPTV(const std::vector<double> &chord, size_t lowestPitch, size_t highestPitch, size_t divisionsPerOctave = 12);

    /**
     * Return an enumeration of all voicings of the chord
     * that are greater than or equal to the lowest pitch,
     * and less than the highest pitch, by adding octaves.
     * Voicings are ordered, but note that normally
     * in this module chords are considered to be unordered.
     * Note that complex chords and/or wide ranges may require
     * more memory than is available.
     * The index of voicings V forms an additive cyclic group.
     * Arithmetic on this group can perform many operations
     * on the voices of the chord such as revoicing, arpeggiation, and so on.
     */
    static std::vector< std::vector<double> > voicings(const std::vector<double> &chord,
                                                       double lowest,
                                                       double highest,
                                                       size_t divisionsPerOctave);
    /**
     * Add an octave to a voicing; can be
     * iterated to enumerate the voicings of a chord.
     * The lowest voicing must initially be set equal to the original voicing.
     * The algorithm treats a chord as a 'numeral' that increments
     * with a radix equal to the number of octaves in the total range of pitches.
     * Returns an empty voicing if adding an octave would
     * create a voicing that exceeds the maximum pitch,
     * i.e. when the highest-order voice needs to 'carry.'
     */
    static bool addOctave(const std::vector<double> &lowestVoicing, std::vector<double> &newVoicing, size_t maximumPitch, size_t divisionsPerOctave);

    /**
     * Wrap chord tones that exceed the highest pitch around to the bottom of the range orbifold.
     */
    static std::vector<double> wrap(const std::vector<double> &chord, size_t lowestPitch, size_t highestPitch, size_t divisionsPerOctave = 12);

    /**
     * Return the chord transposed by the indicated number of semitones.
     */
    static std::vector<double> transpose(const std::vector<double> &chord, double semitones);

    /**
     * Return the pitch-class transposition of pitch p by n semitones.
     */
    static double T(double p, double n);

    /**
     * Return the pitch-class transposition of chord c by n semitones.
     */
    static std::vector<double> T(const std::vector<double> &c, double n);

    /**
     * Return the pitch-class inversion of pitch p by n semitones.
     */
    static double I(double p, double n);

    /**
     * Return the pitch-class inversion of chord c by n semitones.
     */
    static std::vector<double> I(const std::vector<double> &c, double n);

    /**
     * Invert chord c by exchange.
     */
    static std::vector<double> K(const std::vector<double> &c);

    /**
     * Return whether chord Y is a transposed form of chord X; g is the generator of 
     * transpositions.
     */
    static bool Tform(const std::vector<double> &X, const std::vector<double> &Y, double g=1.0);

    /**
     * Return whether chord Y is an inverted form of chord X; g is the generator of 
     * inversions.
     */
    static bool Iform(const std::vector<double> &X, const std::vector<double> &Y, double g=1.0);

    /**
     * Contextually transpose chord c with respect to chord s by n semitones; g is the generator of 
     * transpositions.
     */
    static std::vector<double> Q(const std::vector<double> &c, double n, const std::vector<double> &s, double g=1.0);

    /**
     * Size of the octave in semitones.
     */
    static const double semitonesPerOctave;
  };
}
#endif

