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
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <FL/Fl_File_Chooser.H>
#include <FL/x.H>
#include <Python.h>
#include <algorithm>
#include "CsoundVstFltk.hpp"
#include "CsoundVST.hpp"
#include "System.hpp"

static std::string about = 
"CSOUND VST \n"
"\n"
"A user-programmable and user-extensible sound processing language \n"
"and software synthesizer. \n" 
"\n"
"Csound is copyright (c) 1991 Barry Vercoe, John ffitch.\n"
"CsoundVST is copyright (c) 2001 by Michael Gogins.\n"
"VST PlugIn Interface Technology by Steinberg Soft- und Hardware GmbH\n"
"\n"
"CsoundVST and Csound are free software; you can redistribute them \n"
"and/or modify them under the terms of the GNU Lesser General Public \n"
"License as published by the Free Software Foundation; either \n"
"version 2.1 of the License, or (at your option) any later version. \n"
"\n"
"CsoundVST and Csound are distributed in the hope that they will be useful, \n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of \n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \n"
"GNU Lesser General Public License for more details. \n"
"\n"
"You should have received a copy of the GNU Lesser General Public \n"
"License along with this software; if not, write to the Free Software \n"
"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA \n"
"02111-1307 USA\n"
"\n"
"HELP \n"
"\n"
"For more information and documentation on Csound visit http://csounds.com. \n"
"This version of Csound is programmable in Python, and scores can be \n"
"generated in Python. See the Csound documentation for more information. \n"
"\n"
"CONTRIBUTORS \n"
"\n"
"Csound contains contributions from musicians, scientists, and programmers \n"
"from around the world. They include (but are not limited to): \n"
"\n"
"Allan Lee \n"
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
"Victor Lazzarini \n" 
"Ville Pulkki \n"
"\n";

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

std::vector<CsoundVstFltk *> CsoundVstFltk::instances;

Fl_Preferences CsoundVstFltk::preferences(Fl_Preferences::USER, "gogins@pipeline.com", "CsoundVST");

CsoundVstFltk::CsoundVstFltk(AudioEffect *audioEffect) :
csoundVstUi(0),
csoundVST((CsoundVST *)audioEffect),
useCount(0)
{
	instances.push_back(this);
	csound::System::setMessageCallback(&CsoundVstFltk::messageCallback);
	csoundVST->setEditor(this);
}

CsoundVstFltk::~CsoundVstFltk(void)
{
	instances.erase(std::find(instances.begin(), instances.end(), this));
}

void CsoundVstFltk::updateCaption()
{
	static std::string caption;
    caption = "[ C S O U N D   V S T ] ";
	if(csoundVST->getIsPython())
	{
		caption.append(csoundVST->Shell::getFilename());
	}
	else if(!csoundVST->getIsVst())
	{
        caption.append(csoundVST->getCppSound()->CsoundFile::getFilename());
	}
	Fl::lock();
	csoundVstUi->label(caption.c_str());
	Fl::unlock();
}

void CsoundVstFltk::updateModel()
{
	csound::System::message("BEGAN CsoundVstFltk::updateModel...\n");
	Fl::lock();
	if(csoundVstUi)
	{
		csoundVST->getCppSound()->setCommand(commandInput->value());
		csoundVST->getCppSound()->setOrchestra(orchestraTextBuffer->text());
		csoundVST->getCppSound()->setScore(scoreTextBuffer->text());
		csoundVST->setScript(scriptTextBuffer->text());
	}
	Fl::unlock();
	csound::System::message("ENDED CsoundVstFltk::updateModel.\n");
}

long CsoundVstFltk::getRect(ERect **erect)
{
	static ERect r = {0, 0, kEditorHeight, kEditorWidth};
	*erect = &r;
	return true;
}

