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
#ifndef CSOUNDVST_API_H
#define CSOUNDVST_API_H

#if defined WIN32
#define PUBLIC __declspec(dllexport)
#else
#define PUBLIC
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
* This is a high-level "C" API for CsoundVST,
* for Mathematica and other hosts.
*/

#define CSVST_CLASSIC_MODE 0
#define CSVST_PYTHON_MODE 1

PUBLIC void *csvstCreate();
PUBLIC void csvstDestroy(void *csvst);

PUBLIC void csvstSetMode(void *csvst, int mode);
PUBLIC int csvstGetMode(void *csvst);

PUBLIC void csvstLoad(void *csvst, const char *filename);
PUBLIC void csvstImport(void *csvst, const char *filename);
PUBLIC void csvstSave(void *csvst, const char *filename);
PUBLIC void csvstExport(void *csvst, const char *filename);

PUBLIC void csvstPerform(void *csvst);
PUBLIC void csvstInputScoreLine(void *csvst, const char *scoreline);
PUBLIC void csvstStopPerforming(void *csvst);

PUBLIC void csvstOpenWindow(void *csvst);
PUBLIC void csvstCloseWindow(void *csvst);

PUBLIC void csvstClearScript(void *csvst);
PUBLIC void csvstClearCsd(void *csvst);
PUBLIC void csvstClearCommand(void *csvst);
PUBLIC void csvstClearOrchestra(void *csvst);
PUBLIC void csvstClearArrangement(void *csvst);
PUBLIC void csvstClearScore(void *csvst);

PUBLIC const char *csvstGetScript(void *csvst);
PUBLIC const char *csvstGetCsd(void *csvst);
PUBLIC const char *csvstGetCommand(void *csvst);
PUBLIC const char *csvstGetOrchestra(void *csvst);
PUBLIC int csvstGetArrangementCount(void *csvst);
PUBLIC const char *csvstGetArrangement(void *csvst, int index);
PUBLIC const char *csvstGetScore(void *csvst);

PUBLIC void csvstSetScript(void *csvst, const char *data);
PUBLIC void csvstSetCsd(void *csvst, const char *data);
PUBLIC void csvstSetCommand(void *csvst, const char *data);
PUBLIC void csvstSetOrchestra(void *csvst, const char *data);
PUBLIC void csvstAddArrangement(void *csvst, const char *instrumentName);
PUBLIC void csvstSetScore(void *csvst, const char *data);

PUBLIC void csvstAddScoreLine(void *csvst, const char *scoreline);
PUBLIC void csvstAddNote3(void *csvst, double p1_instrument, double p2_time, double p3_duration);
PUBLIC void csvstAddNote4(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key);
PUBLIC void csvstAddNote5(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity);
PUBLIC void csvstAddNote6(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity, double p6_phase);
PUBLIC void csvstAddNote7(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity, double p6_phase, double p7_x);
PUBLIC void csvstAddNote8(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity, double p6_phase, double p7_x, double p8_y);
PUBLIC void csvstAddNote9(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity, double p6_phase, double p7_x, double p8_y, double p9_z);
PUBLIC void csvstAddNote10(void *csvst, double p1_instrument, double p2_time, double p3_duration, double p4_key, double p5_velocity, double p6_phase, double p7_x, double p8_y, double p9_z, double p10_pitchClassSet);

#ifdef __cplusplus
};
#endif

#endif

