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
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <FL/Fl_File_Chooser.H>
#include <FL/x.H>
#include <Python.h>
#include <algorithm>
#include "ScoreGeneratorVstFltk.hpp"
#include "System.hpp"
#include <boost/tokenizer.hpp>

static std::string about = "SCORE GENERATOR VST\n"
"Version 1.0\n"
"\n"
"This VST plugin permits one to use Python for algorithmic composition \n"
"within VST hosts such as Cubase SX. \n"
"\n"
"ScoreGeneratorVst is free software; you can redistribute it \n"
"and/or modify it under the terms of the GNU Lesser General Public \n"
"License as published by the Free Software Foundation; either \n"
"version 2.1 of the License, or (at your option) any later version. \n"
"\n"
"ScoreGeneratorVst is distributed in the hope that they will be useful, \n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of \n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \n"
"GNU Lesser General Public License for more details. \n"
"\n"
"You should have received a copy of the GNU Lesser General Public \n"
"License along with this software; if not, write to the Free Software\n"
"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA\n"
"02111-1307 USA\n"
"\n";

static ScoreGeneratorVstFltk *oneWaiter = 0;

static const char *removeCarriageReturns(std::string &buffer)
{
  size_t position = 0;
  while((position = buffer.find("\r")) != std::string::npos)
    {
      buffer.erase(position, 1);
    }
  return buffer.c_str();
}

#if defined(_WIN32)

WaitCursor::WaitCursor()
{
  cursor = (void *)SetCursor(LoadCursor(0, IDC_WAIT));
}

WaitCursor::~WaitCursor()
{
  SetCursor((HCURSOR) cursor);
}

#else

WaitCursor::WaitCursor() : cursor(0)
{
}

WaitCursor::~WaitCursor()
{
}

#endif

Fl_Preferences ScoreGeneratorVstFltk::preferences(Fl_Preferences::USER, "gogins@pipeline.com", "scoreGeneratorVst");

ScoreGeneratorVstFltk::ScoreGeneratorVstFltk(AudioEffect *audioEffect) :
  scoreGeneratorVstUi(0),
  scoreGeneratorVst((scoreGeneratorVst *)audioEffect),
  useCount(0)
{
  if (oneWaiter == 0)
    {
      oneWaiter = this;
    }
  scoreGeneratorVst->setEditor(this);
}

ScoreGeneratorVstFltk::~ScoreGeneratorVstFltk(void)
{
  if (oneWaiter == this)
    {
      oneWaiter = 0;
    }
}

void ScoreGeneratorVstFltk::updateCaption()
{
  std::string caption;
  caption = "[ C S O U N D   V S T ] ";
  if(scoreGeneratorVst->getIsPython())
    {
      caption.append(scoreGeneratorVst->Shell::getFilename());
    }
  else if(!scoreGeneratorVst->getIsVst())
    {
      caption.append(scoreGeneratorVst->getCppSound()->CsoundFile::getFilename());
    }
  Fl::lock();
  scoreGeneratorVstUi->label(caption.c_str());
  Fl::unlock();
}

void ScoreGeneratorVstFltk::updateModel()
{
  if(scoreGeneratorVstUi)
    {
      Fl::lock();
      csound::System::message("BEGAN ScoreGeneratorVstFltk::updateModel...\n");
      scoreGeneratorVst->setScript(scriptTextBuffer->text());
      csound::System::message("ENDED ScoreGeneratorVstFltk::updateModel.\n");
      Fl::unlock();
    }
}

long ScoreGeneratorVstFltk::getRect(ERect **erect)
{
  static ERect r = {0, 0, kEditorHeight, kEditorWidth};
  *erect = &r;
  return true;
}

long ScoreGeneratorVstFltk::open(void *parentWindow)
{
  if (oneWaiter == this)
    {
      Fl::lock();
    }
  systemWindow = parentWindow;
  this->scoreGeneratorVstUi = make_window(this);
  this->mainTabs = ::mainTabs;
  this->runtimeMessagesGroup = ::runtimeMessagesGroup;
  this->runtimeMessagesBrowser = ::runtimeMessagesBrowser;
  this->settingsEditSoundfileInput = ::settingsEditSoundfileInput;
  this->scriptTextEdit = ::scriptTextEdit;
  this->scriptTextEdit->buffer(this->scriptTextBuffer);
  this->aboutTextDisplay = ::aboutTextDisplay;
  this->aboutTextDisplay->buffer(this->aboutTextBuffer);
  this->scriptGroup = ::scriptGroup;
  this->mainTabs->value(scriptGroup);
  this->scoreGeneratorVstUi->show();
  this->aboutTextBuffer->text(removeCarriageReturns(about));
  update();
  return true;
}

