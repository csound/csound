/*
    csound_rtio.c: engine messages

    Copyright (C) 2025 The Csound Developers

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/
#include "csoundCore.h"

extern void (*msgcallback_)(CSOUND *, int32_t, const char *, va_list);
void csoundDefaultMessageCallback(CSOUND *csound, int32_t attr,
                                         const char *format, va_list args) {
#if defined(WIN32)
  switch (attr & CSOUNDMSG_TYPE_MASK) {
  case CSOUNDMSG_ERROR:
  case CSOUNDMSG_WARNING:
  case CSOUNDMSG_REALTIME:
    vfprintf(stderr, format, args);
    break;
  default:
    vfprintf(stdout, format, args);
  }
#else
  int flag  = 0;
  FILE *fp = stderr;
  if ((attr & CSOUNDMSG_TYPE_MASK) == CSOUNDMSG_STDOUT)
    fp = stdout;
  if (!attr || !csound->enableMsgAttr) {
    vfprintf(fp, format, args);
    return;
  }
  if ((attr & CSOUNDMSG_TYPE_MASK) == CSOUNDMSG_ORCH)
    if (attr & CSOUNDMSG_BG_COLOR_MASK) {
      flag = 1;
      fprintf(fp, "\033[4%cm", ((attr & 0x70) >> 4) + '0');
    }

  if (attr & CSOUNDMSG_FG_ATTR_MASK) {
    flag = 1;
    if (attr & CSOUNDMSG_FG_BOLD)
      fprintf(fp, "\033[1m");
    if (attr & CSOUNDMSG_FG_UNDERLINE)
      fprintf(fp, "\033[4m");
  }
  if (attr & CSOUNDMSG_FG_COLOR_MASK) {
    fprintf(fp, "\033[3%cm", (attr & 7) + '0');
    flag = 1;
  }
  vfprintf(fp, format, args);
  if(flag) fprintf(fp, "\033[m");
#endif
}

 void csoundSetDefaultMessageCallback(void (*csoundMessageCallback)(
    CSOUND *csound, int32_t attr, const char *format, va_list args)) {
  if (csoundMessageCallback) {
    msgcallback_ = csoundMessageCallback;
  } else {
    msgcallback_ = csoundDefaultMessageCallback;
  }
}

 void csoundSetMessageStringCallback(
    CSOUND *csound,
    void (*csoundMessageStrCallback)(CSOUND *csound, int32_t attr,
                                     const char *str)) {

  if (csoundMessageStrCallback) {
    if (csound->message_string == NULL)
      csound->message_string = (char *)csound->Calloc(csound, MAX_MESSAGE_STR);
    csound->csoundMessageStringCallback = csoundMessageStrCallback;
    csound->csoundMessageCallback_ = NULL;
  }
}

 void csoundSetMessageCallback(
    CSOUND *csound,
    void (*csoundMessageCallback)(CSOUND *csound, int32_t attr,
                                  const char *format, va_list args)) {
  /* Protect against a null callback. */
  if (csoundMessageCallback) {
    csound->csoundMessageCallback_ = csoundMessageCallback;
  } else {
    csound->csoundMessageCallback_ = csoundDefaultMessageCallback;
  }
}

 void csoundMessageV(CSOUND *csound, int32_t attr, const char *format,
                           va_list args) {
  if (!(csound->oparms->msglevel & CS_NOMSG)) {
    if (csound->csoundMessageCallback_) {
      csound->csoundMessageCallback_(csound, attr, format, args);
    } else {
      vsnprintf(csound->message_string, MAX_MESSAGE_STR, format, args);
      csound->csoundMessageStringCallback(csound, attr, csound->message_string);
    }
  }
}

 void csoundMessage(CSOUND *csound, const char *format, ...) {
  if (!(csound->oparms->msglevel & CS_NOMSG)) {
    va_list args;
    va_start(args, format);
    if (csound->csoundMessageCallback_)
      csound->csoundMessageCallback_(csound, 0, format, args);
    else {
      vsnprintf(csound->message_string, MAX_MESSAGE_STR, format, args);
      csound->csoundMessageStringCallback(csound, 0, csound->message_string);
    }
    va_end(args);
  }
}

 void csoundMessageS(CSOUND *csound, int32_t attr, const char *format,
                           ...) {
  if (!(csound->oparms->msglevel & CS_NOMSG)) {
    va_list args;
    va_start(args, format);
    if (csound->csoundMessageCallback_)
      csound->csoundMessageCallback_(csound, attr, format, args);
    else {
      vsnprintf(csound->message_string, MAX_MESSAGE_STR, format, args);
      csound->csoundMessageStringCallback(csound, attr, csound->message_string);
    }
    va_end(args);
  }
}

void csoundDie(CSOUND *csound, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  csound->ErrMsgV(csound, (char *)0, msg, args);
  va_end(args);
  csound->perferrcnt++;
  csound->LongJmp(csound, 1);
}

