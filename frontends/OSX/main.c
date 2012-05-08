/*
    Copyright (C) 2005 Victor Lazzarini

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

    main.c Csound 5 OSX GUI frontend
*/

#include <Carbon/Carbon.h>
#include <stdio.h>
#include <stdlib.h>
#include "csound.h"

#define RUNAPPLICATIONEVENTLOOP 1
#define DEBUG 0

typedef struct _globals {
    WindowRef *mainwindow;
    CSOUND  *cs;
    int     on;
    int     res;
    int     pause;
    int     quit;
    EventLoopTimerRef *timer;
    MenuBarHandle mbar;
    int     manopen;
    int     mess;
} globals;

void DrawTxt(char *str, WindowRef theWindow)
{
    HIViewRef text;
    TXNObject textobject;
    TXNOffset start, end;
    ControlID textID = { 'Outx', 130 };

    HIViewFindByID(HIViewGetRoot(theWindow), textID, &text);
    textobject = HITextViewGetTXNObject(text);
    TXNSelectAll(textobject);
    TXNGetSelection(textobject, &start, &end);
    TXNSetData(textobject, kTXNTextData, str, strlen(str), end, end);
}

void ClearTxt(WindowRef theWindow)
{
    HIViewRef text;
    TXNObject textobject;
    TXNOffset start, end;
    ControlID textID = { 'Outx', 130 };

    HIViewFindByID(HIViewGetRoot(theWindow), textID, &text);
    textobject = HITextViewGetTXNObject(text);
    TXNSelectAll(textobject);
    TXNGetSelection(textobject, &start, &end);
    TXNSetData(textobject, kTXNTextData, NULL, 0, start, end);
}

void MessageCallback(CSOUND *cs, int attr, const char *format, va_list valist)
{
    char    *str;
    globals *g = csoundGetHostData(cs);

    str = (char *) malloc(strlen(format) + 1024);
    vsprintf(str, format, valist);
    if (g->mess)
      DrawTxt(str, *(g->mainwindow));
    else
      fprintf(stderr, "%s", str);
    free(str);
}

int isCsoundFile(char *in)
{
    int     len;
    int     tmp;
    int     i;

    len = strlen(in);
    for (i = 0; i < len; i++, in++) {
      if (*in == '.')
        break;
    }
    if (*in == '.') {
      tmp = *((int *) in);
      if (DEBUG)
        fprintf(stderr, "%d: %c%c%c%c\n", i, in[0], in[1], in[2], in[3]);
      if (tmp == '.csd' || tmp == '.orc' || tmp == '.sco' ||
          tmp == '.CSD' || tmp == '.ORC' || tmp == '.SCO' ||
          tmp == 'dsc.' || tmp == 'cro.' || tmp == 'ocs.' ||
          tmp == 'DSC.' || tmp == 'CRO.' || tmp == 'OCS.' )
        return 1;
      else
        return 0;
    }
    return 0;
}

char   *SetWorkingDirectory(globals *g)
{
    ControlRef textfield;
    OSStatus res;
    char    *text;
    Size    actualsize;
    ControlID textID = { 'Wdir', 129 };

    if ((res = GetControlByID(*(g->mainwindow), &textID, &textfield)) < 0) {
      if (DEBUG)
        fprintf(stderr, "code: %d\n", res);
      return;
    }
    GetControlData(textfield, 0, kControlEditTextTextTag, NULL, NULL,
                   &actualsize);
    text = (char *) malloc(actualsize + 2);
    GetControlData(textfield, 0, kControlEditTextTextTag, actualsize, text,
                   &actualsize);
    if (text[actualsize - 1] != '/') {
      text[actualsize] = '/';
      text[actualsize + 1] = '\0';
    }
    else
      text[actualsize] = '\0';
    return text;
}

void TextSplitter(char *in, char **out, int *argc)
{
    char    tmp[128];
    int     cnt = 0, cntstr = 0, n = 0, len;

    len = strlen(in);
    while (cnt < len) {
      if (in[cnt] != ' ') {
        while (in[cnt] != ' ' && cnt < len) {
          tmp[n] = in[cnt];
          cnt++;
          n++;
        }
        tmp[n] = '\0';
        out[cntstr] = (char *) malloc(n + 1);
        strcpy(out[cntstr], tmp);
        n = 0;
        cnt++;
        cntstr++;
      }
      else
        cnt++;
    }
    *argc = cntstr;
}