long CsoundVstFltk::open(void *parentWindow)
{
	systemWindow = parentWindow; 
	useCount++;
	if(useCount == 1)
	{
		Fl::lock();
		this->csoundVstUi = make_window(this);
		this->mainTabs = ::mainTabs;
		this->commandInput = ::commandInput;
		this->runtimeMessagesGroup = ::runtimeMessagesGroup;
		this->runtimeMessagesBrowser = ::runtimeMessagesBrowser;
		this->orchestraTextBuffer = new Fl_Text_Buffer();
		this->scoreTextBuffer = new Fl_Text_Buffer();
		this->scriptTextBuffer = new Fl_Text_Buffer();
		this->aboutTextBuffer = new Fl_Text_Buffer();
		this->settingsEditSoundfileInput = ::settingsEditSoundfileInput;
		this->settingsVstPluginModeEffect = ::settingsVstPluginModeEffect;
		this->settingsVstPluginModeInstrument = ::settingsVstPluginModeInstrument;
		this->settingsCsoundPerformanceModeClassic = ::settingsCsoundPerformanceModeClassic;
		this->settingsCsoundPerformanceModePython = ::settingsCsoundPerformanceModePython;
		this->orchestraTextEdit = ::orchestraTextEdit;
		this->orchestraTextEdit->buffer(this->orchestraTextBuffer);
		this->scoreTextEdit = ::scoreTextEdit;
		this->scoreTextEdit->buffer(this->scoreTextBuffer);
		this->scriptTextEdit = ::scriptTextEdit;
		this->scriptTextEdit->buffer(this->scriptTextBuffer);
		this->aboutTextDisplay = ::aboutTextDisplay;
		this->aboutTextDisplay->buffer(this->aboutTextBuffer);
		this->orchestraGroup = ::orchestraGroup;
		this->scoreGroup = ::scoreGroup;
		this->scriptGroup = ::scriptGroup;
		//	Read user preferences.
		char buffer[0x500];
		int number = 0;
		preferences.get("SoundfileOpen", (char *)buffer, "mplayer.exe", 0x500);
		this->settingsEditSoundfileInput->value(buffer);
		preferences.get("IsSynth", number, 0);
		csoundVST->setIsSynth(number);
		preferences.get("IsPython", number, 0);
		csoundVST->setIsPython(number);
		this->mainTabs->value(settingsGroup);
		this->csoundVstUi->show();
		if(csoundVST->getIsVst())
		{
			//this->csoundVstUi->make_current();
#if defined(WIN32)
			SetParent((HWND) fl_xid(this->csoundVstUi), (HWND) parentWindow);
#endif
			this->csoundVstUi->position(0, 0);
		}
		this->aboutTextBuffer->text(removeCarriageReturns(about));
		update();
	}
	else
	{
		this->csoundVstUi->show();
		update();
	}
	if(csoundVST->getIsVst() || parentWindow)
	{
		return true;
	}
	else
	{
		return Fl::run();
	}
}

void CsoundVstFltk::close()
{
	useCount--;
	this->csoundVstUi->hide();
	if(useCount == 0)
	{
		delete this->csoundVstUi;
		this->csoundVstUi = 0;
	}
}

void CsoundVstFltk::idle() 
{ 
	// Process events for the FLTK GUI.
	Fl::lock();
	Fl::wait(0);
	Fl::unlock();
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
				Fl::lock();
				this->runtimeMessagesBrowser->add(messages.front().c_str());
				messages.pop_front();
				this->runtimeMessagesBrowser->bottomline(this->runtimeMessagesBrowser->size());
				Fl::flush();
				Fl::unlock();
			}
		}
	}
}

// Updates the widgets from CsoundVST.

void CsoundVstFltk::update()
{
	csound::System::message("BEGAN CsoundVstFltk::update...\n");
	if(csoundVstUi)
	{
		updateCaption();
		Fl::lock();
		static std::string buffer;
		this->settingsVstPluginModeEffect->value(!csoundVST->getIsSynth());
		this->settingsVstPluginModeInstrument->value(csoundVST->getIsSynth());
		this->settingsCsoundPerformanceModeClassic->value(!csoundVST->getIsPython());
		this->settingsCsoundPerformanceModePython->value(csoundVST->getIsPython());
		if(csoundVST->getIsPython())
		{
			onSettingsCsoundPerformanceModePython(settingsCsoundPerformanceModePython, this);	
		}
		else
		{
			onSettingsCsoundPerformanceModeClassic(settingsCsoundPerformanceModeClassic, this);	
		}
		buffer = csoundVST->getCppSound()->getCommand();
		this->commandInput->value(removeCarriageReturns(buffer));
		buffer = csoundVST->getCppSound()->getOrchestra();
		this->orchestraTextBuffer->text(removeCarriageReturns(buffer));
		buffer = csoundVST->getCppSound()->getScore();
		this->scoreTextBuffer->text(removeCarriageReturns(buffer));
		buffer = csoundVST->getScript();
		this->scriptTextBuffer->text(removeCarriageReturns(buffer));
		Fl::unlock();
	}
	csound::System::message("ENDED CsoundVstFltk::update.\n");
}

