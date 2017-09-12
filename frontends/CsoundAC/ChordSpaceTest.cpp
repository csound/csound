#include <ChordSpace.hpp>
#include <System.hpp>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <string>

static bool printPass = true;
static bool failureExits = true;
static int passCount = 0;
static int failureCount = 0;
static int testCount = 0;
static int exitAfterFailureCount = 1;
static int maximumVoiceCountToTest = 4;

static void pass(std::string message) {
    passCount = passCount + 1;
    testCount = passCount + failureCount;
    if (printPass) {
        csound::print("\nPASSED (pass %9d fail %9d of %9d): %s\n", passCount, failureCount, testCount, message.c_str());
    }
}

static void fail(std::string message) {
    failureCount = failureCount + 1;
    testCount = passCount + failureCount;
    csound::print("================================================================================================================\n");
    csound::print("FAILED (pass %9d fail %9d of %9d): %s", passCount, failureCount, testCount, message.c_str());
    csound::print("================================================================================================================\n");
    if (failureExits && (failureCount >= exitAfterFailureCount)) {
        std::exit(-1);
    }
}

static bool test(bool passes, std::string message) {
    if (passes) {
        pass(message);
    } else {
        fail(message);
    }
    return passes;
}

static void printSet(std::string name, const std::set<csound::Chord> &chords) {
    int i = 1;
    for (auto it = chords.begin(); it != chords.end(); ++it, ++i) {
        csound::print("%s %5d: %s\n", name.c_str(), i, it->toString().c_str());
    }
}

static void testChordSpaceGroup(const csound::ChordSpaceGroup &chordSpaceGroup, std::string chordName) {
    csound::print("BEGAN test ChordSpaceGroup for %s...\n", chordName.c_str());
    csound::Chord originalChord = csound::chordForName(chordName);
    csound::Chord optti = originalChord.eOPTTI();
    csound::print("Original chord:\n%s\n", originalChord.information().c_str());
    Eigen::VectorXi pitv = chordSpaceGroup.fromChord(originalChord, true);
    csound::Chord reconstitutedChord = chordSpaceGroup.toChord(pitv[0], pitv[1], pitv[2], pitv[3], true)[0];
    csound::print("Reconstituted chord:\n%s\n", reconstitutedChord.information().c_str());
    test(originalChord == reconstitutedChord, "Reconstituted chord must be the same as the original chord.\n");
    csound::Chord revoicedOriginalChord = originalChord;
    revoicedOriginalChord.setPitch(1,  revoicedOriginalChord.getPitch(1) + 12.);
    revoicedOriginalChord.setPitch(2,  revoicedOriginalChord.getPitch(2) + 24.);
    csound::print("Revoiced original chord:\n%s\n", revoicedOriginalChord.information().c_str());
    pitv = chordSpaceGroup.fromChord(revoicedOriginalChord, true);
    csound::Chord reconstitutedRevoicedChord = chordSpaceGroup.toChord(pitv, true)[0];
    csound::print("Reconstituted revoiced chord:\n%s\n", reconstitutedRevoicedChord.information().c_str());
    test(revoicedOriginalChord == reconstitutedRevoicedChord, "Reconstituted revoiced chord must be the same as the original revoiced chord.\n");
    csound::Chord invertedChord = originalChord.I().eOP();
    csound::print("Inverted original chord:\n%s\n", invertedChord.information().c_str());
    pitv = chordSpaceGroup.fromChord(invertedChord);
    csound::Chord reconstitutedInvertedChord = chordSpaceGroup.toChord(pitv, true)[0];
    csound::print("Reconstituted inverted chord:\n%s\n", reconstitutedInvertedChord.information().c_str());
    test(invertedChord == reconstitutedInvertedChord,"Reconstituted inverted chord must be the same as the original inverted chord.\n");
    csound::print("ENDED test ChordSpaceGroup for %s.\n", chordName.c_str());
    csound::print("");
}

