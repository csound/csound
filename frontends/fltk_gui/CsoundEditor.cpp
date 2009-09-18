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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "CsoundEditor.hpp"

#ifdef __MWERKS__
# define FL_DLL
#endif

#include "CsoundGUI.hpp"
#include "CsoundGUIMain_FLTK.hpp"
#include "Keywords.cpp"


using namespace std;

char               title[256];

Fl_Text_Display::Style_Table_Entry styletable[] = {      // Style table
  { FL_BLACK,      FL_COURIER,        FL_NORMAL_SIZE }, // A - Plain
  { FL_DARK_GREEN, FL_COURIER_ITALIC, FL_NORMAL_SIZE }, // B - Line comments
  { FL_DARK_GREEN, FL_COURIER_ITALIC, FL_NORMAL_SIZE }, // C - Block comments
  { FL_RED,        FL_COURIER,        FL_NORMAL_SIZE }, // D - Strings
  { FL_DARK_RED,   FL_COURIER,        FL_NORMAL_SIZE }, // E - Directives
  { FL_DARK_RED,   FL_COURIER_BOLD,   FL_NORMAL_SIZE }, // F - Types
  { FL_BLUE,       FL_COURIER_BOLD,   FL_NORMAL_SIZE }  // G - Keywords
};

// 'compare_keywords()' - Compare two keywords...

extern "C" {
  int
    compare_keywords(const void *a, const void *b)
  {
    return (strcmp(*((const char **)a), *((const char **)b)));
  }
}

// 'style_parse()' - Parse text and produce style data.

void style_parse(const char *text, char *style,int length)
{
  char        current;
  int         col;
  int         last;
  char        buf[255], *bufptr;
  const char *temp;

//  Style letters:
//
//       A - Plain
//       B - Line comments
//       C - Block comments
//       D - Strings
//       E - Directives
//       F - Types
//       G - Keywords

  for (current = *style, col = 0, last = 0; length > 0; length --, text ++) {
    if (current == 'B' || current == 'F' || current == 'G')current = 'A';
    if (current == 'A') {
     // Check for directives, comments, strings, and keywords...
      if (col == 0 && (*text == '#' || *text == '<')) {
       // Set style to directive
        current = 'E';
      } else if (strncmp(text, ";", 1) == 0) {
        current = 'B';
        for (; length > 0 && *text != '\n'; length --, text ++) *style++ = 'B';

        if (length == 0) break;
      } else if (strncmp(text, "/*", 2) == 0) {
        current = 'C';
      } else if (strncmp(text, "\\\"", 2) == 0) {
       // Quoted quote...
        *style++ = current;
        *style++ = current;
        text ++;
        length --;
        col += 2;
        continue;
      } else if (*text == '\"') {
        current = 'D';
      } else if (!last && (( islower(*text) || isupper(*text) ) || *text == '_')) {
       // Might be a keyword...
        for (temp = text, bufptr = buf;
             (( islower(*temp) || isupper(*temp) ) || *temp == '_') && bufptr < (buf + sizeof(buf) - 1);
             *bufptr++ = *temp++);

        if (!( islower(*temp) || isupper(*temp) ) && *temp != '_') {
          *bufptr = '\0';
          bufptr = buf;
          if (bsearch(&bufptr, code_types,
                      sizeof(code_types) / sizeof(code_types[0]),
                      sizeof(code_types[0]), compare_keywords))
          {
            while (text < temp) {
              *style++ = 'F';
              text ++;
              length --;
              col ++;
            }
            text --;
            length ++;
            last = 1;
            continue;
          } else if (bsearch(&bufptr, code_keywords,
                             sizeof(code_keywords) / sizeof(code_keywords[0]),
                             sizeof(code_keywords[0]), compare_keywords))
          {
            while (text < temp) {
              *style++ = 'G';
              text ++;
              length --;
              col ++;
            }
            text --;
            length ++;
            last = 1;
            continue;
          }
        }
      }
    } else if (current == 'C' && strncmp(text, "*/", 2) == 0) {
     // Close a C comment...
      *style++ = current;
      *style++ = current;
      text ++;
      length --;
      current = 'A';
      col += 2;
      continue;
    } else if (current == 'D') {
     // Continuing in string...
      if (strncmp(text, "\\\"", 2) == 0) {
       // Quoted end quote...
        *style++ = current;
        *style++ = current;
        text ++;
        length --;
        col += 2;
        continue;
      } else if (*text == '\"') {
       // End quote...
        *style++ = current;
        col ++;
        current = 'A';
        continue;
      }
    }
      // Copy style info...
    if (current == 'A' && (*text == '{' || *text == '}')) *style++ = 'G';
    else *style++ = current;
    col ++;
    last = isalnum(*text) || *text == '_' || *text == '.';
    if (*text == '\n') {
     // Reset column and possibly reset the style
      col = 0;
      if (current == 'B' || current == 'E') current = 'A';
    }
  }
}