void CsoundVstFltk::postUpdate() 
{
	updateFlag = 1;
}

//	This is unfortunately elaborate.
//	Cubase won't tolerate FL event handling in the process call.
//	Therefore, for Cubase, we push messages onto a queue for Silence messages,
//	and we pull them off the queue in AeffEditor::onIdle.
//	For non-VST uses, we just go ahead as before.
//	It's also unfortunate that there is a global message callback,
//	rather than one per instance; this may be fixed in future.
void CsoundVstFltk::messageCallback(const char *format, va_list valist)
{
	char buffer[0x1000];
	vsprintf(buffer, format, valist);
	static std::string actualBuffer;
	static std::string lineBuffer;
	actualBuffer.append(buffer);
	size_t position = 0;
	while((position = actualBuffer.find("\n")) != std::string::npos)
	{
	    lineBuffer = actualBuffer.substr(0, position);
        actualBuffer.erase(0, position + 1); 
	    for(std::vector<CsoundVstFltk *>::iterator it = CsoundVstFltk::instances.begin();
	        it != CsoundVstFltk::instances.end(); 
            ++it)
        {
            CsoundVstFltk *csoundVstFltk = *it;
            if(!csoundVstFltk)
            {
			    return;
	        }
	        if(csoundVstFltk->csoundVST->getIsVst())
	        {
	            csoundVstFltk->messages.push_back(lineBuffer);
            }
		    else
		    {
			    if(!csoundVstFltk->csoundVstUi)
			    {
			        return;
                }
			    if(!csoundVstFltk->runtimeMessagesBrowser)
			    {
				    return;
			    }
			    else
			    {
				    Fl::lock();
				    csoundVstFltk->runtimeMessagesBrowser->add(lineBuffer.c_str());
				    csoundVstFltk->runtimeMessagesBrowser->bottomline(csoundVstFltk->runtimeMessagesBrowser->size());
				    Fl::flush();
				    Fl::unlock();
			    }
		    }
		}
		actualBuffer.clear();
	}
}

void CsoundVstFltk::onNew(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onNew...\n");
	if(csoundVST->getIsPython())
	{
		csoundVST->clear();
	}
	else
	{
		csoundVST->getCppSound()->removeAll();
	}
	update();
	csound::System::message("ENDED CsoundVstFltk::onNew.\n");
}

void CsoundVstFltk::onNewVersion(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onNewVersion...\n");
	std::string filename_;
	if(csoundVST->getIsPython())
	{
		csoundVST->save(csoundVST->getFilename());
		csound::System::message("Saved old version: '%s'\n", csoundVST->getFilename().c_str());
		filename_ = csoundVST->generateFilename(); 
		csoundVST->save(filename_);        
		csoundVST->setFilename(filename_);        
		csoundVST->getCppSound()->setFilename(filename_);        
		csound::System::message("Saved new version: '%s'\n", csoundVST->getFilename().c_str());
	}
	else
	{
		csoundVST->getCppSound()->save(csoundVST->getCppSound()->getFilename());
		csound::System::message("Saved old version: '%s'\n", csoundVST->getCppSound()->getFilename().c_str());
		filename_ = csoundVST->getCppSound()->generateFilename();
		csoundVST->getCppSound()->save(filename_);        
		csoundVST->getCppSound()->setFilename(filename_);        
		csound::System::message("Saved new version: '%s'\n", csoundVST->getCppSound()->getFilename().c_str());
	}
	updateCaption();
	csound::System::message("ENDED CsoundVstFltk::onNewVersion.\n");
}

void CsoundVstFltk::onImport(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
	runtimeMessagesBrowser->clear();
	csound::System::message("BEGAN CsoundVstFltk::onImport...\n");
	char *filename_ = 0;
	if(csoundVST->getIsPython())
	{
		std::string oldFilename = csoundVST->getFilename();
		if(oldFilename.length() <= 0)
		{
			oldFilename = "Default.py";
		}
		filename_ = fl_file_chooser("Open a file...", "*.py", oldFilename.c_str(), false);
	}
	else
	{
		std::string oldFilename = csoundVST->getFilename();
		if(oldFilename.length() <= 0)
		{
			oldFilename = "Default.csd";
		}
		filename_ = fl_file_chooser("Import a file...", "*.csd|*.orc|*.sco|*.mid", oldFilename.c_str(), false);
	}
	if(filename_)
	{
		WaitCursor wait;
		if(csoundVST->getIsPython())
		{
			csoundVST->load(filename_);
			csoundVST->setFilename(filename_);
			csound::System::message("Opened file: '%s'.\n", csoundVST->getFilename().c_str());
		}
		else
		{
			csoundVST->getCppSound()->importFile(filename_);
			csoundVST->getCppSound()->setFilename(filename_);
			csound::System::message("Imported file: '%s'.\n", csoundVST->getCppSound()->getFilename().c_str());
		}
		csoundVST->bank[csoundVST->getProgram()].text = csoundVST->getText();
		update();
	}
	csound::System::message("ENDED CsoundVstFltk::onImport.\n");
}