static void testAllOfChordSpaceGroup(int initialVoiceCount, int finalVoiceCount) {
    double range = 48.0;
    for (int voiceCount = initialVoiceCount; voiceCount <= finalVoiceCount; ++voiceCount) {
        bool passes = true;
        csound::ChordSpaceGroup chordSpaceGroup;
        csound::print("Testing all of ChordSpaceGroup: voices: %d  range: %f\n", voiceCount, range);
        csound::print("Testing ChordSpaceGroup to chords and back...\n");
        chordSpaceGroup.initialize(voiceCount, range);
        chordSpaceGroup.list(true, true, true);
        for (int P = 0; P < chordSpaceGroup.countP; ++P) {
            for (int I = 0; I < chordSpaceGroup.countI; ++I) {
                for (int T = 0; T < chordSpaceGroup.countT; ++T) {
                    for (int V = 0; V < chordSpaceGroup.countV; ++V) {
                        csound::Chord fromPITV = chordSpaceGroup.toChord(P, I, T, V)[0];
                        if (printPass) csound::print("%8d     %8d     %8d     %8d => %s\n", P, I, T, V, fromPITV.toString().c_str());
                        Eigen::VectorXi pitv = chordSpaceGroup.fromChord(fromPITV);
                        csound::Chord frompitv = chordSpaceGroup.toChord(pitv(0), pitv(1), pitv(2), pitv(3))[0];
                        if (printPass) csound::print("%8d     %8d     %8d     %8d <= %s\n",pitv(0), pitv(1), pitv(2), pitv(3), frompitv.toString().c_str());
                        bool equals = (fromPITV == frompitv);
                        if (!equals) {
                            chordSpaceGroup.toChord(P, I, T, V, true);
                            csound::print("fromPITV (toChord):\n%s\n", fromPITV.information().c_str());
                            chordSpaceGroup.fromChord(fromPITV, true);
                            csound::print("frompitv (fromChord):\n%s\n", frompitv.information().c_str());
                            passes = false;
                        }
                        test(equals, "fromPITV must match frompitv.\n");
                        if (printPass) csound::print("\n");
                    }
                }
            }
        }
        csound::print("Testing chords to ChordSpaceGroup and back...\n\n");
        bool passes2 = true;
        auto eops = csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_RP>(voiceCount, csound::OCTAVE(), 1.0);
        for(auto it = eops.begin(); it != eops.end(); ++it) {
            auto chord = *it;
            auto origin = chord;
            for(;;) {
                Eigen::VectorXi pitv = chordSpaceGroup.fromChord(chord);
                if (printPass) csound::print("%8d     %8d     %8d     %8d <= %s\n", pitv(0), pitv(1), pitv(2), pitv(3), chord.toString().c_str());
                csound::Chord fromPITV = chordSpaceGroup.toChord(pitv(0), pitv(1), pitv(2), pitv(3))[0];
                if (printPass) csound::print("%8d     %8d     %8d     %8d => %s\n", pitv(0), pitv(1), pitv(2), pitv(3), fromPITV.toString().c_str());
                bool equals = (fromPITV == chord);
                if (!equals) {
                    csound::print("Original chord (fromChord):\n%s\n", chord.information().c_str());
                    csound::print("New chord (toChord):\n%s\n", fromPITV.information().c_str());
                    passes2 = false;
                }
                test(equals, "Original chord must match chord from original chord's PITV.\n");
                if (printPass) csound::print("\n");
                // This was going too far... cut off sooner and all seems well.
                if (csound::next(chord, origin, range - 1.0, csound::OCTAVE()) == false)
                {
                    break;
                }
            }
        }
    }
}

std::vector<std::string> equivalenceRelationsToTest = {"RP", "RPTg", "RPI", "RPTgI"};
typedef csound::Chord(*normalize_t)(const csound::Chord &, double, double);
typedef bool (*isNormal_t)(const csound::Chord &, double, double);
typedef bool (*isEquivalent_t)(const csound::Chord &, const csound::Chord &, double, double);
typedef std::set<csound::Chord> (*fundamentalDomainByNormalize_t)(int, double, double);
typedef std::set<csound::Chord> (*fundamentalDomainByIsNormal_t)(int, double, double);
std::map<std::string, normalize_t> normalizesForEquivalenceRelations;
std::map<std::string, isNormal_t> isNormalsForEquivalenceRelations;
std::map<std::string, isEquivalent_t> isEquivalentsForEquivalenceRelations;
std::map<std::string, std::set<std::string> > equivalenceRelationsForCompoundEquivalenceRelations;
std::map<std::string, fundamentalDomainByNormalize_t> fundamentalDomainByNormalizesForEquivalenceRelations;
std::map<std::string, fundamentalDomainByIsNormal_t> fundamentalDomainByIsNormalsForEquivalenceRelations;

/**
 * The correctness and consistency of the equivalence relations are tested as
 * follows. We identify the elements of a representative fundamental domain
 * for an equivalence relation R both by sending all chords in chord space to
 * R's representative fundamental domain ("normalize") and by identifying those
 * chords in the space that belong to R ("isNormal") (for some equivalence
 * classes, there may be several elements, on opposing facets of the domain).
 * Then, we ensure that the following conditions obtain for a domain:
 * (1) Each element of "normalize" is a member of "isNormal".
 * (2) Each element of "isNormal" is equivalent to some element in
 *     "normalize".
 * For informational purposes, we also print out all elements of "isNormal"
 * that do not belong to "normalize," as this may help when building
 * ChordSpaceGroup.
 */
