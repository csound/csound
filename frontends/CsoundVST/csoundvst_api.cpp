/*
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
#include "csoundvst_api.h"
#include "CsoundVstFltk.hpp"
#include "CsoundVST.hpp"
#include "CppSound.hpp"
#include "CsoundFile.hpp"
#include <csound.h>

#ifdef __cplusplus
extern "C" {
#endif

PUBLIC void *csvstCreate()
{
    return CreateCsoundVST();
}

PUBLIC void csvstDestroy(void *csvst)
{
    delete (CsoundVST *)csvst;
}

PUBLIC void csvstSetMode(void *csvst, int mode)
{
    ((CsoundVST *)csvst)->setIsPython(mode);
}

PUBLIC int csvstGetMode(void *csvst)
{
    return ((CsoundVST *)csvst)->getIsPython();
}

PUBLIC void csvstLoad(void *csvst, const char *filename)
{
    ((CsoundVST *)csvst)->load(filename);
}

PUBLIC void csvstImport(void *csvst, const char *filename)
{
    ((CsoundVST *)csvst)->getCppSound()->importFile(filename);
}

PUBLIC void csvstSave(void *csvst, const char *filename)
{
    ((CsoundVST *)csvst)->save(filename);
}

PUBLIC void csvstExport(void *csvst, const char *filename)
{
    ((CsoundVST *)csvst)->save(filename);
}

PUBLIC void csvstPerform(void *csvst)
{
    ((CsoundVST *)csvst)->perform();
}

PUBLIC void csvstInputScoreLine(void *csvst, const char *scoreline)
{
    ((CsoundVST *)csvst)->getCppSound()->inputMessage(scoreline);
}

PUBLIC void csvstStopPerforming(void *csvst)
{
    ((CsoundVST *)csvst)->stop();
}

PUBLIC void csvstOpenWindow(void *csvst)
{
    AEffEditor *editor = ((CsoundVST *)csvst)->getEditor();
    editor->open(0);
}

PUBLIC void csvstCloseWindow(void *csvst)
{
    AEffEditor *editor = ((CsoundVST *)csvst)->getEditor();
    editor->close();
}

PUBLIC void csvstClearScript(void *csvst)
{
    ((CsoundVST *)csvst)->Shell::clear();
}

PUBLIC void csvstClearCsd(void *csvst)
{
    ((CsoundVST *)csvst)->getCppSound()->removeAll();
}

PUBLIC void csvstClearCommand(void *csvst)
{
    ((CsoundVST *)csvst)->getCppSound()->removeCommand();
}

PUBLIC void csvstClearOrchestra(void *csvst)
{
    ((CsoundVST *)csvst)->getCppSound()->removeOrchestra();
}

PUBLIC void csvstClearArrangement(void *csvst)
{
    ((CsoundVST *)csvst)->getCppSound()->removeArrangement();
}

PUBLIC void csvstClearScore(void *csvst)
{
    ((CsoundVST *)csvst)->getCppSound()->removeScore();
}

PUBLIC const char *csvstGetScript(void *csvst)
{
    return ((CsoundVST *)csvst)->getScript().c_str();
}

PUBLIC const char *csvstGetCsd(void *csvst)
{
    return ((CsoundVST *)csvst)->getCppSound()->getCSD().c_str();
}

PUBLIC const char *csvstGetCommand(void *csvst)
{
    return ((CsoundVST *)csvst)->getCppSound()->getCommand().c_str();
}

PUBLIC const char *csvstGetOrchestra(void *csvst)
{
    return ((CsoundVST *)csvst)->getCppSound()->getOrchestra().c_str();
}

PUBLIC int csvstGetArrangementCount(void *csvst)
{
    return ((CsoundVST *)csvst)->getCppSound()->getArrangementCount();
}

PUBLIC const char *csvstGetArrangement(void *csvst, int index)
{
    return ((CsoundVST *)csvst)->getCppSound()->getArrangement(index).c_str();
}

PUBLIC const char *csvstGetScore(void *csvst)
{
    return ((CsoundVST *)csvst)->getCppSound()->getScore().c_str();
}

PUBLIC void csvstSetScript(void *csvst, const char *data)
{
    ((CsoundVST *)csvst)->setScript(data);
}

PUBLIC void csvstSetCsd(void *csvst, const char *data)
{
    ((CsoundVST *)csvst)->getCppSound()->setCSD(data);
}

PUBLIC void csvstSetCommand(void *csvst, const char *data)
{
    ((CsoundVST *)csvst)->getCppSound()->setCommand(data);
}

PUBLIC void csvstSetOrchestra(void *csvst, const char *data)
{
    ((CsoundVST *)csvst)->getCppSound()->setOrchestra(data);
}

PUBLIC void csvstAddArrangement(void *csvst, const char *instrumentName)
{
    ((CsoundVST *)csvst)->getCppSound()->addArrangement(instrumentName);
}

PUBLIC void csvstSetScore(void *csvst, const char *data)
{
    ((CsoundVST *)csvst)->getCppSound()->setScore(data);
}

PUBLIC void csvstAddScoreLine(void *csvst, const char *scoreline)
{
    ((CsoundVST *)csvst)->getCppSound()->addScoreLine(scoreline);
}

PUBLIC void csvstAddNote3(void *csvst, double p1_instrument, double p2_time, double p3_duration)
{
    ((CsoundVST *)csvst)->getCppSound()->addNote(p1_instrument, p2_time, p3_duration);
}

PUBLIC void csvstAddNote4(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key)
{
    ((CsoundVST *)csvst)->getCppSound()->addNote(p1_instrument, p2_time, p3_duration, p4_key);
}

PUBLIC void csvstAddNote5(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity)
{
    ((CsoundVST *)csvst)->getCppSound()->addNote(p1_instrument, p2_time, p3_duration, p4_key, p5_velocity);
}

PUBLIC void csvstAddNote6(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity, double p6_phase)
{
    ((CsoundVST *)csvst)->getCppSound()->addNote(p1_instrument, p2_time, p3_duration, p4_key, p5_velocity, p6_phase);
}

PUBLIC void csvstAddNote7(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity, double p6_phase, double p7_x)
{
    ((CsoundVST *)csvst)->getCppSound()->addNote(p1_instrument, p2_time, p3_duration, p4_key, p5_velocity, p6_phase, p7_x);
}

PUBLIC void csvstAddNote8(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity, double p6_phase, double p7_x, double p8_y)
{
    ((CsoundVST *)csvst)->getCppSound()->addNote(p1_instrument, p2_time, p3_duration, p4_key, p5_velocity, p6_phase, p7_x, p8_y);
}

PUBLIC void csvstAddNote9(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity, double p6_phase, double p7_x, double p8_y, double p9_z)
{
    ((CsoundVST *)csvst)->getCppSound()->addNote(p1_instrument, p2_time, p3_duration, p4_key, p5_velocity, p6_phase, p7_x, p8_y, p9_z);
}

PUBLIC void csvstAddNote10(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity, double p6_phase, double p7_x, double p8_y, double p9_z, double p10_pitchClassSet)
{
    ((CsoundVST *)csvst)->getCppSound()->addNote(p1_instrument, p2_time, p3_duration, p4_key, p5_velocity, p6_phase, p7_x, p8_y, p9_z, p10_pitchClassSet);
}

#ifdef __cplusplus
};
#endif

