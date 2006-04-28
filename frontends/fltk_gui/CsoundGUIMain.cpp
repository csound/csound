/*
    CsoundGUIMain.cpp:
    Copyright (C) 2006 Istvan Varga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include "CsoundGUI.hpp"

#ifndef WIN32
#  include <sys/types.h>
#  include <unistd.h>
#else
#  include <process.h>
#endif

void CsoundGUIMain::setTimeDisplay(double timeVal)
{
    char    buf[12];
    int     timeVal_i;

    timeVal_i = (timeVal < 0.0 ? -1 : (int) (timeVal * 10.0));
    if (timeVal_i == prvTime)
      return;
    prvTime = timeVal_i;
    buf[0] = ' ';
    buf[3] = ':';
    buf[6] = ':';
    buf[9] = '.';
    buf[11] = '\0';
    if (timeVal_i < 0) {
      buf[1] = '-';
      buf[2] = '-';
      buf[4] = '-';
      buf[5] = '-';
      buf[7] = '-';
      buf[8] = '-';
      buf[10] = '-';
    }
    else {
      buf[10] = (char) (timeVal_i % 10) + '0';
      timeVal_i /= 10;
      buf[8] = (char) (timeVal_i % 10) + '0';
      timeVal_i /= 10;
      buf[7] = (char) (timeVal_i % 6) + '0';
      timeVal_i /= 6;
      buf[5] = (char) (timeVal_i % 10) + '0';
      timeVal_i /= 10;
      buf[4] = (char) (timeVal_i % 6) + '0';
      timeVal_i /= 6;
      buf[2] = (char) (timeVal_i % 10) + '0';
      timeVal_i /= 10;
      buf[1] = (char) (timeVal_i % 10) + '0';
      timeVal_i /= 10;
      if (timeVal_i)
        buf[0] = (char) (timeVal_i % 10) + '0';
    }
    scoreTimeDisplay->value(&(buf[0]));
}

int CsoundGUIMain::runCmd(std::string& cmdLine)
{
    std::string curToken;
    std::vector<std::string>  args;
    std::vector<char *>       argv;
    int         nTokens = 0;
    int         mode = 0;   // 0: white space, 1: collecting, 2: quoted string
    int         len;
    int         err;

    curToken = "";
    len = (int) cmdLine.size();
    for (int i = 0; i < len; i++) {
      char  c;
      c = cmdLine[i];
      if ((c == ' ' || c == '\t' || c == '\r' || c == '\n') && mode == 1) {
        mode = 0;
        args.push_back(curToken);
        argv.push_back(const_cast<char *>(args[nTokens].c_str()));
        nTokens++;
        curToken = "";
      }
      else if (c == '"') {
        if (mode == 2) {
          mode = 0;
          args.push_back(curToken);
          argv.push_back(const_cast<char *>(args[nTokens].c_str()));
          nTokens++;
          curToken = "";
        }
        else {
          mode = 2;
        }
      }
      else {
        if (mode == 0)
          mode = 1;
        curToken += c;
      }
    }
    if (mode == 1) {
      args.push_back(curToken);
      argv.push_back(const_cast<char *>(args[nTokens].c_str()));
      nTokens++;
    }
    if (!nTokens)
      return 0;
    argv.push_back((char*) 0);
#ifndef WIN32
    err = (int) fork();
    if (err < 0)
      return -1;
    if (err == 0) {
      std::exit(execvp(argv[0], &(argv.front())));
    }
#else
    err = (int) _spawnvp(_P_NOWAIT, argv[0], &(argv.front()));
    if (err < 0)
      return -1;
#endif

    return 0;
}

bool CsoundGUIMain::isEmptyString(std::string& s)
{
    int     len;

    len = (int) s.size();
    for (int i = 0; i < len; i++) {
      char  c = s[i];
      if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
        return false;
    }
    return true;
}

void CsoundGUIMain::stripString(std::string& dst, const char *src)
{
    std::string tmp;

    tmp = "";
    if (src != (char*) 0 && src[0] != '\0') {
      int   i, len, pos0, pos1;
      len = (int) std::strlen(src);
      for (pos0 = 0; pos0 < len; pos0++) {
        char  c = src[pos0];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
          break;
      }
      for (pos1 = len - 1; pos1 >= 0; pos1--) {
        char  c = src[pos1];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
          break;
      }
      for (i = pos0; i <= pos1; i++)
        tmp += src[i];
    }
    dst = tmp;
}

bool CsoundGUIMain::isCSDFile(std::string& fileName)
{
    int     len;

    len = (int) fileName.size();
    if (len < 5)
      return false;
    if (fileName[len - 4] != '.' ||
        (fileName[len - 3] | (char) 0x20) != 'c' ||
        (fileName[len - 2] | (char) 0x20) != 's' ||
        (fileName[len - 1] | (char) 0x20) != 'd')
      return false;
    return true;
}

bool CsoundGUIMain::isRtAudioDevice(std::string& fileName, bool isOutput)
{
    const char  *s;

    if ((int) fileName.size() < 3)
      return false;
    s = fileName.c_str();
    if (std::strncmp(s, "devaudio", 8) == 0) {
      s += 8;
    }
    else if ((isOutput && std::strncmp(s, "dac", 3) == 0) ||
             (!isOutput && std::strncmp(s, "adc", 3) == 0)) {
      s += 3;
    }
    else {
      return false;
    }
    if (*s == '\0' || *s == ':')
      return true;
    else {
      int   devNum = 0;
      do {
        if (*s < (char) '0' || *s > (char) '9')
          return false;
        devNum = (devNum * 10) + (int) (*s - (char) '0');
        if (devNum > 1023)
          return false;
      } while (*(++s) != '\0');
    }
    return true;
}

void CsoundGUIMain::updateGUIState_orcName()
{
    if (isCSDFile(currentPerformanceSettings.orcName)) {
      scoreNameInput->value("");
      scoreNameInput->deactivate();
      scoreNameButton->deactivate();
      editScoreButton->deactivate();
    }
    else {
      scoreNameInput->value(currentPerformanceSettings.scoName.c_str());
      scoreNameInput->activate();
      scoreNameButton->activate();
      editScoreButton->activate();
    }
    if ((int) currentPerformanceSettings.orcName.size() == 0)
      editOrcButton->deactivate();
    else
      editOrcButton->activate();
}

void CsoundGUIMain::updateGUIState_scoName()
{
    if ((int) currentPerformanceSettings.scoName.size() == 0 ||
        isCSDFile(currentPerformanceSettings.orcName))
      editScoreButton->deactivate();
    else
      editScoreButton->activate();
}

void CsoundGUIMain::updateGUIState_outFile()
{
    if ((int) currentPerformanceSettings.outputFileName.size() == 0 ||
        isRtAudioDevice(currentPerformanceSettings.outputFileName, true))
      editOutfileButton->deactivate();
    else
      editOutfileButton->activate();
}

void CsoundGUIMain::updateGUIState_controls()
{
    if (!performing || csPerf == (CsoundPerformance*) 0) {
      paused = true;
      rewindButton->deactivate();
      fastForwardButton->deactivate();
      stopButton->deactivate();
      playButton->label("@>");
      applyScoreOffsetButton->deactivate();
    }
    else {
      rewindButton->activate();
      fastForwardButton->activate();
      stopButton->activate();
      if (paused)
        playButton->label("@>");
      else
        playButton->label("@||");
      applyScoreOffsetButton->activate();
    }
}

void CsoundGUIMain::updateGUIState()
{
    updateGUIState_orcName();
    updateGUIState_scoName();
    updateGUIState_outFile();
    updateGUIState_controls();
}

void CsoundGUIMain::updateGUIValues()
{
    orcNameInput->value(currentPerformanceSettings.orcName.c_str());
    scoreNameInput->value(currentPerformanceSettings.scoName.c_str());
    outfileNameInput->value(currentPerformanceSettings.outputFileName.c_str());
    scoreOffsetInput->value(currentPerformanceSettings.scoreOffsetSeconds);
    if (performing && csPerf != (CsoundPerformance*) 0) {
      csPerf->SetScoreOffsetSeconds(
          currentPerformanceSettings.scoreOffsetSeconds, false);
      setTimeDisplay(csPerf->GetScoreTime());
    }
    else
      setTimeDisplay(-1.0);
    updateGUIState();
}

void CsoundGUIMain::run()
{
    readCsound5GUIConfigFile("g_cfg.dat", currentGlobalSettings);
    readCsound5GUIConfigFile("p_cfg.dat", currentPerformanceSettings);
    readCsound5GUIConfigFile("u_cfg.dat", currentUtilitySettings);

    updateGUIValues();

    csound = csoundCreate((void*) &consoleWindow);
    if (!csound)
      return;
    csoundSetMessageCallback(csound,
                             &CsoundGUIConsole::messageCallback_Thread);

    updateGUIState();
    window->show();
    consoleWindow.window->show();

    do {
      if (csPerf != (CsoundPerformance*) 0) {
        if (!performing) {
          bool  usingThreads;
          usingThreads = csPerf->UsingThreads();
          paused = true;
          csPerf->Stop();
          delete csPerf;
          csPerf = (CsoundPerformance*) 0;
          updateGUIState();
          if (!usingThreads) {
            consoleWindow.updateDisplay(true);
            Fl::wait(0.0);
          }
        }
        else {
          int   status;
          // Fl::unlock();
          status = csPerf->Perform();
          // Fl::lock();
          setTimeDisplay(csPerf->GetScoreTime());
          if (status != 0) {
            performing = false;
            paused = true;
            csoundSetMessageCallback(csound,
                                     &CsoundGUIConsole::messageCallback_Thread);
            if (currentGlobalSettings.editSoundFileAfterPerformance &&
                status > 0)
              editSoundFile(csoundGetOutputFileName(csound));
            delete csPerf;
            csPerf = (CsoundPerformance*) 0;
            updateGUIState();
          }
          else if (!csPerf->UsingThreads()) {
            consoleWindow.updateDisplay(true);
            Fl::wait(0.0);
          }
          else {
            Fl::wait(0.02);
          }
        }
      }
      if (!performing) {
        Fl::wait(0.02);
        setTimeDisplay(-1.0);
      }
      if (utilityState != 0)
        checkUtilities();
    } while (window->shown());

    performing = false;
    if (csPerf) {
      csPerf->Stop();
      delete csPerf;
      csPerf = (CsoundPerformance*) 0;
    }
    paused = true;
    csoundDestroy(csound);
    csound = (CSOUND*) 0;
    closePerformanceSettingsWindow();
    closeGlobalSettingsWindow();
    closeUtilitiesWindow();
    utilityState = 0;
}

void CsoundGUIMain::startPerformance()
{
    if (csPerf)
      return;
    performing = false;
    paused = true;
    if ((int) currentPerformanceSettings.orcName.size() == 0)
      return;
    consoleWindow.Clear();
    csoundSetHostData(csound, (void*) &consoleWindow);
    csoundSetMessageCallback(csound,
                             &CsoundGUIConsole::messageCallback_Thread);
    csoundSetYieldCallback(csound, &CsoundGUIMain::yieldCallback);
    if (csoundPreCompile(csound) != 0) {
      csoundReset(csound);
      return;
    }
    csPerf = CreateCsoundPerformance(csound,
                                     currentPerformanceSettings.useThreads);
    if (!csPerf) {
      csoundReset(csound);
      return;
    }
    {
      std::vector<std::string>  args;
      currentPerformanceSettings.buildCommandLine(
          args, currentGlobalSettings.forcePerformanceSettings);
      if (csPerf->Compile(args) != 0) {
        delete csPerf;
        csPerf = (CsoundPerformance*) 0;
        return;
      }
    }
    if (!csPerf->UsingThreads()) {
      csoundSetMessageCallback(csound,
                               &CsoundGUIConsole::messageCallback_NoThread);
      consoleWindow.flushMessages();
    }
    performing = true;
    paused = false;
    csPerf->Play();
}

void CsoundGUIMain::editOrcFile()
{
    std::string cmd;

    if (isEmptyString(currentPerformanceSettings.orcName) ||
        isEmptyString(currentGlobalSettings.textEditorProgram))
      return;
    stripString(cmd, currentGlobalSettings.textEditorProgram.c_str());
    cmd += " \"";
    cmd += currentPerformanceSettings.orcName;
    cmd += '"';
    runCmd(cmd);
}

void CsoundGUIMain::editScoreFile()
{
    std::string cmd;

    if (isEmptyString(currentPerformanceSettings.scoName) ||
        isEmptyString(currentGlobalSettings.textEditorProgram))
      return;
    stripString(cmd, currentGlobalSettings.textEditorProgram.c_str());
    cmd += " \"";
    cmd += currentPerformanceSettings.scoName;
    cmd += '"';
    runCmd(cmd);
}

void CsoundGUIMain::editSoundFile(const char *fileName_)
{
    std::string fileName;
    std::string cmd;

    stripString(fileName, fileName_);
    if ((int) fileName.size() == 0 ||
        isEmptyString(currentGlobalSettings.soundEditorProgram))
      return;
    if (isRtAudioDevice(fileName, true))
      return;
    stripString(cmd, currentGlobalSettings.soundEditorProgram.c_str());
    cmd += " \"";
    cmd += fileName;
    cmd += '"';
    runCmd(cmd);
}

CsoundGUIMain::~CsoundGUIMain()
{
    performing = false;
    if (csPerf) {
      csPerf->Stop();
      delete csPerf;
      csPerf = (CsoundPerformance*) 0;
    }
    paused = true;
    prvTime = -1;
    for (int i = 0; i < 5; i++)
      Fl::wait(0.01);
    consoleWindow.window->hide();
    consoleWindow.Clear();
    for (int i = 0; i < 5; i++)
      Fl::wait(0.01);
    if (csound) {
      csoundDestroy(csound);
      csound = (CSOUND*) 0;
    }
    if (window) {
      delete window;
      window = (Fl_Double_Window*) 0;
      Fl::wait(0.0);
    }
}

int CsoundGUIMain::yieldCallback(CSOUND *csound)
{
    (void) csound;
    return 1;
}

// ----------------------------------------------------------------------------

void CsoundGUIMain::startListOpcodes()
{
    checkUtilities();
    if (utility_listOpcodes)
      return;
    utility_listOpcodes = CreateUtility_ListOpcodes(&consoleWindow,
                                                    currentUtilitySettings);
    if (utility_listOpcodes) {
      utilityState |= CSOUND5GUI_LISTOPCODES_RUNNING;
      if (utilitiesWindow && utilitiesWindow->window->shown())
        utilitiesWindow->listOpcodesButton->label("Stop");
    }
}

void CsoundGUIMain::stopListOpcodes()
{
    if (utility_listOpcodes) {
      utility_listOpcodes->Stop();
      delete utility_listOpcodes;
      utility_listOpcodes = (CsoundUtility*) 0;
      if (utilitiesWindow && utilitiesWindow->window->shown())
        utilitiesWindow->listOpcodesButton->label("Start");
    }
    utilityState &= (~CSOUND5GUI_LISTOPCODES_RUNNING);
}

void CsoundGUIMain::startCvanal()
{
    checkUtilities();
    if (utility_cvanal)
      return;
    utility_cvanal = CreateUtility_Cvanal(&consoleWindow,
                                          currentUtilitySettings);
    if (utility_cvanal) {
      utilityState |= CSOUND5GUI_CVANAL_RUNNING;
      if (utilitiesWindow && utilitiesWindow->window->shown())
        utilitiesWindow->cvanalButton->label("Stop");
    }
}

void CsoundGUIMain::stopCvanal()
{
    if (utility_cvanal) {
      utility_cvanal->Stop();
      delete utility_cvanal;
      utility_cvanal = (CsoundUtility*) 0;
      if (utilitiesWindow && utilitiesWindow->window->shown())
        utilitiesWindow->cvanalButton->label("Start");
    }
    utilityState &= (~CSOUND5GUI_CVANAL_RUNNING);
}

void CsoundGUIMain::startPvanal()
{
    checkUtilities();
    if (utility_pvanal)
      return;
    utility_pvanal = CreateUtility_Pvanal(&consoleWindow,
                                          currentUtilitySettings);
    if (utility_pvanal) {
      utilityState |= CSOUND5GUI_PVANAL_RUNNING;
      if (utilitiesWindow && utilitiesWindow->window->shown())
        utilitiesWindow->pvanalButton->label("Stop");
    }
}

void CsoundGUIMain::stopPvanal()
{
    if (utility_pvanal) {
      utility_pvanal->Stop();
      delete utility_pvanal;
      utility_pvanal = (CsoundUtility*) 0;
      if (utilitiesWindow && utilitiesWindow->window->shown())
        utilitiesWindow->pvanalButton->label("Start");
    }
    utilityState &= (~CSOUND5GUI_PVANAL_RUNNING);
}

// ----------------------------------------------------------------------------

void CsoundGUIMain::openPerformanceSettingsWindow()
{
    if (!performanceSettingsWindow) {
      // performing = false;
      // paused = true;
      // updateGUIValues();
      // Fl::wait(0.0);
      performanceSettingsWindow =
          new CsoundPerformanceSettingsPanel(currentPerformanceSettings);
      if (performanceSettingsWindow) {
        utilityState |= CSOUND5GUI_PCFGWIN_OPEN;
        performanceSettingsWindow->window->show();
      }
    }
}

void CsoundGUIMain::closePerformanceSettingsWindow()
{
    if (performanceSettingsWindow) {
      if (performanceSettingsWindow->status > 0)
        currentPerformanceSettings =
            performanceSettingsWindow->performanceSettings;
      delete performanceSettingsWindow;
      performanceSettingsWindow = (CsoundPerformanceSettingsPanel*) 0;
      updateGUIValues();
      writeCsound5GUIConfigFile("p_cfg.dat", currentPerformanceSettings);
    }
    utilityState &= (~CSOUND5GUI_PCFGWIN_OPEN);
}

void CsoundGUIMain::openGlobalSettingsWindow()
{
    if (!globalSettingsWindow) {
      // performing = false;
      // paused = true;
      // updateGUIValues();
      // Fl::wait(0.0);
      globalSettingsWindow = new CsoundGlobalSettingsPanel(this);
      if (globalSettingsWindow) {
        utilityState |= CSOUND5GUI_GCFGWIN_OPEN;
        globalSettingsWindow->window->show();
      }
    }
}

void CsoundGUIMain::closeGlobalSettingsWindow()
{
    if (globalSettingsWindow) {
      delete globalSettingsWindow;
      globalSettingsWindow = (CsoundGlobalSettingsPanel*) 0;
      updateGUIValues();
      writeCsound5GUIConfigFile("g_cfg.dat", currentGlobalSettings);
    }
    utilityState &= (~CSOUND5GUI_GCFGWIN_OPEN);
}

void CsoundGUIMain::openUtilitiesWindow()
{
    if (!utilitiesWindow) {
      // performing = false;
      // paused = true;
      // updateGUIValues();
      // Fl::wait(0.0);
      utilitiesWindow = new CsoundUtilitiesWindow(this);
      if (utilitiesWindow) {
        utilityState |= CSOUND5GUI_UTILWIN_OPEN;
        utilitiesWindow->window->show();
      }
    }
}

void CsoundGUIMain::closeUtilitiesWindow()
{
    stopListOpcodes();
    stopCvanal();
    stopPvanal();
    if (utilitiesWindow) {
      delete utilitiesWindow;
      utilitiesWindow = (CsoundUtilitiesWindow*) 0;
      writeCsound5GUIConfigFile("u_cfg.dat", currentUtilitySettings);
    }
    utilityState &= (~CSOUND5GUI_UTILWIN_OPEN);
}

// ----------------------------------------------------------------------------

void CsoundGUIMain::checkUtilities()
{
    if (globalSettingsWindow &&
        !globalSettingsWindow->window->shown())
      closeGlobalSettingsWindow();
    if (performanceSettingsWindow &&
        (performanceSettingsWindow->status != 0 ||
         !performanceSettingsWindow->window->shown()))
      closePerformanceSettingsWindow();
    if (utilitiesWindow &&
        !utilitiesWindow->window->shown())
      closeUtilitiesWindow();

    if (utility_listOpcodes && utility_listOpcodes->GetStatus() != 0)
      stopListOpcodes();
    if (utility_cvanal && utility_cvanal->GetStatus() != 0)
      stopCvanal();
    if (utility_pvanal && utility_pvanal->GetStatus() != 0)
      stopPvanal();
}