static bool testNormalsAndEquivalents(std::string equivalence,
        std::set<csound::Chord> &normalize_,
        std::set<csound::Chord> &isNormal_,
        double range,
        double g) {
    char buffer[0x200];
    auto normalize = normalizesForEquivalenceRelations[equivalence];
    auto isEquivalent = isEquivalentsForEquivalenceRelations[equivalence];
    bool passes = true;
    for (auto it = normalize_.begin(); it != normalize_.end(); ++it) {
        if (isNormal_.find(*it) == isNormal_.end()) {
            passes = false;
            csound::print("testNormalsAndEquivalents: %s range %f g %f: normalize %s not in isNormal.\n",
                equivalence.c_str(),
                range,
                g,
                it->toString().c_str());
        }
    }
    std::sprintf(buffer, "testNormalsAndEquivalents: %s range %f g %f: all normalize must be in isNormal.\n",
        equivalence.c_str(),
        range,
        g);
    test(passes, buffer);
    bool passes2 = true;
    std::set<csound::Chord> elementsInIsNormalButNotInNormalize;
    for (auto it = isNormal_.begin(); it != isNormal_.end(); ++it) {
        if (normalize_.find(*it) == normalize_.end()) {
            elementsInIsNormalButNotInNormalize.insert(*it);
        }
    }
    for (auto it = elementsInIsNormalButNotInNormalize.begin(); it != elementsInIsNormalButNotInNormalize.end(); ++it) {
        csound::print("elementsInIsNormalButNotInNormalize: %s %s.\n", equivalence.c_str(), it->toString().c_str());
        bool equivalenceFound = false;
        for (auto jt = normalize_.begin(); jt != normalize_.end(); ++jt) {
            if (isEquivalent(*it, *jt, range, g) == true) {
                equivalenceFound = true;
                csound::print("  Equivalent: %s\n", jt->toString().c_str());
            }
        }
        if (equivalenceFound == false) {
            passes2 = false;
            std::sprintf(buffer, "testNormalsAndEquivalents: %s range %f g %f: no normalize found for isNormal: %s\n",
                equivalence.c_str(),
                range,
                g,
                it->toString().c_str());
            fail(buffer);
        }
    }
    std::sprintf(buffer, "normals and equivalents: %s range %f g %f: all isNormal must have equivalent in normalize.\n",
        equivalence.c_str(),
        range,
        g);
    pass(buffer);
    return passes && passes2;
}

/**
 * Next, we test the consistency of the relationships between different
 * equivalence relations and their fundamental domains:
 * RP <=> R and P
 * RPT <=> RP and R and P and T and V
 * RPTg <=> RP and R and P and Tg and V
 * RPI <=> RP and R and P and I
 * RPTI <=> RPT and RPI and RP and R and P and T and I and V
 * RPTgI <=> RPTg and RPI and RP and R and P and Tg and I and V
 * In doing this, the LHS is the set of "equivalents" under R from above, and
 * the RHS tests each element of that set for belonging to each of the
 * equivalence relations' representative fundamental domain.
 */
static bool testConsistency(std::string compoundEquivalenceRelation, const std::set<csound::Chord> &equivalents, double range, double g) {
    char buffer[0x200];
    bool passes = true;
    auto equivalenceRelations = equivalenceRelationsForCompoundEquivalenceRelations[compoundEquivalenceRelation];
    for (auto equivalenceRelationsI = equivalenceRelations.begin(); equivalenceRelationsI != equivalenceRelations.end(); ++equivalenceRelationsI) {
        auto isNormal = isNormalsForEquivalenceRelations[*equivalenceRelationsI];
        for (auto chordI = equivalents.begin(); chordI != equivalents.end(); ++chordI) {
            if (isNormal(*chordI, range, g) == false) {
                passes = false;
                std::sprintf(buffer, "testConsistency: chord %s in the domain of %s is not in the domain of %s voices %d range %f g %f.\n",
                        chordI->toString().c_str(),
                        compoundEquivalenceRelation.c_str(),
                        equivalenceRelationsI->c_str(),
                        chordI->voices(),
                        range,
                        g);
                fail(buffer);
            }
        }
        if (passes) {
            std::sprintf(buffer, "testConsistency: range %f g %f: %s is consistent with %s.\n",
                    range,
                    g,
                    compoundEquivalenceRelation.c_str(),
                    equivalenceRelationsI->c_str());
            pass(buffer);
        }
    }
    std::sprintf(buffer, "testConsistency: %s for %d voices range %f g %f.\n", compoundEquivalenceRelation.c_str(), equivalents.begin()->voices(), range, g);
    pass(buffer);
    return passes;
}

static bool testEquivalenceRelation(std::string equivalenceRelation, int voiceCount, double range, double g) {
    bool passes = true;
    char buffer[0x200];
    auto normalsForEquivalenceRelation = fundamentalDomainByNormalizesForEquivalenceRelations[equivalenceRelation](voiceCount, range, g);
    std::sprintf(buffer, "%-8s by normalize", equivalenceRelation.c_str());
    printSet(buffer, normalsForEquivalenceRelation);
    std::sprintf(buffer, "%-8s by isNormal ", equivalenceRelation.c_str());
    auto equivalentsForEquivalenceRelation = fundamentalDomainByIsNormalsForEquivalenceRelations[equivalenceRelation](voiceCount, range, g);
    printSet(buffer, equivalentsForEquivalenceRelation);
    if (!testNormalsAndEquivalents(equivalenceRelation,
            normalsForEquivalenceRelation,
            equivalentsForEquivalenceRelation,
            range,
            g)) {
        passes = false;
    }
    if (!testConsistency(equivalenceRelation, equivalentsForEquivalenceRelation, range, g)) {
        passes = false;
    }
    return passes;
}

