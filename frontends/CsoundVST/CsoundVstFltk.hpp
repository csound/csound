//  COUNDVST
//
//  A VST plugin version of Csound.
//
//  VST is a trademark of Steinberg Media Technologies GmbH.
//  VST Plug-In Technology by Steinberg.
//
//  Copyright (C) 2004 Michael Gogins
//
//  The CsoundVST library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  The CsoundVST library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with the CsoundVST library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
//  02110-1301 USA
//
//  Linking CsoundVST statically or dynamically with other modules is making a
//  combined work based on CsoundVST. Thus, the terms and conditions of the GNU
//  Lesser General Public License cover the whole combination.
//
//  In addition, as a special exception, the copyright holders of CsoundVST
//  give you permission to combine CsoundVST with free software programs
//  or libraries that are released under the GNU LGPL and with code included
//  in the standard release of the VST SDK version 2 under the terms of the
//  license stated in the VST SDK version 2 files. You may copy and distribute
//  such a system following the terms of the GNU LGPL for CsoundVST and the
//  licenses of the other code concerned. The source code for the VST SDK
//  version 2 is available in the VST SDK hosted at
//  https://github.com/steinbergmedia/vst3sdk.
//
//  Note that people who make modified versions of CsoundVST are not obligated to
//  grant this special exception for their modified versions; it is their
//  choice whether to do so. The GNU Lesser General Public License gives
//  permission to release a modified version without this exception; this
//  exception also makes it possible to release a modified version which
//  carries forward this exception.

#ifndef CSOUNDVSTFLTK_H
#define CSOUNDVSTFLTK_H

class CsoundVstFltk;

#include "public.sdk/source/vst2.x/aeffeditor.h"
#include <FL/Fl_Help_View.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Group.H>
#include <list>
#undef KeyPress
#include "CsoundVST.hpp"
#include "CsoundVstUi.h"

#if defined(WIN32)
#include <windows.h>

extern HINSTANCE hInstance;

#endif

class SILENCE_PUBLIC WaitCursor
{
  void *cursor;
public:
  WaitCursor();
  ~WaitCursor();
};

class CsoundVstFltk :
  public AEffEditor
{
public:
  typedef enum {
    kEditorWidth = 708,
    kEditorHeight = 389,
    xPad = 4,
    yPad = 4
  } AEffEditorSize;
  static std::string aboutText;
  static Fl_Preferences preferences;
  CsoundVST *csoundVST;
  void *windowHandle;
  Fl_Double_Window *csoundVstUi;
  int useCount;
  bool updateFlag;
  Fl_Tabs *mainTabs;
  Fl_Input *commandInput;
  Fl_Browser *runtimeMessagesBrowser;
  Fl_Text_Editor *orchestraTextEdit;
  Fl_Text_Buffer *orchestraTextBuffer;
  Fl_Text_Editor *scoreTextEdit;
  Fl_Text_Buffer *scoreTextBuffer;
  Fl_Input *settingsEditSoundfileInput;
  Fl_Check_Button* settingsVstPluginModeEffect;
  Fl_Check_Button* settingsVstPluginModeInstrument;
  Fl_Text_Buffer *aboutTextBuffer;
  Fl_Text_Display *aboutTextDisplay;
  Fl_Group *orchestraGroup;
  Fl_Group *scoreGroup;
  std::list<std::string> messages;
  std::string helpFilename;
  std::string messagebuffer;
  static void messageCallback(CSOUND *csound, int attribute, const char *format, va_list valist);
  CsoundVstFltk(AudioEffect *audioEffect);
  virtual ~CsoundVstFltk(void);
  virtual void updateCaption();
  virtual void updateModel();
  //    AEffEditor overrides.
  virtual bool getRect(ERect **rect);
  virtual bool open(void *windowHandle);
  virtual void close();
  virtual void idle();
  virtual void update();
  virtual void postUpdate();
  void onPerformScriptButtonThreadRoutine();
  // FLTK event handlers.
  void onNew(Fl_Button*, CsoundVstFltk* csoundVstFltk);
  void onNewVersion(Fl_Button*, CsoundVstFltk* csoundVstFltk);
  void onOpen(Fl_Button*, CsoundVstFltk* csoundVstFltk);
  void onImport(Fl_Button*, CsoundVstFltk* csoundVstFltk);
  void onSave(Fl_Button*, CsoundVstFltk* csoundVstFltk);
  void onSaveAs(Fl_Button*, CsoundVstFltk* csoundVstFltk);
  void onPerform(Fl_Button*, CsoundVstFltk* csoundVstFltk);
  void onStop(Fl_Button*, CsoundVstFltk* csoundVstFltk);
  void onEdit(Fl_Button*, CsoundVstFltk* csoundVstFltk);
  void onSettingsVstPluginMode(Fl_Check_Button*, CsoundVstFltk* csoundVstFltk);
  void onSettingsVstInstrumentMode(Fl_Check_Button*, CsoundVstFltk* csoundVstFltk);
  void onSettingsApply(Fl_Button*, CsoundVstFltk* csoundVstFltk);
};

#endif
