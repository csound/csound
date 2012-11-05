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
#include "CppSound.hpp"
#include "Cell.hpp"
#include "System.hpp"

namespace csound
{

Cell::Cell() : repeatCount(1), relativeDuration(true), durationSeconds(0)
{
}

Cell::~Cell()
{
}

void Cell::produceOrTransform(Score &score,
        size_t beginAt,
        size_t endAt,
        const Eigen::MatrixXd &compositeCoordinates)
{
    //  Find the total duration of notes produced by the child nodes of this.
    if(score.empty()) {
        return;
    }
    const Event &event = score[beginAt];
    double beginSeconds = event.getTime();
    double endSeconds = beginSeconds;
    double totalDurationSeconds = 0;
    for(size_t i = beginAt; i < endAt; i++) {
        const Event &event = score[i];
        if (beginSeconds > event.getTime()) {
            beginSeconds = event.getTime();
        }
        if (endSeconds < (event.getTime() + event.getDuration())) {
            endSeconds = (event.getTime() + event.getDuration());
        }
    }
    if (relativeDuration) {
        totalDurationSeconds = durationSeconds + (endSeconds - beginSeconds);
    } else {
        totalDurationSeconds = durationSeconds;
    }
    System::message("Repeat section.\n");
    System::message(" Began    %9.4f\n", beginSeconds);
    System::message(" Ended    %9.4f\n", endSeconds);
    System::message(" Duration %9.4f\n", totalDurationSeconds);
    //  Repeatedly clone the notes produced by the child nodes of this,
    //  incrementing the time as required.
    double currentTime = beginSeconds;
    //  First "repeat" is already there!
    for(size_t i = size_t(1); i < (size_t) repeatCount; i++) {
        currentTime += totalDurationSeconds;
        System::message("  Repetition %d time %9.4f\n", i, currentTime);
        for(size_t j = beginAt; j < endAt; j++) {
            Event clonedEvent = score[j];
            clonedEvent.setTime(clonedEvent.getTime() + currentTime);
            score.push_back(clonedEvent);
        }
    }
}

Intercut::Intercut()
{
}

Intercut::~Intercut()
{
}
/**
 * The notes produced by each child node are intercut to produce
 * the notes produced by this; e.g. if there are 3 child nodes, then
 * the notes produced by this are node 0 note 0, node 1 note 0, node
 * 2 note 0; node 0 note 1, node 1 note 1, node node 2 note 1; node 0
 * note 2, node 1 note 2, node 2 note 2, and so on. If the child nodes
 * do not each produce the same number of notes, then the behavior
 * is controlled by the repeatEach flag. Chords are treated as single
 * notes.
 */

Eigen::MatrixXd Intercut::traverse(const Eigen::MatrixXd &globalCoordinates,
        Score &collectingScore)
{
    Eigen::MatrixXd compositeCoordinates = getLocalCoordinates() * globalCoordinates;
    if (children.size() < 2) {
        System::message("Intercut must have at least 2 child nodes.\n");
        return compositeCoordinates;
    }
    size_t beginAt = collectingScore.size();
    std::vector<Score> scores;
    std::vector<size_t> scoreIndexes;
    scores.resize(children.size());
    scoreIndexes.resize(children.size());
    for (int i = 0, n = children.size(); i < n; ++i) {
        children[i]->traverse(compositeCoordinates, scores[i]);
        scores[i].sort();
    }
    bool finished = false;
    double startTime = scores[0][0].getTime();
    for (int intercut = 0; !finished; ++intercut) {
        finished = true;
        int scoreI = intercut % scores.size();
        const Score &intercutScore = scores[scoreI];
        if (scoreIndexes[scoreI] < intercutScore.size()) {
            finished = false;
        }
        for (bool inChord = true; inChord; ) {
            int noteI = scoreIndexes[scoreI] % intercutScore.size();
            ++scoreIndexes[scoreI];
            Event event = intercutScore[noteI];
            double eventTime = event.getTime();
            event.setTime(startTime);
            score.append(event);
            size_t nextNoteI = noteI + 1;
            if (nextNoteI >= intercutScore.size()) {
                inChord = false;
            } else if (Conversions::eq_epsilon(intercutScore[nextNoteI].getTime(), eventTime)) {
                inChord = true;
            } else {
                inChord = false;
            }
            if (!inChord) {
                startTime = event.getOffTime();
            }
        }
    }
    size_t endAt = collectingScore.size();
    produceOrTransform(collectingScore, beginAt, endAt, compositeCoordinates);
    return compositeCoordinates;
}

Stack::Stack() : duration(0.0)
{
}

Stack::~Stack()
{
}

Eigen::MatrixXd Stack::traverse(const Eigen::MatrixXd &globalCoordinates,
        Score &collectingScore)
{
    Eigen::MatrixXd compositeCoordinates = getLocalCoordinates() * globalCoordinates;
    if (children.size() < 2) {
        System::message("Stack must have at least 2 child nodes.\n");
        return compositeCoordinates;
    }
    size_t beginAt = collectingScore.size();
    std::vector<Score> scores;
    scores.resize(children.size());
    for (int i = 0, n = children.size(); i < n; ++i) {
        children[i]->traverse(compositeCoordinates, scores[i]);
    }
    double newDuration = duration;
    if (duration == 0.0) {
        newDuration = scores[0].getDuration();
    }
    for (int i = 0, n = scores.size(); i < n; ++i) {
        Score &subScore = scores[i];
        subScore.setDuration(newDuration);
        for (size_t j = 0, k = subScore.size(); j < k; ++j) {
            score.append(subScore[j]);
        }
    }
    size_t endAt = collectingScore.size();
    produceOrTransform(collectingScore, beginAt, endAt, compositeCoordinates);
    return compositeCoordinates;
}

Koch::Koch()
{
}

Koch::~Koch()
{
}