static bool testEquivalenceRelations(int voiceCount, double range, double g) {
    bool passes = true;
    csound::print("\nTesting equivalence relations for %d voices over range %f with g %f...\n\n", voiceCount, range, g);
    for (auto equivalenceRelationI = equivalenceRelationsToTest.begin();
            equivalenceRelationI != equivalenceRelationsToTest.end();
            ++equivalenceRelationI) {
        if (!testEquivalenceRelation(*equivalenceRelationI, voiceCount, range, g)) {
            passes = false;
        }
    }
    return passes;
}

void printRPIStuff(const csound::Chord &chord)
{
    csound::print("%s\n", chord.information().c_str());
}

void testRPIStuff(const csound::Chord &chord)
{
    csound::print("\nTESTING RPI STUFF\n");
    csound::print("chord...\n");
    printRPIStuff(chord);
    auto chordRP = csound::normalize<csound::EQUIVALENCE_RELATION_RP>(chord, 12.0, 1.0);
    auto inverse = chord.I().eP();
    csound::print("inverse...\n");
    printRPIStuff(inverse);
    auto inverseRP = csound::normalize<csound::EQUIVALENCE_RELATION_RP>(inverse, 12.0, 1.0);
    csound::print("inverseRP...\n");
    printRPIStuff(inverseRP);
    auto midpoint = csound::midpoint(chord.eP(), inverseRP.eP());
    csound::print("midpoint of chord and inverseRP: %s\n", midpoint.eOP().toString().c_str());
    csound::print("inverse of midpoint:             %s\n", midpoint.I().eOP().toString().c_str());
    auto inverseInverseRP = inverseRP.I();
    csound::print("inverseInverseRP...\n");
    printRPIStuff(inverseInverseRP);
    auto inverseRPinverseRP = csound::normalize<csound::EQUIVALENCE_RELATION_RP>(inverseInverseRP, 12.0, 1.0);
    csound::print("inverseRPinverseRP...\n");
    printRPIStuff(inverseRPinverseRP);
    test(inverseRPinverseRP == chordRP, "chordRP must equal inverseRPinverseRP.");
}