void ScoreGeneratorVstFltk::close()
{
  this->scoreGeneratorVstUi->hide();
}

void ScoreGeneratorVstFltk::idle()
{
  // Process events for the FLTK GUI.
  // Only one instance of ScoreGeneratorVstFltk may call Fl::wait().
  if (oneWaiter == this)
    {
      Fl::lock();
      Fl::wait(0.0);
      Fl::unlock();
    }
  // If the VST host has indicated
  // it needs the GUI updated, do it.
  if(updateFlag)
    {
      updateFlag = 0;
      update();
    }
  if(scoreGeneratorVstUi)
    {
      if(this->runtimeMessagesBrowser)
        {
          while(!messages.empty())
            {
              Fl::lock();
              Fl::flush();
              this->runtimeMessagesBrowser->add(messages.front().c_str());
              this->runtimeMessagesBrowser->bottomline(this->runtimeMessagesBrowser->size());
              Fl::unlock();
              messages.pop_front();
            }
        }
    }
}

// Updates the widgets from scoreGeneratorVst.

void ScoreGeneratorVstFltk::update()
{
  if(scoreGeneratorVstUi)
    {
      updateCaption();
      Fl::lock();
      csound::System::message("BEGAN ScoreGeneratorVstFltk::update...\n");
      std::string buffer;
      buffer = scoreGeneratorVst->getScript();
      this->scriptTextBuffer->text(removeCarriageReturns(buffer));
      csound::System::message("ENDED ScoreGeneratorVstFltk::update.\n");
      Fl::unlock();
    }
}

void ScoreGeneratorVstFltk::postUpdate()
{
  updateFlag = 1;
}

void ScoreGeneratorVstFltk::messageCallback(CSOUND *csound, int attribute, const char *format, va_list valist)
{
  if(!csound)
    {
      return;
    }
  scoreGeneratorVst *scoreGeneratorVst = (scoreGeneratorVst *)csoundGetHostData(csound);
  if(!scoreGeneratorVst)
    {
      return;
    }
  ScoreGeneratorVstFltk *ScoreGeneratorVstFltk = (ScoreGeneratorVstFltk *)scoreGeneratorVst->getEditor();
  if(!ScoreGeneratorVstFltk)
    {
      return;
    }
  if(!ScoreGeneratorVstFltk->scoreGeneratorVstUi)
    {
      return;
    }
  if(!ScoreGeneratorVstFltk->runtimeMessagesBrowser)
    {
      return;
    }
  char buffer[0x1002];
  vsnprintf(buffer, 0x1000, format, valist);
  ScoreGeneratorVstFltk->messagebuffer.append(buffer);
  if (ScoreGeneratorVstFltk->messagebuffer.find("\n") != std::string::npos)
    {
      typedef boost::char_separator<char> charsep;
      boost::tokenizer<charsep> tokens(ScoreGeneratorVstFltk->messagebuffer, charsep("\n"));
      for(boost::tokenizer<charsep>::iterator it = tokens.begin(); it != tokens.end(); ++it)
        {
          if(ScoreGeneratorVstFltk->scoreGeneratorVst->getIsVst())
            {
              ScoreGeneratorVstFltk->messages.push_back(*it);
            }
          else
            {
              Fl::lock();
              Fl::flush();
              ScoreGeneratorVstFltk->runtimeMessagesBrowser->add(it->c_str());
              ScoreGeneratorVstFltk->runtimeMessagesBrowser->bottomline(ScoreGeneratorVstFltk->runtimeMessagesBrowser->size());
              Fl::unlock();
            }
        }
      ScoreGeneratorVstFltk->messagebuffer.clear();
    }
}

void ScoreGeneratorVstFltk::onNew(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  csound::System::message("BEGAN ScoreGeneratorVstFltk::onNew...\n");
  scoreGeneratorVst->clear();
  update();
  csound::System::message("ENDED ScoreGeneratorVstFltk::onNew.\n");
}