// 'style_init()' - Initialize the style buffer...

void style_init(void *v)
{
  CsoundEditorWindow *e = (CsoundEditorWindow *)v;
  char *style = new char[e->textbuf->length() + 1];
  char *text = e->textbuf->text();
  memset(style, 'A', e->textbuf->length());
  style[e->textbuf->length()] = '\0';
  if (!e->stylebuf) e->stylebuf = new Fl_Text_Buffer(e->textbuf->length());
  style_parse(text, style, e->textbuf->length());
  e->stylebuf->text(style);
  delete[] style;
  free(text);
}

// 'style_unfinished_cb()' - Update unfinished styles.

void
style_unfinished_cb(int, void*) {
}

// 'style_update()' - Update the style buffer...

void style_update(int pos, int nInserted, int nDeleted,  int nRestyle, const char *deletedText, void *cbArg)
{
  int    start, end;
  char   last, *style, *text;
  CsoundEditorWindow* w = (CsoundEditorWindow*)cbArg;
 // If this is just a selection change, just unselect the style buffer...
  if (nInserted == 0 && nDeleted == 0) {
    w->stylebuf->unselect();
    return;
  }
 // Track changes in the text buffer...
  if (nInserted > 0) {
   // Insert characters into the style buffer...
    style = new char[nInserted + 1];
    memset(style, 'A', nInserted);
    style[nInserted] = '\0';
    w->stylebuf->replace(pos, pos + nDeleted, style);
    delete[] style;
  } else {
   // Just delete characters in the style buffer...
    w->stylebuf->remove(pos, pos + nDeleted);
  }
 // Select the area that was just updated to avoid unnecessary
 // callbacks...
  w->stylebuf->select(pos, pos + nInserted - nDeleted);
 // Re-parse the changed region; we do this by parsing from the
 // beginning of the previous line of the changed region to the end of
 // the line of the changed region...  Then we check the last
 // style character and keep updating if we have a multi-line
 // comment character...
  start = w->textbuf->line_start(pos);
//  if (start > 0) start = textbuf->line_start(start - 1);
  end   = w->textbuf->line_end(pos + nInserted);
  text  = w->textbuf->text_range(start, end);
  style = w->stylebuf->text_range(start, end);
  if (start==end)
    last = 0;
  else
    last  = style[end - start - 1];
//  printf("start = %d, end = %d, text = \"%s\", style = \"%s\", last='%c'...\n",
//         start, end, text, style, last);
  style_parse(text, style, end - start);
//  printf("new style = \"%s\", new last='%c'...\n",
//         style, style[end - start - 1]);
  w->stylebuf->replace(start, end, style);
  w->editor->redisplay_range(start, end);
  if (start==end || last != style[end - start - 1]) {
//    printf("Recalculate the rest of the buffer style\n");
   // Either the user deleted some text, or the last character
   // on the line changed styles, so reparse the
   // remainder of the buffer...
    free(text);
    free(style);
    end   = w->textbuf->length();
    text  = w->textbuf->text_range(start, end);
    style = w->stylebuf->text_range(start, end);
    style_parse(text, style, end - start);
    w->stylebuf->replace(start, end, style);
    w->editor->redisplay_range(start, end);
  }
  free(text);
  free(style);
}


