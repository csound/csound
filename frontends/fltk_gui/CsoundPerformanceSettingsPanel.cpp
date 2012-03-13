
#ifdef HAVE_JACK
#include "jack/jack.h"
#endif

//CS5GUI_EXPERIMENTAL device query section left out for Windows and Mac while the problem is found
#ifdef LINUX
#define CS5GUI_EXPERIMENTAL
#endif


#include "csound.hpp"
#include <FL/fl_ask.H>
  //  #include <unistd.h>
#define MAX_NAME_LEN    32      /* for client and port name */

#include "stdarg.h"

#ifdef CS5GUI_EXPERIMENTAL
static char defaultTmpCsdPre[] ="<CsoundSynthesizer>\n<CsOptions>\n-d -+rtaudio=";
static char defaultTmpCsdPostOut[] =" -odac999\n</CsOptions>\n<CsInstruments>\ninstr 1\nendin\n</CsInstruments>\n<CsScore>\n</CsScore>\n</CsoundSynthesizer>";
static char defaultTmpCsdPostIn[] =" -iadc999\n</CsOptions>\n<CsInstruments>\ninstr 1\nendin\n</CsInstruments>\n<CsScore>\n</CsScore>\n</CsoundSynthesizer>";
static char defaultTmpCsdMidiPre[] ="<CsoundSynthesizer>\n<CsOptions>\n-d -+rtmidi=";
static char defaultTmpCsdMidiPostOut[] =" -M999\n</CsOptions>\n<CsInstruments>\ninstr 1\nendin\n</CsInstruments>\n<CsScore>\n</CsScore>\n</CsoundSynthesizer>";
static char defaultTmpCsdMidiPostIn[] =" -Q999\n</CsOptions>\n<CsInstruments>\ninstr 1\nendin\n</CsInstruments>\n<CsScore>\n</CsScore>\n</CsoundSynthesizer>";
#endif

static void escapeSlash(char *string_)
{
    std::string mystring = string_;
    int len = mystring.size();
    for (int i = 0; i < len; i++) {
      if (mystring[i] == '/')  {
         mystring.insert(i, "\\");
         len++;
         i++;
      }
    }
    strcpy(string_, mystring.c_str());
}

static void stripSlash(std::string &string_)
{
    int len = string_.size();
    for (int i = 0; i < len; i++) {
      if (string_[i] == '\\')  {
         string_.erase(i, 1);
      }
    }
}

static char *csoundMessages;
static char startText[180];
static char endText[180];
static bool relevantText;
static int  errors = 0;

static void getCsoundMessages(CSOUND *csound, int attr,
                                     const char *format, va_list valist)
{
    char newMessage[180];
    vsprintf( newMessage, format,  valist);
    if (strstr(newMessage, startText) && !relevantText) {
      relevantText = true;
      return;
    }
    if (strstr(newMessage, endText)) {
      relevantText = false;
    }
    if (relevantText)
      strcat(csoundMessages, newMessage);
}