void CsoundVstFltk::onOpen(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
	runtimeMessagesBrowser->clear();
	csound::System::message("BEGAN CsoundVstFltk::onOpen...\n");
	char *filename_ = 0;
	if(csoundVST->getIsPython())
	{
		std::string oldFilename = csoundVST->getFilename();
		if(oldFilename.length() <= 0)
		{
			oldFilename = "Default.py";
		}
		filename_ = fl_file_chooser("Open a file...", "*.py", oldFilename.c_str(), false);
	}
	else
	{
		std::string oldFilename = csoundVST->getCppSound()->getFilename();
		if(oldFilename.length() <= 0)
		{
			oldFilename = "Default.csd";
		}
		filename_ = fl_file_chooser("Open a file...", "*.csd|*.orc|*.sco|*.mid", oldFilename.c_str(), false);
	}
	if(filename_)
	{
		WaitCursor wait;
		if(csoundVST->getIsPython())
		{
			csoundVST->load(filename_);
			csoundVST->setFilename(filename_);
		}
		else
		{
			csoundVST->getCppSound()->load(filename_);
		}
		csoundVST->bank[csoundVST->getProgram()].text = csoundVST->getText();
		update();
		csoundVST->getCppSound()->setFilename(filename_);        
		csound::System::message("Opened file: '%s'.\n", csoundVST->getCppSound()->getFilename().c_str());
	}
	csound::System::message("ENDED CsoundVstFltk::onOpen.\n");
}

void CsoundVstFltk::onSave(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onSave...\n");
	updateModel();
	if(csoundVST->getIsPython())
	{
		csoundVST->Shell::save(csoundVST->getFilename());
		csound::System::message("Saved file as: '%s'.\n", csoundVST->getFilename().c_str());
	}
	else
	{
		csoundVST->getCppSound()->save(csoundVST->getCppSound()->getFilename());
		csound::System::message("Saved file as: '%s'.\n", csoundVST->getCppSound()->getFilename().c_str());
	}
	csound::System::message("ENDED CsoundVstFltk::onSave.\n");
}

void CsoundVstFltk::onSaveAs(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onSaveAs...\n");
	updateModel();
	char *filename_ = 0;
	if(csoundVST->getIsPython())
	{
		std::string oldFilename = csoundVST->getFilename();
		if(oldFilename.length() <= 0)
		{
			oldFilename = "Default.py";
		}
		filename_ = fl_file_chooser("Save as...", "*.py", oldFilename.c_str(), false);
	}
	else
	{
		std::string oldFilename = csoundVST->getCppSound()->getFilename();
		if(oldFilename.length() <= 0)
		{
			oldFilename = "Default.csd";
		}
		filename_ = fl_file_chooser("Save as...", "*.csd|*.orc|*.sco|*.mid", oldFilename.c_str(), false);
	}
	if(filename_)
	{
		WaitCursor wait;
		runtimeMessagesBrowser->clear();
		csound::System::message("BEGAN CsoundVstFltk::onSaveAs...\n");
		if(csoundVST->getIsPython())
		{
			csoundVST->save(filename_);
			csoundVST->setFilename(filename_);
			csoundVST->getCppSound()->setFilename(filename_);        
			csound::System::message("Saved file as: '%s'.\n", csoundVST->getFilename().c_str());
		}
		else
		{
			csoundVST->getCppSound()->save(filename_);
			csoundVST->getCppSound()->setFilename(filename_);
			csound::System::message("Saved file as: '%s'.\n", csoundVST->getCppSound()->getFilename().c_str());
		}
		update();
	}
	csound::System::message("ENDED CsoundVstFltk::onSaveAs.\n");
}

