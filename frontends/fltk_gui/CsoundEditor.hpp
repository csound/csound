/*
    cseditor.cxx :

    Copyright (C) 2006, 2007 by David Akbari and Andres Cabrera

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef CSOUNDEDITOR_HPP
#define CSOUNDEDITOR_HPP

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>


#include <string>
#include <vector>

class CsoundGUIMain;

using namespace std;

enum syntaxCategory {
  OPCODE,
  HEADER,
  CONDITIONAL
};

class Keywords
{
  public:
    Keywords();
    ~Keywords();

  private:
    vector<string> opcodesAlpha;
    vector<int> opcodesAlphaIndex;

    vector<string> opcodes;
    vector<string> opcodesInargs;
    vector<string> opcodesOutargs;
    vector<string> opcodesCategory;
    vector<syntaxCategory> opcodesSyntax;

};

/*
class CsoundTextEditor : public Fl_Double_Window
{
 public:
   CsoundTextEditor(int w, int h, const char* t);
   ~CsoundTextEditor();


};*/

class CsoundEditorWindow : public Fl_Double_Window
{
public:
  CsoundEditorWindow(int w, int h, const char* t, const char *file = 0, bool isOrc_ = true);
  ~CsoundEditorWindow();
  int openFile(const char *filename_);

  Fl_Text_Editor     *editor;
  Fl_Text_Buffer     *textbuf;
  Fl_Text_Buffer     *stylebuf;
  Fl_Window          *replace_dlg;
  Fl_Input           *replace_find;
  Fl_Input           *replace_with;

  CsoundGUIMain      *parent;
  char               search[256];
  char               filename[256];
  int loading;
  int changed;
  bool isOrc;
private:
  Fl_Button          *replace_all;
  Fl_Return_Button   *replace_next;
  Fl_Button          *replace_cancel;
};

#endif