void CsoundPerformanceSettingsPanel::updateGUIState_Midi()
{
    /* Currently these toggles are exclusive. Should this be changed?*/
    if (midiKeyMidiOn->value() == 1)  {
      midiKeyMidiSpinner->value(3);
      midiKeyMidiSpinner->activate();
      midiKeyCpsSpinner->value(-1);
      midiKeyCpsSpinner->deactivate();
      midiKeyOctSpinner->value(-1);
      midiKeyOctSpinner->deactivate();
      midiKeyPchSpinner->value(-1);
      midiKeyPchSpinner->deactivate();
    }
    else if (midiKeyCpsOn->value() == 1)  {
      midiKeyCpsSpinner->value(3);
      midiKeyCpsSpinner->activate();
      midiKeyMidiSpinner->value(-1);
      midiKeyMidiSpinner->deactivate();
      midiKeyOctSpinner->value(-1);
      midiKeyOctSpinner->deactivate();
      midiKeyPchSpinner->value(-1);
      midiKeyPchSpinner->deactivate();
    }
    else if (midiKeyOctOn->value() == 1)  {
      midiKeyOctSpinner->value(3);
      midiKeyOctSpinner->activate();
      midiKeyMidiSpinner->value(-1);
      midiKeyMidiSpinner->deactivate();
      midiKeyCpsSpinner->value(-1);
      midiKeyCpsSpinner->deactivate();
      midiKeyPchSpinner->value(-1);
      midiKeyPchSpinner->deactivate();
    }
    else if (midiKeyPchOn->value() == 1)  {
      midiKeyPchSpinner->value(3);
      midiKeyPchSpinner->activate();
      midiKeyMidiSpinner->value(-1);
      midiKeyMidiSpinner->deactivate();
      midiKeyCpsSpinner->value(-1);
      midiKeyCpsSpinner->deactivate();
      midiKeyOctSpinner->value(-1);
      midiKeyOctSpinner->deactivate();
    }
    else {
      midiKeyMidiSpinner->value(-1);
      midiKeyMidiSpinner->deactivate();
      midiKeyCpsSpinner->value(-1);
      midiKeyCpsSpinner->deactivate();
      midiKeyOctSpinner->value(-1);
      midiKeyOctSpinner->deactivate();
      midiKeyPchSpinner->value(-1);
      midiKeyPchSpinner->deactivate();
    }
    performanceSettings.midiKeyMidi = (int) midiKeyMidiSpinner->value();
    performanceSettings.midiKeyCps = (int) midiKeyCpsSpinner->value();
    performanceSettings.midiKeyOct = (int) midiKeyOctSpinner->value();
    performanceSettings.midiKeyPch = (int) midiKeyPchSpinner->value();
    /* For velocity */
    if (midiVelMidiOn->value() == 1)  {
      midiVelMidiSpinner->activate();
      midiVelMidiSpinner->value(4);
      midiVelAmpSpinner->value(-1);
      midiVelAmpSpinner->deactivate();
    }
    else if (midiVelAmpOn->value() == 1)  {
      midiVelAmpSpinner->activate();
      midiVelAmpSpinner->value(4);
      midiVelMidiSpinner->value(-1);
      midiVelMidiSpinner->deactivate();
    }
    else {
      midiVelMidiSpinner->value(-1);
      midiVelMidiSpinner->deactivate();
      midiVelAmpSpinner->value(-1);
      midiVelAmpSpinner->deactivate();
    }
    performanceSettings.midiVelMidi = (int) midiVelMidiSpinner->value();
    performanceSettings.midiVelAmp = (int) midiVelAmpSpinner->value();
}

void CsoundPerformanceSettingsPanel::setPerformanceSettingsWindow(int action)
{
    switch (action)
    {
      case 0 :   /*Set Audio Modules List*/
        rtAudioModules.clear();
        rtAudioModules.push_back("PortAudio");
        rtAudioModules.push_back("pa_cb");
        rtAudioModules.push_back("pa_bl");
#ifdef WIN32
        rtAudioModules.push_back("MME");
#endif
#ifdef LINUX
        rtAudioModules.push_back("ALSA");
#endif
#ifdef MACOSX
        rtAudioModules.push_back("CoreAudio");
#endif
#ifdef HAVE_JACK
        rtAudioModules.push_back("JACK");
#endif
        rtAudioModules.push_back("null");
        rtAudioModuleInput->clear();
        for (int i = 0; i < (int) rtAudioModules.size(); i++)
          rtAudioModuleInput->add(rtAudioModules[i].c_str());
        rtAudioModuleInput->value(getNumber(performanceSettings.rtAudioModule,
                                  rtAudioModules));
        rtAudioOutputDeviceInput->value(performanceSettings.rtAudioOutputDevice.c_str());
        rtAudioInputDeviceInput->value(performanceSettings.rtAudioInputDevice.c_str());
        /* Set MIDI modules list */
        rtMidiModules.clear();
        rtMidiModules.push_back("PortMidi");
        rtMidiModules.push_back("virtual");
#ifdef LINUX
        rtMidiModules.push_back("ALSA");
#endif
#ifdef WIN32
        rtMidiModules.push_back("MME");
#endif
        rtMidiModules.push_back("null");
        rtMidiModulesChoice->clear();
        for (int i = 0; i < (int) rtMidiModules.size(); i++)
          rtMidiModulesChoice->add(rtMidiModules[i].c_str());
        rtMidiModulesChoice->value(getNumber(performanceSettings.rtMidiModule,
                                  rtMidiModules));
        rtMidiOutputDeviceInput->value(performanceSettings.midiOutDevName.c_str());
        rtMidiInputDeviceInput->value(performanceSettings.midiInDevName.c_str());
      break;

      case 1:  /* Set Audio Devices List */
        rtAudioOutputDeviceInput->clear();
        rtAudioInputDeviceInput->clear();
        for (int i = 0; i < (int) rtAudioOutputDevices.size(); i++) {
          std::string deviceName = rtAudioOutputDevices[i].description;
          rtAudioOutputDeviceInput->add(deviceName.c_str());
        }
        for (int i = 0; i < (int) rtAudioInputDevices.size(); i++) {
          std::string deviceName = rtAudioInputDevices[i].description;
          rtAudioInputDeviceInput->add(deviceName.c_str());
        }
        break;
      case 2:  /* Set Midi Devices List */
        rtMidiOutputDeviceInput->clear();
        rtMidiInputDeviceInput->clear();
        for (int i = 0; i < (int) rtMidiOutputDevices.size(); i++) {
          std::string deviceName = rtMidiOutputDevices[i].description;
          rtMidiOutputDeviceInput->add(deviceName.c_str());
        }
        for (int i = 0; i < (int) rtMidiInputDevices.size(); i++) {
          std::string deviceName = rtMidiInputDevices[i].description;
          rtMidiInputDeviceInput->add(deviceName.c_str());
        }
      break;
      default:
        break;
    }
}

