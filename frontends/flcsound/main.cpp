/*
 * flCsound main application
 *
 * Copyright (c) 2003 by John D. Ramsdell. All rights reserved.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <iostream>
#include <string>
#include <vector>
#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Enumerations.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Box.H>
#include "curve.hpp"
#include "synthesizer.hpp"
#include "canvas.hpp"
#include "plots.hpp"
#if defined HAVE_CONFIG_H
#  include "csound/config.h"
#endif
#if defined HAVE_XCREATEBITMAPFROMDATA
#  include "flcsound.xbm"
#endif

// Transcript size in character units
const int transcript_width = 42;
const int transcript_height = 50;

// A separated thread is used for each Csound synthesizer run when
// linked with an FLTK library that was compiled with multithreading
// support enabled.

#if defined HAVE_FL__LOCK
#  if defined HAVE_PTHREAD_H
#    include <errno.h>
#    include <pthread.h>
#    define USE_THREADS
#    define thread_exit() return 0

typedef pthread_t flthread_t;
typedef void *result_t;

namespace {
  int thread_create(flthread_t *t, void *(*f)(void *), void *p)
  {
    int i;
    pthread_attr_t a[1];
    if (pthread_attr_init(a))
      return EINVAL;
    if (pthread_attr_setdetachstate(a, PTHREAD_CREATE_DETACHED))
      return EINVAL;
    i = pthread_create(t, a, f, p);
    if (pthread_attr_destroy(a))
      return EINVAL;
    return i;
  }
};
#  elif defined WIN32
#    include <errno.h>
#    include <windows.h>
#    include <process.h>
#    define USE_THREADS
#    define thread_exit() return

typedef unsigned long flthread_t;
#define result_t void __cdecl

namespace {
  int thread_create(flthread_t *t, void (__cdecl *f)(void *), void *p)
  {
    *t = _beginthread(f, 0, p);
    if (*t != (flthread_t)-1)
      return 0;
    else
      return errno;
  }
};
#  endif
#endif

#if !defined USE_THREADS
#define thread_exit() return
typedef void result_t;
#endif

namespace {
  inline void lock()
  {
#if defined USE_THREADS
    Fl::lock();
#endif
  }

  inline void unlock()
  {
#if defined USE_THREADS
    Fl::unlock();
#endif
  }

  inline void awake()
  {
#if defined USE_THREADS
    Fl::awake();
#endif
  }
};

class Transcript: public Fl_Text_Display
{
public:
  Transcript(int X, int Y, int W, int H, const char* l)
    : Fl_Text_Display(X, Y, W, H, l) {
  }

  static int transcript_copy(int, Transcript *e) {
    if (!e->buffer()->selected()) return 1;
    const char *copy = e->buffer()->selection_text();
    if (*copy) Fl::copy(copy, strlen(copy), 1);
    free((void*)copy);
    e->show_insert_position();
    return 1;
  }

  static int transcript_select_all(int, Transcript *e) {
    e->buffer()->select(0, e->buffer()->length());
    return 1;
  }
};

class Main
{
public:
  Main(char *);
  void show(int argc, char **argv);
  void run(Fl_Widget *);
private:
  const char *m_file;
  bool m_running;
  bool m_abort;
  Synthesizer m_synth;
  Fl_Double_Window *m_root;
  Fl_Tabs *m_tabs;
  Transcript *m_transcript;
  Fl_Text_Buffer m_transcript_buffer;
  Plots *m_plots;
  Fl_Box *m_status;
  Fl_File_Chooser *m_file_chooser;
  static char *make_title(const char *);
  static void draw_graph(void *, Curve *);
  void draw_graph(Curve *);
  static bool s_stop;
  static bool s_exit;
  static int yield(void *);
  static result_t thread_callback(void *);
  void thread();
  static void run_callback(Fl_Widget *, void *);
  static void stop_callback(Fl_Widget *, void *);
  void stop(Fl_Widget *);
  static void exit_callback(Fl_Widget *, void *);
  void exit(Fl_Widget *);
  static void clear_callback(Fl_Widget *, void *);
  void clear(Fl_Widget *);
  static void open_callback(Fl_Widget *, void *);
  void open(Fl_Widget *);
  static void transcript_callback(Fl_Widget *, void *);
  void transcript(Fl_Widget *);
  static void copy_callback(Fl_Widget *, void *);
  static void select_all_callback(Fl_Widget *, void *);
  static void plots_callback(Fl_Widget *, void *);
  void plots(Fl_Widget *);
  static void about_callback(Fl_Widget *, void *);
  void about(Fl_Widget *);
  static void message_callback(void *data, const char *, va_list);
  static const int s_buf_size = 1 << 10;
  char m_buf[s_buf_size];
  int m_last;
  static inline bool is_terminated(const char *buf);
  void message(const char *, va_list);
  void message(const char *);
};

namespace {
  void set_icon(Fl_Window *window)
  {
#if defined HAVE_XCREATEBITMAPFROMDATA
    fl_open_display();
    Pixmap p = XCreateBitmapFromData(fl_display,
				     DefaultRootWindow(fl_display),
				     flcsound_bits,
				     flcsound_width,
				     flcsound_height);
    window->icon((char *)p);
#elif defined WIN32 && defined IDI_ICON
    window->icon((char *)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON)));
#endif
  }
};

Main::Main(char *file)
  : m_file(file), m_running(false), m_abort(false)
{
  int s = FL_NORMAL_SIZE * 2;
  int w = FL_NORMAL_SIZE * transcript_width;
  int h = FL_NORMAL_SIZE * transcript_height;
  m_root = new Fl_Double_Window(w, h + 3 * s, make_title(file));
  Fl_Menu_Bar *bar = new Fl_Menu_Bar(0, 0, w, s, "Menu Bar");
  Fl_Menu_Item menuitems[] = {
    { "&File", 0, 0, 0, FL_SUBMENU },
    { "&Open",  0, open_callback, this },
    { "&Quit", 0, exit_callback, this },
    { 0 },
    { "&Edit", 0, 0, 0, FL_SUBMENU },
    { "&Copy",  0, copy_callback, this },
    { "Select &All",  0, select_all_callback, this },
    { 0 },
    { "&Command", 0, 0, 0, FL_SUBMENU },
    { "&Run",  0, run_callback, this },
    { "&Stop", 0, stop_callback, this },
    { "&Clear", 0, clear_callback, this },
    { 0 },
    { "&Window", 0, 0, 0, FL_SUBMENU },
    { "&Transcript", 0, transcript_callback, this },
    { "&Plots", 0, plots_callback, this },
    { 0 },
    { "&Help", 0, 0, 0, FL_SUBMENU },
    { "&About",  0, about_callback, this },
    { 0 },
    { 0 },
  };
  bar->copy(menuitems);
  m_tabs = new Fl_Tabs(0, s, w, h + s, "Tabs");
  m_tabs->labeltype(FL_NO_LABEL);
  m_transcript = new Transcript(0, 2 * s, w, h, "Transcript");
  m_transcript->textfont(FL_SCREEN);
  m_transcript->buffer(m_transcript_buffer);
  m_transcript->end();
  m_plots = new Plots(0, 2 * s, w, h, "Plots");
  m_tabs->end();
  m_tabs->resizable(m_transcript);
  m_tabs->resizable(m_plots);
  m_status = new Fl_Box(0, 2 * s + h, w, s, "Ready");
  m_root->end();
  m_root->resizable(m_tabs);
  m_root->user_data(this);
  set_icon(m_root);
  m_file_chooser = new Fl_File_Chooser(file, "CSD Files (*.csd)",
				       Fl_File_Chooser::SINGLE,
				       "flCsound Choose File");
  m_file_chooser->preview(0);
  m_synth.set_draw_graph_callback(this, draw_graph);
  m_synth.set_yield_callback(this, yield);
  m_last = 0;
  m_synth.set_message_callback(this, message_callback);
  m_root->callback(exit_callback, this);
}

namespace {
  const int title_size = 80;
  char title_buf[title_size + 1];
};

char *Main::make_title(const char *file)
{
  if (file) {
    int n = snprintf(title_buf, title_size, "flCsound - %s", file);
    title_buf[n] = 0;
    return title_buf;
  }
  else
    return "flCsound";
}

void Main::draw_graph(void *data, Curve *curve)
{
  ((Main *)data)->draw_graph(curve);
}

void Main::draw_graph(Curve *curve)
{
  lock();
  m_plots->append(curve);
  m_status->label("Running and Displaying Plots");
  awake();
  unlock();
}

bool Main::s_stop = false;
bool Main::s_exit = false;

int Main::yield(void *data)
{
#if !defined USE_THREADS
  Fl::check();
#endif
  if (s_stop) {
    s_stop = false;
    return 0;
  }
  else
    return 1;
}

result_t Main::thread_callback(void *data)
{
  ((Main *)data)->thread();
  thread_exit();
}

void Main::thread()
{
  using namespace std;
  vector<string> args;
  args.push_back("csound");
  if (m_file)
    args.push_back(m_file);
  m_synth.perform(args);
  lock();
  m_abort = m_running = false;
  m_status->label("Ready");
  if (s_exit)
    delete m_root;
  unlock();
}

void Main::run_callback(Fl_Widget *w, void *data)
{
  ((Main *)data)->run(w);
}

void Main::run(Fl_Widget *w)
{
  if (!m_running) {
    m_plots->clear();
    m_status->label("Running");
    m_running = true;
    s_stop = false;
#if defined USE_THREADS
    flthread_t t;
    thread_create(&t, Main::thread_callback, this);
#else
    thread();
#endif
  }
}

void Main::stop_callback(Fl_Widget *w, void *data)
{
  ((Main *)data)->stop(w);
}

void Main::stop(Fl_Widget *w)
{
  s_stop = true;
}

void Main::exit_callback(Fl_Widget *w, void *data)
{
  ((Main *)data)->exit(w);
}

void Main::exit(Fl_Widget *w)
{
  if (m_abort)
    std::exit(0);
  else if (m_running)
    m_abort = s_stop = s_exit = true; // Let Csound stop before exiting
  else
    delete m_root;
}

void Main::clear_callback(Fl_Widget *w, void *data)
{
  ((Main *)data)->clear(w);
}

void Main::clear(Fl_Widget *w)
{
  m_transcript_buffer.text("");
}

void Main::open_callback(Fl_Widget *w, void *data)
{
  ((Main *)data)->open(w);
}

void Main::open(Fl_Widget *w)
{
  m_file_chooser->show();
  while (m_file_chooser->visible())
    Fl::wait();
  if (m_file_chooser->count() > 0) {
    m_file = m_file_chooser->value();
    m_root->label(make_title(m_file));
  }
}

void Main::transcript_callback(Fl_Widget *w, void *data)
{
  ((Main *)data)->transcript(w);
}

void Main::transcript(Fl_Widget *w)
{
  m_tabs->value(m_transcript);
}

void Main::copy_callback(Fl_Widget *w, void *data)
{
  Transcript::transcript_copy(0, ((Main *)data)->m_transcript);
}

void Main::select_all_callback(Fl_Widget *w, void *data)
{
  Transcript::transcript_select_all(0, ((Main *)data)->m_transcript);
}

void Main::plots_callback(Fl_Widget *w, void *data)
{
  ((Main *)data)->plots(w);
}

void Main::plots(Fl_Widget *w)
{
  m_tabs->value(m_plots);
}

void Main::about_callback(Fl_Widget *w, void *data)
{
  ((Main *)data)->about(w);
}

void Main::about(Fl_Widget *w)
{
  fl_message("This is flCsound built by %s.\n"
	     "The Csound API version used is %s.\n"
#if !defined USE_THREADS
	     "It was built without multithreading support.\n"
#endif
	     "Select a CSD file and run with it.",
	     PACKAGE_STRING,
	     Synthesizer::get_version().c_str());
}

void Main::message_callback(void *data, const char *format, va_list args)
{
  ((Main *)data)->message(format, args);
}

bool Main::is_terminated(const char *buf)
{
  for (;;) {
    char ch = *buf++;
    if (!ch)
      return false;
    if (ch == '\n')
      return true;
  }
}

// Accumulates text in buf until a newline character appears.
void Main::message(const char *format, va_list args)
{
  const char *msg = 0;
  int n = vsnprintf(m_buf + m_last, s_buf_size - m_last, format, args);
  if (n == 0)
    return;
  if (n < 0)
    msg = "Internal error: Csound print failed\n";
  else if (n >= s_buf_size - m_last)
    msg = "Internal error: Csound output too big\n";
  if (msg) {			// On internal error, print message.
    m_last = 0;
    message(msg);
  }
  else if (m_last + n > (s_buf_size >> 1) || is_terminated(m_buf + m_last)) {
    m_last = 0;			// When newline or buf too full
    message(m_buf);
  }
  else			// Save text until next time
    m_last += n;
}

void Main::message(const char *msg)
{
  lock();
  m_transcript->insert_position(INT_MAX);
  m_transcript->insert(msg);
  m_transcript->scroll(INT_MAX, 0);
  unlock();
}

void Main::show(int argc, char **argv)
{
  m_root->show(argc, argv);
}

int main(int argc, char **argv)
{
  char *file = 0;
  int i;
  Fl::args(argc, argv, i, Fl::arg);
  switch (argc - i) {
  case 1:
    file = argv[i];
    break;
  case 0:
    break;
  default:
    std::cerr << "Usage: " << argv[0] << " [ options ] [ file ]\n";
    std::cerr << "options:\n" << Fl::help << "\n";
    return 1;
  }
  lock();
  Main *main = new Main(file);
  main->show(argc, argv);
  if (file)
    main->run(0);
  return Fl::run();
}
