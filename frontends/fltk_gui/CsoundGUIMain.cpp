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
// #include <FL/Fl_File_Chooser.H>

#include "Fl_Native_File_Chooser.H"
#include <FL/fl_ask.H>

using namespace std;

void CsoundGUIMain::setTimeDisplay(double timeVal)
{
  char    buf[13];
  int     timeVal_i;

  timeVal_i = (timeVal < 0.0 ? -1 : (int) (timeVal * 10.0));
  if (timeVal_i == prvTime)
    return;
  prvTime = timeVal_i;
  buf[0] = ' ';
  buf[1] = ' ';
  buf[4] = ':';
  buf[7] = ':';
  buf[10] = '.';
  buf[12] = '\0';
  if (timeVal_i < 0) {
    buf[2] = '-';
    buf[3] = '-';
    buf[5] = '-';
    buf[6] = '-';
    buf[8] = '-';
    buf[9] = '-';
    buf[11] = '-';
  }
  else {
    buf[11] = (char) (timeVal_i % 10) + '0';
    timeVal_i /= 10;
    buf[9] = (char) (timeVal_i % 10) + '0';
    timeVal_i /= 10;
    buf[8] = (char) (timeVal_i % 6) + '0';
    timeVal_i /= 6;
    buf[6] = (char) (timeVal_i % 10) + '0';
    timeVal_i /= 10;
    buf[5] = (char) (timeVal_i % 6) + '0';
    timeVal_i /= 6;
    buf[3] = (char) (timeVal_i % 10) + '0';
    timeVal_i /= 10;
    buf[2] = (char) (timeVal_i % 10) + '0';
    timeVal_i /= 10;
    if (timeVal_i)
      buf[1] = (char) (timeVal_i % 10) + '0';
  }
  scoreTimeDisplay->value(&(buf[0]));
}