void csoundWarning(CSOUND *csound, const char *msg, ...) {
  va_list args;
  if (!(csound->oparms_.msglevel & CS_WARNMSG))
    return;
  csoundMessageS(csound, CSOUNDMSG_WARNING, Str("warning: "));
  va_start(args, msg);
  csoundMessageV(csound, CSOUNDMSG_WARNING, msg, args);
  va_end(args);
  csoundMessageS(csound, CSOUNDMSG_WARNING, "\n");
}

void csoundDebugMsg(CSOUND *csound, const char *msg, ...) {
  va_list args;
  if (!(csound->oparms_.odebug & (~CS_NOQQ)) ||
        csoundGetDebug(csound) < 99)
    return;
  va_start(args, msg);
  csoundMessageV(csound, 0, msg, args);
  va_end(args);
  csoundMessage(csound, "\n");
}

void csoundErrorMsg(CSOUND *csound, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  csoundMessageV(csound, CSOUNDMSG_ERROR, msg, args);
  va_end(args);
}

void csoundErrMsgV(CSOUND *csound, const char *hdr, const char *msg,
                   va_list args) {
  if (hdr != NULL)
    csound->MessageS(csound, CSOUNDMSG_ERROR, "%s", hdr);
  csoundMessageV(csound, CSOUNDMSG_ERROR, msg, args);
  csound->MessageS(csound, CSOUNDMSG_ERROR, "\n");
}

void csoundErrorMsgS(CSOUND *csound, int32_t attr, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  csoundMessageV(csound, CSOUNDMSG_ERROR | attr, msg, args);
  va_end(args);
}

 void csoundSetMessageLevel(CSOUND *csound, int32_t messageLevel) {
  csound->oparms_.msglevel = messageLevel;
}

 int32_t csoundGetMessageLevel(CSOUND *csound) {
  return csound->oparms_.msglevel;
}


typedef struct csMsgStruct_ {
  struct csMsgStruct_ *nxt;
  int32_t attr;
  char s[1];
} csMsgStruct;

typedef struct csMsgBuffer_ {
  void *mutex_;
  csMsgStruct *firstMsg;
  csMsgStruct *lastMsg;
  int32_t msgCnt;
  char *buf;
} csMsgBuffer;

// callback for storing messages in the buffer only
static void csoundMessageBufferCallback_1_(CSOUND *csound, int32_t attr,
                                           const char *fmt, va_list args);

// callback for writing messages to the buffer, and also stdout/stderr
static void csoundMessageBufferCallback_2_(CSOUND *csound, int32_t attr,
                                           const char *fmt, va_list args);

/**
 * Creates a buffer for storing messages printed by Csound.
 * Should be called after creating a Csound instance; note that
 * the message buffer uses the host data pointer, and the buffer
 * should be freed by calling csoundDestroyMessageBuffer() before
 * deleting the Csound instance.
 * If 'toStdOut' is non-zero, the messages are also printed to
 * stdout and stderr (depending on the type of the message),
 * in addition to being stored in the buffer.
 */

void  csoundCreateMessageBuffer(CSOUND *csound, int32_t toStdOut) {
  csMsgBuffer *pp;
  size_t nBytes;

  pp = (csMsgBuffer *)csound->message_buffer;
  if (pp) {
    csoundDestroyMessageBuffer(csound);
  }
  nBytes = sizeof(csMsgBuffer);
  if (!toStdOut) {
    nBytes += (size_t)16384;
  }
  pp = (csMsgBuffer *)malloc(nBytes);
  pp->mutex_ = csoundCreateMutex(0);
  pp->firstMsg = (csMsgStruct *)NULL;
  pp->lastMsg = (csMsgStruct *)NULL;
  pp->msgCnt = 0;
  if (!toStdOut) {
    pp->buf = (char *)pp + (int32_t)sizeof(csMsgBuffer);
    pp->buf[0] = (char)'\0';
  } else {
    pp->buf = (char *)NULL;
  }
  csound->message_buffer = (void *)pp;
  if (toStdOut) {
    csoundSetMessageCallback(csound, csoundMessageBufferCallback_2_);
  } else {
    csoundSetMessageCallback(csound, csoundMessageBufferCallback_1_);
  }
}

/**
 * Returns the first message from the buffer.
 */
#ifdef MSVC
const char  *csoundGetFirstMessage(CSOUND *csound)
#else
const char * /**/ csoundGetFirstMessage(CSOUND *csound)
#endif
{
  csMsgBuffer *pp = (csMsgBuffer *)csound->message_buffer;
  char *msg = NULL;

  if (pp && pp->msgCnt) {
    csoundLockMutex(pp->mutex_);
    if (pp->firstMsg)
      msg = &(pp->firstMsg->s[0]);
    csoundUnlockMutex(pp->mutex_);
  }
  return msg;
}

/**
 * Returns the attribute parameter (see msg_attr.h) of the first message
 * in the buffer.
 */