void Compile(globals *g)
{
    ControlRef textfield;
    OSStatus res;
    int     args, i;
    char    *text, *tmp[128], *dir;
    Size    actualsize;
    ControlID textID = { 'Comp', 128 };

    if ((res = GetControlByID(*(g->mainwindow), &textID, &textfield)) < 0) {
      if (DEBUG)
        fprintf(stderr, "code: %d\n", res);
      return;
    }
    GetControlData(textfield, 0, kControlEditTextTextTag, NULL, NULL,
                   &actualsize);
    text = (char *) malloc(actualsize + 1);
    GetControlData(textfield, 0, kControlEditTextTextTag, actualsize, text,
                   &actualsize);
    text[actualsize] = '\0';
    dir = SetWorkingDirectory(g);
    if (DEBUG)
      fprintf(stderr, "text event: %s %d\n", text, actualsize);
    tmp[0] = "csound";
    TextSplitter(text, &tmp[1], &args);
    for (i = 1; i < args + 1; i++)
      if (*tmp[i] != '-' && isCsoundFile(tmp[i])) {
        int     siz = strlen(dir) + strlen(tmp[i]);
        char   *tmpstr = tmp[i];

        tmp[i] = (char *) malloc(siz);
        sprintf(tmp[i], "%s%s", dir, tmpstr);
        free(tmpstr);
      }
    g->res = csoundCompile(g->cs, args + 1, tmp);
    for (i = 1; i < args + 1; i++) {
      if (DEBUG)
        fprintf(stderr, "arg[%d]: %s\n", i, tmp[i]);
      free(tmp[i]);
    }
    free(text);
    free(dir);
}

OSStatus MainWindowHandler(EventHandlerCallRef inHandlerCallRef,
                           EventRef inEvent, void *inUserData)
{
    HICommand commandStruct;
    globals *g = (globals *) inUserData;

    GetEventParameter(inEvent, kEventParamDirectObject,
                      typeHICommand, NULL, sizeof(HICommand),
                      NULL, &commandStruct);
    if (commandStruct.commandID == 'Main') {
      ShowWindow(*(g->mainwindow));
    }
    if (commandStruct.commandID == 'Mess') {
      if (g->mess)
        g->mess = 0;
      else
        g->mess = 1;
    }
    if (commandStruct.commandID == 'Rset') {
      if (g->on) {
        csoundReset(g->cs);
        g->on = 0;
      }
    }

    if (commandStruct.commandID == 'Comp') {
      ClearTxt(*(g->mainwindow));
      if (g->on) {
        csoundReset(g->cs);
        Compile(g);
        if (g->res == 0) {
          g->on = 1;
          g->pause = 1;
        }
        else
          g->on = 0;
      }
      else {
        Compile(g);
        if (g->res == 0) {
          g->on = 1;
          g->pause = 1;
        }
        else
          g->on = 0;
      }
      SetMenuBar(g->mbar);
    }
    if (commandStruct.commandID == 'Play') {
      if (g->pause) {
        g->pause = 0;
        DrawTxt("************************  PLAY **************************\n",
                *(g->mainwindow));
      }
      else {
        g->pause = 1;
        DrawTxt("************************ PAUSE **************************\n",
                *(g->mainwindow));
      }
    }
    if (commandStruct.commandID == 'Paus') {
      if (g->pause) {
        g->pause = 0;
        DrawTxt("************************  PLAY **************************\n",
                *(g->mainwindow));
      }
      else {
        g->pause = 1;
        DrawTxt("************************ PAUSE **************************\n",
                *(g->mainwindow));
      }
    }
    if (commandStruct.commandID == 'Rewi') {
      csoundRewindScore(g->cs);
      DrawTxt("rewind\n", *(g->mainwindow));
    }
    if (commandStruct.commandID == 'quit') {
      g->quit = 1;
      g->pause = 1;
      QuitApplicationEventLoop();
    }
    if (commandStruct.commandID == 'abou') {
      IBNibRef nibRef;
      WindowRef aboutwin;

      CreateNibReference(CFSTR("main"), &nibRef);
      CreateWindowFromNib(nibRef, CFSTR("aboutwindow"), &aboutwin);
      ShowWindow(aboutwin);
      DisposeNibReference(nibRef);
    }
    if (commandStruct.commandID == 'Clos') {
      DisposeWindow(FrontWindow());
    }
    if (commandStruct.commandID == 'Manu') {
      if (g->manopen)
        system("killall Safari");
      system("/Applications/Safari.app/Contents/MacOS/Safari "
             "/Library/Frameworks/CsoundLib.Framework/"
             "Resources/Manual/index.html &");
      g->manopen = 1;
    }
}

pascal void Process(EventLoopTimerRef theTimer, void *userData)
{
    globals *g = (globals *) userData;
    int     res = 0;

    if (!g->pause && g->on)
      res = csoundPerformBuffer(g->cs);
    if (res != 0) {
      csoundReset(g->cs);
      g->on = 0;
    }
}

void TimerInit(globals *g)
{
    EventLoopRef mainLoop;
    EventLoopTimerUPP timerUPP;

    mainLoop = GetMainEventLoop();
    timerUPP = NewEventLoopTimerUPP(Process);
    InstallEventLoopTimer(mainLoop, 0.0, 0.001, timerUPP, g, g->timer);
}