int CsoundGUIMain::runCmd(string& cmdLine)
{
  string curToken;
  vector<string>  args;
  vector<char *>       argv;
  int         nTokens = 0;
  int         mode = 0;   // 0: white space, 1: collecting, 2: quoted string
  int         len;

  curToken = "";
  len = (int) cmdLine.size();
  for (int i = 0; i < len; i++) {
    char  c;
    c = cmdLine[i];
    if ((c == ' ' || c == '\t' || c == '\r' || c == '\n') && mode != 2) {
      if (mode != 0) {
        mode = 0;
        args.push_back(curToken);
        argv.push_back(const_cast<char *>(args[nTokens].c_str()));
        nTokens++;
        curToken = "";
      }
    }
    else if (c == '"') {
      if (mode == 2)
        mode = 1;
      else
        mode = 2;
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
  if (csoundRunCommand(&(argv.front()), 1) < 0L)
    return -1;

  return 0;
}

bool CsoundGUIMain::isEmptyString(string& s)
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

void CsoundGUIMain::stripString(string& dst, const char *src)
{
  string tmp;

  tmp = "";
  if (!src)
    src = dst.c_str();
  if (src[0] != '\0') {
    int   i, len, pos0, pos1;
    len = (int) strlen(src);
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

bool CsoundGUIMain::isCSDFile(string& filename)
{
  int position = filename.length() - 4;
  if (position < 0) {
    return false;
  }
  char c = filename[position];
  if (!(c == '.')) {
    return false;
  }
  position++;
  c = filename[position];
  if (!(c == 'c' || c == 'C')) {
    return false;
  }
  position++;
  c = filename[position];
  if (!(c == 's' || c == 'S')) {
    return false;
  }
  position++;
  c = filename[position];
  if (!(c == 'd' || c == 'D')) {
    return false;
  }
  return true;
}

bool CsoundGUIMain::isRtAudioDevice(string& fileName, bool isOutput)
{
  const char  *s;

  if ((int) fileName.size() < 3)
    return false;
  s = fileName.c_str();
  if (strncmp(s, "devaudio", 8) == 0) {
    s += 8;
  }
  else if ((isOutput && strncmp(s, "dac", 3) == 0) ||
           (!isOutput && strncmp(s, "adc", 3) == 0)) {
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

static const char *fileNameFilters[] = {
  (char*) 0,
  "Csound orchestra and CSD files\t*.{csd,orc}\n",
  "Csound score files\t*.sco\n",
  "Sound files\t*.{aif,aiff,au,flac,pcm,raw,sd2,sf,snd,wav}\n",
  "MIDI files\t*.{mid,smf}\n",
  "Convolve files\t*.{con,cv}\n",
  "PVOC files\t*.{pv,pvx}\n",
  (char*) 0,
  (char*) 0,
  "Python files\t*.py\n"
};

bool CsoundGUIMain::browseFile(string& fileName, const char *title,
                               int fileType, bool isOutput)
{
  Fl_Native_File_Chooser *fdlg;
  int             type_;
  bool            retval = false;

  if (fileType == CSOUND5GUI_FILETYPE_DIRECTORY)
    type_ = Fl_Native_File_Chooser::BROWSE_DIRECTORY;
  else if (isOutput)
    type_ = Fl_Native_File_Chooser::BROWSE_SAVE_FILE;
  else
    type_ = Fl_Native_File_Chooser::BROWSE_FILE;
  if (fileType < 0 ||
      fileType >= ((int) sizeof(fileNameFilters) / (int) sizeof(char*)))
    fileType = 0;
  //     fdlg = new Fl_Native_File_Chooser(fileName.c_str(), fileNameFilters[fileType],
  //                                type_, title);
  fdlg = new Fl_Native_File_Chooser(type_);
  fdlg->title(title);
  fdlg->filter(fileNameFilters[fileType]);
  fdlg->preset_file(fileName.c_str());
  // Do not activate preview for binary files or directory browsing
  if (fileType != 3 || fileType != 5 || fileType != 6 ||
      fileType != CSOUND5GUI_FILETYPE_DIRECTORY)
    fdlg->options(Fl_Native_File_Chooser::PREVIEW & Fl_Native_File_Chooser::NEW_FOLDER);
  //       fdlg->preview(1);
  fdlg->show();
  //     do {
  //       Fl::wait(0.02);
  //     } while (fdlg->shown());
  if (fdlg->count() > 0) {
    CsoundGUIMain::stripString(fileName, fdlg->filename());
    retval = true;
  }
  delete fdlg;
  Fl::wait(0.0);

  return retval;
}

void CsoundGUIMain::updateGUIState_orcName()
{
  if (isCSDFile(currentPerformanceSettings.orcName) ||
      currentPerformanceSettings.orcName.size() == 0) {
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
  if (!isRtAudioDevice(currentPerformanceSettings.outputFileName, true)) {
    outfileNameButton->activate();
    outfileNameInput->activate();
    playOutfileButton->activate();
    editOutfileButton->activate();
  }
  else  {
    outfileNameButton->deactivate();
    outfileNameInput->deactivate();
    playOutfileButton->deactivate();
    editOutfileButton->deactivate();
  }
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
  if (currentPerformanceSettings.runRealtime)
    realtimeIOToggle->set();
  updateGUIState();
}

void CsoundGUIMain::run(int argc, char **argv)
{
  readCsound5GUIConfigFile("g_cfg.dat", currentGlobalSettings);
  readCsound5GUIConfigFile("p_cfg.dat", currentPerformanceSettings);
  readCsound5GUIConfigFile("u_cfg.dat", currentUtilitySettings);
  if (argc > 1)
    currentPerformanceSettings.orcName.assign(argv[1]);

  csound = csoundCreate((void*) &consoleWindow);
  if (!csound)
    return;
  csoundSetMessageCallback(csound,
                           &CsoundGUIConsole::messageCallback_Thread);
  updateGUIValues();
  consoleWindow.window->resize(currentGlobalSettings.consolePosX,
                               currentGlobalSettings.consolePosY,
                               currentGlobalSettings.consoleWidth,
                               currentGlobalSettings.consoleHeight);
  consoleWindow.window->show();
  if (currentGlobalSettings.guiPosX > 0 && currentGlobalSettings.guiPosY > 0)
    window->position(currentGlobalSettings.guiPosX, currentGlobalSettings.guiPosY);
  window->show();

  do {
    if (csPerf != (CsoundPerformance*) 0) {
      if (!performing) {
        bool usingThreads = csPerf->UsingThreads();
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
        // Fl::unlock();
        int status = csPerf->Perform();
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

  Fl::wait(0.0);
  writeCsound5GUIConfigFile("p_cfg.dat", currentPerformanceSettings);
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
  closeAboutWindow();
  utilityState = 0;
  Fl::wait(0.0);
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
  csPerf = CreateCsoundPerformance(csound,
                                   currentPerformanceSettings.useThreads);
  if (!csPerf) {
    csoundReset(csound);
    return;
  }
  {
    vector<string>  args;
    currentPerformanceSettings.buildCommandLine(
                                                args, currentGlobalSettings.forcePerformanceSettings);
    for (unsigned int i = 0 ; i < args.size(); i++)  {
      fprintf(stdout, "%s ", args[i].c_str());
    }
    fprintf(stdout, "\n\n");
    //       consoleWindow.textDisplay->textcolor(FL_RED);
    //       consoleWindow.textDisplay->insert_position(0);
    //       for (unsigned int i = 0 ; i < args.size(); i++)  {
    //         consoleWindow.textDisplay->insert(args[i].c_str());
    //         consoleWindow.textDisplay->insert(" ");
    //       }
    //       consoleWindow.textDisplay->textcolor(FL_BLACK);
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
  if (currentGlobalSettings.useBuiltInEditor) {
    openOrcEditor(currentPerformanceSettings.orcName);
  }
  else  {
    string cmd;
    if (isEmptyString(currentPerformanceSettings.orcName) ||
        isEmptyString(currentGlobalSettings.textEditorProgram))
      return;
    stripString(cmd, currentGlobalSettings.textEditorProgram.c_str());
    cmd = cmd.insert(0, "\"");
    cmd += "\" \"";
    cmd += currentPerformanceSettings.orcName;
    cmd += '"';
    runCmd(cmd);
  }
}

void CsoundGUIMain::editScoreFile()
{
  if (currentGlobalSettings.useBuiltInEditor) {
    openScoEditor(currentPerformanceSettings.scoName);
  }
  else  {
    string cmd;
    if (isEmptyString(currentPerformanceSettings.scoName) ||
        isEmptyString(currentGlobalSettings.textEditorProgram))
      return;
    stripString(cmd, currentGlobalSettings.textEditorProgram.c_str());
    cmd = cmd.insert(0, "\"");
    cmd += "\" \"";
    cmd += currentPerformanceSettings.scoName;
    cmd += '"';
    runCmd(cmd);
  }
}

void CsoundGUIMain::editSoundFile(const char *fileName_)
{
  string fileName;
  string cmd;

  stripString(fileName, fileName_);
  if ((int) fileName.size() == 0 ||
      isEmptyString(currentGlobalSettings.soundEditorProgram))
    return;
  if (isRtAudioDevice(fileName, true))
    return;
  stripString(cmd, currentGlobalSettings.soundEditorProgram.c_str());
  cmd = cmd.insert(0, "\"");
  cmd += "\" \"";
  cmd += fileName;
  cmd += '"';
  runCmd(cmd);
}

void CsoundGUIMain::playSoundFile(const char *fileName_)
{
  string fileName;
  string cmd;

  stripString(fileName, fileName_);
  if ((int) fileName.size() == 0 ||
      isEmptyString(currentGlobalSettings.soundPlayerProgram))
    return;
  if (isRtAudioDevice(fileName, true))
    return;
  stripString(cmd, currentGlobalSettings.soundPlayerProgram.c_str());
  cmd = cmd.insert(0, "\"");
  cmd += "\" \"";
  cmd += fileName;
  cmd += '"';
  runCmd(cmd);
}

void CsoundGUIMain::setRealtimeCheckbox(int checked)
{
  if (checked == 1)  {
    currentPerformanceSettings.runRealtime = true;
    if (performanceSettingsWindow)
      performanceSettingsWindow->performanceSettings.runRealtime = true;
    stripString(oldOutFilename, outfileNameInput->value());
    outfileNameInput->value(currentPerformanceSettings.rtAudioOutputDevice.c_str());
    currentPerformanceSettings.outputFileName = currentPerformanceSettings.rtAudioOutputDevice;
  }
  else
    {
      currentPerformanceSettings.runRealtime = false;
      if (performanceSettingsWindow)
        performanceSettingsWindow->performanceSettings.runRealtime = false;
      if (isRtAudioDevice(currentPerformanceSettings.outputFileName, true) &&
          isEmptyString(oldOutFilename))
        oldOutFilename = "test";
      outfileNameInput->value(oldOutFilename.c_str());
      currentPerformanceSettings.outputFileName = oldOutFilename;
    }
  updateGUIState_outFile();
}

void CsoundGUIMain::runHelpBrowser(string page)
{
  string cmd = "";
  // check for CSDOCDIR first, then use PerformanceSettings
  if (getenv("CSDOCDIR"))
    cmd = getenv("CSDOCDIR");
  if (!isEmptyString(currentPerformanceSettings.csdocdirPath))
    cmd = currentPerformanceSettings.csdocdirPath;
  if (isEmptyString(cmd))  {
    fl_alert("CSDOCDIR not set!\nSet locally at Options->Csound->Environment");
    return;
  }
  if (cmd[cmd.size() - 1] != 47)  // 47 = "/"
    cmd.append("/");
  cmd.insert(0, "\"");
  cmd += page + "\"";
  if (FILE * file = fopen(cmd.substr(1, cmd.size()-2).c_str(), "r")) //Check if file exists
    {
      fclose(file);
      cmd.insert(0, " ");
      string app;
#ifdef MACOSX
      app = "/Applications/Safari.app/Contents/MacOS/Safari \" \"/Library/Frameworks/CsoundLib.Framework/\" ";
#else
      app = currentGlobalSettings.helpBrowserProgram + "\" ";
#endif
      if (!isEmptyString(app)) {
        cmd.insert(0, app);
        cmd.insert(0, "\"");
        cmd.append(" &\"");
        runCmd(cmd);
      }
    } else {
    string label = "Manual file not found: \n";
    label += cmd.c_str();
    fl_alert(label.c_str());
  }
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
  int i;
  for (i = 0; i < 5; i++)
    Fl::wait(0.01);
  currentGlobalSettings.guiPosX = window->x();
  currentGlobalSettings.guiPosY = window->y();
  if (consoleWindow.window) {
    currentGlobalSettings.consolePosX = consoleWindow.window->x();
    currentGlobalSettings.consolePosY = consoleWindow.window->y();
    currentGlobalSettings.consoleWidth = consoleWindow.window->w();
    currentGlobalSettings.consoleHeight = consoleWindow.window->h();
  }
  if (orcEditorWindow) {
    currentGlobalSettings.orcEditorPosX = orcEditorWindow->x();
    currentGlobalSettings.orcEditorPosY = orcEditorWindow->y();
    currentGlobalSettings.orcEditorWidth = orcEditorWindow->w();
    currentGlobalSettings.orcEditorHeight = orcEditorWindow->h();
  }
  if (scoEditorWindow) {
    currentGlobalSettings.scoEditorPosX = scoEditorWindow->x();
    currentGlobalSettings.scoEditorPosY = scoEditorWindow->y();
    currentGlobalSettings.scoEditorWidth = scoEditorWindow->w();
    currentGlobalSettings.scoEditorHeight = scoEditorWindow->h();
  }
  writeCsound5GUIConfigFile("g_cfg.dat", currentGlobalSettings);
  consoleWindow.window->hide();
  consoleWindow.Clear();
  for (i = 0; i < 5; i++)
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

void CsoundGUIMain::startHetro()
{
  checkUtilities();
  if (utility_hetro)
    return;
  utility_hetro = CreateUtility_Hetro(&consoleWindow,
                                      currentUtilitySettings);
  if (utility_hetro) {
    utilityState |= CSOUND5GUI_HETRO_RUNNING;
    if (utilitiesWindow && utilitiesWindow->window->shown())
      utilitiesWindow->hetroButton->label("Stop");
  }
}

void CsoundGUIMain::stopHetro()
{
  if (utility_hetro) {
    utility_hetro->Stop();
    delete utility_hetro;
    utility_hetro = (CsoundUtility*) 0;
    if (utilitiesWindow && utilitiesWindow->window->shown())
      utilitiesWindow->hetroButton->label("Start");
  }
  utilityState &= (~CSOUND5GUI_HETRO_RUNNING);
}

void CsoundGUIMain::startLpanal()
{
  checkUtilities();
  if (utility_lpanal)
    return;
  utility_lpanal = CreateUtility_Lpanal(&consoleWindow,
                                        currentUtilitySettings);
  if (utility_lpanal) {
    utilityState |= CSOUND5GUI_LPANAL_RUNNING;
    if (utilitiesWindow && utilitiesWindow->window->shown())
      utilitiesWindow->lpanalButton->label("Stop");
  }
}

void CsoundGUIMain::stopLpanal()
{
  if (utility_lpanal) {
    utility_lpanal->Stop();
    delete utility_lpanal;
    utility_lpanal = (CsoundUtility*) 0;
    if (utilitiesWindow && utilitiesWindow->window->shown())
      utilitiesWindow->lpanalButton->label("Start");
  }
  utilityState &= (~CSOUND5GUI_LPANAL_RUNNING);
}

void CsoundGUIMain::startSndinfo()
{
  checkUtilities();
  if (utility_sndinfo)
    return;
  utility_sndinfo = CreateUtility_Sndinfo(&consoleWindow,
                                          currentUtilitySettings);
  if (utility_sndinfo) {
    utilityState |= CSOUND5GUI_SNDINFO_RUNNING;
    if (utilitiesWindow && utilitiesWindow->window->shown())
      utilitiesWindow->sndinfoButton->label("Stop");
  }
}

void CsoundGUIMain::stopSndinfo()
{
  if (utility_sndinfo) {
    utility_sndinfo->Stop();
    delete utility_sndinfo;
    utility_sndinfo = (CsoundUtility*) 0;
    if (utilitiesWindow && utilitiesWindow->window->shown())
      utilitiesWindow->sndinfoButton->label("Start");
  }
  utilityState &= (~CSOUND5GUI_SNDINFO_RUNNING);
}

void CsoundGUIMain::startSrconv()
{
  checkUtilities();
  if (utility_srconv)
    return;
  utility_srconv = CreateUtility_Srconv(&consoleWindow,
                                        currentUtilitySettings);
  if (utility_srconv) {
    utilityState |= CSOUND5GUI_SRCONV_RUNNING;
    if (utilitiesWindow && utilitiesWindow->window->shown())
      utilitiesWindow->srconvButton->label("Stop");
  }
}

void CsoundGUIMain::stopSrconv()
{
  if (utility_srconv) {
    utility_srconv->Stop();
    delete utility_srconv;
    utility_srconv = (CsoundUtility*) 0;
    if (utilitiesWindow && utilitiesWindow->window->shown())
      utilitiesWindow->srconvButton->label("Start");
  }
  utilityState &= (~CSOUND5GUI_SRCONV_RUNNING);
}

void CsoundGUIMain::startDnoise()
{
  checkUtilities();
  if (utility_dnoise)
    return;
  utility_dnoise = CreateUtility_Dnoise(&consoleWindow,
                                        currentUtilitySettings);
  if (utility_dnoise) {
    utilityState |= CSOUND5GUI_DNOISE_RUNNING;
    if (utilitiesWindow && utilitiesWindow->window->shown())
      utilitiesWindow->dnoiseButton->label("Stop");
  }
}

void CsoundGUIMain::stopDnoise()
{
  if (utility_dnoise) {
    utility_dnoise->Stop();
    delete utility_dnoise;
    utility_dnoise = (CsoundUtility*) 0;
    if (utilitiesWindow && utilitiesWindow->window->shown())
      utilitiesWindow->dnoiseButton->label("Start");
  }
  utilityState &= (~CSOUND5GUI_DNOISE_RUNNING);
}

// ----------------------------------------------------------------------------

void CsoundGUIMain::openPerformanceSettingsWindow(int tab)
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
      if (tab == 2) //Realtime Audio Tab
        performanceSettingsWindow->tabs->value(performanceSettingsWindow->RTaudioTab);
      performanceSettingsWindow->window->show();
    }
  }
}

void CsoundGUIMain::closePerformanceSettingsWindow()
{
  if (performanceSettingsWindow) {
    if (performanceSettingsWindow->status > 0)  {
      currentPerformanceSettings =
        performanceSettingsWindow->performanceSettings;
      if (currentPerformanceSettings.runRealtime)
        currentPerformanceSettings.outputFileName =
          performanceSettingsWindow->rtAudioOutputDeviceInput->value();
    }
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
  stopHetro();
  stopLpanal();
  stopSndinfo();
  stopSrconv();
  stopDnoise();
  if (utilitiesWindow) {
    delete utilitiesWindow;
    utilitiesWindow = (CsoundUtilitiesWindow*) 0;
    writeCsound5GUIConfigFile("u_cfg.dat", currentUtilitySettings);
  }
  utilityState &= (~CSOUND5GUI_UTILWIN_OPEN);
}

void CsoundGUIMain::openAboutWindow()
{
  if (!aboutWindow) {
    // performing = false;
    // paused = true;
    // updateGUIValues();
    // Fl::wait(0.0);
    aboutWindow = new CsoundAboutWindow();
    if (aboutWindow) {
      utilityState |= CSOUND5GUI_ABOUTWIN_OPEN;
      aboutWindow->window->show();
    }
  }
}

void CsoundGUIMain::closeAboutWindow()
{
  if (aboutWindow) {
    delete aboutWindow;
    aboutWindow = (CsoundAboutWindow*) 0;
  }
  utilityState &= (~CSOUND5GUI_ABOUTWIN_OPEN);
}

void CsoundGUIMain::openOrcEditor(string file)
{
  if (!orcEditorWindow) {
    //       // performing = false;
    //       // paused = true;
    //       // updateGUIValues();
    //       // Fl::wait(0.0);
    orcEditorWindow = new CsoundEditorWindow(660,400, "Editor", file.c_str());
    if (orcEditorWindow) {
      utilityState |= CSOUND5GUI_ORCEDITORWIN_OPEN;
      orcEditorWindow->resize(currentGlobalSettings.orcEditorPosX,
                              currentGlobalSettings.orcEditorPosY,
                              currentGlobalSettings.orcEditorWidth,
                              currentGlobalSettings.orcEditorHeight);
      orcEditorWindow->parent = this;
      orcEditorWindow->show();
    }
  }
}

void CsoundGUIMain::closeOrcEditor()
{
  if (orcEditorWindow) {
    currentGlobalSettings.orcEditorPosX = orcEditorWindow->x();
    currentGlobalSettings.orcEditorPosY = orcEditorWindow->y();
    currentGlobalSettings.orcEditorWidth = orcEditorWindow->w();
    currentGlobalSettings.orcEditorHeight = orcEditorWindow->h();
    delete orcEditorWindow;
    orcEditorWindow = (CsoundEditorWindow*) 0;
    updateGUIValues();
    writeCsound5GUIConfigFile("g_cfg.dat", currentGlobalSettings);
  }
  utilityState &= (~CSOUND5GUI_ORCEDITORWIN_OPEN);
}

void CsoundGUIMain::openScoEditor(string file)
{
  if (!scoEditorWindow) {
    //       // performing = false;
    //       // paused = true;
    //       // updateGUIValues();
    //       // Fl::wait(0.0);
    scoEditorWindow = new CsoundEditorWindow(660,400, "Editor", file.c_str());
  }
  if (scoEditorWindow) {
    utilityState |= CSOUND5GUI_SCOEDITORWIN_OPEN;
    scoEditorWindow->resize(currentGlobalSettings.scoEditorPosX,
                            currentGlobalSettings.scoEditorPosY,
                            currentGlobalSettings.scoEditorWidth,
                            currentGlobalSettings.scoEditorHeight);
    scoEditorWindow->parent = this;
    scoEditorWindow->show();
  }
}

void CsoundGUIMain::closeScoEditor()
{
  if (scoEditorWindow) {
    currentGlobalSettings.scoEditorPosX = scoEditorWindow->x();
    currentGlobalSettings.scoEditorPosY = scoEditorWindow->y();
    currentGlobalSettings.scoEditorWidth = scoEditorWindow->w();
    currentGlobalSettings.scoEditorHeight = scoEditorWindow->h();
    delete scoEditorWindow;
    scoEditorWindow = (CsoundEditorWindow*) 0;
    updateGUIValues();
    writeCsound5GUIConfigFile("g_cfg.dat", currentGlobalSettings);
  }
  utilityState &= (~CSOUND5GUI_SCOEDITORWIN_OPEN);
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
  if (utilitiesWindow && !utilitiesWindow->window->shown())
    closeUtilitiesWindow();
  if (aboutWindow && !aboutWindow->window->shown())
    closeAboutWindow();

  if ((utilityState
       & (CSOUND5GUI_LISTOPCODES_RUNNING | CSOUND5GUI_CVANAL_RUNNING
          | CSOUND5GUI_PVANAL_RUNNING | CSOUND5GUI_HETRO_RUNNING
          | CSOUND5GUI_LPANAL_RUNNING | CSOUND5GUI_SNDINFO_RUNNING
          | CSOUND5GUI_SRCONV_RUNNING | CSOUND5GUI_DNOISE_RUNNING)) != 0) {
    if (utility_listOpcodes && utility_listOpcodes->GetStatus() != 0)
      stopListOpcodes();
    if (utility_cvanal && utility_cvanal->GetStatus() != 0)
      stopCvanal();
    if (utility_pvanal && utility_pvanal->GetStatus() != 0)
      stopPvanal();
    if (utility_hetro && utility_hetro->GetStatus() != 0)
      stopHetro();
    if (utility_lpanal && utility_lpanal->GetStatus() != 0)
      stopLpanal();
    if (utility_sndinfo && utility_sndinfo->GetStatus() != 0)
      stopSndinfo();
    if (utility_srconv && utility_srconv->GetStatus() != 0)
      stopSrconv();
    if (utility_dnoise && utility_dnoise->GetStatus() != 0)
      stopDnoise();
  }
}

void CsoundGUIMain::pushPlayPauseButton()
{
  if (!performing || csPerf == (CsoundPerformance*) 0)
    startPerformance();
  else if (paused) {
    paused = false;
    csPerf->Play();
  }
  else {
    paused = true;
    csPerf->Pause();
  }
  updateGUIState_controls();
}

void CsoundGUIMain::pushStopButton()
{
  if (performing && csPerf != (CsoundPerformance *) 0) {
    performing = false;
    updateGUIState_controls();
  }
}

void CsoundGUIMain::pushRewindButton()
{
  if (performing && csPerf != (CsoundPerformance*) 0) {
    csPerf->Rewind();
    consoleWindow.Clear();
  }
}

void CsoundGUIMain::pushOpenOrcButton()
{
  CsoundGUIMain::browseFile(currentPerformanceSettings.orcName,
                            "Select orchestra or CSD file",
                            CSOUND5GUI_FILETYPE_ORC_CSD,
                            false);
  orcNameInput->value(currentPerformanceSettings.orcName.c_str());
  if (orcEditorWindow) {
    if (orcEditorWindow->openFile(currentPerformanceSettings.orcName.c_str()) == 0)
      {
        stripString(currentPerformanceSettings.orcName, orcNameInput->value());
        return;
      }
  }
  closeScoEditor();
  string tempScoName = currentPerformanceSettings.orcName;
  int pos = tempScoName.rfind(".orc");
  if (pos != (int) string::npos)
    {
      tempScoName.replace(pos, 4, ".sco");
      if (FILE * file = fopen(tempScoName.c_str(), "r")) //Check if file exists
        {
          fclose(file);
          currentPerformanceSettings.scoName = tempScoName;
          scoreNameInput->value(currentPerformanceSettings.scoName.c_str());
        }
    }
  updateGUIState_orcName();
}

void CsoundGUIMain::pushOpenScoButton()
{
  CsoundGUIMain::browseFile(currentPerformanceSettings.scoName,
                            "Select score file",
                            CSOUND5GUI_FILETYPE_SCORE,
                            false);
  scoreNameInput->value(currentPerformanceSettings.scoName.c_str());
  if (scoEditorWindow) {
    if (scoEditorWindow->openFile(currentPerformanceSettings.scoName.c_str()) == 0)
      {
        stripString(currentPerformanceSettings.scoName, scoreNameInput->value());
        return;
      }
    updateGUIState_scoName();
  }
}
