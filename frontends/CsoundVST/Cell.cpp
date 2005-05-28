/**
 * C S O U N D   V S T
 *
 * A VST plugin version of Csound, with Python scripting.
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
#ifdef _MSC_VER
#pragma warning (disable:4786)
#endif
#include "Cell.hpp"
#include "System.hpp"

namespace csound
{
  Cell::Cell()
  {
  }

  Cell::~Cell()
  {
  }

  void Cell::produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &compositeCoordinates)
  {
    //  Find the total duration of notes produced by the child nodes of this.
    if(score.empty())
      {
        return;
      }
    const Event &event = score[beginAt];
    double beginSeconds = event.getTime();
    double endSeconds = beginSeconds;
    double totalDurationSeconds = 0;
    for(size_t i = beginAt; i < endAt; i++)
      {
        const Event &event = score[i];
        if (beginSeconds > event.getTime())
          {
            beginSeconds = event.getTime();
          }
        if (endSeconds < (event.getTime() + event.getDuration()))
          {
            endSeconds = (event.getTime() + event.getDuration());
          }
      }
    if (relativeDuration)
      {
        totalDurationSeconds = durationSeconds + (endSeconds - beginSeconds);
      }
    else
      {
        totalDurationSeconds = durationSeconds;
      }
    System::message("Repeat section.\n");
    System::message(" Began %f\n", beginSeconds);
    System::message(" Ended %f\n", endSeconds);
    System::message(" Duration %f\n", totalDurationSeconds);
    //  Repeatedly clone the notes produced by the child nodes of this,
    //  incrementing the time as required.
    double currentTime = beginSeconds;
    //  First "repeat" is already there!
    for(size_t i = size_t(1); i < (size_t) repeatCount; i++)
      {
        currentTime += totalDurationSeconds;
        System::message("  Repetition %d time %f\n", i, currentTime);
        for(size_t j = beginAt; j < endAt; j++)
          {
            Event *clonedEvent = new Event(score[j]);
            clonedEvent->setTime(clonedEvent->getTime() + currentTime);
            score.push_back(*clonedEvent);
          }
      }
  }
}