// Editor window functions and class...
void save_cb(Fl_Widget*, void *v);
void saveas_cb(Fl_Widget*, void *v);
void find2_cb(Fl_Widget*, void*);
void changed_cb(int, int nInserted, int nDeleted,int, const char*, void* v);
void replall_cb(Fl_Widget*, void*);
void replace2_cb(Fl_Widget*, void*);
void replcan_cb(Fl_Widget*, void*);

int check_save(void* v)
{
  CsoundEditorWindow* w = (CsoundEditorWindow *)v;
  if (w->changed != 1) return 1;
  int r = fl_choice("The current file has not been saved.\n"
                    "Would you like to save it now?",
                    "Cancel", "Save", "Don't Save");
  if (r == 1) {
    save_cb(0 , v); // Save the file...
    return 1;
  }
  return (r == 2) ? 1 : 0;
}

void load_file(const char *newfile, int ipos, void *v)
{
  CsoundEditorWindow* w = (CsoundEditorWindow *)v;
  w->loading = 1;
  int insert = (ipos != -1);
  w->changed = insert;
  if (!insert) {
    strcpy(w->filename, "");
  }
  int r;
  if (!insert) r = w->textbuf->loadfile(newfile);
  else r = w->textbuf->insertfile(newfile, ipos);
  if (r)
    fl_alert("Error reading from file \'%s\':\n%s.", newfile, strerror(errno));
  else
    if (!insert) strcpy(w->filename, newfile);
  w->loading = 0;
  w->textbuf->call_modify_callbacks();
}

void save_file(char *newfile, void *v)
{
  CsoundEditorWindow* w = (CsoundEditorWindow *)v;
  if (w->textbuf->savefile(newfile))
    fl_alert("Error writing to file \'%s\':\n%s.", newfile, strerror(errno));
  else
    strcpy(w->filename, newfile);
  w->changed = 0;
  w->textbuf->call_modify_callbacks();
}

void new_cb(Fl_Widget*, void *v)
{
  CsoundEditorWindow* w = (CsoundEditorWindow*)v;
  if (!check_save(v)) return;
  w->filename[0] = '\0';
  w->textbuf->select(0, w->textbuf->length());
  w->textbuf->remove_selection();
  w->changed = 0;
  w->textbuf->call_modify_callbacks();
}

void open_cb(Fl_Widget*, void *v)
{
  CsoundEditorWindow* w = (CsoundEditorWindow*)v;
  if (!check_save(v)) return;
//   char *newfile = fl_file_chooser("Open File?", "*", w->filename);
//   if (newfile != NULL) load_file(newfile, -1, v );
  if (w->isOrc)
    w->parent->pushOpenOrcButton();
  else
    w->parent->pushOpenScoButton();
}

void insert_cb(Fl_Widget*, void *v)
{
  CsoundEditorWindow *w = (CsoundEditorWindow *)v;
  char *newfile = fl_file_chooser("Insert File?", "*", w->filename);
  if (newfile != NULL) load_file(newfile, w->editor->insert_position(), v);
}

void save_cb(Fl_Widget*, void *v)
{
  CsoundEditorWindow *w = (CsoundEditorWindow *)v;
  if (w->filename[0] == '\0') {
// No filename - get one!
    saveas_cb(0, v);
    return;
  }
  else save_file(w->filename, v);
}