OSStatus ResizeWindowHandler(EventHandlerCallRef inHandlerCallRef,
                             EventRef inEvent, void *inUserData)
{
    WindowRef window = *((WindowRef *) inUserData);
    HIViewRef win, scrl;

    HIViewFindByID(HIViewGetRoot(window), kHIViewWindowContentID, &win);
    scrl = HIViewGetFirstSubview(win);
    HIRect  viewRect, winRect;

    HIViewGetFrame(win, &winRect);
    viewRect.origin.x = 20.0;
    viewRect.origin.y = 130.0;
    viewRect.size.width = winRect.size.width - 40.0;
    viewRect.size.height = winRect.size.height - 143.0;
    HIViewSetFrame(scrl, &viewRect);
}

void EventHandlerInitialise(globals *g)
{
    EventTypeSpec eventType;
    EventHandlerUPP handlerUPP;

    eventType.eventClass = kEventClassCommand;
    eventType.eventKind = kEventProcessCommand;
    handlerUPP = NewEventHandlerUPP(MainWindowHandler);
    InstallWindowEventHandler(*(g->mainwindow), handlerUPP, 1, &eventType, g,
                              NULL);
    InstallApplicationEventHandler(handlerUPP, 1, &eventType, g, NULL);

    eventType.eventClass = kEventClassWindow;
    eventType.eventKind = kEventWindowBoundsChanged;
    handlerUPP = NewEventHandlerUPP(ResizeWindowHandler);
    InstallWindowEventHandler(*(g->mainwindow), handlerUPP, 1, &eventType,
                              g->mainwindow, NULL);
    if (DEBUG)
      fprintf(stderr, "installed\n");
}

int main(int argc, char *argv[])
{
    IBNibRef nibRef;
    OSStatus err;
    globals g;
    WindowRef window;
    EventLoopTimerRef timer;

    g.timer = &timer;
    g.mainwindow = &window;
    g.manopen = 0;
    g.mess = 1;

    err = CreateNibReference(CFSTR("main"), &nibRef);
    require_noerr(err, CantGetNibRef);
    err = SetMenuBarFromNib(nibRef, CFSTR("MenuBar"));
    g.mbar = GetMenuBar();
    require_noerr(err, CantSetMenuBar);
    err = CreateWindowFromNib(nibRef, CFSTR("MainWindow"), g.mainwindow);
    require_noerr(err, CantCreateWindow);

    HIViewRef win, txt, scrl;
    ControlID textID = { 'Outx', 130 };
    HIViewFindByID(HIViewGetRoot(window), kHIViewWindowContentID, &win);
    HIViewFindByID(HIViewGetRoot(window), textID, &txt);
    HIScrollViewCreate(kHIScrollViewOptionsVertScroll, &scrl);
    HIViewSetVisible(scrl, true);
    HIViewAddSubview(win, scrl);

    HIRect  viewRect, winRect;

    HIViewGetFrame(win, &winRect);
    viewRect.origin.x = 20.0;
    viewRect.origin.y = 130.0;
    viewRect.size.width = winRect.size.width - 40.0;
    viewRect.size.height = winRect.size.height - 143.0;
    HIViewSetFrame(scrl, &viewRect);
    HIViewAddSubview(scrl, txt);

    g.cs = csoundCreate(&g);

    DrawTxt("Csound 5 console: ready.\nType in the Csound parameters (e.g."
            " orchestra+score or CSD files, plus any options you require)\n"
            "Some useful options:\n"
            "-odac                   RT audio output\n"
            "-iadc                   RT audio input\n"
            "-o <filename>           file output to <filename>\n"
            "-+rtaudio=coreaudio     use coreaudio native RT module\n"
            "-+rtaudio=portaudio     use portaudio RT module\n"
            "-W                      use RIFF-Wave format output\n"
            "-A                      use AIFF format output\n"
            "-B<buffsize>            set HW buffer to <buffsize> samples\n"
            "-b<buffsize>            set SW buffer to <buffsize> samples\n"
            "\nif SFDIR (SADIR/SSDIR) is not set, output (and input) sound "
            "filenames need to be full-path\n"
            "TIP: you can set these variables in the "
            "~/.MacOSX/environment.plist file",
            window);

    csoundSetMessageCallback(g.cs, MessageCallback);
    g.on = g.quit = g.res = 0;
    EventHandlerInitialise(&g);
    ShowWindow(*(g.mainwindow));
    DisposeNibReference(nibRef);
    TimerInit(&g);
    {
      EventRef theEvent;
      EventTargetRef theTarget;

      theTarget = GetEventDispatcherTarget();
      int     res;

      while (!g.quit) {
        if (!RUNAPPLICATIONEVENTLOOP)
          while ((res = ReceiveNextEvent(0, NULL, -1.0, true,
                                         &theEvent) == noErr)) {
            SendEventToEventTarget(theEvent, theTarget);
            if (g.quit)
              break;
            ReleaseEvent(theEvent);

          }
        else
          RunApplicationEventLoop();
      }
    }
    RemoveEventLoopTimer(timer);
    csoundReset(g.cs);
    csoundDestroy(g.cs);
    if (g.manopen)
      system("killall Safari");

CantCreateWindow:
CantSetMenuBar:
CantGetNibRef:
    return err;
}