int CsoundPerformanceSettingsPanel::getNumber(std::string module, std::vector<std::string> list)
{
    int i;
    for (i = 0; i < (int) list.size(); i++)
      if (list[i] == module)
        break;
    if (i == (int) list.size())
      i = -1;
    return i;
}

void CsoundPerformanceSettingsPanel::makeAudioDeviceName(std::string Name, bool isInput)
{
    std::string deviceName = (isInput ? "adc":"dac");
    if (performanceSettings.rtAudioModule == "JACK" && Name != "NONE")  {
      deviceName.append(":");
      deviceName.append(Name);
    }
    else  {
      for (int i = 0; i < (int) (isInput ? rtAudioInputDevices.size(): rtAudioOutputDevices.size()); i++)
      {
        std::string inputDeviceName = "", outputDeviceName = "";
        if (isInput) {
          inputDeviceName = rtAudioInputDevices[i].description;
          stripSlash(inputDeviceName);
        }
        else {
          outputDeviceName = rtAudioOutputDevices[i].description;
          stripSlash(outputDeviceName);
        }
        if ((isInput && inputDeviceName == Name) ||
            (!isInput && outputDeviceName == Name))  {
          if (performanceSettings.rtAudioModule == "ALSA")  {
            if (Name != "default") {
              char temp [] = "  ";
              deviceName.append(":hw:");
              sprintf(temp, "%i", (isInput ? rtAudioInputDevices[i].cardNum :
                  rtAudioOutputDevices[i].cardNum) );
              deviceName.append(std::string(temp));
              deviceName.append(",");
              sprintf(temp, "%i", (isInput ? rtAudioInputDevices[i].portNum :
                  rtAudioOutputDevices[i].portNum) );
              deviceName.append(std::string(temp));
            }
          }
          else {
            char temp [] = "  ";
            sprintf(temp, "%i", (isInput ? rtAudioInputDevices[i].cardNum :
                rtAudioOutputDevices[i].cardNum) );
            if (strcmp(temp, "-1") != 0)
              deviceName.append(std::string(temp));
          }
        }
      }
    }
    if (isInput) {
      rtAudioInputDeviceInput->value(deviceName.c_str());
      performanceSettings.rtAudioInputDevice = deviceName;
    }
    else {
      rtAudioOutputDeviceInput->value(deviceName.c_str());
      performanceSettings.rtAudioOutputDevice = deviceName;
      if (performanceSettings.runRealtime)
        performanceSettings.outputFileName = deviceName;
    }
}

