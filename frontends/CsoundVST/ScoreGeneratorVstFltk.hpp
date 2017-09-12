/**
 * S C O R E   G E N E R A T O R   V S T
 *
 * A VST plugin for writing score generators in Python.
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
#ifndef CSOUNDVSTFLTK_H
#define CSOUNDVSTFLTK_H

class ScoreGeneratorVst;

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
#include "ScoreGeneratorVst.hpp"
#include "ScoreGeneratorVstUi.h"

#if defined(WIN32)

extern HINSTANCE hInstance;

#endif

class WaitCursor
{
  void *cursor;
public:
  WaitCursor();
  virtual ~WaitCursor();
};

class ScoreGeneratorVstFltk :
  public AEffEditor
{
public:
  void *windowHandle;
  Fl_Window *scoreGeneratorVstUi;
  ScoreGeneratorVst *scoreGeneratorVst;
  int useCount;
  static std::string aboutText;
  typedef enum {
    kEditorWidth = 610,
    kEditorHeight = 430,
    xPad = 4,
    yPad = 4
  } AEffEditorSize;
  bool updateFlag;
  Fl_Pack *mainPack;
  Fl_Tabs *mainTabs;
  Fl_Group *runtimeMessagesGroup;
  Fl_Browser *runtimeMessagesBrowser;
  Fl_Text_Editor *scriptTextEdit;
  Fl_Text_Buffer *scriptTextBuffer;
  Fl_Text_Buffer *aboutTextBuffer;
  Fl_Text_Display *aboutTextDisplay;
  Fl_Group *scriptGroup;
  std::list<std::string> messages;
  std::string helpFilename;
  std::string messagebuffer;
  ScoreGeneratorVstFltk(AudioEffect *audioEffect);
  virtual ~ScoreGeneratorVstFltk(void);
  virtual void updateCaption();
  virtual void updateModel();
  virtual void log(char *message);
  virtual void logv(char *message,...);
  //    AEffEditor overrides.
  virtual bool getRect(ERect **rect);
  virtual bool open(void *windowHandle);
  virtual void close();
  virtual void idle();
  virtual void update();
  virtual void postUpdate();
  // FLTK event handlers.
  void onNew(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk);
  void onNewVersion(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk);
  void onOpen(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk);
  void onSave(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk);
  void onSaveAs(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk);
  void onGenerate(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk);
};

#endif