int32_t  csoundGetFirstMessageAttr(CSOUND *csound) {
  csMsgBuffer *pp = (csMsgBuffer *)csound->message_buffer;
  int32_t attr = 0;

  if (pp && pp->msgCnt) {
    csoundLockMutex(pp->mutex_);
    if (pp->firstMsg) {
      attr = pp->firstMsg->attr;
    }
    csoundUnlockMutex(pp->mutex_);
  }
  return attr;
}

/**
 * Removes the first message from the buffer.
 */

void  csoundPopFirstMessage(CSOUND *csound) {
  csMsgBuffer *pp = (csMsgBuffer *)csound->message_buffer;

  if (pp) {
    csMsgStruct *tmp;
    csoundLockMutex(pp->mutex_);
    tmp = pp->firstMsg;
    if (tmp) {
      pp->firstMsg = tmp->nxt;
      pp->msgCnt--;
      if (!pp->firstMsg)
        pp->lastMsg = (csMsgStruct *)0;
    }
    csoundUnlockMutex(pp->mutex_);
    if (tmp)
      free((void *)tmp);
  }
}

/**
 * Returns the number of pending messages in the buffer.
 */

int32_t  csoundGetMessageCnt(CSOUND *csound) {
  csMsgBuffer *pp = (csMsgBuffer *)csound->message_buffer;
  int32_t cnt = -1;

  if (pp) {
    csoundLockMutex(pp->mutex_);
    cnt = pp->msgCnt;
    csoundUnlockMutex(pp->mutex_);
  }
  return cnt;
}

/**
 * Releases all memory used by the message buffer.
 */

void  csoundDestroyMessageBuffer(CSOUND *csound) {
  csMsgBuffer *pp = (csMsgBuffer *)csound->message_buffer;
  if (!pp) {
    csound->Warning(csound, Str("csoundDestroyMessageBuffer: "
                                "Message buffer not allocated."));
    return;
  }
  csMsgStruct *msg = pp->firstMsg;
  while (msg) {
    csMsgStruct *tmp = msg;
    msg = tmp->nxt;
    free(tmp);
  }
  csound->message_buffer = NULL;
  csoundSetMessageCallback(csound, NULL);
  while (csoundGetMessageCnt(csound) > 0) {
    csoundPopFirstMessage(csound);
  }
  csoundSetHostData(csound, NULL);
  csoundDestroyMutex(pp->mutex_);
  free((void *)pp);
}

static void csoundMessageBufferCallback_1_(CSOUND *csound, int32_t attr,
                                           const char *fmt, va_list args) {
  csMsgBuffer *pp = (csMsgBuffer *)csound->message_buffer;
  csMsgStruct *p;
  int32_t len;

  csoundLockMutex(pp->mutex_);
  len = vsnprintf(pp->buf, 16384, fmt, args); // FIXEDME: this can overflow
  va_end(args);
  if (UNLIKELY((uint32_t)len >= (uint32_t)16384)) {
    csoundUnlockMutex(pp->mutex_);
    fprintf(stderr, Str("csound: internal error: message buffer overflow\n"));
    exit(-1);
  }
  p = (csMsgStruct *)malloc(sizeof(csMsgStruct) + (size_t)len);
  p->nxt = (csMsgStruct *)NULL;
  p->attr = attr;
  strcpy(&(p->s[0]), pp->buf);
  if (pp->firstMsg == (csMsgStruct *)0) {
    pp->firstMsg = p;
  } else {
    pp->lastMsg->nxt = p;
  }
  pp->lastMsg = p;
  pp->msgCnt++;
  csoundUnlockMutex(pp->mutex_);
}

static void csoundMessageBufferCallback_2_(CSOUND *csound, int32_t attr,
                                           const char *fmt, va_list args) {
  csMsgBuffer *pp = (csMsgBuffer *)csound->message_buffer;
  csMsgStruct *p;
  int32_t len = 0;
  va_list args_save;

  va_copy(args_save, args);
  switch (attr & CSOUNDMSG_TYPE_MASK) {
  case CSOUNDMSG_ERROR:
  case CSOUNDMSG_REALTIME:
  case CSOUNDMSG_WARNING:
    len = vfprintf(stderr, fmt, args);
    break;
  default:
    len = vfprintf(stdout, fmt, args);
  }
  va_end(args);
  p = (csMsgStruct *)malloc(sizeof(csMsgStruct) + (size_t)len);
  p->nxt = (csMsgStruct *)NULL;
  p->attr = attr;
  vsnprintf(&(p->s[0]), len, fmt, args_save);
  va_end(args_save);
  csoundLockMutex(pp->mutex_);
  if (pp->firstMsg == (csMsgStruct *)NULL)
    pp->firstMsg = p;
  else
    pp->lastMsg->nxt = p;
  pp->lastMsg = p;
  pp->msgCnt++;
  csoundUnlockMutex(pp->mutex_);
}