  void Koch::setPitchOffsetForLayer(int layer, double offset)
  {
    pitchOffsetsForLayers[layer] = offset;
  }

Eigen::MatrixXd Koch::traverse(const Eigen::MatrixXd &globalCoordinates,
        Score &collectingScore)
{
    Eigen::MatrixXd compositeCoordinates = getLocalCoordinates() * globalCoordinates;
    if (children.size() < 2) {
        System::message("Koch must have at least 2 child nodes.\n");
        return compositeCoordinates;
    }
    size_t beginAt = collectingScore.size();
    Eigen::MatrixXd rescaler = Eigen::MatrixXd::Identity(Event::ELEMENT_COUNT, Event::ELEMENT_COUNT);
    // All notes produced by upper are stacked on top of each note produced by lower.
    // This is the uppermost layer.
    Score upperScore;
    children.back()->traverse(compositeCoordinates, score);
    for (int lowerI = children.size() - 2; lowerI > -1; lowerI--) {
        upperScore = score;
        upperScore.sort();
        score.clear();
        System::message("level: %4d  upperScore: %8d events.\n", lowerI, upperScore.size());
	upperScore.findScale();
        Event upperScoreToOrigin = upperScore.scaleActualMinima;
        Score lowerScore;
        children[lowerI]->traverse(compositeCoordinates, lowerScore);
        lowerScore.sort();
        System::message("level: %4d  lowerScore: %8d events.\n", lowerI, lowerScore.size());
	double pitchOffset = 0.0;
	int layer = lowerI + 1;
	if (pitchOffsetsForLayers.find(layer) != pitchOffsetsForLayers.end()) {
	  pitchOffset = pitchOffsetsForLayers[layer];
	}
        for (size_t lowerNoteI = 0, lowerNoteN = lowerScore.size();
                lowerNoteI < lowerNoteN;
                ++lowerNoteI) {
            Event lowerEvent = lowerScore[lowerNoteI];
            score.append(lowerEvent);
            double durationRatio = lowerEvent.getDuration() / upperScore.getDuration();
            Eigen::MatrixXd rescaler = Eigen::MatrixXd::Identity(Event::ELEMENT_COUNT, Event::ELEMENT_COUNT);
            // Time: Move to origin of lower note and rescale by duration of lower note.
            rescaler(Event::TIME, Event::HOMOGENEITY) = lowerEvent.getTime();
            rescaler(Event::TIME, Event::TIME) = durationRatio;
            // Duration: Rescale by duration of lower note.
            rescaler(Event::DURATION, Event::DURATION) = durationRatio;
            // Pitch: Move to lower note minus first upper note.
            rescaler(Event::KEY, Event::HOMOGENEITY) = lowerEvent.getKey() - pitchOffset + upperScoreToOrigin.getKey();
            // Velocity: Move to lower note minus first upper note.
            rescaler(Event::VELOCITY, Event::HOMOGENEITY) = lowerEvent.getVelocity() - upperScoreToOrigin.getVelocity();
            for (size_t upperNoteI = 0, upperNoteN = upperScore.size();
                    upperNoteI < upperNoteN;
                    ++upperNoteI) {
                Eigen::VectorXd upperEvent = rescaler * upperScore[upperNoteI];
                score.append(upperEvent);
            }
        }
        System::message("level: %4d  generated:  %8d events.\n", lowerI, score.size());
    }
    size_t endAt = collectingScore.size();
    produceOrTransform(collectingScore, beginAt, endAt, compositeCoordinates);
    return compositeCoordinates;
}

}
