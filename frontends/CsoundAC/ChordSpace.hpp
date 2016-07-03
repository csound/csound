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
#ifndef CHORD_SPACES_H
#define CHORD_SPACES_H
#define EIGEN_INITIALIZE_MATRICES_BY_ZERO
#include "Platform.hpp"
#ifdef SWIG
%module CsoundAC
%{
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <eigen3/Eigen/Dense>
#include "Event.hpp"
#include <iostream>
#include <iterator>
#include <map>
#include "Score.hpp"
#include <set>
#include <sstream>
#include <vector>
%}
%include "std_string.i"
%include "std_vector.i"
#else
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <eigen3/Eigen/Dense>
#include "Event.hpp"
#include <iostream>
#include <iterator>
#include <map>
#include <Score.hpp>
#include <set>
#include <sstream>
#include <vector>
#endif

namespace csound {
/**
This class, part of CsoundAC, implements a geometric approach to some common
operations on chords in neo-Riemannian music theory for use in score
generating procedures:

--  Identifying whether a chord belongs to some equivalence class of music
    theory, or sending a chord to its equivalent within a representative
    ("normal") fundamental domain of some equivalence relation. The
    equivalence relations are octave (O), permutational (P), transpositional,
    (T), inversional (I), and their compounds OP, OPT (set-class or chord
    type), and OPTI (similar to prime form), among others.

--  Causing chord progressions to move strictly within an orbifold that
    generates some equivalence class.

--  Implementing chord progressions based on the L, P, R, D, K, and Q
    operations of neo-Riemannian theory (thus implementing some aspects of
    "harmony").

--  Implementing chord progressions performed within a more abstract
    equivalence class by means of the closest voice-leading within a less
    abstract equivalence class (thus implementing some fundamentals of
    "counterpoint").

DEFINITIONS

Pitch is the perception of a distinct sound frequency. It is a logarithmic
perception; octaves, which sound 'equivalent' in some sense, represent
doublings or halvings of frequency.

Pitches and intervals are represented as real numbers. Middle C is 60 and the
octave is 12. Our usual system of 12-tone equal temperament, as well as MIDI
key numbers, are completely represented by the whole numbers; any and all
other pitches can be represented simply by using fractions.

A voice is a distinct sound that is heard as having a pitch.

A chord is simply a set of voices heard at the same time, represented here
as a point in a chord space having one dimension of pitch for each voice
in the chord.

For the purposes of algorithmic composition in CsoundAC, a score is considered
to be a sequence of more or less fleeting chords.

EQUIVALENCE RELATIONS AND CLASSES

An equivalence relation identifies elements of a set as belonging to
classes. For example the octave is an equivalence relation that identifies
C1, C2, and C3 as belonging to the equivalence class C. Operations that send
elements to their equivalents induce quotient spaces or orbifolds, where
the equivalence operation identifies points on one face of the orbifold with
points on an opposing face. The fundamental domain of the equivalence relation
is the space "within" the orbifold.

Plain chord space has no equivalence relation. Ordered chords are represented
as vectors in parentheses (p1, ..., pN). Unordered chords are represented as
sorted vectors in braces {p1, ..., pN}. Unordering is itself an equivalence
relation -- permutational equivalence.

The following equivalence relations apply to pitches and chords, and exist in
different orbifolds. Equivalence relations can be combined (Callendar, Quinn,
and Tymoczko, "Generalized Voice-Leading Spaces," _Science_ 320, 2008), and
the more equivalence relations are combined, the more abstract is the
resulting orbifold compared to the parent space.

In most cases, a chord space can be divided into a number, possibly
infinite, of geometrically equivalent fundamental domains for the same
equivalence relation. Therefore, here we use the notion of 'representative'
or 'normal' fundamental domain. For example, the representative fundamental
domain of unordered sequences, out of all possible orderings, consists of all
sequences in their ordinary sorted order. It is important, in the following,
to identify representative fundamental domains that combine properly, e.g.
such that the representative fundamental domain of OP / the representative
fundamental domain of PI equals the representative fundamental domain of OPI.
And this in turn may require accounting for duplicate elements of the
representative fundamental domain caused by reflections or singularities in
the orbifold.

C       Cardinality equivalence, e.g. {1, 1, 2} == {1, 2}. _Not_ assuming
        cardinality equivalence ensures that there is a proto-metric in plain
        chord space that is inherited by all child chord spaces. Cardinality
        equivalence is never assumed here, because we are working in chord
        spaces of fixed dimensionality; e.g. we represent the note middle C
        not only as {60}, but also as {60, 60, ..., 60}.

O       Octave equivalence. The fundamental domain is defined by the pitches
        in a chord spanning the range of an octave or less, and summing to
        an octave or less.

P       Permutational equivalence. The fundamental domain is defined by a
        "wedge" of plain chord space in which the voices of a chord are always
        sorted by pitch.

T       Transpositional equivalence, e.g. {1, 2} == {7, 8}. The fundamental
        domain is defined as a plane in chord space at right angles to the
        diagonal of unison chords. Represented by the chord always having a
        sum of pitches equal to 0.

Tg      Transpositional equivalence "rounded off" to the nearest generator
        of transposition (in 12 tone equal temperament, this is one semitone).

I       Inversional equivalence. Care is needed to distinguish the
        mathematician's sense of 'invert', which means 'pitch-space inversion'
        or 'reflect in a point', from the musician's sense of 'invert', which
        varies according to context but in practice often means 'registral
        inversion' or 'revoice by adding an octave to the lowest tone of a
        chord.' Here, we use 'invert' and 'inversion' in the mathematician's
        sense, and we use the terms 'revoice' and 'voicing' for the musician's
        'invert' and 'inversion'. The inversion point for any inversion lies
        on the unison diagonal. A fundamental domain is defined as any half of
        chord space that is bounded by a plane containing the inversion point.
        Represented as the chord having the first interval between voices be
        smaller than or equal to the final interval (recursing for chords of
        more than 3 voices).

PI      Inversional equivalence with permutational equivalence. The
        'inversion flat' of unordered chord space is a hyperplane consisting
        of all those unordered chords that are invariant under inversion. A
        fundamental domain is defined by any half space bounded by a
        hyperplane containing the inversion flat. It is represented as that
        half of the space on or lower than the hyperplane defined by the
        inversion flat and the unison diagonal.

OP      Octave equivalence with permutational equivalence. Tymoczko's orbifold
        for chords; i.e. chords with a fixed number of voices in a harmonic
        context. The fundamental domain is defined as a hyperprism one octave
        long with as many sides as voices and the ends identified by octave
        equivalence and one cyclical permutation of voices, modulo the
        unordering. In OP for trichords in 12TET, the augmented triads run up
        the middle of the prism, the major and minor triads are in 6
        alternating columns around the augmented triads, the two-pitch chords
        form the 3 sides, and the one-pitch chords form the 3 edges that join
        the sides.

OPT     The layer of the OP prism as close as possible to the origin, modulo
        the number of voices. Chord type. Note that CM and Cm are different
        OPT. Because the OP prism is canted down from the origin, at least one
        pitch in each OPT chord (excepting the origin itself) is negative.

OPI     The OP prism modulo inversion, i.e. 1/2 of the OP prism. The
        representative fundamental consits of those chords less than or equal
        to their inversions modulo OP.

OPTI    The OPT layer modulo inversion, i.e. 1/2 of the OPT layer.
        Set-class. Note that CM and Cm are the same OPTI.

OPERATIONS

Each of the above equivalence relations is, of course, an operation that sends
chords outside a fundamental domain to chords inside the fundamental domain.

We define the following additional operations:

T(p, x)         Translate p by x.

I(p [, x])      Reflect p in x, by default the origin.

P               Send a major triad to the minor triad with the same root,
                or vice versa (Riemann's parallel transformation).

L               Send a major triad to the minor triad one major third higher,
                or vice versa (Riemann's Leittonwechsel or leading-tone
                exchange transformation).

R               Send a major triad to the minor triad one minor third lower,
                or vice versa (Riemann's relative transformation).

D               Send a triad to the next triad a perfect fifth lower
                (dominant transformation).

P, L, and R have been extended as follows, see Fiore and Satyendra,
"Generalized Contextual Groups", _Music Theory Online_ 11, August 2008:

K(c)            Interchange by inversion;
                K(c) := I(c, c[1] + c[2]).
                This is a generalized form of P; for major and minor triads,
                it is exactly the same as P, but it also works with other
                chord types.

Q(c, n, m)      Contexual transposition;
                Q(c, n, m) := T(c, n) if c is a T-form of m,
                or T(c, -n) if c is an I-form of M. Not a generalized form
                of L or R; but, like them, K and Q generate the T-I group.

*/

static bool debug = false;

inline SILENCE_PUBLIC void print(const char *format, va_list valist) {
	std::vfprintf(stderr, format, valist);
}

/**
 * Convenience function for unbuffered printing of messages to the console.
 */
inline SILENCE_PUBLIC void print(const char *format,...) {
	va_list marker;
	va_start(marker, format);
	print(format, marker);
	va_end(marker);
}

/**
 * Returns n!
 */
inline SILENCE_PUBLIC double factorial(double n) {
    double result = 1.0;
    for (int i = 0; i <= n; ++i) {
        result = result * i;
    }
    return result;
}

inline SILENCE_PUBLIC double EPSILON() {
	static double epsilon = 1.0;
	if (epsilon == 1.0) {
		for (;;) {
			epsilon = epsilon / 2.0;
			double nextEpsilon = epsilon / 2.0;
			double onePlusNextEpsilon = 1.0 + nextEpsilon;
			if (onePlusNextEpsilon == 1.0) {
				break;
			}
		}
	}
	return epsilon;
}

inline SILENCE_PUBLIC double &epsilonFactor() {
	static double epsilonFactor = 100000.0;
	return epsilonFactor;
}

inline SILENCE_PUBLIC bool eq_epsilon(double a, double b) {
	if (std::abs(a - b) < (EPSILON() * epsilonFactor())) {
		return true;
	} else {
		return false;
	}

}

inline SILENCE_PUBLIC double gt_epsilon(double a, double b) {
	if (eq_epsilon(a, b)) {
		return false;
	} else {
		return (a > b);
	}
}

inline SILENCE_PUBLIC double lt_epsilon(double a, double b) {
	if (eq_epsilon(a, b)) {
		return false;
	} else {
		return (a < b);
	}
}

inline SILENCE_PUBLIC double ge_epsilon(double a, double b) {
	if (eq_epsilon(a, b)) {
		return true;
	} else {
		return (a > b);
	}
}

inline SILENCE_PUBLIC double le_epsilon(double a, double b) {
	if (eq_epsilon(a, b)) {
		return true;
	} else {
		return (a < b);
	}
}

/**
 * The size of the octave, defined to be consistent with
 * 12 tone equal temperament and MIDI.
 */
inline SILENCE_PUBLIC double OCTAVE() {
	return 12.0;
}

inline SILENCE_PUBLIC double MIDDLE_C() {
	return 60.0;
}

inline SILENCE_PUBLIC double C4() {
	return MIDDLE_C();
}

/**
 * Returns the pitch transposed by semitones, which may be any scalar.
 * NOTE: Does NOT return an equivalent under any requivalence relation.
 */
inline SILENCE_PUBLIC double T(double pitch, double semitones) {
	return pitch + semitones;
}

/**
 * Returns the pitch reflected in the center, which may be any pitch.
 * NOTE: Does NOT return an equivalent under any requivalence relation.
 */
inline SILENCE_PUBLIC double I(double pitch, double center = 0.0) {
	return center - pitch;
}

/**
 * Returns the remainder of the dividend divided by the divisor,
 * according to the Euclidean definition.
 */
inline SILENCE_PUBLIC double modulo(double dividend, double divisor) {
	double quotient = dividend / divisor;
	if (divisor < 0.0) {
		quotient = std::ceil(quotient);
	}
	if (divisor > 0.0) {
		quotient = std::floor(quotient);
	}
	double remainder = dividend - (quotient * divisor);
	return remainder;
}

/**
 * Returns the equivalent of the pitch under pitch-class equivalence, i.e.
 * the pitch is in the interval [0, OCTAVE). Implemented using the Euclidean
 * definition.
 */
inline SILENCE_PUBLIC double epc(double pitch) {
	double pc = modulo(pitch, OCTAVE());
	return pc;
}

/**
 * Enums for all defined equivalence relations,
 * used to specialize template functions.
 * If relation R takes no range argument,
 * it defaults to a range of one octave.
 * T is transposition to layer 0,
 * Tg is transposition to the layer close as one
 * can get to layer 0 but all chord pitches are
 * generated by g (default = 1 semitone).
 *
 * NOTE: Not all of these are currently implemented.
 */
typedef enum {
	EQUIVALENCE_RELATION_r = 0,
	EQUIVALENCE_RELATION_R,
	EQUIVALENCE_RELATION_P,
	EQUIVALENCE_RELATION_T,
	EQUIVALENCE_RELATION_Tg,
	EQUIVALENCE_RELATION_I,
    EQUIVALENCE_RELATION_V,
	EQUIVALENCE_RELATION_RP,
	EQUIVALENCE_RELATION_RT,
	EQUIVALENCE_RELATION_RTg,
	EQUIVALENCE_RELATION_RI,
	EQUIVALENCE_RELATION_PT,
	EQUIVALENCE_RELATION_PTg,
	EQUIVALENCE_RELATION_PI,
	EQUIVALENCE_RELATION_TI,
	EQUIVALENCE_RELATION_RPT,
	EQUIVALENCE_RELATION_RPTg,
	EQUIVALENCE_RELATION_RPI,
	EQUIVALENCE_RELATION_RTI,
	EQUIVALENCE_RELATION_RTgI,
	EQUIVALENCE_RELATION_RPTI,
	EQUIVALENCE_RELATION_RPTgI,
} EQUIVALENCE_RELATIONS;

static const char* namesForEquivalenceRelations[] = {
    "r",
    "R",
    "P",
    "T",
    "Tg",
    "I",
    "V",
    "RP",
    "RT",
    "RTg",
    "RI",
    "PT",
    "PTg",
    "PI",
    "TI",
    "RPT",
    "RPTg",
    "RPI",
    "RTI",
    "RTgI",
    "RPTI",
    "RPTgI"
};

// Forward declarations:

class SILENCE_PUBLIC Chord;
SILENCE_PUBLIC double euclidean(const csound::Chord &a, const csound::Chord &b);
SILENCE_PUBLIC double pitchClassForName(std::string pitchclass);
SILENCE_PUBLIC std::string nameForChord(const Chord &chord);
SILENCE_PUBLIC const Chord &chordForName(std::string name);
SILENCE_PUBLIC std::vector<std::string> split(std::string);

// Equivalence relations are implemented first as template functions at namespace scope,
// and then as class member functions delegating to the corresponding namespace functions.

template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC bool isNormal(const Chord &chord,
                                                                double range, double g);
template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC bool isNormal(const Chord &chord,
                                                                double range);
template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC bool isNormal(const Chord &chord);
template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC bool isEquivalent(const Chord &a,
                                                                    const Chord &b,
                                                                    double range, double g);
template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC bool isEquivalent(const Chord &a,
                                                                    const Chord &b,
                                                                    double range);
template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC bool isEquivalent(const Chord &a,
                                                                    const Chord &b);
template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC Chord normalize(const Chord &chord,
                                                                  double range, double g);
template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC Chord normalize(const Chord &chord,
                                                                  double range);
template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC Chord normalize(const Chord &chord);

template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC std::set<Chord> allNormalizedFundamentalDomain(int voices, double range, double g);
template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC std::set<Chord> uniqueNormalizedFundamentalDomain(int voices, double range, double g);

/**
 * Returns a chord with the specified number of voices all set to a first
 * pitch, useful as an iterator.
 */
SILENCE_PUBLIC Chord iterator(int voiceN, double first);
/**
 * Increment a chord voicewise through chord space,
 * from a low point on the unison diagonal through a high point
 * on the unison diagonal. g is the generator of transposition.
 * It may be necessary to set the chord to the low point to start.
 */
SILENCE_PUBLIC bool next(Chord &odometer, const Chord &low, double high, double g = 1.0);
SILENCE_PUBLIC bool operator == (const Chord &a, const Chord &b);
SILENCE_PUBLIC bool operator < (const Chord &a, const Chord &b);
SILENCE_PUBLIC bool operator <= (const Chord &a, const Chord &b);
SILENCE_PUBLIC bool operator > (const Chord &a, const Chord &b);
SILENCE_PUBLIC bool operator >= (const Chord &a, const Chord &b);
/**
 * Chords represent simultaneously sounding pitches. The pitches are
 * represented as semitones with 0 at the origin and middle C as 60.
 * Each voice also has a duration, velocity, channel, and pan.
 * Eigen matrices are accessed (row, column) and stored as column
 * vectors, so a Chord is accessed (voice (same as row), attribute).
 */
class SILENCE_PUBLIC Chord : public Eigen::MatrixXd {
public:
	enum {
		PITCH = 0,
		DURATION = 1,
		LOUDNESS = 2,
		INSTRUMENT = 3,
		PAN = 4,
		COUNT = 5
	};
	Chord() {
		resize(3);
	}
	Chord(const Chord &other) {
		*this = other;
	}
	virtual Chord &operator=(const Chord &other) {
		if (this != &other) {
			resizeLike(other);
			for (int i = 0, n = rows(); i < n; ++i) {
				for (int j = 0, m = cols(); j < m; ++j) {
					coeffRef(i, j) = other.coeff(i, j);
				}
			}
		}
		return *this;
	}
	virtual ~Chord() {
	}
	virtual size_t voices() const {
		return rows();
	}
	virtual void resize(size_t voiceN) {
		Eigen::MatrixXd::resize(voiceN, COUNT);
	}
	/**
	 * Returns a string representation of the chord's pitches (only).
	 * Quadratic complexity, but short enough not to matter.
	 */
	virtual std::string toString() const {
		char buffer[0x1000];
		std::stringstream stream;
		for (size_t voice = 0; voice < voices(); ++voice) {
			std::snprintf(buffer, 0x100, "%12.7f", getPitch(voice));
			if (voice > 0) {
				stream << " ";
			}
			stream << buffer;
		}
		return stream.str();
	}
	/**
	 * Rebuilds the chord's pitches (only) from a line of text.
	 */
	virtual void fromString(std::string text) {
		double scalar;
		std::vector<double> vector_;
		std::stringstream stream(text);
		while (stream >> scalar) {
			vector_.push_back(scalar);
		}
		resize(vector_.size());
		for (int i = 0, n = vector_.size(); i < n; ++i) {
			setPitch(i, vector_[i]);
		}
	}
	virtual double getPitch(int voice) const {
		return coeff(voice, PITCH);
	}
	virtual void setPitch(int voice, double value) {
		coeffRef(voice, PITCH) = value;
	}
	virtual double getDuration(int voice = 0) const {
		return coeff(voice, DURATION);
	}
	virtual void setDuration(double value, int voice = -1) {
		if (voice == -1) {
			for (voice = 0; voice < rows(); ++voice) {
				coeffRef(voice, DURATION) = value;
			}
		} else {
			coeffRef(voice, DURATION) = value;
		}
	}
	virtual double getLoudness(int voice = 0) const {
		return coeff(voice, LOUDNESS);
	}
	virtual void setLoudness(double value, int voice = -1) {
		if (voice == -1) {
			for (voice = 0; voice < rows(); ++voice) {
				coeffRef(voice, LOUDNESS) = value;
			}
		} else {
			coeffRef(voice, LOUDNESS) = value;
		}
	}
	virtual double getInstrument(int voice = 0) const {
		return coeff(voice, INSTRUMENT);
	}
	virtual void setInstrument(double value, int voice = -1) {
		if (voice == -1) {
			for (voice = 0; voice < rows(); ++voice) {
				coeffRef(voice, INSTRUMENT) = value;
			}
		} else {
			coeffRef(voice, INSTRUMENT) = value;
		}
	}
	virtual double getPan(int voice = 0) const {
		return coeff(voice, PAN);
	}
	virtual void setPan(double value, int voice = -1) {
		if (voice == -1) {
			for (voice = 0; voice < rows(); ++voice) {
				coeffRef(voice, PAN) = value;
			}
		} else {
			coeffRef(voice, PAN) = value;
		}
	}
	virtual size_t count(double pitch) const {
		size_t n = 0;
		for (size_t voice = 0; voice < voices(); ++voice) {
			if (eq_epsilon(getPitch(voice), pitch)) {
				n++;
			}
		}
		return n;
	}
	/**
	 * Returns whether or not the chord contains the pitch.
	 */
	virtual bool contains(double pitch_) const {
		for (size_t voice = 0; voice < voices(); voice++) {
			if (eq_epsilon(getPitch(voice), pitch_)) {
				return true;
			}
		}
		return false;
	}
	/**
	* Returns the lowest pitch in the chord,
	* and also its voice index.
	*/
	virtual std::vector<double> min() const {
		std::vector<double> result;
		result.resize(2);
		int voice = 0;
		double pitch = getPitch(voice);
		result[0] = pitch;
		result[1] = double(voice);
		for (int voice = 1; voice < voices(); voice++) {
			double pitch = getPitch(voice);
			if (lt_epsilon(pitch, result[0])) {
				result[0] = pitch;
				result[1] = double(voice);
			}
		}
		return result;
	}
	/**
	* Returns the highest pitch in the chord,
	* and also its voice index.
	*/
	virtual std::vector<double> max() const {
		std::vector<double> result(2);
		int voice = 0;
		double pitch = getPitch(voice);
		result[0] = pitch;
		result[1] = double(voice);
		for (voice = 1; voice < voices(); voice++) {
			pitch = getPitch(voice);
			if (gt_epsilon(pitch, result[0])) {
				result[0] = pitch;
				result[1] = double(voice);
			}
		}
		return result;
	}
	virtual double minimumInterval() const {
		double minimumInterval_ = std::abs(getPitch(0) - getPitch(1));
		for (size_t v1 = 0; v1 < voices(); v1++) {
			for (size_t v2 = 0; v2 < voices(); v2++) {
				double interval = std::abs(getPitch(v1) - getPitch(v2));
				if (lt_epsilon(interval, minimumInterval_)) {
					minimumInterval_ = interval;
				}
			}
		}
		return minimumInterval_;
	}
	virtual double maximumInterval() const {
		double maximumInterval_ = std::abs(getPitch(0) - getPitch(1));
		for (size_t v1 = 0; v1 < voices(); v1++) {
			for (size_t v2 = 0; v2 < voices(); v2++) {
				double interval = std::abs(getPitch(v1) - getPitch(v2));
				if (gt_epsilon(interval, maximumInterval_)) {
					maximumInterval_ = interval;
				}
			}
		}
		return maximumInterval_;
	}
	/**
	 * Returns a new chord whose pitches are the floors of this chord's pitches.
	 */
	virtual Chord floor() const {
		Chord clone = *this;
		for (size_t voice = 0; voice  < voices(); voice++) {
			clone.setPitch(voice, std::floor(getPitch(voice)));
		}
		return clone;
	}
	/**
	 * Returns a new chord whose pitches are the ceilings of this chord's pitches.
	 */
	virtual Chord ceiling() const {
		Chord clone = *this;
		for (size_t voice = 0; voice  < voices(); voice++) {
			clone.setPitch(voice, std::ceil(getPitch(voice)));
		}
		return clone;
	}
	/**
	 * Returns the origin of the chord's space.
	 */
	virtual Chord origin() const {
		Chord clone;
		clone.resize(voices());
		return clone;
	}
	/**
	 * Returns the Euclidean distance of this chord from its space's
	 * origin.
	 */
	virtual double distanceToOrigin() const {
		Chord origin_ = origin();
		return euclidean(*this, origin_);
	}
	/**
	 * Returns the sum of the pitches in the chord.
	 */
	virtual double layer() const {
		double sum = 0.0;
		for (size_t voice = 0; voice < voices(); ++voice) {
			sum += getPitch(voice);
		}
		return sum;
	}
	/**
	 * Returns the Euclidean distance from this chord
     * to the unison diagonal of its chord space.
     */
	virtual double distanceToUnisonDiagonal() const {
		Chord unison = origin();
		double pitch = layer() / double(voices());
		for (size_t voice = 0; voice < voices(); voice ++) {
			unison.setPitch(voice, pitch);
		}
		return euclidean(*this, unison);
	}
	/**
	 * Returns the maximally even chord in the chord's space,
	 * e.g. the augmented triad for 3 dimensions.
	 */
	virtual Chord maximallyEven() const {
		Chord clone = *this;
		double g = OCTAVE() / double(voices());
		for (size_t voice = 0; voice < voices(); voice++) {
			clone.setPitch(voice,  double(voice) * g);
		}
		return clone;
	}
	/**
	 * Transposes the chord by the indicated interval (may be a fraction).
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord T(double interval) const {
		Chord clone = *this;
		for (size_t voice = 0; voice < voices(); voice++) {
			clone.setPitch(voice, csound::T(getPitch(voice), interval));
		}
		return clone;
	}
	/**
	 * Inverts the chord by another chord that is on the unison diagonal, by
	 * default the origin.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord I(double center = 0.0) const {
		Chord inverse = *this;
		for (size_t voice = 0; voice < voices(); voice++) {
			inverse.setPitch(voice, csound::I(getPitch(voice), center));
		}
		return inverse;
	}
	/**
	 * Returns a copy of the chord cyclically permuted by a stride, by default 1.
	 * The direction of rotation is by default the same as musicians' first
	 * inversion, second inversion, and so on; but negative sign will reverse
	 * the direction of rotation.
	 * + 1 is pop the front and push it on the back, shifting the middle down.
	 * 0 1 2 3 4 => 1 2 3 4 0
	 * - 1 is pop the back and push it on the front, shifting the middle up.
	 * 0 1 2 3 4 => 4 0 1 2 3
	 */
	virtual Chord cycle(int stride = 1) const {
		Chord permuted = *this;
		int voicesToPopAndPush = std::abs(stride) % voices();
		int voicesToShift = voices() - voicesToPopAndPush;
		if (stride < 0) {
			permuted.bottomRows(voicesToShift) = topRows(voicesToShift);
			permuted.topRows(voicesToPopAndPush) = bottomRows(voicesToPopAndPush);
		}
		if (stride > 0) {
			permuted.topRows(voicesToShift) = bottomRows(voicesToShift);
			permuted.bottomRows(voicesToPopAndPush) = topRows(voicesToPopAndPush);
		}
		return permuted;
	}
	/**
	 * Returns the permutations of the pitches in a chord. The permutations
	 * starting from any particular permutation are always returned in the same order.
	 */
	virtual std::vector<Chord> permutations() const {
		std::vector<Chord> permutations_;
		Chord permutation = *this;
		permutations_.push_back(permutation);
		for (size_t i = 1; i < voices(); i++) {
			permutation = permutation.cycle();
			permutations_.push_back(permutation);
		}
		std::sort(permutations_.begin(), permutations_.end());
		return permutations_;
	}
	/**
	 * Returns a copy of the chord 'inverted' in the musician's sense,
	 * i.e. revoiced by cyclically permuting the chord and
	 * adding (or subtracting) an octave to the highest (or lowest) voice.
	 * The revoicing will move the chord up or down in pitch.
	 * A positive direction is the same as a musician's first inversion,
	 * second inversion, etc.
	 */
	virtual Chord v(int direction = 1) const {
		Chord chord = *this;
		int head = voices() - 1;
		while (direction > 0) {
			chord = chord.cycle(1);
			chord.setPitch(head, chord.getPitch(head) + OCTAVE());
			direction--;
		}
		while (direction < 0) {
			chord = chord.cycle(-1);
			chord.setPitch(0, chord.getPitch(0) + OCTAVE());
			direction++;
		}
		return chord;
	}
	/**
	 * Returns all the 'inversions' (in the musician's sense)
	 * or octavewise revoicings of the chord.
	 */
	virtual std::vector<Chord> voicings() const {
		Chord chord = *this;
		std::vector<Chord> voicings;
		voicings.push_back(chord);
		for (size_t voicing = 1; voicing < voices(); voicing++) {
			chord = chord.v();
			voicings.push_back(chord);
		}
		return voicings;
	}
	/**
	 * Returns whether the chord is within the fundamental domain of
	 * pitch-class equivalence, i.e. is a pitch-class set.
	 */
	virtual bool isepcs() const {
		for (size_t voice = 0; voice < voices(); voice++) {
			if (!eq_epsilon(getPitch(voice), epc(getPitch(voice)))) {
				return false;
			}
		}
		return true;
	}
	/**
	 * Returns the equivalent of the chord under pitch-class equivalence,
	 * i.e. the pitch-class set of the chord.
	 */
	virtual Chord epcs() const {
		Chord chord = *this;
		for (size_t voice = 0; voice < voices(); voice++) {
			chord.setPitch(voice, epc(getPitch(voice)));
		}
		return chord;
	}
	/**
	 * Returns whether the chord is within the fundamental domain of
	 * transposition to 0.
	 */
	virtual bool iset() const {
		Chord et_ = et();
		if (!(*this == et_)) {
			return false;
		}
		return true;
	}
	/**
	 * Returns the equivalent of the chord within the fundamental domain of
	 * transposition to 0.
	 */
	virtual Chord et() const {
		double min_ = min()[0];
		return T(-min_);
	}
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of the indicated range equivalence.
	 */
	virtual bool iseR(double range_) const;
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of octave equivalence.
	 */
	virtual bool iseO() const {
		return iseR(OCTAVE());
	}
	/**
	 * Returns the equivalent of the chord within the representative
	 * fundamental domain of a range equivalence.
	 */
	virtual Chord eR(double range) const;
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of octave equivalence.
	 */
	virtual Chord eO() const {
		return eR(OCTAVE());
	}
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of permutational equivalence.
	 */
	virtual bool iseP() const;
	/**
	 * Returns the equivalent of the chord within the representative
	 * fundamental domain of permutational equivalence.	The implementation
	 * uses a bubble sort to swap out of order voices in the Eigen matrix.
	 */
	virtual Chord eP() const;
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of transpositional equivalence.
	 */
	virtual bool iseT() const;
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of transpositonal equivalence.
	 */
	virtual Chord eT() const;
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of transpositonal equivalence and the equal temperament generated
	 * by g. I.e., returns the chord transposed such that its layer is 0 or, under
	 * transposition, the positive layer closest to 0. NOTE: Does NOT return the
	 * result under any other equivalence class.
	 */
	virtual Chord eTT(double g = 1.0) const;
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of translational equivalence and the equal temperament generated by g.
	 */
	virtual bool iseTT(double g = 1.0) const;
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of inversional equivalence.
	 */
	virtual bool iseI(Chord *inverse) const;
	virtual bool iseI() const {
		return iseI(0);
	}
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of inversional equivalence.
	 */
	virtual Chord eI() const;
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of range and permutational equivalence.
	 */
	virtual bool iseRP(double range) const;
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of octave and permutational equivalence.
	 */
	virtual bool iseOP() const {
		return iseRP(OCTAVE());
	}
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of range and permutational equivalence.
	 */
	virtual Chord eRP(double range) const;
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of octave and permutational equivalence.
	 */
	virtual Chord eOP() const {
		return eRP(OCTAVE());
	}
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of voicing equivalence.
	 */
	virtual bool iseV() const;
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of voicing equivalence.
	 */
	virtual Chord eV() const;
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of range, permutational, and transpositional equivalence.
	 */
	virtual bool iseRPT(double range) const;
	virtual bool iseRPTT(double range, double g = 1.0) const;
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of octave, permutational, and transpositional equivalence.
	 */
	virtual bool iseOPT() const {
		return iseRPT(OCTAVE());
	}
	virtual bool iseOPTT(double g = 1.0) const {
		return iseRPTT(OCTAVE(), g);
	}
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of range, permutational, and transpositional equivalence; the same
	 * as set-class type, or chord type.
	 */
	virtual Chord eRPT(double range) const;
	virtual Chord eRPTT(double range, double g = 1.0) const;
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of octave, permutational, and transpositional equivalence.
	 */
	virtual Chord eOPT() const {
		return eRPT(OCTAVE());
	}
	virtual Chord eOPTT(double g = 1.0) const {
		return eRPTT(OCTAVE(), g);
	}
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of range, permutational, and inversional equivalence.
	 */
	virtual bool iseRPI(double range) const;
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of octave, permutational, and inversional equivalence.
	 */
	virtual bool iseOPI() const {
		return iseRPI(OCTAVE());
	}
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of range, permutational, and inversional equivalence.
	 */
	virtual Chord eRPI(double range) const;
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of octave, permutational, and inversional equivalence.
	 */
	virtual Chord eOPI() const {
		return eRPI(OCTAVE());
	}
	/** Returns whether the chord is within the representative fundamental domain
	 * of range, permutational, transpositional, and inversional equivalence.
	 */
	virtual bool iseRPTI(double range) const;
	virtual bool iseRPTTI(double range) const;
	/**
	 * Returns whether the chord is within the representative fundamental domain
	 * of octave, permutational, transpositional, and inversional equivalence.
	 */
	virtual bool iseOPTI() const {
		return iseRPTI(OCTAVE());
	}
	virtual bool iseOPTTI() const {
		return iseRPTTI(OCTAVE());
	}
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of range, permutational, transpositional, and inversional
	 * equivalence.
	 */
	virtual Chord eRPTI(double range) const;
	virtual Chord eRPTTI(double range) const;
	/**
	 * Returns the equivalent of the chord within the representative fundamental
	 * domain of range, permutational, transpositional, and inversional
	 * equivalence.
	 */
	virtual Chord eOPTI() const {
		return eRPTI(OCTAVE());
	}
	virtual Chord eOPTTI() const {
		return eRPTTI(OCTAVE());
	}
	virtual std::string name() const {
		std::string name_ = nameForChord(*this);
		return name_;
	}
	virtual std::string information() const;
	/**
	 * Move 1 voice of the chord.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord move(int voice, double interval) const {
		Chord chord = *this;
		chord.setPitch(voice, csound::T(getPitch(voice), interval));
		return chord;
	}
	/**
	 * Performs the neo-Riemannian Lettonwechsel transformation.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord nrL() const {
		// TODO: Is this right for anything but triads and sevenths?
		Chord cv = eV();
		Chord cvt = eV().et();
		if (cvt.getPitch(1) == 4.0) {
			cv.setPitch(0, cv.getPitch(0) - 1.0);
		} else {
			if (cvt.getPitch(1) == 3.0) {
				cv.setPitch(2, cv.getPitch(2) + 1.0);
			}
		}
		return cv;
	}
	/**
	 * Performs the neo-Riemannian parallel transformation.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord nrP() const {
		// TODO: Is this right for anything but triads and sevenths?
		Chord cv = eV();
		Chord cvt = eV().et();
		if (cvt.getPitch(1) == 4.0) {
			cv.setPitch(1, cv.getPitch(1) - 1.0);
		} else {
			if (cvt.getPitch(1) == 3.0) {
				cv.setPitch(1, cv.getPitch(1) + 1.0);
			}
		}
		return cv;
	}
	/**
	 * Performs the neo-Riemannian relative transformation.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord nrR() const {
		// TODO: Is this right for anything but triads and sevenths?
		Chord cv = eV();
		Chord cvt = eV().et();
		if (cvt.getPitch(1) == 4.0) {
			cv.setPitch(2, cv.getPitch(2) + 2.0);
		} else {
			if (cvt.getPitch(1) == 3.0) {
				cv.setPitch(0, cv.getPitch(0) - 2.0);
			}
		}
		return cv;
	}
	/**
	 * Performs the neo-Riemannian Nebenverwandt transformation.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord nrN() const {
		return nrR().nrL().nrP();
	}
	/**
	 * Performs the neo-Riemannian Slide transformation.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord nrS() const {
		return nrL().nrP().nrR();
	}
	/**
	 * Performs the neo-Riemannian hexatonic pole transformation.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord nrH() const {
		return nrL().nrP().nrL();
	}
	/**
	 * Performs the neo-Riemannian dominant transformation.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord nrD() const {
		return T(-7.0);
	}
	/**
	 * Returns the chord inverted by the sum of its first two voices.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord K(double range = OCTAVE()) const {
		Chord chord = *this;
		if (chord.voices() < 2) {
			return chord;
		}
		Chord ep = chord.eP();
		double center = ep.getPitch(0) + ep.getPitch(1);
		return I(center);
	}
	/**
	 * Returns whether the chord is a transpositional form of Y with interval size g.
	 * Only works in equal temperament.
	 */
	virtual bool Tform(const Chord &Y, double g = 1.0) const {
		Chord eopx = epcs().eP();
		double i = 0.0;
		while (i < OCTAVE()) {
			Chord ty = Y.T(i);
			Chord eopty = ty.epcs().eP();
			if (eopx == eopty) {
				return true;
			}
			i = i + g;
		}
		return false;
	}
	/**
	 * Returns whether the chord is an inversional form of Y with interval size g.
	 * Only works in equal temperament.
	 */
	virtual bool Iform(const Chord &Y, double g = 1.0) const {
		Chord eopx = epcs().eP();
		double i = 0.0;
		while (i < OCTAVE()) {
			Chord iy = Y.I(i);
			Chord eopiy = iy.epcs().eP();
			if (eopx == eopiy) {
				return true;
			}
			i = i + g;
		}
		return false;
	}
	/**
	 * Returns the contextual transposition of the chord by x with respect to m
	 * with minimum interval size g.
	 * NOTE: Does NOT return an equivalent under any requivalence relation.
	 */
	virtual Chord Q(double x, const Chord &m, double g = 1.0) const {
		if (Tform(m, g)) {
			return T(x);
		}
		if (Iform(m, g)) {
			return T(-x);
		}
		return *this;
	}
	/**
	 * Creates a complete "note on" Event for the
	 * indicated voice of the chord. If the optional
	 * duration, channel, velocity, and pan parameters
	 * are not passed, then the Chord's own values for
	 * these are used.
	 */
	virtual Event note(int voice,
	        double time_,
	        double duration_ = DBL_MAX,
	        double channel_ = DBL_MAX,
	        double velocity_ = DBL_MAX,
	        double pan_ = DBL_MAX) const {
		Event note;
		note.setTime(time_);
		note.setKey(getPitch(voice));
		if (duration_ != DBL_MAX) {
			note.setDuration(duration_);
		} else {
			note.setDuration(getDuration(voice));
		}
		if (channel_ != DBL_MAX) {
			note.setInstrument(channel_);
		} else {
			note.setInstrument(getInstrument(voice));
		}
		if (velocity_ != DBL_MAX) {
			note.setVelocity(velocity_);
		} else {
			note.setVelocity(getLoudness(voice));
		}
		if (pan_ != DBL_MAX) {
			note.setPan(pan_);
		} else {
			note.setPan(getPan(voice));
		}
                return note;
	}
	/**
	 * Returns an individual note for each voice of the chord.
	 * If the optional
	 * duration, channel, velocity, and pan parameters
	 * are not passed, then the Chord's own values for
	 * these are used.
	 */
	virtual Score notes(double time_,
	        double duration_ = DBL_MAX,
	        double channel_ = DBL_MAX,
	        double velocity_ = DBL_MAX,
	        double pan_ = DBL_MAX) const {
		Score score;
		for (int voice = 0; voice < voices(); ++voice) {
			Event event = note(voice, time_, duration_, channel_, velocity_, pan_);
			score.append(event);
		}
		return score;
	}
	/**
	 * Returns an individual note for each voice of the chord.
	 * If the optional
	 * duration, channel, velocity, and pan parameters
	 * are not passed, then the Chord's own values for
	 * these are used.
	 */
	virtual void toScore(Score &score,
	        double time_, bool voiceIsInstrument=true) const {
		for (int voice = 0; voice < voices(); ++voice) {
			double instrument = double(voice);
			if (!voiceIsInstrument) {
				instrument = getInstrument(voice);
			}
			score.append(time_,
			        getDuration(voice),
			        144.0,
			        instrument,
			        getPitch(voice),
			        getLoudness(voice),
			        0.0,
			        getPan(voice));
		}
	}
	/**
	 * Returns the ith arpeggiation, current voice, and corresponding revoicing
	 * of the chord. Positive arpeggiations start with the lowest voice of the
	 * chord and revoice up; negative arpeggiations start with the highest voice
	 * of the chord and revoice down.
	 */
	virtual Chord a(int arpeggiation, double &resultPitch, int &resultVoice) const {
		Chord resultChord = v(arpeggiation);
		if (arpeggiation < 0) {
			resultVoice = resultChord.voices() - 1;
		} else {
			resultVoice = 0;
		}
		resultPitch = resultChord.getPitch(resultVoice);
		return resultChord;
	}
};

inline SILENCE_PUBLIC bool operator == (const Chord &a, const Chord &b) {
	if (&a == &b) {
		return true;
	}
	if (a.voices() != b.voices()) {
		return false;
	}
	for (size_t voice = 0; voice < a.voices(); ++voice) {
		if (!eq_epsilon(a.getPitch(voice), b.getPitch(voice))) {
			return false;
		}
	}
	return true;
}

inline SILENCE_PUBLIC bool operator < (const Chord &a, const Chord &b) {
	size_t n = std::min(a.voices(), b.voices());
	for (size_t voice = 0; voice < n; voice++) {
		if (lt_epsilon(a.getPitch(voice), b.getPitch(voice))) {
			return true;
		}
		if (gt_epsilon(a.getPitch(voice), b.getPitch(voice))) {
			return false;
		}
	}
	if (a.voices() < b.voices()) {
		return true;
	}
	return false;
}

inline SILENCE_PUBLIC bool operator <= (const Chord &a, const Chord &b) {
	if (a == b) {
		return true;
	}
	return (a < b);
}

inline SILENCE_PUBLIC bool operator > (const Chord &a, const Chord &b) {
	size_t n = std::min(a.voices(), b.voices());
	for (size_t voice = 0; voice < n; voice++) {
		if (gt_epsilon(a.getPitch(voice), b.getPitch(voice))) {
			return true;
		}
		if (lt_epsilon(a.getPitch(voice), b.getPitch(voice))) {
			return false;
		}
	}
	if (a.voices() > b.voices()) {
		return true;
	}
	return false;
}

inline SILENCE_PUBLIC bool operator >= (const Chord &a, const Chord &b) {
	if (a == b) {
		return true;
	}
	return (a > b);
}

/**
 * Returns the Euclidean distance between chords a and b,
 * which must have the same number of voices.
 */
inline SILENCE_PUBLIC double euclidean(const Chord &a, const Chord &b) {
	double sumOfSquaredDifferences = 0.0;
	for (size_t voice = 0, voices = a.voices(); voice < voices; ++voice) {
		sumOfSquaredDifferences += std::pow((a.getPitch(voice) - b.getPitch(voice)), 2.0);
	}
	return std::sqrt(sumOfSquaredDifferences);
}

/**
 * Returns the chord that is the midpoint between two chords,
 * which must have the same number of voices.
 */
inline SILENCE_PUBLIC Chord midpoint(const Chord &a, const Chord &b) {
    Chord midpoint_ = a;
	for (int voice = 0, voices = a.voices(); voice < voices; ++voice) {
        double voiceSum = a.getPitch(voice) + b.getPitch(voice);
        double voiceMidpoint = voiceSum / 2.0;
        midpoint_.setPitch(voice, voiceMidpoint);
	}
    //csound::print("a: %s  b: %s  mid: %s\n", a.toString().c_str(), b.toString().c_str(), midpoint_.toString().c_str());
	return midpoint_;
}


inline SILENCE_PUBLIC const std::map<std::string, double> &pitchClassesForNames() {
	static bool pitchClassesForNamesInitialized = false;
	static std::map<std::string, double> pitchClassesForNames_;
	if (!pitchClassesForNamesInitialized) {
		pitchClassesForNamesInitialized = true;
		pitchClassesForNames_["Ab"] =   8.;
		pitchClassesForNames_["A" ] =   9.;
		pitchClassesForNames_["A#"] =  10.;
		pitchClassesForNames_["Bb"] =  10.;
		pitchClassesForNames_["B" ] =  11.;
		pitchClassesForNames_["B#"] =   0.;
		pitchClassesForNames_["Cb"] =  11.;
		pitchClassesForNames_["C" ] =   0.;
		pitchClassesForNames_["C#"] =   1.;
		pitchClassesForNames_["Db"] =   1.;
		pitchClassesForNames_["D" ] =   2.;
		pitchClassesForNames_["D#"] =   3.;
		pitchClassesForNames_["Eb"] =   3.;
		pitchClassesForNames_["E" ] =   4.;
		pitchClassesForNames_["E#"] =   5.;
		pitchClassesForNames_["Fb"] =   4.;
		pitchClassesForNames_["F" ] =   5.;
		pitchClassesForNames_["F#"] =   6.;
		pitchClassesForNames_["Gb"] =   6.;
		pitchClassesForNames_["G" ] =   7.;
		pitchClassesForNames_["G#"] =   8.;
	}
	return const_cast<std::map<std::string, double> &>(pitchClassesForNames_);
}

inline SILENCE_PUBLIC double pitchClassForName(std::string name) {
	const std::map<std::string, double> &pitchClassesForNames_ = pitchClassesForNames();
	std::map<std::string, double>::const_iterator it = pitchClassesForNames_.find(name);
	if (it == pitchClassesForNames_.end()) {
		return DBL_MAX;
	} else {
		return it->second;
	}
}

inline SILENCE_PUBLIC std::map<Chord, std::string> &namesForChords() {
	static std::map<Chord, std::string> namesForChords_;
	return namesForChords_;
}

inline SILENCE_PUBLIC std::map<std::string, Chord> &chordsForNames() {
	static std::map<std::string, Chord> chordsForNames_;
	return chordsForNames_;
}

inline SILENCE_PUBLIC std::vector<std::string> split(std::string string_) {
	std::vector<std::string> tokens;
	std::istringstream iss(string_);
	std::copy(std::istream_iterator<std::string>(iss),
	        std::istream_iterator<std::string>(),
	        std::back_inserter<std::vector<std::string> >(tokens));
	return tokens;
}

inline void fill(std::string rootName, double rootPitch, std::string typeName, std::string typePitches, bool debug = false) {
	Chord chord;
	std::string chordName = rootName + typeName;
	std::vector<std::string> splitPitches = split(typePitches);
	if (debug) {
		csound::print("chordName: %s = rootName: %s  rootPitch: %f  typeName: %s  typePitches: %s\n", chordName.c_str(), rootName.c_str(), rootPitch, typeName.c_str(), typePitches.c_str());
	}
	chord.resize(splitPitches.size());
	for (int voice = 0, voiceN = splitPitches.size(); voice < voiceN; ++voice) {
		double pitch = pitchClassForName(splitPitches[voice]);
		if (debug) {
			csound::print("voice: %3d  pc: %-4s  pitch: %9.4f\n", voice, splitPitches[voice].c_str(), pitch);
		}
		chord.setPitch(voice, pitch);
	}
	if (debug) {
		print("chord type: %s\n", chord.toString().c_str());
	}
	chord = chord.T(rootPitch);
	Chord eOP_ = chord.eOP();
	if (debug) {
		print("eOP_:   %s  chordName: %s\n", eOP_.toString().c_str(), chordName.c_str());
	}
	chordsForNames()[chordName] = eOP_;
	namesForChords()[eOP_] = chordName;
}

inline void initializeNames() {
	static bool initializeNamesInitialized = false;
	if (!initializeNamesInitialized) {
		initializeNamesInitialized = true;
		csound::print("Initializing chord names...\n");
		const std::map<std::string, double> &pitchClassesForNames_ = pitchClassesForNames();
		for (std::map<std::string, double>::const_iterator it = pitchClassesForNames_.begin();
		        it != pitchClassesForNames_.end();
		        ++it) {
			const std::string &rootName = it->first;
			const double &rootPitch = it->second;
			print("rootName: %-3s  rootPitch: %9.5f\n", rootName.c_str(), rootPitch);
			fill(rootName, rootPitch, " minor second",     "C  C#                             ");
			fill(rootName, rootPitch, " major second",     "C     D                           ");
			fill(rootName, rootPitch, " minor third",      "C        Eb                       ");
			fill(rootName, rootPitch, " major third",      "C           E                     ");
			fill(rootName, rootPitch, " perfect fourth",   "C              F                  ");
			fill(rootName, rootPitch, " tritone",          "C                 F#              ");
			fill(rootName, rootPitch, " perfect fifth",    "C                    G            ");
			fill(rootName, rootPitch, " augmented fifth",  "C                       G#        ");
			fill(rootName, rootPitch, " sixth",            "C                          A      ");
			fill(rootName, rootPitch, " minor seventh  ",  "C                             Bb  ");
			fill(rootName, rootPitch, " major seventh",    "C                                B");
			// Scales.
			fill(rootName, rootPitch, " major",            "C     D     E  F     G     A     B");
			fill(rootName, rootPitch, " minor",            "C     D  Eb    F     G  Ab    Bb  ");
			fill(rootName, rootPitch, " natural minor",    "C     D  Eb    F     G  Ab    Bb  ");
			fill(rootName, rootPitch, " harmonic minor",   "C     D  Eb    F     G  Ab       B");
			fill(rootName, rootPitch, " chromatic",        "C  C# D  D# E  F  F# G  G# A  A# B");
			fill(rootName, rootPitch, " whole tone",       "C     D     E     F#    G#    A#  ");
			fill(rootName, rootPitch, " diminished",       "C     D  D#    F  F#    G# A     B");
			fill(rootName, rootPitch, " pentatonic",       "C     D     E        G     A      ");
			fill(rootName, rootPitch, " pentatonic major", "C     D     E        G     A      ");
			fill(rootName, rootPitch, " pentatonic minor", "C        Eb    F     G        Bb  ");
			fill(rootName, rootPitch, " augmented",        "C        Eb E        G  Ab    Bb  ");
			fill(rootName, rootPitch, " Lydian dominant",  "C     D     E     Gb G     A  Bb  ");
			fill(rootName, rootPitch, " 3 semitone",       "C        D#       F#       A      ");
			fill(rootName, rootPitch, " 4 semitone",       "C           E           G#        ");
			fill(rootName, rootPitch, " blues",            "C     D  Eb    F  Gb G        Bb  ");
			fill(rootName, rootPitch, " bebop",            "C     D     E  F     G     A  Bb B");
			// Major chords.
			fill(rootName, rootPitch, "M",                 "C           E        G            ");
			fill(rootName, rootPitch, "6",                 "C           E        G     A      ");
			fill(rootName, rootPitch, "69",                "C     D     E        G     A      ");
			fill(rootName, rootPitch, "69b5",              "C     D     E     Gb       A      ");
			fill(rootName, rootPitch, "M7",                "C           E        G           B");
			fill(rootName, rootPitch, "M9",                "C     D     E        G           B");
			fill(rootName, rootPitch, "M11",               "C     D     E  F     G           B");
			fill(rootName, rootPitch, "M#11",              "C     D     E  F#    G           B");
			fill(rootName, rootPitch, "M13",               "C     D     E  F     G     A     B");
			// Minor chords.
			fill(rootName, rootPitch, "m",                 "C        Eb          G            ");
			fill(rootName, rootPitch, "m6",                "C        Eb          G     A      ");
			fill(rootName, rootPitch, "m69",               "C     D  Eb          G     A      ");
			fill(rootName, rootPitch, "m7",                "C        Eb          G        Bb  ");
			fill(rootName, rootPitch, "m#7",               "C        Eb          G           B");
			fill(rootName, rootPitch, "m7b5",              "C        Eb       Gb          Bb  ");
			fill(rootName, rootPitch, "m9",                "C     D  Eb          G        Bb  ");
			fill(rootName, rootPitch, "m9#7",              "C     D  Eb          G           B");
			fill(rootName, rootPitch, "m11",               "C     D  Eb    F     G        Bb  ");
			fill(rootName, rootPitch, "m#11",              "C     D  Eb    F     G        Bb  ");
			fill(rootName, rootPitch, "m13",               "C     D  Eb    F     G     A  Bb  ");
			// Augmented chords.
			fill(rootName, rootPitch, "+",                 "C            E         G#         ");
			fill(rootName, rootPitch, "7#5",               "C            E         G#     Bb  ");
			fill(rootName, rootPitch, "7b9#5",             "C  Db        E         G#     Bb  ");
			fill(rootName, rootPitch, "9#5",               "C     D      E         G#     Bb  ");
			// Diminished chords.
			fill(rootName, rootPitch, "o",                 "C        Eb       Gb              ");
			fill(rootName, rootPitch, "o7",                "C        Eb       Gb       A      ");
			// Suspended chords.
			fill(rootName, rootPitch, "6sus",              "C              F     G     A      ");
			fill(rootName, rootPitch, "69sus",             "C     D        F     G     A      ");
			fill(rootName, rootPitch, "7sus",              "C              F     G        Bb  ");
			fill(rootName, rootPitch, "9sus",              "C     D        F     G        Bb  ");
			fill(rootName, rootPitch, "M7sus",             "C              F     G           B");
			fill(rootName, rootPitch, "M9sus",             "C     D        F     G           B");
			// Dominant chords.
			fill(rootName, rootPitch, "7",                 "C            E       G        Bb  ");
			fill(rootName, rootPitch, "7b5",               "C            E    Gb          Bb  ");
			fill(rootName, rootPitch, "7b9",               "C  Db        E       G        Bb  ");
			fill(rootName, rootPitch, "7b9b5",             "C  Db        E    Gb          Bb  ");
			fill(rootName, rootPitch, "9",                 "C     D      E       G        Bb  ");
			fill(rootName, rootPitch, "9#11",              "C     D      E F#    G        Bb  ");
			fill(rootName, rootPitch, "13",                "C     D      E F     G     A  Bb  ");
			fill(rootName, rootPitch, "13#11",             "C     D      E F#    G     A  Bb  ");
		}
	}
}

inline SILENCE_PUBLIC std::string nameForChord(const Chord &chord) {
	static bool nameForChordInitialized = false;
	if (!nameForChordInitialized) {
		nameForChordInitialized = true;
		initializeNames();
	}
	std::map<Chord, std::string> &namesForChords_ = namesForChords();
	if (namesForChords_.find(chord) == namesForChords_.end()) {
		return "";
	} else {
		return namesForChords_[chord];
	}
}

inline SILENCE_PUBLIC const Chord &chordForName(std::string name) {
	static bool chordForNameInitialized = false;
	if (!chordForNameInitialized) {
		chordForNameInitialized = true;
		initializeNames();
	}
	const std::map<std::string, Chord> chordsForNames_ = chordsForNames();
	std::map<std::string, Chord>::const_iterator it = chordsForNames_.find(name);
	if (it == chordsForNames_.end()) {
		static Chord chord;
		chord.resize(0);
		return chord;
	} else {
		return it->second;
	}
}

inline SILENCE_PUBLIC bool next(Chord &iterator_, const Chord &origin, double range, double g) {
    int leastSignificantVoice = iterator_.voices() - 1;
	int mostSignificantVoice = 0;
    // Increment, as in an odometer.
	iterator_.setPitch(leastSignificantVoice, iterator_.getPitch(leastSignificantVoice) + g);
	// If necessary, carry the increment to the next most significant voice.
	for (int voice = leastSignificantVoice; voice > mostSignificantVoice; --voice) {
		if (gt_epsilon(iterator_.getPitch(voice), (origin.getPitch(voice) + range))) {
			iterator_.setPitch(voice, origin.getPitch(voice));
			iterator_.setPitch(voice - 1, iterator_.getPitch(voice - 1) + g);
		}
	}
	if (gt_epsilon(iterator_.getPitch(mostSignificantVoice), (origin.getPitch(mostSignificantVoice) + range))) {
		return false;
	}
	return true;
}

/**
 * Returns a chord with the specified number of voices all set to a first
 * pitch, useful as an iterator.
 */
inline SILENCE_PUBLIC Chord iterator(int voiceN, double first) {
	Chord odometer;
	odometer.resize(voiceN);
	for (int voice = 0; voice < voiceN; ++voice) {
		odometer.setPitch(voice, first);
	}
	return odometer;
}

/**
 * TODO: Change this to use strictly the representative fundamental domains.
 * Each iteration must be sent to the representative fundamental domain, then
 * added to the set.
 */
inline SILENCE_PUBLIC std::vector<Chord> allOfEquivalenceClass(int voiceN, std::string equivalence, double g = 1.0) {
	std::set<Chord> equivalentChords;
	int chordCount = 0;
	int equivalentChordCount = 0;
    Chord origin = iterator(voiceN, -13.0);
    Chord iterator_ = origin;
	if (equivalence == "OP") {
		while (next(iterator_, origin, 13.0, g) == true) {
			chordCount++;
			Chord chord = iterator_.eP();
			if (chord.iseOP() == true) {
				equivalentChordCount++;
				equivalentChords.insert(chord);
			}
		}
	}
	if (equivalence == "OPT") {
		while (next(iterator_, origin, 13.0, g) == true) {
			chordCount++;
			Chord chord = iterator_.eP();
			if (chord.iseOPT() == true) {
				equivalentChordCount++;
				equivalentChords.insert(chord);
			}
		}
	}
	if (equivalence == "OPTT") {
		while (next(iterator_, origin, 13.0, g) == true) {
			chordCount++;
			Chord chord = iterator_.eP();
			if (chord.iseOPTT() == true) {
				equivalentChordCount++;
				equivalentChords.insert(chord);
			}
		}
	}
	if (equivalence == "OPI") {
		while (next(iterator_, origin, 13.0, g) == true) {
			chordCount++;
			Chord chord = iterator_.eP();
			if (chord.iseOPI() == true) {
				equivalentChordCount++;
				equivalentChords.insert(chord);
			}
		}
	}
	if (equivalence == "OPTI") {
		while (next(iterator_, origin, 13.0, g) == true) {
			chordCount++;
			Chord chord = iterator_.eP();
			if (chord.iseOPTI() == true) {
				equivalentChordCount++;
				equivalentChords.insert(chord);
			}
		}
	}
	if (equivalence == "OPTTI") {
		while (next(iterator_, origin, 13.0, g) == true) {
			chordCount++;
			Chord chord = iterator_.eP();
			if (chord.iseOPTTI() == true) {
				equivalentChordCount++;
				equivalentChords.insert(chord);
			}
		}
	}
	std::vector<Chord> result;
	std::copy(equivalentChords.begin(), equivalentChords.end(), std::back_inserter(result));
	return result;
}
/**
 * Returns the voice-leading between chords a and b,
 * i.e. what you have to add to a to get b, as a
 * chord of directed intervals.
 */
inline SILENCE_PUBLIC Chord voiceleading(const Chord &a, const Chord &b) {
	Chord voiceleading_ = a;
	for (int voice = 0; voice < a.voices(); ++voice) {
		voiceleading_.setPitch(voice, b.getPitch(voice) - a.getPitch(voice));
	}
	return voiceleading_;
}

/**
 * Returns whether the voiceleading
 * between chords a and b contains a parallel fifth.
 */
inline SILENCE_PUBLIC bool parallelFifth(const Chord &a, const Chord &b) {
	Chord voiceleading_ = voiceleading(a, b);
	if (voiceleading_.count(7) > 1) {
		return true;
	} else {
		return false;
	}
}

/**
 * Returns the smoothness of the voiceleading between
 * chords a and b by L1 norm.
 */
inline SILENCE_PUBLIC double voiceleadingSmoothness(const Chord &a, const Chord &b) {
	double L1 = 0.0;
	for (int voice = 0; voice < a.voices(); ++voice) {
		L1 = L1 + std::abs(b.getPitch(voice) - a.getPitch(voice));
	}
	return L1;
}

/**
 * Returns which of the voiceleadings (source to d1, source to d2)
 * is the smoother (shortest moves), optionally avoiding parallel fifths.
 */
inline SILENCE_PUBLIC Chord voiceleadingSmoother(const Chord &source, const Chord &d1, const Chord &d2, bool avoidParallels = false, double range = OCTAVE()) {
	if (avoidParallels) {
		if (parallelFifth(source, d1)) {
			return d2;
		}
		if (parallelFifth(source, d2)) {
			return d1;
		}
	}
	double s1 = voiceleadingSmoothness(source, d1);
	double s2 = voiceleadingSmoothness(source, d2);
	if (s1 <= s2) {
		return d1;
	} else {
		return d2;
	}
}
/**
 * Returns which of the voiceleadings (source to d1, source to d2)
 * is the simpler (fewest moves), optionally avoiding parallel fifths.
 */
inline SILENCE_PUBLIC Chord voiceleadingSimpler(const Chord &source, const Chord &d1, const Chord &d2, bool avoidParallels = false) {
	if (avoidParallels) {
		if (parallelFifth(source, d1)) {
			return d2;
		}
		if (parallelFifth(source, d2)) {
			return d1;
		}
	}
	// TODO: Verify this.
 	int s1 = voiceleading(source, d1).count(0.0);
	int s2 = voiceleading(source, d2).count(0.0);
	if (s1 > s2) {
		return d1;
	}
	if (s2 > s1) {
		return d2;
	}
	return d1;
}

/**
 * Returns which of the voiceleadings (source to d1, source to d2)
 * is the closer (first smoother, then simpler), optionally avoiding parallel fifths.
 */
inline SILENCE_PUBLIC Chord voiceleadingCloser(const Chord &source, const Chord &d1, const Chord &d2, bool avoidParallels = false) {
	if (avoidParallels) {
		if (parallelFifth(source, d1)) {
			return d2;
		}
		if (parallelFifth(source, d2)) {
			return d1;
		}
	}
	double s1 = voiceleadingSmoothness(source, d1);
	double s2 = voiceleadingSmoothness(source, d2);
	if (s1 < s2) {
		return d1;
	}
	if (s2 > s1) {
		return d2;
	}
	return voiceleadingSimpler(source, d1, d2, avoidParallels);
}

/**
 * Returns the voicing of the destination which has the closest voice-leading
 * from the source within the range, optionally avoiding parallel fifths.
 */
inline SILENCE_PUBLIC Chord voiceleadingClosestRange(const Chord &source, const Chord &destination, double range, bool avoidParallels = false) {
	Chord destinationOP = destination.eOP();
	Chord d = destinationOP;
    Chord origin = source.eOP();
	Chord odometer = origin;
	while (next(odometer, origin, range, OCTAVE())) {
		Chord revoicing = odometer;
		for (int voice = 0; voice < revoicing.voices(); ++voice) {
			revoicing.setPitch(voice, revoicing.getPitch(voice) + destinationOP.getPitch(voice));
		}
		d = voiceleadingCloser(source, d, revoicing, avoidParallels);
	}
	return d;
}

/**
 * Returns the pitch from the chord that is closest to the pitch.
 */
inline SILENCE_PUBLIC double closestPitch(double pitch, const Chord &chord) {
	std::map<double, double> pitchesForDistances;
	for (int voice = 0; voice < chord.voices(); ++voice) {
		double chordPitch = chord.getPitch(voice);
		double distance = std::fabs(chordPitch - pitch);
		pitchesForDistances[distance] = chordPitch;
	}
	return pitchesForDistances.begin()->second;
}

/**
* Conform the pitch to the pitch-class set, but in its original register.
*/
inline SILENCE_PUBLIC double conformToPitchClassSet(double pitch, const Chord &pcs) {
	double pc_ = epc(pitch);
	double closestPc = closestPitch(pc_, pcs);
	double register_ = std::floor(pitch / OCTAVE()) * OCTAVE();
	double closestPitch = register_ + closestPc;
	return closestPitch;
}

/**
 * If the Event is a note, moves its pitch
 * to the closest pitch of the chord.
 * If octaveEquivalence is true (the default),
 * the pitch-class of the note is moved to the closest pitch-class
 * of the chord, i.e. keeping the note more or less in its original register;
 * otherwise, the pitch of the note is moved to the closest
 * absolute pitch of the chord.
 */
inline SILENCE_PUBLIC void conformToChord(Event &event, const Chord &chord, bool octaveEquivalence = true) {
	if (!event.isNoteOn()) {
		return;
	}
	double pitch = event.getKey();
	if (octaveEquivalence) {
		Chord pcs = chord.epcs();
		pitch = conformToPitchClassSet(pitch, pcs);
	} else {
		pitch = closestPitch(pitch, chord);
	}
	event.setKey(pitch);
}

/**
 * Inserts the notes of the chord into the score at the specified time.
 */
inline SILENCE_PUBLIC void insert(Score &score,
        const Chord &chord,
        double time_) {
	chord.toScore(score, time_);
}

/**
 * Returns a slice of the Score starting at the start time and extending up
 * to but not including the end time. The slice contains pointers to the Events
 * in the Score.
 */
inline SILENCE_PUBLIC std::vector<Event *> slice(Score &score, double startTime, double endTime) {
	std::vector<Event *> result;
	for (int i = 0, n = score.size(); i < n; ++i) {
		Event *event = &score[i];
		if (event->isNoteOn()) {
			double eventStart = event->getTime();
			if (eventStart >= startTime && eventStart < endTime) {
				result.push_back(event);
			}
		}
	}
	return result;
}

/**
 * For all the notes in the Score
 * beginning at or later than the start time,
 * and up to but not including the end time,
 * moves the pitch of the note to belong to the chord, using the
 * conformToChord function.
 */
inline SILENCE_PUBLIC void apply(Score &score, const Chord &chord, double startTime, double endTime, bool octaveEquivalence = true) {
	std::vector<Event *> slice_ = slice(score, startTime, endTime);
	for (int i = 0; i < slice_.size(); ++i) {
		Event &event = *slice_[i];
		conformToChord(event, chord, octaveEquivalence);
	}
}

/**
 * Returns a chord containing all the pitches of the score
 * beginning at or later than the start time,
 * and up to but not including the end time.
 */
inline SILENCE_PUBLIC Chord gather(Score &score, double startTime, double endTime) {
	std::vector<Event *> slice_ = slice(score, startTime, endTime);
	std::set<double> pitches;
	for (int i = 0; i < slice_.size(); ++i) {
		pitches.insert(slice_[i]->getKey());
	}
	Chord chord;
	chord.resize(pitches.size());
	int voice = 0;
	for (std::set<double>::iterator it = pitches.begin(); it != pitches.end(); ++it) {
		chord.setPitch(voice, *it);
		voice++;
	}
        return chord;
}

inline SILENCE_PUBLIC int octavewiseRevoicings(const Chord &chord,
                                               double range = OCTAVE()) {
    Chord origin = chord.eOP();
    Chord odometer = origin;
    // Enumerate the permutations.
    int voicings = 0;
    while (next(odometer, origin, range, OCTAVE())) {
      voicings = voicings + 1;
    }
    if (debug) {
      print("octavewiseRevoicings: chord:    %s\n", chord.toString().c_str());
      print("octavewiseRevoicings: eop:      %s\n", chord.eOP().toString().c_str());
      print("octavewiseRevoicings: odometer: %s\n", odometer.toString().c_str());
      print("octavewiseRevoicings: voicings: %5d\n", voicings);
    }
    return voicings;
}

SILENCE_PUBLIC Chord octavewiseRevoicing(const Chord &chord, int revoicingNumber_, double range, bool debug=false);
/**
 * Returns the index of the octavewise revoicing that this chord is,
 * relative to its OP equivalent, within the indicated range. Returns
 * -1 if there is no such chord within the range.
 */
SILENCE_PUBLIC int indexForOctavewiseRevoicing(const Chord &chord, double range, bool debug=false);

/*
template<int EQUIVALENCE_RELATION> SILENCE_PUBLIC bool isNormal(const Chord &chord, double range, double g) {
	Chord normal = normalize<EQUIVALENCE_RELATION>(chord, range, g);
	if (normal == chord) {
		return true;
	} else {
		return false;
	}
}
*/

template<int EQUIVALENCE_RELATION> inline SILENCE_PUBLIC bool isNormal(const Chord &chord, double range) {
	bool result = isNormal<EQUIVALENCE_RELATION>(chord, range, 1.0);
    return result;
}

template<int EQUIVALENCE_RELATION> inline SILENCE_PUBLIC bool isNormal(const Chord &chord) {
	bool result = isNormal<EQUIVALENCE_RELATION>(chord, OCTAVE());
    return result;
}

template<int EQUIVALENCE_RELATION> inline SILENCE_PUBLIC bool isEquivalent(const Chord &a, const Chord &b, double range, double g) {
	if (isNormal<EQUIVALENCE_RELATION>(a, range, g) == false) {
		return false;
	}
	if (isNormal<EQUIVALENCE_RELATION>(b, range, g) == false) {
		return false;
	}
    Chord normalA = normalize<EQUIVALENCE_RELATION>(a, range, g);
    Chord normalB = normalize<EQUIVALENCE_RELATION>(b, range, g);
    if (normalA == normalB) {
        return true;
    } else {
        return false;
    }
}

template<int EQUIVALENCE_RELATION> inline SILENCE_PUBLIC bool isEquivalent(const Chord &a, const Chord &b, double range) {
    return isEquivalent<EQUIVALENCE_RELATION>(a, b, range, 1.0);
}

template<int EQUIVALENCE_RELATION> inline SILENCE_PUBLIC bool isEquivalent(const Chord &a, const Chord &b) {
    return isEquivalent<EQUIVALENCE_RELATION>(a, b, OCTAVE());
}

template<int EQUIVALENCE_RELATION> inline SILENCE_PUBLIC Chord normalize(const Chord &chord, double range) {
    return normalize<EQUIVALENCE_RELATION>(chord, range, 1.0);
}

template<int EQUIVALENCE_RELATION> inline SILENCE_PUBLIC Chord normalize(const Chord &chord) {
    return normalize<EQUIVALENCE_RELATION>(chord, OCTAVE());
}

//	EQUIVALENCE_RELATION_r

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_r>(const Chord &chord, double range, double g) {
    for (int voice = 0; voice < chord.voices(); ++voice) {
        double pitch = chord.getPitch(voice);
        if (le_epsilon(0.0, pitch) == false) {
            return false;
        }
        if (lt_epsilon(pitch, range) == false) {
            return false;
        }
    }
    return true;
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_r>(const Chord &chord, double range, double g) {
	Chord normal = chord;
	for (int voice = 0; voice < chord.voices(); ++voice) {
		double pitch = chord.getPitch(voice);
		pitch = modulo(pitch, range);
		normal.setPitch(voice, pitch);
	}
	return normal;
}

//	EQUIVALENCE_RELATION_R

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_R>(const Chord &chord, double range, double g) {
    double max = chord.max()[0];
    double min = chord.min()[0];
    if (le_epsilon(max, (min + range)) == false) {
        return false;
    }
    double layer = chord.layer();
    if (le_epsilon(0.0, layer) == false) {
        return false;
    }
    if (lt_epsilon(layer, range) == false) {
        return false;
    }
    return true;
}

inline bool Chord::iseR(double range_) const {
    return isNormal<EQUIVALENCE_RELATION_R>(*this, range_, 1.0);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_R>(const Chord &chord, double range, double g) {
    Chord normal = normalize<EQUIVALENCE_RELATION_r>(chord, range, g);
    //Chord normal = chord;
	//while (le_epsilon(0.0, normal.layer()) == false) {
	//	std::vector<double> minimum = normal.min();
	//	normal.setPitch(minimum[1], minimum[0] + range);
	//}
	while (lt_epsilon(normal.layer(), range) == false) {
		std::vector<double> maximum = normal.max();
		normal.setPitch(maximum[1], maximum[0] - range);
	}
	return normal;
}

inline Chord Chord::eR(double range) const {
    return csound::normalize<EQUIVALENCE_RELATION_R>(*this, range, 1.0);
}

//	EQUIVALENCE_RELATION_P

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_P>(const Chord &chord, double range, double g) {
    for (size_t voice = 1; voice < chord.voices(); voice++) {
        if (gt_epsilon(chord.getPitch(voice - 1), chord.getPitch(voice))) {
            return false;
        }
    }
    return true;
}

inline bool Chord::iseP() const {
    return isNormal<EQUIVALENCE_RELATION_P>(*this, OCTAVE(), 1.0);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_P>(const Chord &chord, double range, double g) {
	Chord normal = chord;
	bool sorted = false;
	while (!sorted) {
		sorted = true;
		for (int voice = 1; voice < normal.voices(); voice++) {
			if (gt_epsilon(normal.getPitch(voice - 1), normal.getPitch(voice))) {
				sorted = false;
				normal.row(voice - 1).swap(normal.row(voice));
			}
		}
	}
	return normal;
}

inline Chord Chord::eP() const {
    return csound::normalize<EQUIVALENCE_RELATION_P>(*this, OCTAVE(), 1.0);
}

//	EQUIVALENCE_RELATION_T

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_T>(const Chord &chord, double range, double g) {
    double layer_ = chord.layer();
    if (!(eq_epsilon(layer_, 0.0))) {
        return false;
    } else {
        return true;
    }
}

inline bool Chord::iseT() const {
    return isNormal<EQUIVALENCE_RELATION_T>(*this, OCTAVE(), 1.0);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_T>(const Chord &chord, double range, double g) {
	Chord normal = chord;
    double layer_ = normal.layer();
    double sumPerVoice = layer_ / double(normal.voices());
    normal = normal.T(-sumPerVoice);
	return normal;
}

inline Chord Chord::eT() const {
    return csound::normalize<EQUIVALENCE_RELATION_T>(*this, OCTAVE(), 1.0);
}

//	EQUIVALENCE_RELATION_Tg

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_Tg>(const Chord &chord, double range, double g) {
	Chord normal = normalize<EQUIVALENCE_RELATION_T>(chord, range, g);
    // Make it work for any g not just 1: double transposition = std::ceil(normal.getPitch(0)) - normal.getPitch(0);
    double ng = std::ceil(normal.getPitch(0) / g);
    double transposition = (ng * g) - normal.getPitch(0);
    normal = normal.T(transposition);
	return normal;
}

inline Chord Chord::eTT(double g) const {
    return csound::normalize<EQUIVALENCE_RELATION_Tg>(*this, OCTAVE(), g);
}

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_Tg>(const Chord &chord, double range, double g) {
    Chord normalP = normalize<EQUIVALENCE_RELATION_P>(chord, range, g);
    Chord normalPTg = normalize<EQUIVALENCE_RELATION_Tg>(normalP, range, g);
    if (normalP == normalPTg) {
        return true;
    } else {
        return false;
    }
}

inline bool Chord::iseTT(double g) const {
    return isNormal<EQUIVALENCE_RELATION_Tg>(*this, OCTAVE(), g);
}

//	EQUIVALENCE_RELATION_I

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_I>(const Chord &chord, double range, double g) {
    int lowerVoice = 1;
    int upperVoice = chord.voices() - 1;
    // Compare the intervals in a chord with those in its inverse,
    // starting with the "first." This is NOT the same as
    // whether the chord is less than or equal to its inverse.
    while (lowerVoice < upperVoice) {
        int lowerInterval = chord.getPitch(lowerVoice) - chord.getPitch(lowerVoice - 1);
        int upperInterval = chord.getPitch(upperVoice) - chord.getPitch(upperVoice - 1);
        if (lt_epsilon(lowerInterval, upperInterval)) {
            return true;
        }
        if (gt_epsilon(lowerInterval, upperInterval)) {
            return false;
        }
        lowerVoice = lowerVoice + 1;
        upperVoice = upperVoice - 1;
    }
    return true;
}

inline bool Chord::iseI(Chord *inverse) const {
    return isNormal<EQUIVALENCE_RELATION_I>(*this, OCTAVE(), 1.0);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_I>(const Chord &chord, double range, double g) {
    if (isNormal<EQUIVALENCE_RELATION_I>(chord, range, g)) {
        return chord;
    } else {
        return chord.I();
    }
}

inline Chord Chord::eI() const {
    return csound::normalize<EQUIVALENCE_RELATION_I>(*this, OCTAVE(), 1.0);
}

//	EQUIVALENCE_RELATION_V

//  TODO: Is this correct?

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_V>(const Chord &chord, double range, double g) {
    double outer = chord.getPitch(0) + range - chord.getPitch(chord.voices() - 1);
    bool isNormal = true;
    for (size_t voice = 0; voice < chord.voices() - 1; voice++) {
        double inner = chord.getPitch(voice + 1) - chord.getPitch(voice);
        if (!(ge_epsilon(outer, inner))) {
            isNormal = false;
        }
    }
    return isNormal;
}

inline bool Chord::iseV() const {
    return isNormal<EQUIVALENCE_RELATION_V>(*this, OCTAVE(), 1.0);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_V>(const Chord &chord, double range, double g) {
    const std::vector<Chord> permutations = chord.permutations();
    for (size_t i = 0; i < permutations.size(); i++) {
        const Chord &permutation = permutations[i];
        if (isNormal<EQUIVALENCE_RELATION_V>(permutation, range, g)) {
            return permutation;
        }
    }
    throw "Shouldn't come here.";
}

inline Chord Chord::eV() const {
    return csound::normalize<EQUIVALENCE_RELATION_V>(*this, OCTAVE(), 1.0);
}

//  EQUIVALENCE_RELATION_RP

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_RP>(const Chord &chord, double range, double g) {
    if (!isNormal<EQUIVALENCE_RELATION_P>(chord, range, g)) {
        return false;
    }
    if (!isNormal<EQUIVALENCE_RELATION_R>(chord, range, g)) {
        return false;
    }
    return true;
}

inline bool Chord::iseRP(double range) const {
    return isNormal<EQUIVALENCE_RELATION_RP>(*this, range, 1.0);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_RP>(const Chord &chord, double range, double g) {
    Chord normal = normalize<EQUIVALENCE_RELATION_R>(chord, range, g);
    normal = normalize<EQUIVALENCE_RELATION_P>(normal, range, g);
    return normal;
}

inline Chord Chord::eRP(double range) const {
    return csound::normalize<EQUIVALENCE_RELATION_RP>(*this, range, 1.0);
}

//	EQUIVALENCE_RELATION_RT

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_RT>(const Chord &chord, double range, double g) {
    Chord normalR = normalize<EQUIVALENCE_RELATION_R>(chord, range, g);
    std::vector<Chord> voicings_ = normalR.voicings();
    for (size_t voice = 0; voice < normalR.voices(); voice++) {
        const Chord &voicing = voicings_[voice];
        if (isNormal<EQUIVALENCE_RELATION_V>(voicing, range, g)) {
            return normalize<EQUIVALENCE_RELATION_T>(voicing, range, g);
        }
    }
    throw "Shouldn't come here.";
}

//	EQUIVALENCE_RELATION_RTg

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_RTg>(const Chord &chord, double range, double g) {
    Chord normalR = normalize<EQUIVALENCE_RELATION_R>(chord, range, g);
    std::vector<Chord> voicings_ = normalR.voicings();
    for (size_t voice = 0; voice < normalR.voices(); voice++) {
        const Chord &voicing = voicings_[voice];
        const Chord normalTg = normalize<EQUIVALENCE_RELATION_Tg>(voicing, range, g);
        if (isNormal<EQUIVALENCE_RELATION_V>(normalTg, range, g)) {
            return normalTg;
        }
    }
    throw "Shouldn't come here.";
}

//	EQUIVALENCE_RELATION_RI

//	EQUIVALENCE_RELATION_PT

//	EQUIVALENCE_RELATION_PTg

//	EQUIVALENCE_RELATION_PI

//	EQUIVALENCE_RELATION_TI

//	EQUIVALENCE_RELATION_RPT

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_RPT>(const Chord &chord, double range, double g) {
    if (!isNormal<EQUIVALENCE_RELATION_R>(chord, range, g)) {
        return false;
    }
    if (!isNormal<EQUIVALENCE_RELATION_P>(chord, range, g)) {
        return false;
    }
    if (!isNormal<EQUIVALENCE_RELATION_T>(chord, range, g)) {
        return false;
    }
    // TODO: Should this be here?
    if (!isNormal<EQUIVALENCE_RELATION_V>(chord, range, g)) {
        return false;
    }
    return true;
}

inline bool Chord::iseRPT(double range) const {
    return isNormal<EQUIVALENCE_RELATION_RPT>(*this, range, 1.0);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_RPT>(const Chord &chord, double range, double g) {
    Chord normalRP = normalize<EQUIVALENCE_RELATION_RP>(chord, range, g);
    std::vector<Chord> voicings_ = normalRP.voicings();
    for (size_t voice = 0; voice < normalRP.voices(); voice++) {
        const Chord &voicing = voicings_[voice];
        if (isNormal<EQUIVALENCE_RELATION_V>(voicing, range, g)) {
            return normalize<EQUIVALENCE_RELATION_T>(voicing, range, g);
        }
    }
    throw "Shouldn't come here.";
}

inline Chord Chord::eRPT(double range) const {
    return csound::normalize<EQUIVALENCE_RELATION_RPT>(*this, range, 1.0);
}

//	EQUIVALENCE_RELATION_RPTg

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_RPTg>(const Chord &chord, double range, double g) {
    if (!isNormal<EQUIVALENCE_RELATION_R>(chord, range, g)) {
        return false;
    }
    if (!isNormal<EQUIVALENCE_RELATION_P>(chord, range, g)) {
        return false;
    }
    if (!isNormal<EQUIVALENCE_RELATION_Tg>(chord, range, g)) {
        return false;
    }
    // TODO: Should this be here?
    if (!isNormal<EQUIVALENCE_RELATION_V>(chord, range, g)) {
        return false;
    }
    return true;
}

inline bool Chord::iseRPTT(double range, double g) const {
    return isNormal<EQUIVALENCE_RELATION_RPTg>(*this, range, g);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_RPTg>(const Chord &chord, double range, double g) {
    Chord normalRP = normalize<EQUIVALENCE_RELATION_RP>(chord, range, g);
    std::vector<Chord> voicings_ = normalRP.voicings();
    for (size_t voice = 0; voice < normalRP.voices(); voice++) {
        const Chord &voicing = voicings_[voice];
        Chord normalTg = normalize<EQUIVALENCE_RELATION_Tg>(voicing, range, g);
        if (isNormal<EQUIVALENCE_RELATION_V>(normalTg, range, g) == true) {
            return normalTg;
        }
    }
    throw "Shouldn't come here.";
}

inline Chord Chord::eRPTT(double range, double g) const {
    return csound::normalize<EQUIVALENCE_RELATION_RPTg>(*this, range, g);
}

//	EQUIVALENCE_RELATION_RPI

/*
That Ic acts on R 2 / P by reflection in the line of Ic -invariant P-classes, or the
inversion flat, points to a general principle of the action of Ic on any of the spaces from
sections four and five: Ic corresponds to a reflection in the flat of chords or equivalence
classes of chords that remain invariant under Ic . Therefore, the operative question when
considering Ic -equivalence on any space is "Where are the Ic -invariant points?" Once
we know the answer to this question, we know the precise action of Ic on the original
space. We have already seen this principle at work in the quotient spaces R n / Ic : there is
precisely one fixed point, A, such that Ic ( A) = A , and R n is transformed into R n / Ic by
gluing together points with their reflection in A. (Callender, Quinn, and Tymoczko, draft.)
*/

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_RPI>(const Chord &chord, double range, double g) {
    if (isNormal<EQUIVALENCE_RELATION_RP>(chord, range, g) == false) {
        return false;
    }
    Chord inverse = chord.I();
    Chord inverseRP = normalize<EQUIVALENCE_RELATION_RP>(inverse, range, g);
    if (chord <= inverseRP) {
        return true;
    }
    return false;
}

inline bool Chord::iseRPI(double range) const {
    return isNormal<EQUIVALENCE_RELATION_RPI>(*this, range, 1.0);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_RPI>(const Chord &chord, double range, double g) {
    if (isNormal<EQUIVALENCE_RELATION_RPI>(chord, range, g) == true) {
        return chord;
    }
    Chord normalRP = normalize<EQUIVALENCE_RELATION_RP>(chord, range, g);
    Chord normalRPInverse = normalRP.I();
    Chord normalRPInverseRP = normalize<EQUIVALENCE_RELATION_RP>(normalRPInverse, range, g);
    if (normalRP <= normalRPInverseRP) {
        return normalRP;
    } else {
        return normalRPInverseRP;
    }
}

inline Chord Chord::eRPI(double range) const {
    return csound::normalize<EQUIVALENCE_RELATION_RPI>(*this, range, 1.0);
}

//	EQUIVALENCE_RELATION_RTI

//	EQUIVALENCE_RELATION_RTgI

//	EQUIVALENCE_RELATION_RPTI

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_RPTI>(const Chord &chord, double range, double g) {
    if (!isNormal<EQUIVALENCE_RELATION_P>(chord, range, g)) {
        return false;
    }
    if (!isNormal<EQUIVALENCE_RELATION_R>(chord, range, g)) {
        return false;
    }
    if (!isNormal<EQUIVALENCE_RELATION_T>(chord, range, g)) {
        return false;
    }
    if (!isNormal<EQUIVALENCE_RELATION_V>(chord, range, g)) {
        return false;
    }
    return true;
}

inline bool Chord::iseRPTI(double range) const {
    return isNormal<EQUIVALENCE_RELATION_RPTI>(*this, range, 1.0);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_RPTI>(const Chord &chord, double range, double g) {
    Chord normalRPT = normalize<EQUIVALENCE_RELATION_RPT>(chord, range, g);
    if (isNormal<EQUIVALENCE_RELATION_I>(normalRPT, range, g) == true) {
        return normalRPT;
    } else {
        Chord normalI = normalize<EQUIVALENCE_RELATION_RPI>(normalRPT, range, g);
        Chord normalRPT = normalize<EQUIVALENCE_RELATION_RPT>(normalI, range, g);
        return normalRPT;
    }
}

inline Chord Chord::eRPTI(double range) const {
    return csound::normalize<EQUIVALENCE_RELATION_RPTI>(*this, range, 1.0);
}

//	EQUIVALENCE_RELATION_RPTgI

template<> inline SILENCE_PUBLIC bool isNormal<EQUIVALENCE_RELATION_RPTgI>(const Chord &chord, double range, double g) {
    if (isNormal<EQUIVALENCE_RELATION_RPTg>(chord, range, g) == false) {
        return false;
    }
    Chord inverse = chord.I();
    Chord normalRPTg = normalize<EQUIVALENCE_RELATION_RPTg>(inverse, range, g);
    if (chord <= normalRPTg) {
        return true;
    }
    return false;
}

inline bool Chord::iseRPTTI(double range) const {
    return isNormal<EQUIVALENCE_RELATION_RPTgI>(*this, range, 1.0);
}

template<> inline SILENCE_PUBLIC Chord normalize<EQUIVALENCE_RELATION_RPTgI>(const Chord &chord, double range, double g) {
    Chord normalRPTg = normalize<EQUIVALENCE_RELATION_RPTg>(chord, range, g);
    Chord inverse = normalRPTg.I();
    Chord inverseNormalRPTg = normalize<EQUIVALENCE_RELATION_RPTg>(inverse, range, g);
    if (normalRPTg <= inverseNormalRPTg) {
        return normalRPTg;
    }
    return inverseNormalRPTg;
}

inline Chord Chord::eRPTTI(double range) const {
    return csound::normalize<EQUIVALENCE_RELATION_RPTgI>(*this, range, 1.0);
}

template<int EQUIVALENCE_RELATION> inline SILENCE_PUBLIC std::set<Chord> fundamentalDomainByIsNormal(int voiceN, double range, double g)
{
    std::set<Chord> fundamentalDomain;
    int upperI = 2 * range + 1;
    int lowerI = - (range + 1);
    Chord origin = iterator(voiceN, lowerI);
    Chord chord = origin;
    int chords = 0;
    while (next(chord, origin, upperI, g) == true) {
        chords++;
        Chord copy = chord;
        bool isNormal_ = isNormal<EQUIVALENCE_RELATION>(copy, range, g);
        if (isNormal_ == true) {
            fundamentalDomain.insert(copy);
        }
        if (false) {
            print("By isNormal %-8s: chord: %6d  domain: %6d  range: %.2f  g: %7.2f  %s  isNormal: %d\n",
                namesForEquivalenceRelations[EQUIVALENCE_RELATION],
                chords,
                fundamentalDomain.size(),
                range,
                g,
                copy.toString().c_str(),
                isNormal_);
        }
    }
    return fundamentalDomain;
}

template<int EQUIVALENCE_RELATION> inline SILENCE_PUBLIC std::set<Chord> fundamentalDomainByNormalize(int voiceN, double range, double g)
{
    std::set<Chord> fundamentalDomain;
    int upperI = 2 * range + 1;
    int lowerI = - (range + 1);
    Chord origin = iterator(voiceN, lowerI);
    Chord chord = origin;
    int chords = 0;
    while (next(chord, origin, upperI, g) == true) {
        chords++;
        bool isNormal_ = isNormal<EQUIVALENCE_RELATION>(chord, range, g);
        Chord normal = normalize<EQUIVALENCE_RELATION>(chord, range, g);
        fundamentalDomain.insert(normal);
        if (false) {
            print("By normalize %-8s: chord: %6d  domain: %6d  range: %7.2f  g: %7.2f  %s  normalized: %s  isNormal: %d\n",
                namesForEquivalenceRelations[EQUIVALENCE_RELATION],
                chords,
                fundamentalDomain.size(),
                range,
                g,
                chord.toString().c_str(),
                normal.toString().c_str(),
                isNormal_);
        }
    }
    return fundamentalDomain;
}

/**
 * Orthogonal additive groups for unordered chords of given arity under range
 * equivalence (RP): prime form or P, inversion or I, transposition or T, and
 * voicing or V. P x I x T = OP, P x I x T x V = RP. Therefore, an
 * operation on P, I, T, or V may be used to independently transform the
 * respective symmetry of any chord. Some of these operations will reflect
 * in RP. Please note: some equivalence classes define quotient spaces
 * with singularities, meaning that more than one chord on the surface of
 * the space may have the same equivalence.
 */
class SILENCE_PUBLIC ChordSpaceGroup {
public:
	/**
	 * Number of voices in the chord space.
	 */
	int N;
	/**
	 * The generator of transposition.
	 */
	double g;
	/**
	 * The zero-based range of the chord space.
	 */
	double range;
	int countP;
	int countI;
	int countT;
	int countV;
	/**
	 * Ordered table of all OPTTI chords for g.
	 */
	std::vector<Chord> opttisForIndexes;
	std::map<Chord, int> indexesForOpttis;
	/**
	 * Ordered table of all octavewise permutations
	 * in RP (note: not OP).
	 */
	std::vector<Chord> voicingsForIndexes;
	std::map<Chord, int> indexesForVoicings;
	virtual void preinitialize(int N_, double range_, double g_ = 1.0) {
		opttisForIndexes.clear();
		indexesForOpttis.clear();
		voicingsForIndexes.clear();
		indexesForVoicings.clear();
		N = N_;
		range = range_;
		g = g_;
		countP = 0;
		countI = 2;
		countT = OCTAVE() / g;
		Chord chord;
		chord.resize(N);
		countV = octavewiseRevoicings(chord, range);
	}
	virtual void initialize(int N_, double range_, double g_ = 1.0) {
		preinitialize(N_, range_, g_);
        std::set<Chord> opttisForIndexes_ = fundamentalDomainByNormalize<EQUIVALENCE_RELATION_RPTgI>(N, OCTAVE(), g);
        for (std::set<Chord>::iterator it = opttisForIndexes_.begin(); it != opttisForIndexes_.end(); ++it) {
            opttisForIndexes.push_back(*it);
        }
		for (int i = 0, n = opttisForIndexes.size(); i < n; ++i) {
			indexesForOpttis[opttisForIndexes[i]] = i;
			countP = countP + 1;
		}
	}
	virtual void list(bool listheader = true, bool listopttis = false, bool listvoicings = false) const {
		if (listheader) {
			print("ChordSpaceGroup.voices: %8d\n", N);
			print("ChordSpaceGroup.range : %13.4f\n", range);
			print("ChordSpaceGroup.g     : %13.4f\n", g);
			print("ChordSpaceGroup.countP: %8d\n", countP);
			print("ChordSpaceGroup.countI: %8d\n", countI);
			print("ChordSpaceGroup.countT: %8d\n", countT);
			print("ChordSpaceGroup.countV: %8d\n", countV);
		}
		if (listopttis) {
			for (int i = 0, n = opttisForIndexes.size(); i < n; ++i) {
				const Chord &optti = opttisForIndexes[i];
				int index = indexesForOpttis.at(optti);
				print("index: %5d  optti: %s  index from optti: %5d  %s\n", i, optti.toString().c_str(), index, optti.name().c_str());
			}
		}
		// Doesn't currently do anything as these collections are not currently initialized.
		if (listvoicings) {
			for (int i = 0, n = voicingsForIndexes.size(); i < n; ++i) {
				const Chord &voicing = voicingsForIndexes[i];
				int index = indexesForVoicings.at(voicing);
				print("voicing index: %5d  voicing: %s  index from voicing: %5d\n", i,  voicing.toString().c_str(), index);
			}
		}
	}
	virtual std::string createFilename(int voices, double range, double g = 1.0) const {
		std::string extension = ".txt";
		char buffer[0x200];
		std::sprintf(buffer, "ChordSpaceGroup_V%d_R%d_g%d.txt", voices, int(range), int(1000 * g));
		return buffer;
	}
	/**
	 * Loads the group if found, creates and saves it otherwise.
	 */
	virtual void createChordSpaceGroup(int voices, double range, double g = 1.0) {
		std::string filename = createFilename(voices, range, g);
		std::fstream stream;
		stream.open(filename.c_str());
		if (!stream.is_open()) {
			print("No data in ChordSpaceGroup file \"%s\", initializing and saving...\n", filename.c_str());
			stream.close();
			stream.open(filename.c_str(), std::fstream::out);
			initialize(voices, range, g);
			save(stream);
		} else {
			print("Loading ChordSpaceGroup data from file \"%s\"...\n", filename.c_str());
			load(stream);
		}
		stream.close();
	}
	virtual void save(std::fstream &stream) const {
		stream << "N " << N << std::endl;
		stream << "range " << range << std::endl;
		stream << "g " << g << std::endl;
		for (int i = 0, n = opttisForIndexes.size(); i < n; ++i) {
			stream << opttisForIndexes[i].toString().c_str() << std::endl;
		}
	}
	virtual void load(std::fstream &stream) {
		std::string junk;
		stream >> junk >> N;
		stream >> junk >> range;
		stream >> junk >> g;
		preinitialize(N, range, g);
		char buffer[0x500];
		for (;;) {
			stream.getline(buffer, 0x500);
			if (stream.eof()) {
				break;
			}
			Chord chord;
			chord.fromString(buffer);
			if (chord.voices() > 1) {
				opttisForIndexes.push_back(chord);
				indexesForOpttis[chord] = opttisForIndexes.size() - 1;
			}
		}
		countP = opttisForIndexes.size();
	}
	/**
	 * Returns the indices of prime form, inversion, transposition,
	 * and voicing for a chord, as the first 4 elements, respectively,
	 * of a homogeneous vector.
	 *
	 * Please note: where are there singularities
	 * in the quotient spaces for chords, there may be several chords that
	 * belong to the same equivalence class. In such cases, although there
	 * will aways be a mapping from each set of indices to one chord, there
	 * may be several chords that map to the same set of indices.
	 * The result is returned in a homogeneous vector.
	 */
	Eigen::VectorXi fromChord(const Chord &chord, bool printme = false) const {
            bool isNormalOP = csound::isNormal<EQUIVALENCE_RELATION_RP>(chord, OCTAVE(), g);
            if (printme) {
              print("BEGAN fromChord()...\n");
              print("chord:          %s  %d\n", chord.toString().c_str(), isNormalOP);
            }
            Chord normalOP;
            if (isNormalOP) {
              normalOP = chord;
            } else {
              normalOP = csound::normalize<EQUIVALENCE_RELATION_RP>(chord, OCTAVE(), g);
            }
            if (printme) {
              print("normalOP:       %s  %d\n", normalOP.toString().c_str(), csound::isNormal<EQUIVALENCE_RELATION_RP>(normalOP, OCTAVE(), g));
            }
            Chord normalOPTg = csound::normalize<EQUIVALENCE_RELATION_RPTg>(chord, OCTAVE(), g);
            if (printme) {
              print("normalOPTg:     %s\n", normalOPTg.toString().c_str());
            }
            int T_ = 0;
            for (double t = 0.0; t < OCTAVE(); t += g) {
              Chord normalOPTg_t = normalOPTg.T(t);
              normalOPTg_t = csound::normalize<EQUIVALENCE_RELATION_RP>(normalOPTg_t, OCTAVE(), g);
              if (printme) {
                print("normalOPTg_t:   %s    %f\n", normalOPTg_t.toString().c_str(), t);
              }
              if (normalOPTg_t == normalOP) {
                if (printme) {
                  print("equals\n");
                }
                T_ = t;
                break;
              }
            }
            // Breaks here, this form may not be indexed.
            // Try iterating over opttis and comparing eO, eP, eT, eI separately.
            // Alternatively, put in same index for equivalent opttis.
            Chord normalOPTgI = csound::normalize<EQUIVALENCE_RELATION_RPTgI>(chord, OCTAVE(), g);
            std::map<Chord, int>::const_iterator it = indexesForOpttis.find(normalOPTgI);
            if (it == indexesForOpttis.end()) {
              // Falling through here means there is a bug that I want to know about.
              csound::print("Error: normalOPTgI %s not found! Please report an issue, this should not appear.\n");
            }
            int P_ = it->second;
            if (printme) {
              print("normalOPTgI:    %s    %d\n", normalOPTgI.toString().c_str(), P_);
            }
            int I_;
            if (normalOPTg == normalOPTgI) {
              I_ = 0;
            } else {
              I_ = 1;
            }
            int V_ = indexForOctavewiseRevoicing(chord, range, printme);
            if (V_ == -1) {
              V_ = 0;
            }
            Eigen::VectorXi pitv(4);
            pitv(0) = P_;
            pitv(1) = I_;
            pitv(2) = T_;
            pitv(3) = V_;
            if (printme) {
              print("PITV:       %8d     %8d     %8d     %8d\n", pitv(0), pitv(1), pitv(2), pitv(3));
              print("ENDED fromChord().\n");
            }
            return pitv;
	}
	/**
	 * Returns the chord for the indices of prime form, inversion,
	 * transposition, and voicing. The chord is not in RP; rather, each voice of
	 * the chord's OP may have zero or more octaves added to it.
	 *
	 * Please note: where are there singularities
	 * in the quotient spaces for chords, there may be several chords that
	 * belong to the same equivalence class. In such cases, although there
	 * will aways be a mapping from each set of indices to one chord, there
	 * may be several chords that map to the same set of indices.
	 */
	std::vector<Chord> toChord(int P, int I, int T, int V, bool printme = false) const {
		P = P % countP;
		I = I % countI;
		T = T % countT;
		V = V % countV;
		if (printme) {
			print("BEGAN toChord()...\n");
			print("PITV:       %8d     %8d     %8d     %8d\n", P, I, T, V);
		}
		Chord normalOPTgI = opttisForIndexes[P];
		if (printme) {
			print("normalOPTgI:    %s\n", normalOPTgI.toString().c_str());
		}
		Chord normalOPTg;
		if (I == 0) {
			normalOPTg = normalOPTgI;
		} else {
            Chord inverse = normalOPTgI.I();
            normalOPTg = csound::normalize<EQUIVALENCE_RELATION_RPTg>(inverse, OCTAVE(), g);
		}
		if (printme) {
			print("normalOPTg:     %s\n", normalOPTg.toString().c_str());
		}
		Chord normalOPTg_t = normalOPTg.T(T);
		if (printme) {
			print("normalOPTg_t:   %s\n", normalOPTg_t.toString().c_str());
		}
        Chord normalOP = csound::normalize<EQUIVALENCE_RELATION_RP>(normalOPTg_t, OCTAVE(), g);
		if (printme) {
			print("normalOP:       %s\n", normalOP.toString().c_str());
		}
		Chord revoicing = octavewiseRevoicing(normalOP, V, range, printme);
		std::vector<Chord> result(3);
		result[0] = revoicing;
		result[1] = normalOPTgI;
		result[2] = normalOP;
		if (printme) {
			print("revoicing:      %s\n", result[0].toString().c_str());
			print("ENDED toChord().\n");
		}
		return result;
	}
	std::vector<Chord> toChord(const Eigen::VectorXi &pitv, bool printme = false) const {
		return toChord(pitv(0), pitv(1), pitv(2), pitv(3), printme);
	}
};

inline std::string Chord::information() const {
    char buffer[0x1000];
        Chord normalOP =    csound::normalize<EQUIVALENCE_RELATION_RP>(*this);
        std::string chordName = nameForChord(normalOP);
        Chord inverse =     I();
        Chord normalO =     csound::normalize<EQUIVALENCE_RELATION_R>(*this, OCTAVE(), 1.0);
        Chord normalP =     csound::normalize<EQUIVALENCE_RELATION_P>(*this, OCTAVE(), 1.0);
        Chord normalT =     csound::normalize<EQUIVALENCE_RELATION_T>(*this, OCTAVE(), 1.0);
        Chord normalt =     normalT.et();
        Chord normalI =     csound::normalize<EQUIVALENCE_RELATION_I>(*this, OCTAVE(), 1.0);
        Chord normalV =     csound::normalize<EQUIVALENCE_RELATION_V>(*this, OCTAVE(), 1.0);
        Chord normalvt =    normalV.et();
        Chord normalpcs =   epcs().eP();
        Chord normalOPT =   csound::normalize<EQUIVALENCE_RELATION_RPT>(*this, OCTAVE(), 1.0);
        Chord normalOPTg =  csound::normalize<EQUIVALENCE_RELATION_RPTg>(*this, OCTAVE(), 1.0);
        Chord normalopt =   normalOPT.et();
        Chord normalOPI =   csound::normalize<EQUIVALENCE_RELATION_RPI>(*this, OCTAVE(), 1.0);
        Chord normalOPTI =  csound::normalize<EQUIVALENCE_RELATION_RPTI>(*this, OCTAVE(), 1.0);
        Chord normalopti =  normalOPTI.et();
        Chord normalOPTgI = csound::normalize<EQUIVALENCE_RELATION_RPTgI>(*this, OCTAVE(), 1.0);
    std::sprintf(buffer,"pitches:  %s  %s\n"
"I:        %s\n"
"eO:       %s  iseO:    %d\n"
"eP:       %s  iseP:    %d\n"
"eT:       %s  iseT:    %d\n"
"          %s\n"
"eI:       %s  iseI:    %d\n"
"eV:       %s  iseV:    %d\n"
"          %s\n"
"eOP:      %s  iseOP:   %d\n"
"pcs:      %s\n"
"eOPT:     %s  iseOPT:  %d\n"
"eOPTT:    %s  iseOPTT: %d\n\n"
"          %s\n"
"eOPI:     %s  iseOPI:  %d\n"
"eOPTI:    %s  iseOPTI: %d\n"
"eOPTTI:   %s  iseOPTTI:%d\n"
"          %s\n"
"layer:     %6.2f",
            toString().c_str(),             chordName.c_str(),
            inverse.toString().c_str(),
            normalO.toString().c_str(),     isNormal<EQUIVALENCE_RELATION_R>(*this, OCTAVE(), 1.0),
            normalP.toString().c_str(),     isNormal<EQUIVALENCE_RELATION_P>(*this, OCTAVE(), 1.0),
            normalT.toString().c_str(),     isNormal<EQUIVALENCE_RELATION_T>(*this, OCTAVE(), 1.0),
            normalt.toString().c_str(),
            normalI.toString().c_str(),     isNormal<EQUIVALENCE_RELATION_I>(*this, OCTAVE(), 1.0),
            normalV.toString().c_str(),     isNormal<EQUIVALENCE_RELATION_V>(*this, OCTAVE(), 1.0),
            normalvt.toString().c_str(),
            normalOP.toString().c_str(),    isNormal<EQUIVALENCE_RELATION_RP>(*this, OCTAVE(), 1.0),
            normalpcs.toString().c_str(),
            normalOPT.toString().c_str(),   isNormal<EQUIVALENCE_RELATION_RPT>(*this, OCTAVE(), 1.0),
            normalOPTg.toString().c_str(),  isNormal<EQUIVALENCE_RELATION_RPTg>(*this, OCTAVE(), 1.0),
            normalopt.toString().c_str(),
            normalOPI.toString().c_str(),   isNormal<EQUIVALENCE_RELATION_RPI>(*this, OCTAVE(), 1.0),
            normalOPTI.toString().c_str(),  isNormal<EQUIVALENCE_RELATION_RPTI>(*this, OCTAVE(), 1.0),
            normalOPTgI.toString().c_str(), isNormal<EQUIVALENCE_RELATION_RPTgI>(*this, OCTAVE(), 1.0),
            normalopti.toString().c_str(),
            layer());
		return buffer;
	}

inline SILENCE_PUBLIC Chord octavewiseRevoicing(const Chord &chord, int revoicingNumber_, double range, bool debug) {
    int revoicingN = octavewiseRevoicings(chord, range); // answer could be zero -- JPff
    if (revoicingN==0) revoicingN = 1;                   // jpff patch
    int revoicingNumber = revoicingNumber_ % revoicingN;
    Chord origin = csound::normalize<EQUIVALENCE_RELATION_RP>(chord, OCTAVE(), 1.0);
    Chord revoicing = origin;
    int revoicingI = 0;
    while (true) {
        if (debug) {
            print("octavewiseRevoicing %d (%d) of %s in range %7.3f: %5d: %s\n",
                revoicingNumber,
                revoicingNumber_,
                chord.toString().c_str(),
                range,
                revoicingI,
                revoicing.toString().c_str());
        }
        if (revoicingI == revoicingNumber) {
            return revoicing;
        }
        next(revoicing, origin, range, OCTAVE());
        revoicingI++;
    }
    return origin;
}

/**
 * Returns the index of the octavewise revoicing that this chord is,
 * relative to its OP equivalent, within the indicated range. Returns
 * -1 if there is no such chord within the range.
 */
inline SILENCE_PUBLIC int indexForOctavewiseRevoicing(const Chord &chord, double range, bool debug) {
	int revoicingN = octavewiseRevoicings(chord, range);
    Chord origin = csound::normalize<EQUIVALENCE_RELATION_RP>(chord, OCTAVE(), 1.0);
	Chord revoicing = origin;
    int revoicingI = 0;
    while (true) {
        if (debug) {
            print("indexForOctavewiseRevoicing of %s in range %7.3f: %5d of %5d: %s\n",
                chord.toString().c_str(),
                range,
                revoicingI,
                revoicingN,
                revoicing.toString().c_str());
        }
        if (revoicing == chord) {
            return revoicingI;
        }
        next(revoicing, origin, range, OCTAVE());
        revoicingI++;
        if (revoicingI > revoicingN) {
            return -1;
        }
    }
}

} // End of namespace csound.
#endif