void ScoreGeneratorVstFltk::onNewVersion(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  csound::System::message("BEGAN ScoreGeneratorVstFltk::onNewVersion...\n");
  std::string filename_;
  scoreGeneratorVst->save(scoreGeneratorVst->getFilename());
  csound::System::message("Saved old version: '%s'\n", scoreGeneratorVst->getFilename().c_str());
  filename_ = scoreGeneratorVst->generateFilename();
  scoreGeneratorVst->save(filename_);
  scoreGeneratorVst->setFilename(filename_);
  csound::System::message("Saved new version: '%s'\n", scoreGeneratorVst->getFilename().c_str());
  updateCaption();
  csound::System::message("ENDED ScoreGeneratorVstFltk::onNewVersion.\n");
}


void ScoreGeneratorVstFltk::onOpen(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  runtimeMessagesBrowser->clear();
  csound::System::message("BEGAN ScoreGeneratorVstFltk::onOpen...\n");
  char *filename_ = 0;
  std::string oldFilename = scoreGeneratorVst->getFilename();
  if(oldFilename.length() <= 0)
    {
      oldFilename = "Default.py";
    }
  filename_ = fl_file_chooser("Open a file...", "*.py|*.csd|*.orc|*.sco|*.mid", oldFilename.c_str(), false);
  if(filename_)
    {
      scoreGeneratorVst->openFile(filename_);
    }
  csound::System::message("ENDED ScoreGeneratorVstFltk::onOpen.\n");
}

void ScoreGeneratorVstFltk::onSave(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  csound::System::message("BEGAN ScoreGeneratorVstFltk::onSave...\n");
  updateModel();
  scoreGeneratorVst->Shell::save(scoreGeneratorVst->getFilename());
  csound::System::message("Saved file as: '%s'.\n", scoreGeneratorVst->getFilename().c_str());
  csound::System::message("ENDED ScoreGeneratorVstFltk::onSave.\n");
}

void ScoreGeneratorVstFltk::onSaveAs(Fl_Button*, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  csound::System::message("BEGAN ScoreGeneratorVstFltk::onSaveAs...\n");
  updateModel();
  char *filename_ = 0;
  std::string oldFilename = scoreGeneratorVst->getFilename();
  if(oldFilename.length() <= 0)
    {
      oldFilename = "Default.py";
    }
  filename_ = fl_file_chooser("Save as...", "*.py", oldFilename.c_str(), false);
  if(filename_)
    {
      WaitCursor wait;
      runtimeMessagesBrowser->clear();
      csound::System::message("BEGAN ScoreGeneratorVstFltk::onSaveAs...\n");
      scoreGeneratorVst->save(filename_);
      scoreGeneratorVst->setFilename(filename_);
      csound::System::message("Saved file as: '%s'.\n", scoreGeneratorVst->getFilename().c_str());
      update();
    }
  csound::System::message("ENDED ScoreGeneratorVstFltk::onSaveAs.\n");
}

void ScoreGeneratorVstFltk::onGenerate(Fl_Button* fl_button, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  runtimeMessagesBrowser->clear();
  csound::System::message("BEGAN ScoreGeneratorVstFltk::onPerform...\n");
  updateModel();
  mainTabs->value(runtimeMessagesGroup);
  scoreGeneratorVst->perform();
  csound::System::message("ENDED ScoreGeneratorVstFltk::onPerform.\n");
}


void onNew(Fl_Button* fl_button, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  ScoreGeneratorVstFltk->onNew(fl_button, ScoreGeneratorVstFltk);
}

void onNewVersion(Fl_Button* fl_button, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  ScoreGeneratorVstFltk->onNewVersion(fl_button, ScoreGeneratorVstFltk);
}

void onOpen(Fl_Button* fl_button, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  ScoreGeneratorVstFltk->onOpen(fl_button, ScoreGeneratorVstFltk);
}

void onSave(Fl_Button* fl_button, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  ScoreGeneratorVstFltk->onSave(fl_button, ScoreGeneratorVstFltk);
}

void onSaveAs(Fl_Button* fl_button, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  ScoreGeneratorVstFltk->onSaveAs(fl_button, ScoreGeneratorVstFltk);
}

void onGenerate(Fl_Button* fl_button, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  ScoreGeneratorVstFltk->onGenerate(fl_button, ScoreGeneratorVstFltk);
}

void onStop(Fl_Button* fl_button, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  ScoreGeneratorVstFltk->onStop(fl_button, ScoreGeneratorVstFltk);
}

void onEdit(Fl_Button* fl_button, ScoreGeneratorVstFltk* ScoreGeneratorVstFltk)
{
  ScoreGeneratorVstFltk->onEdit(fl_button, ScoreGeneratorVstFltk);
}

