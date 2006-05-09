/*
    CsoundGUIConsole.cpp:
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

const Fl_Text_Display::Style_Table_Entry CsoundGUIConsole::styleTable[18] = {
    { FL_BLACK,                 FL_COURIER,             12,     0U },   // 'A'
    { (Fl_Color) 0xE0000000U,   FL_COURIER,             12,     0U },   // 'B'
    { (Fl_Color) 0x00C00000U,   FL_COURIER,             12,     0U },   // 'C'
    { (Fl_Color) 0x8C880000U,   FL_COURIER,             12,     0U },   // 'D'
    { (Fl_Color) 0x0000FF00U,   FL_COURIER,             12,     0U },   // 'E'
    { (Fl_Color) 0xE000E000U,   FL_COURIER,             12,     0U },   // 'F'
    { (Fl_Color) 0x00B0B000U,   FL_COURIER,             12,     0U },   // 'G'
    { (Fl_Color) 0x7C7C7C00U,   FL_COURIER,             12,     0U },   // 'H'
    { FL_BLACK,                 FL_COURIER_BOLD,        12,     0U },   // 'I'
    { (Fl_Color) 0xE0000000U,   FL_COURIER_BOLD,        12,     0U },   // 'J'
    { (Fl_Color) 0x00C00000U,   FL_COURIER_BOLD,        12,     0U },   // 'K'
    { (Fl_Color) 0x8C880000U,   FL_COURIER_BOLD,        12,     0U },   // 'L'
    { (Fl_Color) 0x0000FF00U,   FL_COURIER_BOLD,        12,     0U },   // 'M'
    { (Fl_Color) 0xE000E000U,   FL_COURIER_BOLD,        12,     0U },   // 'N'
    { (Fl_Color) 0x00B0B000U,   FL_COURIER_BOLD,        12,     0U },   // 'O'
    { (Fl_Color) 0x7C7C7C00U,   FL_COURIER_BOLD,        12,     0U },   // 'P'
    { (Fl_Color) 0x0000C000U,   FL_COURIER_BOLD,        12,     0U },   // 'Q'
    { (Fl_Color) 0xA0000000U,   FL_COURIER_BOLD,        12,     0U }    // 'R'
};

static inline Csound_Message *allocateMessage_(int attr, const char *fmt,
                                               va_list args)
{
    char            buf_[512];
    Csound_Message  *msg;
    int             n, maxLen;

    maxLen = 512;
#if defined(_MSC_VER) && !defined(__GNUC__)
    n = _vsnprintf(&(buf_[0]), (size_t) maxLen, fmt, args);
#else
    n = vsnprintf(&(buf_[0]), (size_t) maxLen, fmt, args);
#endif
    if (!n)
      return (Csound_Message*) 0;
    if (n > 0 && n < maxLen) {
      msg = (Csound_Message*) malloc(sizeof(Csound_Message) + (size_t) n);
      if (msg == (Csound_Message*) 0)
        return (Csound_Message*) 0;
      strcpy(&(msg->msg[0]), &(buf_[0]));
    }
    else {
      while (1) {
        maxLen = (n > 0 ? (n + 1) : (maxLen << 1));
        msg = (Csound_Message*) malloc(sizeof(Csound_Message)
                                       + (size_t) (maxLen - 1));
        if (msg == (Csound_Message*) 0)
          return (Csound_Message*) 0;
#if defined(_MSC_VER) && !defined(__GNUC__)
        n = _vsnprintf(&(msg->msg[0]), (size_t) maxLen, fmt, args);
#else
        n = vsnprintf(&(msg->msg[0]), (size_t) maxLen, fmt, args);
#endif
        if (n > 0 && n < maxLen)
          break;
        free((void*) msg);
      }
    }
    msg->nxt = (Csound_Message*) 0;
    msg->attr = attr;

    return msg;
}

void CsoundGUIConsole::messageCallback_NoThread(CSOUND *csound,
                                                int attr, const char *fmt,
                                                va_list args)
{
    Csound_Message    *msg;
    CsoundGUIConsole  *p;

    msg = allocateMessage_(attr, fmt, args);
    if (!msg)
      return;
    p = (CsoundGUIConsole*) csoundGetHostData(csound);
    p->mutex_.Lock();
    if (p->firstMsg != (Csound_Message*) 0) {
      p->lastMsg->nxt = msg;
      p->lastMsg = msg;
    }
    else {
      p->firstMsg = msg;
      p->lastMsg = msg;
    }
    p->mutex_.Unlock();
}

void CsoundGUIConsole::messageCallback_Thread(CSOUND *csound,
                                              int attr, const char *fmt,
                                              va_list args)
{
    Csound_Message    *msg;
    CsoundGUIConsole  *p;

    msg = allocateMessage_(attr, fmt, args);
    if (!msg)
      return;
    p = (CsoundGUIConsole*) csoundGetHostData(csound);
    p->mutex_.Lock();
    if (p->firstMsg != (Csound_Message*) 0) {
      p->lastMsg->nxt = msg;
      p->lastMsg = msg;
    }
    else {
      p->firstMsg = msg;
      p->lastMsg = msg;
    }
    p->mutex_.Unlock();
    p->msgNotifyLock.Unlock();
}

void CsoundGUIConsole::Clear()
{
    Csound_Message *msg;

    do {
      mutex_.Lock();
      msg = firstMsg;
      if (msg != (Csound_Message*) 0) {
        firstMsg = msg->nxt;
        if (firstMsg == (Csound_Message*) 0)
          lastMsg = (Csound_Message*) 0;
      }
      mutex_.Unlock();
      if (msg != (Csound_Message*) 0)
        free((void*) msg);
    } while (msg != (Csound_Message*) 0);
    clearLock.Unlock();
    msgNotifyLock.Unlock();
}

void CsoundGUIConsole::flushMessages()
{
    volatile Csound_Message **msgpp;

    msgpp = (volatile Csound_Message **) &firstMsg;
    msgNotifyLock.Unlock();
    while (*msgpp != (Csound_Message*) 0) {
      Fl::wait(0.005);
    }
    msgNotifyLock.TryLock();
}

void CsoundGUIConsole::updateDisplay(bool isMainThread)
{
    char    styleChar;

    while (1) {
      Csound_Message *msg;
      mutex_.Lock();
      msg = firstMsg;
      if (msg != (Csound_Message*) 0) {
        firstMsg = msg->nxt;
        if (firstMsg == (Csound_Message*) 0)
          lastMsg = (Csound_Message*) 0;
      }
      mutex_.Unlock();
      if (!msg)
        break;
      switch (msg->attr & CSOUNDMSG_TYPE_MASK) {
      case CSOUNDMSG_WARNING:
        styleChar = 'Q';
        break;
      case CSOUNDMSG_ERROR:
        styleChar = 'R';
        break;
      default:
        switch (msg->attr & CSOUNDMSG_FG_COLOR_MASK) {
        case CSOUNDMSG_FG_RED:
          styleChar = 'B';
          break;
        case CSOUNDMSG_FG_GREEN:
          styleChar = 'C';
          break;
        case CSOUNDMSG_FG_YELLOW:
          styleChar = 'D';
          break;
        case CSOUNDMSG_FG_BLUE:
          styleChar = 'E';
          break;
        case CSOUNDMSG_FG_MAGENTA:
          styleChar = 'F';
          break;
        case CSOUNDMSG_FG_CYAN:
          styleChar = 'G';
          break;
        case CSOUNDMSG_FG_WHITE:
          styleChar = 'H';
          break;
        default:
          styleChar = 'A';
        }
        if ((msg->attr & CSOUNDMSG_FG_BOLD) != 0 ||
            (msg->attr & CSOUNDMSG_TYPE_MASK) == CSOUNDMSG_ORCH)
          styleChar += (char) 8;
      }
      if (!isMainThread)
        Fl::lock();
      if (window->shown()) {
        textDisplay->insert_position(buf->length());
        textDisplay->insert(&(msg->msg[0]));
        for (int i = 0; msg->msg[i] != (char) 0; i++)
          msg->msg[i] = styleChar;
        styleBuf->append(&(msg->msg[0]));
        textDisplay->show_insert_position();
      }
      if (!isMainThread)
        Fl::unlock();
      free((void*) msg);
    }
}

uintptr_t CsoundGUIConsole::consoleThreadFunc(void *userData)
{
    CsoundGUIConsole *p = (CsoundGUIConsole*) userData;

    do {
      p->msgNotifyLock.Lock();
      if (p->clearLock.TryLock() == 0) {
        int len;
        Fl::lock();
        len = (int) p->buf->length();
        p->buf->remove(0, len);
        len = (int) p->styleBuf->length();
        p->styleBuf->remove(0, len);
        Fl::unlock();
      }
      p->updateDisplay(false);
    } while (p->quitLock.TryLock() != 0);

    return (uintptr_t) 0;
}