void CsoundPerformanceSettingsPanel::makeMidiDeviceName(std::string Name, bool isInput)
{
    std::string deviceName = "";
    for (int i = 0; i < (int) (isInput ? rtMidiInputDevices.size(): rtMidiOutputDevices.size()); i++)
    {
      std::string inputDeviceName = "", outputDeviceName = "";
      if (isInput) {
        inputDeviceName = rtMidiInputDevices[i].description;
        stripSlash(inputDeviceName);
      }
      else {
        outputDeviceName = rtMidiOutputDevices[i].description;
        stripSlash(outputDeviceName);
      }
      if ((isInput && inputDeviceName == Name) ||
          (!isInput && outputDeviceName == Name))  {
        if (performanceSettings.rtMidiModule == "ALSA")  {
          if (Name != "default" && Name != "NONE") {
            char temp [] = "  ";
            deviceName.append("hw:");
            sprintf(temp, "%i", (isInput ? rtMidiInputDevices[i].cardNum :
                rtMidiOutputDevices[i].cardNum) );
            deviceName.append(std::string(temp));
            deviceName.append(",");
            sprintf(temp, "%i", (isInput ? rtMidiInputDevices[i].portNum :
                rtMidiOutputDevices[i].portNum) );
            deviceName.append(std::string(temp));
          }
        }
        else {
          if (Name != "NONE") {
            char temp [] = "  ";
            sprintf(temp, "%i", (isInput ? rtMidiInputDevices[i].cardNum :
                rtMidiOutputDevices[i].cardNum) );
            deviceName.append(std::string(temp));
          }
        }
      }
    }
    if (isInput) {
      rtMidiInputDeviceInput->value(deviceName.c_str());
      performanceSettings.midiInDevName = deviceName;
    }
    else {
      rtMidiOutputDeviceInput->value(deviceName.c_str());
      performanceSettings.midiOutDevName = deviceName;
    }
}