void CsoundVstFltk::onPerform(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
	runtimeMessagesBrowser->clear();
	csound::System::message("BEGAN CsoundVstFltk::onPerform...\n");
	updateModel();
	mainTabs->value(runtimeMessagesGroup);
	csoundVST->perform();
	csound::System::message("ENDED CsoundVstFltk::onPerform.\n");
}

void CsoundVstFltk::onStop(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onStop...\n");
	if(csoundVST->getIsPython())
	{
		csoundVST->stop();
	}
	csoundVST->getCppSound()->stop();
	csound::System::message("ENDED CsoundVstFltk::onStop.\n");
}

void CsoundVstFltk::onEdit(Fl_Button*, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onEdit...\n");
	csoundVST->getCppSound()->stop();
	updateModel();
	char buffer[0x500];
	preferences.get("SoundfileOpen", (char *)buffer, "mplayer2.exe", 0x500);
	std::string command = buffer;
	command.append(" ");
	std::string soundfileName;
	soundfileName = csoundVST->getCppSound()->getOutputSoundfileName();
	command.append(soundfileName);
	csound::System::message("Executing command: '%s'\n", command.c_str());
	csound::System::execute(command.c_str());
	csound::System::message("ENDED CsoundVstFltk::onEdit.\n");
}

void CsoundVstFltk::onSettingsVstPluginMode(Fl_Check_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onSettingsVstPluginMode...\n");
	csoundVST->setIsSynth(false);
	csound::System::message("ENDED CsoundVstFltk::onSettingsVstPluginMode.\n");
}

void CsoundVstFltk::onSettingsVstInstrumentMode(Fl_Check_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onSettingsVstInstrumentMode...\n");
	csoundVST->setIsSynth(true);
	csound::System::message("ENDED CsoundVstFltk::onSettingsVstInstrumentMode.\n");
}

void CsoundVstFltk::onSettingsCsoundPerformanceModeClassic(Fl_Check_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onSettingsCsoundPerformanceModeClassic...\n");
	mainTabs->hide();
	csoundVST->clear();
	csoundVST->setIsPython(false);
	mainTabs->remove(orchestraGroup);
	mainTabs->remove(scoreGroup);
	mainTabs->remove(scriptGroup);
	mainTabs->insert(*orchestraGroup, mainTabs->children() - 2);
	mainTabs->insert(*scoreGroup, mainTabs->children() - 2);
	mainTabs->show();
	orchestraGroup->size(settingsGroup->w(), settingsGroup->h());
	scoreGroup->size(settingsGroup->w(), settingsGroup->h());
	this->commandInput->show();
	csound::System::message("ENDED CsoundVstFltk::onSettingsCsoundPerformanceModeClassic.\n");
}

void CsoundVstFltk::onSettingsCsoundPerformanceModePython(Fl_Check_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onSettingsCsoundPerformanceModePython...\n");
	mainTabs->hide();
	if(csoundVST->getCppSound())
	{
		csoundVST->getCppSound()->removeAll();
	}
	csoundVST->setIsPython(true);
	mainTabs->remove(orchestraGroup);
	mainTabs->remove(scoreGroup);
	mainTabs->remove(scriptGroup);
	mainTabs->insert(*scriptGroup, mainTabs->children() - 2);
	mainTabs->show();
	scriptGroup->size(settingsGroup->w(), settingsGroup->h());
	this->commandInput->hide();
	csound::System::message("ENDED CsoundVstFltk::onSettingsCsoundPerformanceModePython.\n");
}

void CsoundVstFltk::onSettingsApply(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
	csound::System::message("BEGAN CsoundVstFltk::onSettingsApply...\n");
	preferences.set("SoundfileOpen", this->settingsEditSoundfileInput->value());
	preferences.set("IsSynth", csoundVST->getIsSynth());
	preferences.set("IsPython", csoundVST->getIsPython());
	csound::System::message("ENDED CsoundVstFltk::onSettingsApply.\n");
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

void onSettingsCsoundPerformanceModeClassic(Fl_Check_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
	csoundVstFltk->onSettingsCsoundPerformanceModeClassic(fl_button, csoundVstFltk);
}

void onSettingsCsoundPerformanceModePython(Fl_Check_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
	csoundVstFltk->onSettingsCsoundPerformanceModePython(fl_button, csoundVstFltk);
}

void onSettingsApply(Fl_Button* fl_button, CsoundVstFltk* csoundVstFltk)
{
	csoundVstFltk->onSettingsApply(fl_button, csoundVstFltk);
}