void saveas_cb(Fl_Widget*, void *v)
{
  CsoundEditorWindow *w = (CsoundEditorWindow *)v;
  char *newfile;
  newfile = fl_file_chooser("Save File As?", "*", w->filename);
  if (newfile != NULL) {
    save_file(newfile, v);
    if (w->isOrc) {
      w->parent->orcNameInput->value(newfile);
      w->parent->stripString(w->parent->currentPerformanceSettings.orcName, newfile);
    }
    else {
      w->parent->scoreNameInput->value(newfile);
      w->parent->stripString(w->parent->currentPerformanceSettings.scoName, newfile);
    }
  }
}

void close_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow* w = (CsoundEditorWindow*)v;
  if (!check_save(v)) return;
  w->hide();
  //w->textbuf->remove_modify_callback(changed_cb, w);
  if (w->isOrc)
    w->parent->closeOrcEditor();
  else
    w->parent->closeScoEditor();
}

void cut_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow* w = (CsoundEditorWindow*)v;
  Fl_Text_Editor::kf_cut(0, w->editor);
}

void copy_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow* w = (CsoundEditorWindow*)v;
  Fl_Text_Editor::kf_copy(0, w->editor);
}

void paste_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow* e = (CsoundEditorWindow*)v;
  Fl_Text_Editor::kf_paste(0, e->editor);
}

void delete_cb(Fl_Widget*, void *v) {
  CsoundEditorWindow* w = (CsoundEditorWindow*)v;
  w->textbuf->remove_selection();
}

void find_cb(Fl_Widget* w, void* v) {
  CsoundEditorWindow* e = (CsoundEditorWindow*)v;
  const char *val;

  val = fl_input("Search String:", e->search);
  if (val != NULL) {
   // User entered a string - go find it!
    strcpy(e->search, val);
    find2_cb(w, v);
  }
}

void find2_cb(Fl_Widget* w, void* v) {
  CsoundEditorWindow* e = (CsoundEditorWindow*)v;
  if (e->search[0] == '\0') {
   // Search string is blank; get a new one...
    find_cb(w, v);
    return;
  }
  int pos = e->editor->insert_position();
  int found = e->textbuf->search_forward(pos, e->search, &pos);
  if (found) {
   // Found a match; select and update the position...
    e->textbuf->select(pos, pos+strlen(e->search));
    e->editor->insert_position(pos+strlen(e->search));
    e->editor->show_insert_position();
  }
}

void replace_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow* e = (CsoundEditorWindow*)v;
  e->replace_dlg->show();
}

void replace2_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow* e = (CsoundEditorWindow*)v;
  const char *find = e->replace_find->value();
  const char *replace = e->replace_with->value();
  if (find[0] == '\0') {
   // Search string is blank; get a new one...
    e->replace_dlg->show();
    return;
  }
  e->replace_dlg->hide();
  int pos = e->editor->insert_position();
  int found = e->textbuf->search_forward(pos, find, &pos);
  if (found) {
   // Found a match; update the position and replace text...
    e->textbuf->select(pos, pos+strlen(find));
    e->textbuf->remove_selection();
    e->textbuf->insert(pos, replace);
    e->textbuf->select(pos, pos+strlen(replace));
    e->editor->insert_position(pos+strlen(replace));
    e->editor->show_insert_position();
  }
  else fl_alert("No occurrences of \'%s\' found!", find);
}

void manual_cb(Fl_Widget*, void* v)
{
  CsoundEditorWindow* w = (CsoundEditorWindow*)v;
  w->parent->runHelpBrowser("index.html");
}

void reference_cb(Fl_Widget*, void* v)
{
  CsoundEditorWindow* w = (CsoundEditorWindow*)v;
  int start = w->textbuf->word_start(w->editor->insert_position());
  int end = w->textbuf->word_end(w->editor->insert_position());
  char *page = (char *)calloc(128, sizeof(char));
  strcpy(page, w->textbuf->text_range(start,end));
  strcat(page, ".html\0");
  w->parent->runHelpBrowser(string(page));
  free(page);
}