/* Partly based on java code from blue by Steven Yi */
void CsoundPerformanceSettingsPanel::querySoundDevices()
{
    if (getNumber(performanceSettings.rtAudioModule, rtAudioModules) < 0)
      return;
    rtAudioOutputDevices.clear();
    rtAudioInputDevices.clear();
    std::string rtModule = rtAudioModules[getNumber(performanceSettings.rtAudioModule, rtAudioModules)];
    rtAudioOutputDeviceInput->label("Output Device");
    rtAudioInputDeviceInput->label("Input Device");
    jackClientNameInput->deactivate();  /* Only activate below if JACK is input module*/
    if (rtModule == "null") {
      setPerformanceSettingsWindow(1);
      return;
    }
#ifdef LINUX
    if (rtModule == "ALSA") {
      deviceInfo a;
      a.cardNum = -1;
      a.description = "default";
      rtAudioOutputDevices.push_back(a);
      rtAudioInputDevices.push_back(a);
      FILE * f = fopen("/proc/asound/pcm", "r");
      /* file presents this format:
      02-00: Analog PCM : Mona : playback 6 : capture 4*/
      char *line,*line_;
      line = (char *) calloc (128, sizeof(char));
      line_ = (char *) calloc (128, sizeof(char));
      if (f)  {
        while (fgets(line, 128, f))  {   /* Read one line*/
          char card_[] = "  ";
          char num_[] = "  ";
          char *temp;
          char *name = (char *) calloc (128, sizeof(char));
          strcpy(line_, line);
          if (strstr(line_, "playback"))  {  /*Output devices*/
            temp = strtok (line_, "-");
            strncpy (card_, temp, 2);
            temp = strtok (NULL, ":");
            strncpy (num_, temp, 2);
            int card = atoi (card_);
            int num = atoi (num_);
            temp = strtok ( NULL , ":" );
            strcpy(name, temp);
            strcat(name, ":");
            temp = strtok ( NULL , ":" );
            strcat(name, temp);
            /* name contains spaces at the beginning and the end. */
            deviceInfo a;
            a.cardNum = card;
            a.portNum = num;
            int i = 1;  /* Get rid of initial space */
            while (name[i])  {
              a.description.push_back(name[i]);
              i++;
            }
            rtAudioOutputDevices.push_back(a);
          }
          if (strstr(line, "capture"))  {  /*Input Devices*/
            temp = strtok (line, "-");
            strncpy (card_, temp, 2);
            temp = strtok (NULL, ":");
            strncpy (num_, temp, 2);
            int card = atoi (card_);
            int num = atoi (num_);
            temp = strtok ( NULL , ":" );
            strcpy(name, temp);
            strcat(name, ":");
            temp = strtok ( NULL , ":" );
            strcat(name, temp);
            /* name contains spaces at the beginning and the end.*/
            deviceInfo a;
            a.cardNum = card;
            a.portNum = num;
            int i = 1;  /* Get rid of initial space */
            while (name[i])  {
              a.description.push_back(name[i]);
              i++;
            }
            rtAudioInputDevices.push_back(a);
          }
        }
      }
      free(line);
      free(line_);
    }
    else
#endif //LINUX
      //The following code is problematic on other platforms. Taken out while things are sorted
#ifdef CS5GUI_EXPERIMENTAL
    {
      /* Use the Csound API to query Output Devices */
      csoundMessages = (char *) calloc(800, sizeof(char));
      char *csoundMessagesIn = (char *) calloc(800, sizeof(char));
      char *csoundMessagesOut = (char *) calloc(800, sizeof(char));
      Csound c = new Csound;
      FILE *tmpfile;
      char tmpfileName[] = "________temp.csd";
      tmpfile = fopen( tmpfileName, "w" );
      if (!tmpfile)
        return;
      fprintf(tmpfile, "%s%s%s", defaultTmpCsdPre, rtModule.c_str(), defaultTmpCsdPostOut);
      fclose(tmpfile);
      relevantText = false;
      if (rtModule == "PortAudio" or rtModule == "pa_cb" or rtModule == "pa_bl")  {
        strcpy(startText, "PortAudio: available");
        strcpy(endText, " *** PortAudio: error:");
      }
      else if (rtModule == "MME")  {
        strcpy(startText, "The available output devices are");
        strcpy(endText, "device number is out of range");
      }
      else if (rtModule == "CoreAudio") {
        strcpy(startText, "CoreAudio Module: found");
        strcpy(endText, "");
      }
      else if (rtModule == "JACK") {
        strcpy(startText, "The available JACK output devices are:");
        strcpy(endText, " *** rtjack: must specify a device name, not a number");
        rtAudioOutputDeviceInput->label("Connect Output ports to Jack Inputs");
        jackClientNameInput->activate();
      }
      c.SetMessageCallback(&getCsoundMessages);
      c.Compile(tmpfileName);
      c.Reset();
      strcpy(csoundMessagesOut, csoundMessages);
      strcpy(csoundMessages, "");

      /* Use the Csound API to query Input Devices */
      tmpfile = fopen( tmpfileName, "w" );
      if (!tmpfile)
        return;
      fprintf(tmpfile, "%s%s%s", defaultTmpCsdPre, rtModule.c_str(), defaultTmpCsdPostIn);
      fclose(tmpfile);
      relevantText = false;
      if (rtModule == "PortAudio" or rtModule == "pa_cb" or rtModule == "pa_bl")  {
        strcpy(startText, "PortAudio: available input devices");
        strcpy(endText, " *** PortAudio: error:");
      }
#ifdef WIN32
      else if (rtModule == "MME")  {
        strcpy(startText, "The available input devices are");
        strcpy(endText, "device number is out of range");
      }
#endif //WIN32
#ifdef MACOSX
      else if (rtModule == "CoreAudio") {
        strcpy(startText, "CoreAudio Module: found");
        strcpy(endText, "");
      }
#endif  //MACOSX
#ifdef HAVE_JACK
      else if (rtModule == "JACK") {
        strcpy(startText, "The available JACK input devices are:");
        strcpy(endText, " *** rtjack: must specify a device name, not a number");
        rtAudioOutputDeviceInput->label("Connect Input ports to Jack Outputs");
      }
#endif   //HAVE_JACK
      c.Compile(tmpfileName);
      c.Reset();
      strcpy(csoundMessagesIn, csoundMessages);
      strcpy(csoundMessages, "\0");
      if (system("rm ________temp.csd")<0) {
        errors++;
        fprintf(stderr, "Failed to remove temp file\n");
      }
      /* Set output device names */
      char *tmp;
      char *tmp2 = (char *) calloc (80, sizeof(char));
      tmp = strtok(csoundMessagesOut, "\n");
      while (tmp) {
        deviceInfo a;
        if (rtModule == "CoreAudio") {
          char *end = strstr(tmp, "=> CoreAudio device");
          if (end) {
            char *start = strchr(tmp, ':');
            size_t len1 = start - tmp;
            size_t len2 = end - tmp;
            strncpy(tmp2, &tmp[len1 + 1], len2 - len1);
            tmp2[len2 - len1] = '\0';
            escapeSlash(tmp2);
            a.description = std::string(tmp2);
            strncpy(tmp2, tmp, len2);
            tmp2[len1] = '\0';
            a.cardNum = atoi(tmp2);
            rtAudioInputDevices.push_back(a);
          }
        }
        else if (rtModule == "JACK") {
          deviceInfo a;
          a.cardNum = -1;
          a.description = "NONE";
          rtAudioOutputDevices.push_back(a);
          rtAudioInputDevices.push_back(a);
          char *begin = strchr(tmp, '"');
          char *end = strrchr(tmp, '"');
          size_t len = end - begin - 1;
          strncpy(tmp2, begin + 1,  len );
          tmp2[len] = '\0';
          a.description = std::string(tmp2);
          rtAudioOutputDevices.push_back(a);
        }
        else {
          char *token = strchr(tmp, ':');
          if (token) {
            size_t len1 = token - tmp;
            strncpy(tmp2, &tmp[len1 + 1], strlen(tmp) - len1);
            tmp2[strlen(tmp) - len1] = '\0';
            escapeSlash(tmp2);
            a.description = std::string(tmp2);
            strncpy(tmp2, tmp, len1);
            tmp2[len1] = '\0';
            a.cardNum = atoi(tmp2);
            rtAudioOutputDevices.push_back(a);
          }
        }
        tmp = strtok(NULL, "\n");
        strcpy(tmp2, "\0");
      }
      /* Set output device names */
      tmp = strtok(csoundMessagesIn, "\n");
      while (tmp) {
        deviceInfo a;
        if (rtModule == "CoreAudio") {
          char *end = strstr(tmp, "=> CoreAudio device");
          if (end) {
            char *start = strchr(tmp, ':');
            size_t len1 = start - tmp;
            size_t len2 = end - tmp;
            strncpy(tmp2, &tmp[len1 + 1], len2 - len1);
            tmp2[len2 - len1] = '\0';
            escapeSlash(tmp2);
            a.description = std::string(tmp2);
            strncpy(tmp2, tmp, len2);
            tmp2[len2] = '\0';
            a.cardNum = atoi(tmp2);
            rtAudioInputDevices.push_back(a);
          }
        }
        else if (rtModule == "JACK") {
          char *begin = strchr(tmp, '"');
          char *end = strrchr(tmp, '"');
          size_t len = end - begin - 1;
          strncpy(tmp2, begin + 1,  len);
          tmp2[len] = '\0';
          a.description = std::string(tmp2);
          rtAudioInputDevices.push_back(a);
        }
        else {
          char *token = strchr(tmp, ':');
          if (token) {
            size_t len1 = token - tmp;
            strncpy(tmp2, &tmp[len1 + 1], strlen(tmp) - len1);
            tmp2[strlen(tmp) - len1] = '\0';
            escapeSlash(tmp2);
            a.description = std::string(tmp2);
            strncpy(tmp2, tmp, len1);
            tmp2[len1] = '\0';
            a.cardNum = atoi(tmp2);
            rtAudioInputDevices.push_back(a);
          }
        }
        tmp = strtok(NULL, "\n");
        strcpy(tmp2, "\0");
      }
      free(csoundMessages);
      free(csoundMessagesIn);
      free(csoundMessagesOut);
      free(tmp2);
    }
#endif //CS5GUI_EXPERIMENTAL
    setPerformanceSettingsWindow(1);
}