int main(int argc, char **argv) {
    csound::print("C H O R D S P A C E   U N I T   T E S T S\n\n");
#if defined(USE_OLD_EQUIVALENCES)
    csound::print("Using OLD implementation of equivalence relations.\n\n");
#else
    csound::print("Using NEW implementation of equivalence relations.\n\n");
#endif
    normalizesForEquivalenceRelations["R"] =        csound::normalize<csound::EQUIVALENCE_RELATION_R>;
    normalizesForEquivalenceRelations["P"] =        csound::normalize<csound::EQUIVALENCE_RELATION_P>;
    normalizesForEquivalenceRelations["T"] =        csound::normalize<csound::EQUIVALENCE_RELATION_T>;
    normalizesForEquivalenceRelations["Tg"] =       csound::normalize<csound::EQUIVALENCE_RELATION_Tg>;
    normalizesForEquivalenceRelations["I"] =        csound::normalize<csound::EQUIVALENCE_RELATION_I>;
    normalizesForEquivalenceRelations["RP"] =       csound::normalize<csound::EQUIVALENCE_RELATION_RP>;
    normalizesForEquivalenceRelations["RPT"] =      csound::normalize<csound::EQUIVALENCE_RELATION_RPT>;
    normalizesForEquivalenceRelations["RPTg"] =     csound::normalize<csound::EQUIVALENCE_RELATION_RPTg>;
    normalizesForEquivalenceRelations["RPI"] =      csound::normalize<csound::EQUIVALENCE_RELATION_RPI>;
    normalizesForEquivalenceRelations["RPTI"] =     csound::normalize<csound::EQUIVALENCE_RELATION_RPTI>;
    normalizesForEquivalenceRelations["RPTgI"] =    csound::normalize<csound::EQUIVALENCE_RELATION_RPTgI>;
    isNormalsForEquivalenceRelations["R"] =         csound::isNormal<csound::EQUIVALENCE_RELATION_R>;
    isNormalsForEquivalenceRelations["P"] =         csound::isNormal<csound::EQUIVALENCE_RELATION_P>;
    isNormalsForEquivalenceRelations["T"] =         csound::isNormal<csound::EQUIVALENCE_RELATION_T>;
    isNormalsForEquivalenceRelations["Tg"] =        csound::isNormal<csound::EQUIVALENCE_RELATION_Tg>;
    isNormalsForEquivalenceRelations["I"] =         csound::isNormal<csound::EQUIVALENCE_RELATION_I>;
    isNormalsForEquivalenceRelations["RP"] =        csound::isNormal<csound::EQUIVALENCE_RELATION_RP>;
    isNormalsForEquivalenceRelations["RPT"] =       csound::isNormal<csound::EQUIVALENCE_RELATION_RPT>;
    isNormalsForEquivalenceRelations["RPTg"] =      csound::isNormal<csound::EQUIVALENCE_RELATION_RPTg>;
    isNormalsForEquivalenceRelations["RPI"] =       csound::isNormal<csound::EQUIVALENCE_RELATION_RPI>;
    isNormalsForEquivalenceRelations["RPTI"] =      csound::isNormal<csound::EQUIVALENCE_RELATION_RPTI>;
    isNormalsForEquivalenceRelations["RPTgI"] =     csound::isNormal<csound::EQUIVALENCE_RELATION_RPTgI>;
    isEquivalentsForEquivalenceRelations["R"] =     csound::isEquivalent<csound::EQUIVALENCE_RELATION_R>;
    isEquivalentsForEquivalenceRelations["P"] =     csound::isEquivalent<csound::EQUIVALENCE_RELATION_P>;
    isEquivalentsForEquivalenceRelations["T"] =     csound::isEquivalent<csound::EQUIVALENCE_RELATION_T>;
    isEquivalentsForEquivalenceRelations["Tg"] =    csound::isEquivalent<csound::EQUIVALENCE_RELATION_Tg>;
    isEquivalentsForEquivalenceRelations["I"] =     csound::isEquivalent<csound::EQUIVALENCE_RELATION_I>;
    isEquivalentsForEquivalenceRelations["RP"] =    csound::isEquivalent<csound::EQUIVALENCE_RELATION_RP>;
    isEquivalentsForEquivalenceRelations["RPT"] =   csound::isEquivalent<csound::EQUIVALENCE_RELATION_RPT>;
    isEquivalentsForEquivalenceRelations["RPTg"] =  csound::isEquivalent<csound::EQUIVALENCE_RELATION_RPTg>;
    isEquivalentsForEquivalenceRelations["RPI"] =   csound::isEquivalent<csound::EQUIVALENCE_RELATION_RPI>;
    isEquivalentsForEquivalenceRelations["RPTI"] =  csound::isEquivalent<csound::EQUIVALENCE_RELATION_RPTI>;
    isEquivalentsForEquivalenceRelations["RPTgI"] = csound::isEquivalent<csound::EQUIVALENCE_RELATION_RPTgI>;
    equivalenceRelationsForCompoundEquivalenceRelations["RP"] =      {"R", "P"};
    equivalenceRelationsForCompoundEquivalenceRelations["RPT"] =     {"R", "P", "T"}; // V?
    equivalenceRelationsForCompoundEquivalenceRelations["RPTg"] =    {"R", "P", "Tg"}; // V?
    equivalenceRelationsForCompoundEquivalenceRelations["RPI"] =     {"R", "P"};
    equivalenceRelationsForCompoundEquivalenceRelations["RPTgI"] =   {"RPTg", "RP", "R", "P", "Tg"}; // V?
    fundamentalDomainByNormalizesForEquivalenceRelations["R"] =        csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_R>;
    fundamentalDomainByNormalizesForEquivalenceRelations["P"] =        csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_P>;
    fundamentalDomainByNormalizesForEquivalenceRelations["T"] =        csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_T>;
    fundamentalDomainByNormalizesForEquivalenceRelations["Tg"] =       csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_Tg>;
    fundamentalDomainByNormalizesForEquivalenceRelations["I"] =        csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_I>;
    fundamentalDomainByNormalizesForEquivalenceRelations["RP"] =       csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_RP>;
    fundamentalDomainByNormalizesForEquivalenceRelations["RPT"] =      csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_RPT>;
    fundamentalDomainByNormalizesForEquivalenceRelations["RPTg"] =     csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_RPTg>;
    fundamentalDomainByNormalizesForEquivalenceRelations["RPI"] =      csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_RPI>;
    fundamentalDomainByNormalizesForEquivalenceRelations["RPTI"] =     csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_RPTI>;
    fundamentalDomainByNormalizesForEquivalenceRelations["RPTgI"] =    csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_RPTgI>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["R"] =           csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_R>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["P"] =           csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_P>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["T"] =           csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_T>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["Tg"] =          csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_Tg>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["I"] =           csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_I>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["RP"] =          csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_RP>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["RPT"] =         csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_RPT>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["RPTg"] =        csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_RPTg>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["RPI"] =         csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_RPI>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["RPTI"] =        csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_RPTI>;
    fundamentalDomainByIsNormalsForEquivalenceRelations["RPTgI"] =       csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_RPTgI>;

    csound::Chord original;
    original.resize(3);
    original.setPitch(0, -8.0);
    original.setPitch(1,  4.0);
    original.setPitch(2,  4.0);

    // normals for T and equals for T are of course very very different, modify test for this.

    auto normalized = csound::normalize<csound::EQUIVALENCE_RELATION_R>(original, 24.0, 1.0);
    csound::print("R:     original: %s  normalized: %s:\n", original.toString().c_str(), normalized.toString().c_str());

    original.setPitch(0,  3.0);
    original.setPitch(1,  4.0);
    original.setPitch(2,  4.0);
    testRPIStuff(original);

    original.setPitch(0, -5.0);
    original.setPitch(1, -2.0);
    original.setPitch(2,  7.0);
    testRPIStuff(original);

    original.setPitch(0, -7.0);
    original.setPitch(1,  3.0);
    original.setPitch(2,  5.0);
    testRPIStuff(original);

    original.setPitch(0, -7.0);
    original.setPitch(1,  3.0);
    original.setPitch(2,  3.0);
    testRPIStuff(original);

    original.setPitch(0, -7.0);
    original.setPitch(1,  3.0);
    original.setPitch(2,  5.0);
    testRPIStuff(original);

    original.setPitch(0, -5.0);
    original.setPitch(1,  2.0);
    original.setPitch(2,  7.0);
    testRPIStuff(original);

    original.setPitch(0, -3.0);
    original.setPitch(1,  7.0);
    original.setPitch(2,  7.0);
    testRPIStuff(original);

    original.setPitch(0, -5.0);
    original.setPitch(1,  5.0);
    original.setPitch(2,  7.0);
    testRPIStuff(original);

    original.setPitch(0, -8.0);
    original.setPitch(1,  4.0);
    original.setPitch(2,  4.0);
    testRPIStuff(original);

    original.setPitch(0, -5.0);
    original.setPitch(1,  5.0);
    original.setPitch(2,  5.0);
    testRPIStuff(original);

    original.setPitch(0, -4.0);
    original.setPitch(1,  1.0);
    original.setPitch(2,  3.0);
    testRPIStuff(original);

    original.setPitch(0, -3.0);
    original.setPitch(1, -1.0);
    original.setPitch(2,  4.0);
    testRPIStuff(original);

    original.setPitch(0, -3.0);
    original.setPitch(1, -1.0);
    original.setPitch(2,  4.0);
    testRPIStuff(original);

    original.setPitch(0,  0.0);
    original.setPitch(1,  0.0);
    original.setPitch(2,  1.0);
    testRPIStuff(original);

    original.setPitch(0,  0.0);
    original.setPitch(1,  1.0);
    original.setPitch(2,  1.0);
    testRPIStuff(original);

    original.setPitch(0,  0.0);
    original.setPitch(1,  0.0);
    original.setPitch(2,  1.0);
    testRPIStuff(original);

    original.setPitch(0, -3.0);
    original.setPitch(1,  3.0);
    original.setPitch(2,  9.0);
    testRPIStuff(original);

    original.setPitch(0, -1.0);
    original.setPitch(1, -1.0);
    original.setPitch(2, 12.0);
    testRPIStuff(original);

    auto index = csound::indexForOctavewiseRevoicing(original, 48.0, true);
    index = csound::indexForOctavewiseRevoicing(original.eOP(), 48.0, true);
    //int x;
    //std::cin >> x;

    csound::print("\nBehavior of std::fmod and std::remainder:\n\n");
    for (double pitch = -24.0; pitch < 24.0; pitch += 1.0) {
        double modulusFmod = std::fmod(pitch, csound::OCTAVE());
        double modulusRemainder = std::remainder(pitch, csound::OCTAVE());
        double pc = csound::epc(pitch);
        double modulus = csound::modulo(pitch, csound::OCTAVE());
        csound::print("Pitch: %9.4f  modulo: %9.4f  std::fmod: %9.4f  std::remainder: %9.4f  epc: %9.4f\n", pitch, modulus, modulusFmod, modulusRemainder, pc);
    }
    csound::Chord pcs = csound::chordForName("C major").epcs();
    csound::print("Should be C major scale:\n%s\n", pcs.information().c_str());
    for (double pitch = 36.0; pitch < 96.0; pitch += 1.0) {
        double conformed = csound::conformToPitchClassSet(pitch, pcs);
        csound::print("pitch: %9.4f  conformed: %9.4f\n", pitch, conformed);
    }
    std::cout << "EPSILON: " << csound::EPSILON() << std::endl;
    std::cout << "epsilonFactor: " << csound::epsilonFactor() << std::endl;
    std::cout << "EPSILON * epsilonFactor: " << csound::EPSILON() * csound::epsilonFactor() << std::endl;
    std::cout << "csound::eq_epsilon(0.15, 0.15): " << csound::eq_epsilon(0.15, 0.15) << std::endl;
    std::cout << "csound::eq_epsilon(0.1500001, 0.15): " << csound::eq_epsilon(0.1500001, 0.15) << std::endl;
    std::cout << "csound::gt_epsilon(14.0, 12.0): " << csound::gt_epsilon(14.0, 12.0) << std::endl;
    std::cout << "csound::ge_epsilon(14.0, 12.0): " << csound::ge_epsilon(14.0, 12.0) << std::endl;
    csound::Chord chord;
    std::cout << "Default chord: " << chord.toString() << std::endl;
    std::cout << "chord.count(0.0): " << chord.count(0.0) << std::endl;
    csound::Chord other;
    other.setPitch(1, 2);
    std::cout << "Other chord: " << other.toString() << std::endl;
    std::cout << "other.count(0.0): " << other.count(0.0) << std::endl;
    std::cout << "(chord == other): " << (chord == other) << std::endl;
    std::cout << "other.contains(2.0): " << other.contains(2.0) << std::endl;
    std::cout << "other.contains(2.00000001): " << other.contains(2.00000001) << std::endl;
    std::vector<double> result = other.min();
    std::cout << "other.min(): " << result[0] << ", " << result[1] << ", " << result.size() << std::endl;
    std::cout << "other.minimumInterval(): " << other.minimumInterval() << std::endl;
    std::cout << "other.maximumInterval(): " << other.maximumInterval() << std::endl;
    csound::Chord clone = other;
    std::cout << "clone = other: " << clone.toString() << std::endl;
    clone.setPitch(1, .5);
    std::cout << "clone: " << clone.toString() << std::endl;
    csound::Chord floor = clone.floor();
    std::cout << "floor: " << floor.toString() << std::endl;
    csound::Chord ceiling = clone.ceiling();
    std::cout << "ceiling: " << ceiling.toString() << std::endl;
    chord.setPitch(0, 1);
    chord.setPitch(1, 1);
    std::cout << "chord: " << chord.toString() << std::endl;
    std::cout << "chord.distanceToOrigin(): " << chord.distanceToOrigin() << std::endl;
    std::cout << "chord.distanceToUnisonDiagonal(): " << chord.distanceToUnisonDiagonal() << std::endl;
    std::cout << "chord.maximallyEven(): " << chord.maximallyEven().toString() << std::endl;
    std::cout << "chord.T(3): " << chord.T(3).toString() << std::endl;
    std::cout << "chord.I(): " << chord.I().toString() << std::endl;
    std::cout << "csound::epc(13.2): " << csound::epc(13.2) << std::endl;
    std::cout << "chord.isepcs(): " << chord.isepcs() << std::endl;
    chord.setPitch(2, 14);
    std::cout << "chord: " << chord.toString() << std::endl;
    std::cout << "chord.isepcs(): " << chord.isepcs() << std::endl;
    csound::Chord epcs = chord.epcs();
    std::cout << "chord.epcs(): " << epcs.toString() << std::endl;
    std::cout << "csound::epc(14.0): " << csound::epc(14.0) << std::endl;
    csound::Chord transposed = chord.T(5.0);
    std::cout << "chord::T(5.0): " << transposed.toString() << std::endl;
    std::cout << "transposed.iset(): " << transposed.iset() << std::endl;
    csound::Chord et = transposed.et();
    std::cout << "et = transposed.et(): " << et.toString() << std::endl;
    std::cout << "transposed.iseO(): " << transposed.iseO() << std::endl;
    csound::Chord eO = transposed.eO();
    std::cout << "transposed.eO(): " << eO.toString() << std::endl;
    std::cout << "eO.iseO(): " << eO.iseO() << std::endl;
    std::cout << "eO.iseP(): " << eO.iseP() << std::endl;
    csound::Chord eP = eO.eP();
    std::cout << "eP = eO.eP(): " << eP.toString() << std::endl;
    std::cout << "eP.iseT(): " << eP.iseT() << std::endl;
    csound::Chord eT= eP.eT();
    std::cout << "eT = eP.eT(): " << eT.toString() << std::endl;
    std::cout << "eT.iseT(): " << eT.iseT() << std::endl;
    csound::Chord eTT= eP.eTT();
    std::cout << "eTT = eP.eTT(): " << eTT.toString() << std::endl;
    std::cout << "eTT.iseT(): " << eTT.iseTT() << std::endl;
    std::cout << "eT.iseTT(): " << eT.iseTT() << std::endl;
    std::cout << "eTT: " << eTT.toString() << std::endl;
    csound::Chord inverse = eTT.I();
    std::cout << "csound::Chord inverse = eTT.I(): " << inverse.toString() << std::endl;
    csound::Chord inverseOfInverse = inverse.I();
    std::cout << "csound::Chord inverseOfInverse = inverse.I(): " << inverseOfInverse.toString() << std::endl;
    std::cout << "inverse.iseI(): " << inverse.iseI() << std::endl;
    csound::Chord eI = eTT.eI();
    std::cout << "csound::Chord eI = eTT.eI(): " << eI.toString() << std::endl;
    std::cout << "eI.iseI(): " << eI.iseI() << std::endl;
    std::cout << "eTT.iseI(): " << eTT.iseI() << std::endl;
    std::cout << "(inverse < eTT): " << (inverse < eTT) << std::endl;
    std::cout << "(eTT < inverse): " << (eTT < inverse) << std::endl;
    std::cout << "chord: " << chord.toString() << std::endl;
    std::cout << "chord.cycle(): " << chord.cycle().toString() << std::endl;
    std::cout << "chord.cycle(2): " << chord.cycle(2).toString() << std::endl;
    std::cout << "chord.cycle().cycle(): " << chord.cycle().cycle().toString() << std::endl;
    std::cout << "eI: " << eI.toString() << std::endl;
    std::cout << "eI.cycle(-1): " << eI.cycle(-1).toString() << std::endl;
    std::vector<csound::Chord> permutations = chord.permutations();
    std::cout << "Permutations, iseV, eV:" << std::endl;
    for (size_t i = 0; i < permutations.size(); i++) {
        const csound::Chord permutation = permutations[i];
        const csound::Chord &eV = permutation.eV();
        std::cout << i << " " << permutation.toString() << "  iseV: " << permutation.iseV() << "  eV: " << eV.toString() << "  iseP: " <<  permutation.iseP() << std::endl;
    }
    std::vector<csound::Chord> voicings = chord.voicings();
    std::cout << "voicing, iseV, eV:" << std::endl;
    for (size_t i = 0; i < voicings.size(); i++) {
        const csound::Chord voicing = voicings[i];
        const csound::Chord &eV = voicing.eV();
        std::cout << i << " " << voicing.toString() << "  iseV: " << voicing.iseV() << "  eV: " << eV.toString() << std::endl;
    }
    std::string tosplit = "C     D     E        G           B";
    csound::fill("C", 0., "M9", tosplit, true);
    csound::Chord C7 = csound::chordForName("C7");
    std::cout << "Should be C7:" << std::endl << C7.information().c_str() << std::endl;
    csound::Chord G7 = csound::chordForName("G7");
    std::cout << "Should be G7:" << std::endl << G7.information().c_str() << std::endl;
    csound::Chord CM9 = csound::chordForName("CM9");
    std::cout << "Should be CM9:" << std::endl << CM9.information().c_str() << std::endl;
    CM9.setPitch(0, 0.);
    CM9.setPitch(1, 4.);
    CM9.setPitch(2, 7.);
    CM9.setPitch(3,-1.);
    CM9.setPitch(4, 2.);
    std::cout << "Should be CM9:" << std::endl << CM9.information().c_str() << std::endl;
    csound::Chord Dm9 = csound::chordForName("Dm9");
    std::cout << "Should be Dm9:" << std::endl << Dm9.information().c_str() << std::endl;
    Dm9.setPitch(0, 2.);
    Dm9.setPitch(1, 5.);
    Dm9.setPitch(2, 9.);
    Dm9.setPitch(3, 0.);
    Dm9.setPitch(4, 4.);
    std::cout << "Should be Dm9:" << std::endl << Dm9.information().c_str() << std::endl;
    csound::Chord chordForName_ = csound::chordForName("CM9");
    csound::print("chordForName(%s): %s\n", "CM9", chordForName_.information().c_str());
    csound::print("\nTesting all equivalence relations...\n\n");
    for (int voiceCount = 3; voiceCount <= maximumVoiceCountToTest; ++voiceCount) {
        testEquivalenceRelations(voiceCount, csound::OCTAVE(), 1.0);
    }
    auto optgiByNormalize = csound::fundamentalDomainByNormalize<csound::EQUIVALENCE_RELATION_RPTgI>(4, 12.0, 1.0);
    printSet("optgiByNormalize", optgiByNormalize);
    auto optgiByIsNormal = csound::fundamentalDomainByIsNormal<csound::EQUIVALENCE_RELATION_RPTgI>(4, 12.0, 1.0);
    printSet("optgiByIsNormal", optgiByIsNormal);
    if (optgiByNormalize == optgiByIsNormal) {
        csound::print("optgiByNormalize == optgiByIsNormal\n");
    } else {
        csound::print("optgiByNormalize != optgiByIsNormal\n");
    }
    csound::ChordSpaceGroup chordSpaceGroup;
    chordSpaceGroup.createChordSpaceGroup(4, csound::OCTAVE() * 5.0, 1.0);
    chordSpaceGroup.list(true, true, true);
    testChordSpaceGroup(chordSpaceGroup, "Gb7");
    csound::print("\nTesting all of chord space groups...\n\n");
    testAllOfChordSpaceGroup(3, maximumVoiceCountToTest);
    csound::print("\nFINISHED.\n\n");
    return 0;
}