void replall_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow* e = (CsoundEditorWindow*)v;
  const char *find = e->replace_find->value();
  const char *replace = e->replace_with->value();
  find = e->replace_find->value();
  if (find[0] == '\0') {
   // Search string is blank; get a new one...
    e->replace_dlg->show();
    return;
  }
  e->replace_dlg->hide();
  e->editor->insert_position(0);
  int times = 0;
 // Loop through the whole string
  for (int found = 1; found;) {
    int pos = e->editor->insert_position();
    found = e->textbuf->search_forward(pos, find, &pos);
    if (found) {
// Found a match; update the position and replace text...
      e->textbuf->select(pos, pos+strlen(find));
      e->textbuf->remove_selection();
      e->textbuf->insert(pos, replace);
      e->editor->insert_position(pos+strlen(replace));
      e->editor->show_insert_position();
      times++;
    }
  }
  if (times) fl_message("Replaced %d occurrences.", times);
  else fl_alert("No occurrences of \'%s\' found!", find);
}

void replcan_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow *e = (CsoundEditorWindow *)v;
  e->replace_dlg->hide();
}

void play_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow *w = (CsoundEditorWindow *)v;
  if (w->changed == 1)
    save_cb(0 , v); // Save the file...
  w->parent->pushPlayPauseButton();
}

void stop_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow *w = (CsoundEditorWindow *)v;
  w->parent->pushStopButton();
}

void rewind_cb(Fl_Widget*, void* v) {
  CsoundEditorWindow *w = (CsoundEditorWindow *)v;
  w->parent->pushRewindButton();
}

void set_title(CsoundEditorWindow* w) {
  if (!w->filename[0]) strcpy(title, "Untitled");
  else {
    char *slash;
    slash = strrchr(w->filename, '/');
#ifdef WIN32
    if (slash == NULL) slash = strrchr(w->filename, '\\');
#endif
    if (slash != NULL) strcpy(title, slash + 1);
    else strcpy(title, w->filename);
  }
  if (w->changed) strcat(title, " (modified)");
  w->label(title);
}

void changed_cb(int, int nInserted, int nDeleted,int, const char*, void* v)
{
  CsoundEditorWindow *w = (CsoundEditorWindow *)v;
  if ((nInserted || nDeleted) && w->loading == 0)
    w->changed = 1;
  set_title(w);
  if (w->loading == 1) w->editor->show_insert_position();
}

void quit_cb(Fl_Widget*, void *v)
{
  CsoundEditorWindow *w = (CsoundEditorWindow *)v;
  if (w->changed && !check_save(v))
    return;
  exit(0);
}

// Keywords::Keywords()
// {
//
// };