void CsoundPerformanceSettingsPanel::queryMidiDevices()
{
   if (getNumber(performanceSettings.rtMidiModule, rtMidiModules) < 0)
      return;
    rtMidiOutputDevices.clear();
    rtMidiInputDevices.clear();
    std::string rtModule = rtMidiModules[getNumber(performanceSettings.rtMidiModule, rtMidiModules)];
    if (rtModule == "null") {
      setPerformanceSettingsWindow(2);
      return;
    }
    deviceInfo a;
    a.cardNum = -1;
    a.description = "NONE";
  // virtual keyboard not working with csound5gui on Mac OS X for now
#ifndef MACOSX
    if (rtModule != "virtual") {
      rtMidiOutputDevices.push_back(a);
      rtMidiInputDevices.push_back(a);
    }
#endif
#ifdef LINUX
    if (rtModule == "ALSA") {
      a.description = "";
      a.portNum = -1;
      a.cardNum = -1;
      FILE * f = fopen("/proc/asound/seq/clients", "r");
      char *line;
      line = (char *) calloc (128, sizeof(char));
      if (f)  {
        line = fgets(line, 128, f);   /* Read one line*/
        while (line)  {
          char card_[] = "    ";
          char num_[] = "    ";
          char *temp = (char *) calloc (128, sizeof(char));
          char *name = (char *) calloc (128, sizeof(char));
          name[0] = '\0';
          int i;
          char *line_ = (char *) calloc (128, sizeof(char));
          strcpy(line_, line);
          if (strstr(line, "Client") == line && strchr(line, ':'))  {
            temp = strtok (line, "\"");
            temp = strtok (NULL, "\"");
            strncpy (name, temp, 20);
            name[strlen(temp)] = '\0';
            /* Discard client type for now */
            strncpy(card_, line + 6, 4);
            int card = atoi (card_);
            a.cardNum = card;
            bool isPort = true;
            while (isPort)  {
              line = fgets(line, 128, f);
//               strcpy(line_,line);
              if (strstr(line, "  Port") == line)  {
                temp = strtok (line, "\"");
                temp = strtok (NULL, "\"");
                strncpy(num_, line + 6, 4);
                int num = atoi (num_);
                a.portNum = num;
                strcat (name, " - ");
                strncat (name, temp, 20);
                strcat (name, "\0");
                i = 0;
                while (name[i])  {
                  a.description.push_back(name[i]);
                  i++;
                }
                temp = strtok (NULL, "\"");
                strcpy(name, "\0");
                if (strchr(temp, 'R'))
                  rtMidiInputDevices.push_back(a);
                if (strchr(temp, 'W'))
                  rtMidiOutputDevices.push_back(a);
                a.description = "";
                a.portNum = -1;
                a.cardNum = -1;
                name[0] = '\0';
              }
              else
                isPort = false;
            }
          }
          else
            line = fgets(line, 128, f);
        }
      }
      fclose(f);
      if (system("amidi -l > _csound5guitmpfile.txt")<0) {
        errors++;
        fprintf(stderr, "amidi failed\n");
      }
      f = fopen("_csound5guitmpfile.txt", "r");
      line = (char *) calloc (128, sizeof(char));
      if (f)  {
        line = fgets(line, 128, f);   /* Read one line*/
        while (line)  {
          bool isInput = false;
          bool isOutput = false;
          char card_[] = "    ";
          char num_[] = "    ";
          char *temp /*= (char *) calloc (128, sizeof(char))*/;
          char *name = (char *) calloc (128, sizeof(char));
          char *temp2 = (char *) calloc (10, sizeof(char));
          name[0] = '\0';
          int i;
          char *line_ = (char *) calloc (128, sizeof(char));
          strcpy(line_, line);
          if (strstr(line, " hw:"))  {
            temp = strtok (line, " ");
            if (strchr(temp, 'I'))
              isInput = true;
            if (strchr(temp, 'O'))
              isOutput = true;
            strcpy (temp2, temp);
            temp = strtok (NULL, " ");
            strcpy (name, line_ + 4);
//             temp = strchr(name, '\n');
//             *temp = '\0';
//             name[strlen(temp)] = '\0';
            i = 0;
            while (name[i])  {
              a.description.push_back(name[i]);
              i++;
            }
            temp = strtok(name, ":");
            temp = strtok(NULL, ",");
            strcpy(card_, temp);
            int card = atoi (card_);
            a.cardNum = card;
            temp = strtok(NULL, " ,-");
            strcpy(num_, temp);
            int num = atoi (num_);
            a.portNum = num;
            strcpy(name, "\0");
            if (isInput)
              rtMidiInputDevices.push_back(a);
            if (isOutput)
              rtMidiOutputDevices.push_back(a);
            a.description = "";
            a.portNum = -1;
            a.cardNum = -1;
          }
          else
            line = fgets(line, 128, f);
        }
      }
      fclose(f);
      if (system("rm _csound5guitmpfile.txt")<0) {
        errors++;
        fprintf(stderr, "failed to remove txt file\n");
      }
    }
    else
#endif //LINUX
      //The following code is problematic on other platforms. Taken out while things are sorted
#ifdef CS5GUI_EXPERIMENTAL
    {
      /* Use the Csound API to query Output Devices */
      csoundMessages = (char *) calloc(800, sizeof(char));
      char *csoundMessagesIn = (char *) calloc(800, sizeof(char));
      char *csoundMessagesOut = (char *) calloc(800, sizeof(char));
      Csound c = new Csound;
      FILE *tmpfile;
      char tmpfileName[] = "________temp.csd";
      tmpfile = fopen( tmpfileName, "w" );
      if (!tmpfile)
        return;
      fprintf(tmpfile, "%s%s%s", defaultTmpCsdMidiPre, rtModule.c_str(), defaultTmpCsdMidiPostOut);
      fclose(tmpfile);
      relevantText = false;
      if (rtModule == "PortMidi")  {
        strcpy(startText, "The available MIDI out devices are:");
        strcpy(endText, "error: device number is out of range");
      }
#ifdef WIN32
      else if (rtModule == "MME")  {
        strcpy(startText, "The available MIDI output devices are:");
        strcpy(endText, "rtmidi: output device number is out of range");
      }
#endif //WIN32
      c.SetMessageCallback(&getCsoundMessages);
      c.Compile(tmpfileName);
      c.Reset();
      strcpy(csoundMessagesOut, csoundMessages);
      strcpy(csoundMessages, "");

      /* Use the Csound API to query Input Devices */
      tmpfile = fopen( tmpfileName, "w" );
      if (!tmpfile)
        return;
      fprintf(tmpfile, "%s%s%s", defaultTmpCsdMidiPre, rtModule.c_str(), defaultTmpCsdMidiPostIn);
      fclose(tmpfile);
      relevantText = false;
      if (rtModule == "PortMidi")  {
        strcpy(startText, "The available MIDI in devices are:");
        strcpy(endText, "error: device number is out of range");
      }
#ifdef WIN32
      else if (rtModule == "MME")  {
        strcpy(startText, "The available MIDI input devices are:");
        strcpy(endText, "rtmidi: input device number is out of range");
      }
#endif //WIN32
      c.Compile(tmpfileName);
      c.Reset();
      strcpy(csoundMessagesIn, csoundMessages);
      strcpy(csoundMessages, "\0");
      if (system("rm ________temp.csd")<0) {
        errors++;
        fprintf(stderr, "Failed to remobve temp file\n");
      }

      /* Set output device names */
      char *tmp;
      char *tmp2 = (char *) calloc (80, sizeof(char));
      tmp = strtok(csoundMessagesOut, "\n");
      while (tmp) {
        char *token = strchr(tmp, ':');
        if (token) {
          size_t len1 = token - tmp;
          strncpy(tmp2, &tmp[len1 + 1], strlen(tmp) - len1);
          tmp2[strlen(tmp) - len1] = '\0';
          escapeSlash(tmp2);
          a.description = std::string(tmp2);
          strncpy(tmp2, tmp, len1);
          tmp2[len1] = '\0';
          a.cardNum = atoi(tmp2);
          rtAudioOutputDevices.push_back(a);
        }
        tmp = strtok(NULL, "\n");
        strcpy(tmp2, "\0");
      }
      /* Set output device names */
      tmp = strtok(csoundMessagesIn, "\n");
      while (tmp) {
        char *token = strchr(tmp, ':');
        if (token) {
          size_t len1 = token - tmp;
          strncpy(tmp2, &tmp[len1 + 1], strlen(tmp) - len1);
          tmp2[strlen(tmp) - len1] = '\0';
          escapeSlash(tmp2);
          a.description = std::string(tmp2);
          strncpy(tmp2, tmp, len1);
          tmp2[len1] = '\0';
          a.cardNum = atoi(tmp2);
          rtAudioInputDevices.push_back(a);
        }
        tmp = strtok(NULL, "\n");
        strcpy(tmp2, "\0");
      }
      free(csoundMessages);
      free(csoundMessagesIn);
      free(csoundMessagesOut);
      free(tmp2);
    }
#endif //CS5GUI_EXPERIMENTAL
    setPerformanceSettingsWindow(2);
}


