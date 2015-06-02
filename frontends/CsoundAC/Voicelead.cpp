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
#include "System.hpp"
#include "Voicelead.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>
#include <map>
#include <vector>
#include <set>

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
  static int debug = 1;

  std::map<size_t, std::vector< std::vector<double> > > primeChordsForDivisionsPerOctave;
  std::map<size_t, std::map<double, double> > pForCForDivisionsPerOctave;
  std::map<size_t, std::map<double, double> > cForPForDivisionsPerOctave;
  std::map<size_t, std::map< std::vector<double>, double> > pForPrimeChordsForDivisionsPerOctave;

  const double Voicelead::semitonesPerOctave = double(12);

  void Voicelead::initializePrimeChordsForDivisionsPerOctave(size_t divisionsPerOctave)
  {
    if (primeChordsForDivisionsPerOctave.find(divisionsPerOctave) == primeChordsForDivisionsPerOctave.end()) {
      double C = 0.0;
      double P = 0.0;
      double N = std::pow(2.0, double(divisionsPerOctave)) - 1.0;
      double M = 0.0;
      for ( ; C < N; C = C + 1.0) {
        M = cToM(C, divisionsPerOctave);
        std::vector<double> chord = mToPitchClassSet(M, divisionsPerOctave);
        std::vector<double> normalChord_ = normalChord(chord);
        std::vector<double> zeroChord = toOrigin(normalChord_);
        if (normalChord_ == zeroChord) {
          primeChordsForDivisionsPerOctave[divisionsPerOctave].push_back(zeroChord);
          pForCForDivisionsPerOctave[divisionsPerOctave][C] = P;
          cForPForDivisionsPerOctave[divisionsPerOctave][P] = C;
          pForPrimeChordsForDivisionsPerOctave[divisionsPerOctave][zeroChord] = P;
          P = P + 1.0;
        }
      }
    }
  }

  std::vector<double> Voicelead::transpose(const std::vector<double> &chord, double semitones)
  {
    std::vector<double> transposed = chord;
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      transposed[i] += semitones;
    }
    return transposed;
  }

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

  double Voicelead::pc(double pitchSemitones, size_t divisionsPerOctave)
  {
    double absolutePitchSemitones = std::abs(pitchSemitones);
    double pitchClass = std::fmod(absolutePitchSemitones, 12.0);
    return pitchClass;
  }

  double Voicelead::pitchClassSetToM(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::set<double> pcs_;
    double M = 0.0;
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      double pc_ = pc(chord[i], divisionsPerOctave);
      if (pcs_.find(pc_) == pcs_.end()) {
        pcs_.insert(pc_);
        M = M + std::pow(2.0, pc_);
      }
    }
    return M;
  }

  std::vector<double> Voicelead::mToPitchClassSet(double M, size_t divisionsPerOctave)
  {
    size_t M_ = size_t(round(M));
    std::vector<double> pcs;
    if (M != 0) {
      double i = 0.0;
      for ( ; i < double(divisionsPerOctave); i = i + 1.0) {
        size_t p2 = size_t(std::pow(2.0, i));
        if ((p2 & M_) == p2) {
          pcs.push_back(i);
        }
      }
    }
    return pcs;
  }

  std::vector<double> Voicelead::pitchClassSetToPandT(const std::vector<double> &pcs_,
                                                      size_t divisionsPerOctave)
  {
    std::vector<double> normalChord_ = normalChord(pcs_);
    std::vector<double> zeroChord_ = toOrigin(normalChord_);
    double M = pitchClassSetToM(zeroChord_, divisionsPerOctave);
    double C = mToC(M, divisionsPerOctave);
    double P = cToP(C, divisionsPerOctave);
    std::vector<double> result(2);
    result[0] = P;
    result[1] = normalChord_[0];
    return result;
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
    for (int i = v1.size() - 1; i >= 0; i--) {
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
              std::cerr << "Parallel fifth: " << std::endl;
              std::cerr << " chord 1: " << a << std::endl;
              std::cerr << " leading: " << voiceleading(a, b) << std::endl;
              std::cerr << " chord 2: " << b << std::endl;
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
      std::cerr << "rotating:   " << chord << std::endl;
      std::cerr << "rotation 1: " << inversion << std::endl;
    }
    rotations_.push_back(inversion);
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion = rotate(inversion);
      if (debug > 1) {
        std::cerr << "rotation " << (i+1) << ": " << inversion << std::endl;
      }
      rotations_.push_back(inversion);
    }
    if (debug > 1) {
      std::cerr << std::endl;
    }
    return rotations_;
  }

  std::vector< std::vector<double> > pitchRotations(const std::vector<double> &chord)
  {
    std::vector< std::vector<double> > rotations_;
    std::vector<double> inversion = chord;
    rotations_.push_back(inversion);
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion = Voicelead::rotate(inversion);
      rotations_.push_back(inversion);
    }
    return rotations_;
  }

  std::vector<double> Voicelead::pcs(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::vector<double> pcs_(chord.size());
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      pcs_[i] = pc(chord[i], divisionsPerOctave);
    }
    if (debug > 2) {
      std::cerr << "chord: " << chord << std::endl;
      std::cerr << "pcs: " << pcs_ << std::endl;
    }
    std::sort(pcs_.begin(), pcs_.end());
    return pcs_;
  }

  std::vector<double> Voicelead::uniquePcs(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::vector<double> uniquepcs;
    for (size_t i = 0, n = chord.size(); i < n; ++i) {
      double pc_ = pc(chord[i]);
      if (std::find(uniquepcs.begin(), uniquepcs.end(), pc_) == uniquepcs.end()) {
        uniquepcs.push_back(pc_);
      }
    }
    sort(uniquepcs);
    return uniquepcs;
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

  // Recursively enumerate inversions of the original chord
  // that fit within the maximum pitch.

  void inversions(const std::vector<double> &original,
                  const std::vector<double> &iterator,
                  size_t voice,
                  double maximum,
                  std::set< std::vector<double> > &chords,
                  size_t divisionsPerOctave)
  {
    if (voice >= original.size()) {
      return;
    }
    std::vector<double> iterator_ = iterator;
    for (double pitch = original[voice]; pitch < maximum; pitch = pitch + double(divisionsPerOctave)) {
      iterator_[voice] = pitch;
      chords.insert(sort(iterator_));
      inversions(original, iterator_, voice + 1, maximum, chords, divisionsPerOctave);
    }
  }

  /**
   * Bijective voiceleading first by closeness, then by simplicity,
   * with optional avoidance of parallel fifths.
   * If source and target are the same, parallel fifths are not avoided.
   */
  std::vector<double> Voicelead::voicelead(const std::vector<double> &source_,
                                           const std::vector<double> &target_,
                                           double lowest,
                                           double range,
                                           bool avoidParallels,
                                           size_t divisionsPerOctave)
  {
    std::vector<double> source = source_;
    std::vector<double> target = target_;
    std::vector<double> voicing;
    if (source != target) {
      std::vector< std::vector<double> > voicings_ = voicings(target, lowest, range, divisionsPerOctave);
      voicing = closest(source, voicings_, avoidParallels);
    } else {
      voicing = target_;
    }
    if (debug) {
      std::cerr << "   From: " << source_ << std::endl;
      std::cerr << "     To: " << target_ << std::endl;
      std::cerr << "Leading: " << voiceleading(source_, voicing) << std::endl;
      std::cerr << "     Is: " << voicing << std::endl << std::endl;
    }
    return voicing;
  }

  void recursiveVoicelead_(const std::vector<double> &source,
                           const std::vector<double> &original,
                           const std::vector<double> &iterator,
                           std::vector<double> &target,
                           size_t voice,
                           double maximum,
                           bool avoidParallels,
                           size_t divisionsPerOctave)
  {
    if (voice >= original.size()) {
      return;
    }
    std::vector<double> iterator_ = iterator;
    for (double pitch = original[voice]; pitch < maximum; pitch = pitch + double(divisionsPerOctave)) {
      iterator_[voice] = pitch;
      target = Voicelead::closer(source, iterator_, target, avoidParallels);
      recursiveVoicelead_(source, original, iterator_, target, voice + 1, maximum, avoidParallels, divisionsPerOctave);
    }
  }

  /**
   * Bijective voiceleading first by closeness, then by simplicity,
   * with optional avoidance of parallel fifths.
   */
  std::vector<double> Voicelead::recursiveVoicelead(const std::vector<double> &source_,
                                                    const std::vector<double> &target_,
                                                    double lowest,
                                                    double range,
                                                    bool avoidParallels,
                                                    size_t divisionsPerOctave)
  {
    std::vector<double> source = source_;
    std::vector<double> target = target_;
    // Find the smallest inversion of the target chord
    // that is closest to the lowest pitch, but no lower.
    std::vector<double> inversion = pcs(target, divisionsPerOctave);
    for(;;) {
      std::vector<double>::iterator it = std::min_element(inversion.begin(), inversion.end());
      if (lowest <= *it) {
        break;
      }
      inversion = invert(inversion);
    }
    // Generate all permutations of that inversion.
    std::vector< std::vector<double> > rotations_ = pitchRotations(inversion);
    // Iterate through all inversions of those permutations within the range.
    std::set< std::vector<double> > inversions_;
    std::vector<double> voicing;
    for (size_t i = 0, n = rotations_.size(); i < n; i++) {
      std::vector<double> iterator = rotations_[i];
      if (i == 0) {
        voicing = iterator;
      }
      recursiveVoicelead_(source, rotations_[i], iterator,  voicing, 0,         lowest + range, avoidParallels, divisionsPerOctave);
    }
    if (debug) {
      std::cerr << "   From: " << source_ << std::endl;
      std::cerr << "     To: " << target_ << std::endl;
      std::cerr << "Leading: " << voiceleading(source_, voicing) << std::endl;
      std::cerr << "     Is: " << voicing << std::endl << std::endl;
    }
    return voicing;
  }

  double Voicelead::closestPitch(double pitch, const std::vector<double> &pitches_)
  {
    std::map<double, double> pitchesForDistances;
    for (size_t i = 0, n = pitches_.size(); i < n; i++) {
      double pitch_ = pitches_[i];
      double distance = std::fabs(pitch_ - pitch);
      pitchesForDistances[distance] = pitch_;
    }
    return pitchesForDistances.begin()->second;
  }

  double Voicelead::conformToPitchClassSet(double pitch, const std::vector<double> &pcs, size_t divisionsPerOctave_)
  {
    double divisionsPerOctave = round(double(divisionsPerOctave_));
    double pc_ = pc(pitch);
    double closestPc = closestPitch(pc_, pcs);
    double octave = std::floor(pitch / divisionsPerOctave) * divisionsPerOctave;
    double closestPitch = octave + closestPc;
    return closestPitch;
  }

  double Voicelead::euclideanDistance(const std::vector<double> &chord1, const std::vector<double> &chord2)
  {
    double ss = 0.0;
    for (size_t i = 0, n = chord1.size(); i < n; i++) {
      ss  = ss + std::pow((chord1[i] - chord2[i]), 2.0);
    }
    return std::sqrt(ss);
  }

  std::vector<double>  Voicelead::toOrigin(const std::vector<double> &chord_)
  {
    std::vector<double> chord = chord_;
    double minimum = *std::min_element(chord.begin(), chord.end());
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      chord[i] = chord[i] - minimum;
    }
    return chord;
  }

  std::vector<double> Voicelead::invert(const std::vector<double> &chord)
  {
    std::vector<double> inversion;
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion.push_back(chord[i]);
    }
    inversion.push_back(chord[0] + 12.0);
    return inversion;
  }

  std::vector< std::vector<double> > Voicelead::inversions(const std::vector<double> &chord)
  {
    std::vector< std::vector<double> > inversions_;
    std::vector<double> inversion = pcs(chord);
    inversions_.push_back(inversion);
    for (size_t i = 1, n = chord.size(); i < n; i++) {
      inversion = invert(inversion);
      inversions_.push_back(inversion);
    }
    return inversions_;
  }

  std::vector<double>  Voicelead::normalChord(const std::vector<double> &chord)
  {
    std::vector< std::vector<double> > inversions_ = inversions(chord);
    std::vector<double> origin(chord.size(), 0.0);
    std::vector<double> normalChord;
    double minDistance = 0.0;
    for (size_t i = 0, n = inversions_.size(); i < n; i++) {
      std::vector<double> zeroChordInversion = toOrigin(inversions_[i]);
      if (i == 0) {
        normalChord = inversions_[i];
        minDistance = euclideanDistance(zeroChordInversion, origin);
      } else {
        double distance = euclideanDistance(zeroChordInversion, origin);
        if (distance < minDistance) {
          minDistance = distance;
          normalChord = inversions_[i];
        }
      }
    }
    return normalChord;
  }

  std::vector<double> Voicelead::primeChord(const std::vector<double> &chord)
  {
    return toOrigin(normalChord(chord));
  }

  double Voicelead::nameToC(std::string name, size_t divisionsPerOctave_)
  {
    double M = Conversions::nameToM(name);
    return mToC(M, divisionsPerOctave_);
  }

  double Voicelead::mToC(double M, size_t divisionsPerOctave)
  {
    // Only C has a modulus.
    int modulus = int(std::pow(2.0, double(divisionsPerOctave))) - 1;
    // Round off the absolute value of M.
    M = int(std::fabs(M + 0.5));
    // C is always 1 less than M.
    int C = M - 1;
    // Take the modulus of C.
    C = C % modulus;
    return double(C);
  }

  double Voicelead::cToM(double C, size_t divisionsPerOctave)
  {
    // Only C has a modulus.
    int modulus = int(std::pow(2.0, double(divisionsPerOctave))) - 1;
    // Round off the absolute value of C.
    int C_ = int(std::fabs(C + 0.5));
    // Take the modulus of C.
    C_ = C_ % modulus;
    // M is always 1 more than C.
    int M = C_ + 1;
    return double(M);
  }

  double Voicelead::cToP(double C, size_t divisionsPerOctave)
  {
    initializePrimeChordsForDivisionsPerOctave(divisionsPerOctave);
    double M = cToM(C, divisionsPerOctave);
    std::vector<double> pitchClassSet = mToPitchClassSet(M, divisionsPerOctave);
    std::vector<double> primeChord_ = primeChord(pitchClassSet);
    return pForPrimeChordsForDivisionsPerOctave[divisionsPerOctave][primeChord_];
  }

  double Voicelead::pToC(double P, size_t divisionsPerOctave)
  {
    initializePrimeChordsForDivisionsPerOctave(divisionsPerOctave);
    int p = int(std::fabs(P + 0.5));
    int modulus = int(primeChordsForDivisionsPerOctave.size());
    P = double(p % modulus);
    return cForPForDivisionsPerOctave[divisionsPerOctave][P];
  }

  std::vector<double> Voicelead::pToPrimeChord(double P, size_t divisionsPerOctave)
  {
    initializePrimeChordsForDivisionsPerOctave(divisionsPerOctave);
    size_t p = size_t(round(P));
    p = p % primeChordsForDivisionsPerOctave[divisionsPerOctave].size();
    return primeChordsForDivisionsPerOctave[divisionsPerOctave][p];
  }


  std::vector<double> Voicelead::pAndTtoPitchClassSet(double P,
                                                      double T,
                                                      size_t divisionsPerOctave)
  {
    std::vector<double> pitchClassSet = pToPrimeChord(P, divisionsPerOctave);
    for (size_t i = 0, n = pitchClassSet.size(); i < n; i++) {
      double pitch = pitchClassSet[i] + T;
      pitchClassSet[i] = pc(pitch, divisionsPerOctave);
    }
    std::sort(pitchClassSet.begin(), pitchClassSet.end());
    return pitchClassSet;
  }

  std::vector<double> Voicelead::orderedPcs(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::vector<double> pcs_(chord.size());
    for (size_t i = 0, n = chord.size(); i < n; i++) {
      pcs_[i] = pc(chord[i], divisionsPerOctave);
    }
    return pcs_;
  }

  struct AscendingDistanceComparator
  {
    double origin;
    size_t divisionsPerOctave;
    AscendingDistanceComparator(double origin_, size_t divisionsPerOctave_) : origin(origin_), divisionsPerOctave(divisionsPerOctave_)
    {
    }
    double ascendingDistance(double a, double b)
    {
      double pcA = Voicelead::pc(a, divisionsPerOctave);
      double pcB = Voicelead::pc(b, divisionsPerOctave);
      double d = pcB - pcA;
      if (d < 0.0) {
        d  = (pcB + double(divisionsPerOctave)) - pcA;
      }
      return d;
    }
    bool operator()(double a, double b)
    {
      double dA = ascendingDistance(origin, a);
      double dB = ascendingDistance(origin, b);
      return (dA < dB);
    }
  };

  struct MatrixCell
  {
    size_t i;
    size_t j;
    std::vector<double> s;
    std::vector<double> a;
    std::vector<double> b;
    std::vector<double> v;
    double d;
    MatrixCell() : i(0), j(0), d(0.0)
    {
    }
    //     MatrixCell(const MatrixCell &a)
    //     {
    //       operator=(a);
    //     }
    //     MatrixCell &operator=(const MatrixCell &a)
    //     {
    //       if (this != &a) {
    //  i = a.i;
    //  j = a.j;
    //  s = a.s;
    //  b = a.b;
    //  v = a.v;
    //  d = a.d;
    //       }
    //       return *this;
    //     }
  };

  const MatrixCell &minimumCell(const MatrixCell &a, const MatrixCell &b, const MatrixCell &c)
  {
    if (a.d < b.d && a.d < c.d) {
      return a;
    } else if (b.d < a.d && b.d < c.d) {
      return b;
    } else {
      return c;
    }
  }

  std::vector< std::vector<MatrixCell> > createMatrix(const std::vector<double> &sourceMultiset_,
                                                      const std::vector<double> &targetMultiset_,
                                                      const std::vector<double> &sourceChord_)
  {
    std::vector<double> sourceMultiset = sourceMultiset_;
    std::vector<double> targetMultiset = targetMultiset_;
    std::vector<double> sourceChord =    sourceChord_;
    sourceMultiset.push_back(sourceMultiset[0]);
    targetMultiset.push_back(targetMultiset[0]);
    sourceChord.push_back   (sourceChord   [0]);
    size_t N = sourceMultiset.size();
    std::vector< std::vector<MatrixCell> > matrix;
    for (size_t i = 0; i < N; i++) {
      std::vector<MatrixCell> row;
      for (size_t j = 0; j < N; j++) {
        MatrixCell cell;
        row.push_back(cell);
      }
      matrix.push_back(row);
    }
    size_t i;
    int im1;
    size_t j;
    int jm1;
    for (i = 0, im1 = -1; i < N; i++, im1++) {
      for (j = 0, jm1 = -1; j < N; j++, jm1++) {
        MatrixCell cell;
        //System::message("N: %d  i: %d  j: %d\n", N, i, j);
        if        (i == 0 && j == 0) {
          cell = matrix[i  ][j  ];
        } else if (i == 0 && j >  0) {
          cell = matrix[i  ][jm1];
        } else if (i >  0 && j == 0) {
          cell = matrix[im1][j    ];
        } else {
          const MatrixCell &a = matrix[i  ][jm1];
          const MatrixCell &b = matrix[im1][j  ];
          const MatrixCell &c = matrix[im1][jm1];
          cell = minimumCell(a, b, c);
        }
        cell.i = i;
        cell.j = j;
        cell.s.push_back(sourceChord[i]);
        cell.a.push_back(sourceMultiset[i]);
        cell.b.push_back(targetMultiset[j]);
        cell.v = Voicelead::voiceleading(cell.a, cell.b);
        cell.d = Voicelead::smoothness(cell.a, cell.b);
        matrix[i][j] = cell;
        //System::message("cell.d:         %d\n", cell.d);
        //System::message("matrix[i][j].d: %d\n", matrix[i][j].d);
      }
    }
    return matrix;
  }

  std::vector<double> Voicelead::sortByAscendingDistance(const std::vector<double> &chord, size_t divisionsPerOctave)
  {
    std::vector<double> copy(chord);
    AscendingDistanceComparator comparator(chord[0], divisionsPerOctave);
    std::sort(copy.begin(), copy.end(), comparator);
    return copy;
  }

  std::vector< std::vector<double> > Voicelead::nonBijectiveVoicelead(const std::vector<double> &sourceChord,
                                                                      const std::vector<double> &targetPitchClassSet,
                                                                      size_t divisionsPerOctave)
  {
    std::vector<double> sortedSourceChord = sortByAscendingDistance(sourceChord, divisionsPerOctave);
    std::vector<double> resultChord = sortedSourceChord;
    std::vector<double> sourceTones = orderedPcs(sortedSourceChord, divisionsPerOctave);
    std::vector<double> targetTones = orderedPcs(targetPitchClassSet, divisionsPerOctave);
    std::vector<double> sourceMultiset = sortByAscendingDistance(sourceTones, divisionsPerOctave);
    std::vector<double> targetMultiset = sortByAscendingDistance(targetTones, divisionsPerOctave);
    std::vector< std::vector<double> > targetMultisets = rotations(targetMultiset);
    std::map<double, MatrixCell> cellsForDistances;
    for (size_t i = 0, n = targetMultisets.size(); i < n; i++) {
      const std::vector<double> &targetMultiset = targetMultisets[i];
      std::vector< std::vector<MatrixCell> > matrix = createMatrix(sourceMultiset, targetMultiset, sortedSourceChord);
      size_t corner = sourceMultiset.size();
      const MatrixCell &cell = matrix[corner][corner];
      cellsForDistances[cell.d] = cell;
    }
    MatrixCell resultCell = std::min_element(cellsForDistances.begin(), cellsForDistances.end(), cellsForDistances.value_comp())->second;
    std::vector<double> returnedVoiceleading(resultCell.v);
    returnedVoiceleading.pop_back();
    std::vector<double> returnedSourceChord(resultCell.s);
    returnedSourceChord.pop_back();
    std::vector<double> returnedResultChord = returnedSourceChord;
    for (size_t i = 0, n = returnedVoiceleading.size(); i < n; i++) {
      returnedResultChord[i] = returnedSourceChord[i] + returnedVoiceleading[i];
    }
    std::vector< std::vector<double> > result;
    result.push_back(returnedSourceChord);
    result.push_back(returnedVoiceleading);
    result.push_back(returnedResultChord);
    return result;
  }

  bool Voicelead::addOctave(const std::vector<double>
                            &lowestVoicing,
                            std::vector<double> &newVoicing,
                            size_t maximumPitch,
                            size_t divisionsPerOctave)
  {
    for (size_t voice = 0, voices = lowestVoicing.size(); voice < voices; voice++) {
      double newPitch = newVoicing[voice] + 12.0;
      if (newPitch >= maximumPitch) {
        newVoicing[voice] = lowestVoicing[voice];
      } else {
        newVoicing[voice] = newPitch;
        if (debug > 1) {
          std::cerr << "addOctave: " << newVoicing << std::endl;
        }
        return true;
      }
    }
    if (debug > 1) {
      std::cerr << "addOctave: exceeded range." << std::endl;
    }
    return false;
  }

  std::vector<double> Voicelead::wrap(const std::vector<double> &chord, size_t lowestPitch, size_t highestPitch, size_t divisionsPerOctave)
  {
    std::vector<double> wrapped = chord;
    for(size_t i = 0, n = chord.size(); i < n; i++) {
      if (wrapped[i] < lowestPitch) {
        while ((wrapped[i] + double(divisionsPerOctave)) < (highestPitch)) {
          wrapped[i] += double(divisionsPerOctave);
        }
      } else if (wrapped[i] >= highestPitch) {
        while ((wrapped[i] - double(divisionsPerOctave)) >= lowestPitch) {
          wrapped[i] -= double(divisionsPerOctave);
        }
      }
    }
    return wrapped;
  }

  std::vector<double> Voicelead::ptvToChord(size_t P, size_t T, size_t V_, size_t lowestPitch, size_t highestPitch, size_t divisionsPerOctave)
  {
    std::vector<double> voicing;
    std::vector<double> zeroVoicing = normalChord(pAndTtoPitchClassSet(P, T, divisionsPerOctave));
    while (zeroVoicing[0] < lowestPitch) {
      for (size_t i = 0, n = zeroVoicing.size(); i < n; i++) {
        zeroVoicing[i] += 12.0;
      }
    }
    while (zeroVoicing[0] >= (lowestPitch + double(divisionsPerOctave))) {
      for (size_t i = 0, n = zeroVoicing.size(); i < n; i++) {
        zeroVoicing[i] -= 12.0;
      }
    }
    std::vector<double> zeroVoicing_ = sort(zeroVoicing);
    std::vector<double> zeroIterator = pcs(zeroVoicing, divisionsPerOctave);
    for(size_t i = 0, n = zeroIterator.size(); i < n; i++) {
      while (zeroIterator[i] < lowestPitch) {
        zeroIterator[i] += 12.0;
      }
      while (zeroIterator[i] >= (lowestPitch + double(divisionsPerOctave))) {
        zeroIterator[i] -= 12.0;
      }
    }
    size_t zeroVoicingEnumeration = 0;
    bool zeroVoicingEnumerationFound = false;
    size_t modulus = 0;
    bool modulusFound = false;
  found:                                                \
    std::vector<double> iterator = sort(zeroIterator);
    for(size_t V = 0; ; V++) {
      if (!zeroVoicingEnumerationFound) {
        if (zeroVoicing_ == sort(iterator)) {
          zeroVoicingEnumerationFound = true;
          zeroVoicingEnumeration = V;
          goto found;
        }
      } else {
        size_t actualV = V - zeroVoicingEnumeration;
        if (V_ == actualV) {
          return sort(iterator);
        }
      }
      if (!addOctave(zeroIterator, iterator, highestPitch, divisionsPerOctave)) {
        if (!modulusFound) {
          modulusFound = true;
          modulus = V;
          V_ = V % modulus;
          goto found;
        }
        break;
      }
    }
    return voicing;
  }

  std::vector<double> Voicelead::chordToPTV(const std::vector<double> &chord_, size_t lowestPitch, size_t highestPitch, size_t divisionsPerOctave)
  {
    std::vector<double> ptv;
    std::vector<double> sortedChord = sort(chord_);
    std::vector<double> zeroVoicing = normalChord(chord_);
    while (zeroVoicing[0] < lowestPitch) {
      for (size_t i = 0, n = zeroVoicing.size(); i < n; i++) {
        zeroVoicing[i] += 12.0;
      }
    }
    while (zeroVoicing[0] >= (lowestPitch + double(divisionsPerOctave))) {
      for (size_t i = 0, n = zeroVoicing.size(); i < n; i++) {
        zeroVoicing[i] -= 12.0;
      }
    }
    std::vector<double> zeroVoicing_ = sort(zeroVoicing);
    std::vector<double> zeroIterator = pcs(zeroVoicing, divisionsPerOctave);
    for(size_t i = 0, n = zeroIterator.size(); i < n; i++) {
      while (zeroIterator[i] < lowestPitch) {
        zeroIterator[i] += 12.0;
      }
      while (zeroIterator[i] >= (lowestPitch + double(divisionsPerOctave))) {
        zeroIterator[i] -= 12.0;
      }
    }
    size_t zeroVoicingEnumeration = 0;
    bool zeroVoicingEnumerationFound = false;
   found:                                                \
    std::vector<double> iterator = sort(zeroIterator);
    for(size_t V = 0; ; V++) {
      if (!zeroVoicingEnumerationFound) {
        if (zeroVoicing_ == sort(iterator)) {
          zeroVoicingEnumerationFound = true;
          zeroVoicingEnumeration = V;
          goto found;
        }
      } else {
        size_t actualV = V - zeroVoicingEnumeration;
        if (sortedChord == sort(iterator)) {
          ptv = pitchClassSetToPandT(chord_, divisionsPerOctave);
          ptv.push_back(actualV);
          return ptv;
        }
      }
      if (!addOctave(zeroIterator, iterator, highestPitch, divisionsPerOctave)) {
        break;
      }
    }
    return ptv;
  }

  std::vector< std::vector<double> > Voicelead::voicings(const std::vector<double> &chord_,
                                                         double lowestPitch,
                                                         double highestPitch,
                                                         size_t divisionsPerOctave)
  {
    std::vector< std::vector<double> > voicings;
    std::vector<double> zeroIterator = pcs(chord_, divisionsPerOctave);
    for(size_t i = 0, n = zeroIterator.size(); i < n; i++) {
      while (zeroIterator[i] < lowestPitch) {
        zeroIterator[i] += double(divisionsPerOctave);
      }
      while (zeroIterator[i] >= (lowestPitch + double(divisionsPerOctave))) {
        zeroIterator[i] -= double(divisionsPerOctave);
      }
    }
    std::vector<double> iterator = sort(zeroIterator);
    for(;;) {
      voicings.push_back(sort(iterator));
      if (!addOctave(zeroIterator, iterator, highestPitch, divisionsPerOctave)) {
        break;
      }
    }
    return voicings;
  }

  double Voicelead::T(double p, double n)
  {
    return pc(p + n);
  }

  std::vector<double> Voicelead::T(const std::vector<double> &c, double t)
  {
    std::vector<double> returnValue(c.size());
    for (size_t i = 0, n = c.size(); i < n; ++i) {
      returnValue[i] = pc(c[i] + t);
    }
    sort(returnValue);
    return returnValue;
  }

  double Voicelead::I(double p, double n)
  {
    return pc((12.0 - pc(p)) + n);
  }

  std::vector<double> Voicelead::I(const std::vector<double> &c, double ii)
  {
    std::vector<double> returnValue(c.size());
    for (size_t i = 0, n = c.size(); i < n; ++i) {
      returnValue[i] = I(c[i], ii);
    }
    sort(returnValue);
    return returnValue;
  }

  std::vector<double> Voicelead::K(const std::vector<double> &c)
  {
    if (c.size() < 2) {
      return c;
    }
    double n = c[0] + c[1];
    return I(c, n);
  }

  bool Voicelead::Tform(const std::vector<double> &X, const std::vector<double> &Y, double g)
  {
    double i = 0.0;
    std::vector<double> pcsx = pcs(X);
    while (i < 12.0) {
      std::vector<double> ty = T(Y, i);
      std::vector<double> pcsty = pcs(ty);
      if (pcsx == pcsty) {
        return true;
      }
      i = i + g;
    }
    return false;
  }

  bool Voicelead::Iform(const std::vector<double> &X, const std::vector<double> &Y, double g)
  {
    double i = 0.0;
    std::vector<double> pcsx = pcs(X);
    while (i < 12.0) {
      std::vector<double> iy = I(Y, i);
      std::vector<double> pcsiy = pcs(iy);
      if (pcsx == pcsiy) {
        return true;
      }
      i = i + g;
    }
    return false;
  }

  std::vector<double> Voicelead::Q(const std::vector<double> &c, double n, const std::vector<double> &s, double g)
  {
    if (Tform(c, s, g)) {
      return T(c, n);
    }
    if (Iform(c, s, g)) {
      return T(c, -n);
    } else {
      return c;
    }
  }

}
