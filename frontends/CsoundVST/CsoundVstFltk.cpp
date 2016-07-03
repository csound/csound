/**
 * C S O U N D   V S T
 *
 * A VST plugin version of Csound.
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
#include <algorithm>
#include "CsoundVstFltk.hpp"
#include "CsoundVST.hpp"
#include <boost/tokenizer.hpp>

static std::string about = "CSOUND AND CSOUND VST\n"
#if defined(BETA)
  "Version "
  CS_PACKAGE_VERSION
  " beta "
  CS_PACKAGE_DATE
  "\n"
#else
  "Version "
  CS_PACKAGE_VERSION
  " "
  CS_PACKAGE_DATE
  "\n"
#endif
  "\n"
  "A user-programmable and user-extensible sound processing language \n"
  "and software synthesizer. \n"
  "\n"
  "Csound is copyright (c) 1991 Barry Vercoe, John ffitch.\n"
  "CsoundVST is copyright (c) 2001 by Michael Gogins.\n"
  "VST PlugIn Interface Technology by Steinberg Soft- und Hardware GmbH\n"
  "\n"
  "Csound and CsoundVST are free software; you can redistribute them \n"
  "and/or modify them under the terms of the GNU Lesser General Public \n"
  "License as published by the Free Software Foundation; either \n"
  "version 2.1 of the License, or (at your option) any later version. \n"
  "\n"
  "Csound and CsoundVST are distributed in the hope that they will be useful, \n"
  "but WITHOUT ANY WARRANTY; without even the implied warranty of \n"
  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \n"
  "GNU Lesser General Public License for more details. \n"
  "\n"
  "You should have received a copy of the GNU Lesser General Public \n"
  "License along with this software; if not, write to the Free Software\n"
  "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA\n"
  "02111-1307 USA\n"
  "\n"
  "INSTALLATION\n"
  "\n"
  "See the Csound documentation for more information, \n"
  "or go to http://sourceforge.net/projects/csound.\n"
  "\n"
  "HELP\n"
  "\n"
  "This version of Csound is programmable in Python, and scores can be \n"
  "generated in Python. See the Csound documentation for more information. \n"
  "\n"
  "CONTRIBUTORS\n"
  "\n"
  "Csound contains contributions from musicians, scientists, and programmers \n"
  "from around the world. They include (but are not limited to): \n"
  "\n"
  "Allan Lee  \n"
  "Bill Gardner \n"
  "Bill Verplank \n"
  "Dan Ellis \n"
  "David Macintyre \n"
  "Eli Breder \n"
  "Gabriel Maldonado \n"
  "Greg Sullivan \n"
  "Hans Mikelson \n"
  "Istvan Varga \n"
  "Jean Piché \n"
  "John ffitch \n"
  "John Ramsdell \n"
  "Marc Resibois \n"
  "Mark Dolson \n"
  "Matt Ingalls \n"
  "Max Mathews \n"
  "Michael Casey \n"
  "Michael Clark \n"
  "Michael Gogins \n"
  "Mike Berry \n"
  "Paris Smaragdis \n"
  "Perry Cook \n"
  "Peter Neubäcker \n"
  "Peter Nix \n"
  "Rasmus Ekman \n"
  "Richard Dobson \n"
  "Richard Karpen \n"
  "Rob Shaw \n"
  "Robin Whittle \n"
  "Sean Costello \n"
  "Steven Yi \n"
  "Tom Erbe \n"
  "Victor Lazzarini\n"
  "Ville Pulkki\n";

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

Fl_Preferences CsoundVstFltk::preferences(Fl_Preferences::USER, "gogins@pipeline.com", "CsoundVST");

CsoundVstFltk::CsoundVstFltk(AudioEffect *audioEffect) :
  csoundVST((CsoundVST *)audioEffect),
  windowHandle (0),
  csoundVstUi (0),
  useCount (0),
  updateFlag (0),
  mainTabs (0),
  commandInput (0),
  runtimeMessagesBrowser (0),
  orchestraTextEdit (0),
  orchestraTextBuffer (0),
  settingsEditSoundfileInput (0),
  settingsVstPluginModeEffect (0),
  settingsVstPluginModeInstrument (0),
  aboutTextBuffer (0),
  aboutTextDisplay (0),
  orchestraGroup (0),
  scoreGroup (0)
{
  this->csoundVstUi = make_window(this);
  this->mainTabs = ::mainTabs;
  this->commandInput = ::commandInput;
  this->runtimeMessagesBrowser = ::runtimeMessagesBrowser;
  this->orchestraTextEdit = ::orchestraTextEdit;
  this->orchestraTextBuffer = new Fl_Text_Buffer();
  this->scoreTextEdit = ::scoreTextEdit;
  this->scoreTextBuffer = new Fl_Text_Buffer();
  this->settingsEditSoundfileInput = ::settingsEditSoundfileInput;
  this->settingsVstPluginModeEffect = ::settingsVstPluginModeEffect;
  this->settingsVstPluginModeInstrument = ::settingsVstPluginModeInstrument;
  this->aboutTextBuffer = new Fl_Text_Buffer();
  this->aboutTextDisplay = ::aboutTextDisplay;
  this->orchestraGroup = ::orchestraGroup;
  this->scoreGroup = ::scoreGroup;
  this->orchestraTextEdit->buffer(this->orchestraTextBuffer);
  this->scoreTextEdit->buffer(this->scoreTextBuffer);
  this->aboutTextDisplay->buffer(this->aboutTextBuffer);
  csoundVST->setEditor(this);
}

CsoundVstFltk::~CsoundVstFltk(void)
{
}

void CsoundVstFltk::updateCaption()
{
  std::string caption;
  caption = "[ C S O U N D   V S T ] ";
  if(!csoundVST->getIsVst())
    {
      std::string filename = csoundVST->getFilename();
      caption += filename;
    }
  csoundVST->fltklock();
  csoundVstUi->label(caption.c_str());
  csoundVST->fltkunlock();
}

void CsoundVstFltk::updateModel()
{
  if(csoundVstUi)
    {
      csoundVST->fltklock();
      //csoundVST->Message("BEGAN CsoundVstFltk::updateModel...\n");
      csoundVST->setCommand(commandInput->value());
      csoundVST->setOrchestra(orchestraTextBuffer->text());
      csoundVST->setScore(scoreTextBuffer->text());
      //csoundVST->Message("ENDED CsoundVstFltk::updateModel.\n");
      csoundVST->fltkunlock();
    }
}

bool CsoundVstFltk::getRect(ERect **erect)
{
  static ERect r = {0, 0, kEditorHeight, kEditorWidth};
  *erect = &r;
  return true;
}

bool CsoundVstFltk::open(void *parentWindow)
{
  //csoundVST->Message("BEGAN CsoundVstFltk::open()...\n");
  csoundVST->Message("csoundVST = 0x%x\n", csoundVST);
  this->csoundVstUi->show();
  csoundVST->fltklock();
  systemWindow = parentWindow;
  //    Read user preferences.
  char buffer[0x500];
  int number = 0;
  preferences.get("SoundfileOpen", (char *)buffer, "mplayer.exe", 0x500);
  this->settingsEditSoundfileInput->value(buffer);
  preferences.get("IsSynth", number, 0);
  csoundVST->setIsSynth(number);
  this->mainTabs->value(settingsGroup);
  csoundVST->SetHostData(csoundVST);
  if(csoundVST->getIsVst())
    {
#if defined(WIN32)
      SetParent((HWND) fl_xid(this->csoundVstUi), (HWND) parentWindow);
#endif
      this->csoundVstUi->position(0, 0);
    }
  this->aboutTextBuffer->text(removeCarriageReturns(about));
  csoundVST->SetMessageCallback(CsoundVstFltk::messageCallback);
  //csoundVST->Message("ENDED CsoundVstFltk::open().\n");
  csoundVST->fltkunlock();
  update();
  return true;
}

void CsoundVstFltk::close()
{
  this->csoundVstUi->hide();
}

void CsoundVstFltk::idle()
{
  // Process events for the FLTK GUI.
  // Only one instance of CsoundVstFltk may call Fl::wait().
  csoundVST->fltkwait();
  // If the VST host has indicated
  // it needs the GUI updated, do it.
  if(updateFlag)
    {
      updateFlag = 0;
      update();
    }
  if(csoundVstUi)
    {
      if(this->runtimeMessagesBrowser)
        {
          while(!messages.empty())
            {
              csoundVST->fltklock();
              //csoundVST->fltkflush();
              this->runtimeMessagesBrowser->add(messages.front().c_str());
              this->runtimeMessagesBrowser->bottomline(this->runtimeMessagesBrowser->size());
              csoundVST->fltkunlock();
              messages.pop_front();
            }
        }
    }
}

// Updates the widgets from CsoundVST.

void CsoundVstFltk::update()
{
  //csoundVST->Message("BEGAN CsoundVstFltk::update...\n");
  if(csoundVstUi)
    {
      updateCaption();
      csoundVST->fltklock();
      std::string buffer;
      this->settingsVstPluginModeEffect->value(!csoundVST->getIsSynth());
      this->settingsVstPluginModeInstrument->value(csoundVST->getIsSynth());
      buffer = csoundVST->getCommand();
      this->commandInput->value(removeCarriageReturns(buffer));
      buffer = csoundVST->getOrchestra();
      this->orchestraTextBuffer->text(removeCarriageReturns(buffer));
      buffer = csoundVST->getScore();
      this->scoreTextBuffer->text(removeCarriageReturns(buffer));
      csoundVST->fltkunlock();
    }
  //csoundVST->Message("BEGAN CsoundVstFltk::update.\n");
}

void CsoundVstFltk::postUpdate()
{
  updateFlag = 1;
}

void CsoundVstFltk::messageCallback(CSOUND *csound, int attribute, const char *format, va_list valist)
{
  if(!csound)
    {
      return;
    }
  CsoundVST *csoundVST = (CsoundVST *)csoundGetHostData(csound);
  if(!csoundVST)
    {
      return;
    }
  CsoundVstFltk *csoundVstFltk = (CsoundVstFltk *)csoundVST->getEditor();
  if(!csoundVstFltk)
    {
      return;
    }
  if(!csoundVstFltk->csoundVstUi)
    {
      return;
    }
  if(!csoundVstFltk->runtimeMessagesBrowser)
    {
      return;
    }
  char buffer[0x1002];
  vsnprintf(buffer, 0x1000, format, valist);
  csoundVstFltk->messagebuffer.append(buffer);
  if (csoundVstFltk->messagebuffer.find("\n") != std::string::npos)
    {
      typedef boost::char_separator<char> charsep;
      boost::tokenizer<charsep> tokens(csoundVstFltk->messagebuffer, charsep("\n"));
      for(boost::tokenizer<charsep>::iterator it = tokens.begin(); it != tokens.end(); ++it)
        {
          if(csoundVstFltk->csoundVST->getIsVst())
            {
              csoundVstFltk->messages.push_back(*it);
            }
          else
            {
              csoundVstFltk->csoundVST->fltklock();
              //csoundVstFltk->csoundVST->fltkflush();
              csoundVstFltk->runtimeMessagesBrowser->add(it->c_str());
              csoundVstFltk->runtimeMessagesBrowser->bottomline(csoundVstFltk->runtimeMessagesBrowser->size());
              csoundVstFltk->csoundVST->fltkunlock();
            }
        }
      csoundVstFltk->messagebuffer.clear();
    }
}

void CsoundVstFltk::onNew(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
  //csoundVST->Message("BEGAN CsoundVstFltk::onNew...\n");
  csoundVST->removeAll();
  update();
  //csoundVST->Message("ENDED CsoundVstFltk::onNew.\n");
}

void CsoundVstFltk::onNewVersion(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
  //csoundVST->Message("BEGAN CsoundVstFltk::onNewVersion...\n");
  std::string filename_;
  csoundVST->save(csoundVST->getFilename());
  csoundVST->Message("Saved old version: '%s'\n", csoundVST->getFilename().c_str());
  filename_ = csoundVST->generateFilename();
  csoundVST->save(filename_);
  csoundVST->setFilename(filename_);
  csoundVST->Message("Saved new version: '%s'\n", csoundVST->getFilename().c_str());
  updateCaption();
  //csoundVST->Message("ENDED CsoundVstFltk::onNewVersion.\n");
}

void CsoundVstFltk::onImport(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
  runtimeMessagesBrowser->clear();
  //csoundVST->Message("BEGAN CsoundVstFltk::onImport...\n");
  char *filename_ = 0;
  std::string oldFilename = csoundVST->getFilename();
  if(oldFilename.length() <= 0)
    {
      oldFilename = "Default.csd";
    }
  filename_ = fl_file_chooser("Import a file...", "*.csd|*.orc|*.sco|*.mid", oldFilename.c_str(), false);
  if(filename_)
    {
      WaitCursor wait;
      csoundVST->importFile(filename_);
      csoundVST->setFilename(filename_);
      csoundVST->Message("Imported file: '%s'.\n", csoundVST->getFilename().c_str());
      csoundVST->bank[csoundVST->getProgram()].text = csoundVST->getText();
      update();
    }
  //csoundVST->Message("ENDED CsoundVstFltk::onImport.\n");
}

void CsoundVstFltk::onOpen(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
  runtimeMessagesBrowser->clear();
  //csoundVST->Message("BEGAN CsoundVstFltk::onOpen...\n");
  char *filename_ = 0;
  std::string oldFilename = csoundVST->getFilename();
  if(oldFilename.length() <= 0)
    {
      oldFilename = "Default.csd";
    }
  filename_ = fl_file_chooser("Open a file...", "*.csd|*.orc|*.sco|*.mid|*.py", oldFilename.c_str(), false);
  if(filename_)
    {
      csoundVST->openFile(filename_);
      csoundVST->Message("Opened file: '%s'.\n", csoundVST->getFilename().c_str());
    }
  //csoundVST->Message("ENDED CsoundVstFltk::onOpen.\n");
}

void CsoundVstFltk::onSave(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
  //csoundVST->Message("BEGAN CsoundVstFltk::onSave...\n");
  updateModel();
  csoundVST->save(csoundVST->getFilename());
  csoundVST->Message("Saved file as: '%s'.\n", csoundVST->getFilename().c_str());
  //csoundVST->Message("ENDED CsoundVstFltk::onSave.\n");
}

void CsoundVstFltk::onSaveAs(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
  //csoundVST->Message("BEGAN CsoundVstFltk::onSaveAs...\n");
  updateModel();
  char *filename_ = 0;
  std::string oldFilename = csoundVST->getFilename();
  if(oldFilename.length() <= 0)
    {
      oldFilename = "Default.csd";
    }
  filename_ = fl_file_chooser("Save as...", "*.csd|*.orc|*.sco|*.mid", oldFilename.c_str(), false);
  if(filename_)
    {
      WaitCursor wait;
      runtimeMessagesBrowser->clear();
      csoundVST->Message("BEGAN CsoundVstFltk::onSaveAs...\n");
      csoundVST->save(filename_);
      csoundVST->setFilename(filename_);
      csoundVST->Message("Saved file as: '%s'.\n", csoundVST->getFilename().c_str());
      update();
    }
  //csoundVST->Message("ENDED CsoundVstFltk::onSaveAs.\n");
}

void CsoundVstFltk::onPerform(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  runtimeMessagesBrowser->clear();
  //csoundVST->Message("BEGAN CsoundVstFltk::onPerform...\n");
  updateModel();
  mainTabs->value(settingsGroup);
  csoundVST->performance();
  //csoundVST->Message("ENDED CsoundVstFltk::onPerform.\n");
}

void CsoundVstFltk::onStop(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
  //csoundVST->Message("BEGAN CsoundVstFltk::onStop...\n");
  csoundVST->stop();
  //csoundVST->Message("ENDED CsoundVstFltk::onStop.\n");
}

void CsoundVstFltk::onEdit(Fl_Button*, CsoundVstFltk*)
{
  //csoundVST->Message("BEGAN CsoundVstFltk::onEdit...\n");
  csoundVST->stop();
  updateModel();
  char buffer[0x500];
  preferences.get("SoundfileOpen", (char *)buffer, "mplayer2.exe", 0x500);
  std::string command = buffer;
  command.append(" ");
  std::string soundfileName;
  soundfileName = csoundVST->getOutputSoundfileName();
  fl_filename_absolute(buffer, soundfileName.c_str());
  command.append(buffer);
  csoundVST->Message("Executing command: '%s'\n", command.c_str());
  std::vector<const char *> argv;
  typedef boost::char_separator<char> charsep;
  boost::tokenizer<charsep> tokens(command, charsep(" ,\n\r\t"));
  for(boost::tokenizer<charsep>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
    argv.push_back(it->c_str());
  }
  argv.push_back(0);
  csoundRunCommand(&argv.front(), true);
  //csoundVST->Message("ENDED CsoundVstFltk::onEdit.\n");
}

void CsoundVstFltk::onSettingsVstPluginMode(Fl_Check_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  //csoundVST->Message("BEGAN CsoundVstFltk::onSettingsVstPluginMode...\n");
  csoundVST->setIsSynth(false);
  //csoundVST->Message("ENDED CsoundVstFltk::onSettingsVstPluginMode.\n");
}

void CsoundVstFltk::onSettingsVstInstrumentMode(Fl_Check_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  //csoundVST->Message("BEGAN CsoundVstFltk::onSettingsVstInstrumentMode...\n");
  csoundVST->setIsSynth(true);
  //csoundVST->Message("ENDED CsoundVstFltk::onSettingsVstInstrumentMode.\n");
}

void CsoundVstFltk::onSettingsApply(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  //csoundVST->Message("BEGAN CsoundVstFltk::onSettingsApply...\n");
  preferences.set("SoundfileOpen", this->settingsEditSoundfileInput->value());
  preferences.set("IsSynth", csoundVST->getIsSynth());
  //csoundVST->Message("ENDED CsoundVstFltk::onSettingsApply.\n");
}

void onNew(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onNew(fl_button, csoundVstFltk);
}

void onNewVersion(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onNewVersion(fl_button, csoundVstFltk);
}

void onOpen(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onOpen(fl_button, csoundVstFltk);
}

void onImport(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onImport(fl_button, csoundVstFltk);
}

void onSave(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onSave(fl_button, csoundVstFltk);
}

void onSaveAs(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onSaveAs(fl_button, csoundVstFltk);
}

void onPerform(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onPerform(fl_button, csoundVstFltk);
}

void onStop(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onStop(fl_button, csoundVstFltk);
}

void onEdit(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onEdit(fl_button, csoundVstFltk);
}

void onSettingsVstPluginMode(Fl_Check_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onSettingsVstPluginMode(fl_button, csoundVstFltk);
}

void onSettingsVstInstrumentMode(Fl_Check_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onSettingsVstInstrumentMode(fl_button, csoundVstFltk);
}

void onSettingsApply(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
  csoundVstFltk->onSettingsApply(fl_button, csoundVstFltk);
}