CsoundEditorWindow::CsoundEditorWindow(int w, int h, const char* t, const char *file, bool isOrc_) :
Fl_Double_Window(w, h, t), isOrc(isOrc_)
{
  Fl_Menu_Bar* m = new Fl_Menu_Bar(0, 0, 660, 30);
  Fl_Menu_Item menuitems[] = {
    { "&File",              0, 0, 0, FL_SUBMENU },
    { "&New File",        0, new_cb, (void *) (this) },
    { "&Open File...",    FL_CTRL + 'o', open_cb, (void *) (this) },
    { "&Insert File...",  FL_CTRL + 'i', insert_cb, (void *) (this),
        FL_MENU_DIVIDER },
    { "&Save File",       FL_CTRL + 's', save_cb, (void *) (this) },
    { "Save File &As...", FL_CTRL + FL_SHIFT + 's', (Fl_Callback
      *)saveas_cb, (void *) (this), FL_MENU_DIVIDER },
//     { "New &View", FL_ALT + 'v', view_cb, (void *) (this) },
    { "&Close View", FL_CTRL + 'w', close_cb, (void *) (this)},
//     { "Close", FL_CTRL + 'q', (Fl_Callback *)quit_cb, (void *) (this) },
    { 0 },

    { "&Edit", 0, 0, 0, FL_SUBMENU },
    { "Cu&t",        FL_CTRL + 'x', cut_cb, (void *) (this) },
    { "&Copy",       FL_CTRL + 'c', copy_cb, (void *) (this) },
    { "&Paste",      FL_CTRL + 'v', paste_cb, (void *) (this) },
    { "&Delete",     0, delete_cb, (void *) (this) },
    { 0 },

    { "&Search", 0, 0, 0, FL_SUBMENU },
    { "&Find...",       FL_CTRL + 'f', find_cb, (void *) (this) },
    { "F&ind Again",    FL_CTRL + 'g', find2_cb, (void *) (this) },
    { "&Replace...",    FL_CTRL + 'r', replace_cb, (void *) (this)},
    { "Re&place Again", FL_CTRL + 't', replace2_cb, (void *) (this) },
    { 0 },

    { "&Control", 0, 0, 0, FL_SUBMENU },
    { "&Play/Pause",       FL_F+8, play_cb, (void *) (this) },
    { "&Stop",    FL_F+9, stop_cb, (void *) (this) },
    { "&Rewind",    FL_CTRL + FL_F+8, rewind_cb, (void *) (this)},
//     { "&Fast forward", FL_F+10, replace2_cb, (void *) (this) },
    { 0 },

    { "&Help", 0, 0, 0, FL_SUBMENU },
    { "&Manual",       FL_F+1, manual_cb, (void *) (this) },
    { "&Opcode Reference",    FL_SHIFT + FL_F+1, reference_cb, (void *) (this) },
    { 0 },

    { 0 }
  };

  m->copy(menuitems);
  replace_dlg = new Fl_Window(300, 105, "Replace");
  replace_find = new Fl_Input(80, 10, 210, 25, "Find:");
  replace_find->align(FL_ALIGN_LEFT);

  replace_with = new Fl_Input(80, 40, 210, 25, "Replace:");
  replace_with->align(FL_ALIGN_LEFT);

  replace_all = new Fl_Button(10, 70, 90, 25, "Replace All");
  replace_all->callback((Fl_Callback *)replall_cb, this);

  replace_next = new Fl_Return_Button(105, 70, 120, 25, "Replace Next");
  replace_next->callback((Fl_Callback *)replace2_cb, this);

  replace_cancel = new Fl_Button(230, 70, 60, 25, "Cancel");
  replace_cancel->callback((Fl_Callback *)replcan_cb, this);
  replace_dlg->end();
  replace_dlg->set_non_modal();

  editor = 0;
  *search = (char)0;
  loading = 0;
  changed = 0;
  begin();
  textbuf = 0;
  stylebuf = 0;
  textbuf = new Fl_Text_Buffer;
  style_init(this);

  editor = new Fl_Text_Editor(0, 30, 660, 370);
  editor->buffer(textbuf);
  editor->highlight_data(stylebuf, styletable,
                            sizeof(styletable) / sizeof(styletable[0]),
                            'A', style_unfinished_cb, 0);
  editor->textfont(FL_SCREEN);
  end();
  resizable(editor);
  callback((Fl_Callback *)close_cb, this);
  textbuf->add_modify_callback(style_update, this);
  textbuf->add_modify_callback(changed_cb, this);
  textbuf->call_modify_callbacks();
  filename[0] = '\0';
  if (file)
    load_file(file, -1, this);
}


CsoundEditorWindow::~CsoundEditorWindow()
{
  //FIXME: Do all cleaning up.
  delete editor;
  delete textbuf;
  delete replace_dlg;
}

int CsoundEditorWindow::openFile(const char *filename_)
{
  int ret = check_save(this);
  if (ret == 0) return 0;
  if (filename_ != NULL) load_file(filename_, -1, this);
  return 1;
}
