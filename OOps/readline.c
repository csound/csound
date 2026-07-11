/*
  readline.c:

  Copyright (C) 2026 The Csound Developers

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

#include "readline.h"
#include "csound_threads.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if defined(WIN32)
#include <conio.h>
#include <io.h>
#include <windows.h>
#elif defined(HAVE_TERMIOS_H) && defined(HAVE_UNISTD_H) && \
      !defined(__EMSCRIPTEN__) && !defined(__wasi__)
#define CSOUND_READLINE_POSIX 1
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

#define READLINE_GLOBALS_NAME "::readline_globals::"
#define READLINE_INITIAL_CAPACITY 128
#define READLINE_OUTPUT_CAPACITY 65536
#define READLINE_OUTPUT_CHUNK 1024

#if defined(WIN32) && !defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING)
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

enum {
  READLINE_POLL_ERROR = -2,
  READLINE_POLL_EOF = -1,
  READLINE_POLL_NONE = 0,
  READLINE_POLL_CHARACTER = 1
};

enum {
  READLINE_ESCAPE_NONE = 0,
  READLINE_ESCAPE_CONSUMED,
  READLINE_ESCAPE_LEFT,
  READLINE_ESCAPE_RIGHT
};

enum {
  READLINE_KEY_LEFT = 0x100,
  READLINE_KEY_RIGHT
};

enum {
  READLINE_PENDING_NONE = 0,
  READLINE_PENDING_LINE = 1,
  READLINE_PENDING_EOF = -1
};

typedef struct {
  const char *data;
  size_t length;
} READLINE_OUTPUT_PART;

typedef struct {
  READLINE_OPCODE *active;
  int32_t resetRegistered;
} READLINE_GLOBALS;

typedef struct {
  CSOUND *csound;
  READLINE_GLOBALS *globals;
  char *buffer;
  void *outputBuffer;
  void *writerThread;
  size_t length;
  size_t cursor;
  size_t capacity;
  int32_t writerStop;
  int32_t outputError;
  int32_t outputQueued;
  int32_t outputCompleted;
  int32_t pendingStatus;
  int32_t pendingTarget;
  int32_t promptPending;
  int32_t lineOpen;
  int32_t eof;
  int32_t escapeState;
  int32_t interactive;
  int32_t terminalModeSet;
  int32_t ownsProcessInput;
#if defined(WIN32)
  HANDLE outputHandle;
  DWORD savedOutputMode;
  int32_t outputModeSet;
#endif
#if defined(CSOUND_READLINE_POSIX)
  struct termios savedTerminalMode;
#endif
} READLINE_STATE;

#if defined(HAVE_PTHREAD_SPIN_LOCK)
static spin_lock_t processOwnerLock;
static pthread_once_t processOwnerLockOnce = PTHREAD_ONCE_INIT;

static void init_process_owner_lock(void)
{
  (void) csoundSpinLockInit(&processOwnerLock);
}
#else
static spin_lock_t processOwnerLock = SPINLOCK_INIT;
#endif
static CSOUND *processOwner = NULL;

static int32_t readline_reset(CSOUND *, void *);

static int32_t claim_process_input(CSOUND *csound)
{
  int32_t claimed;

#if defined(HAVE_PTHREAD_SPIN_LOCK)
  (void) pthread_once(&processOwnerLockOnce, init_process_owner_lock);
#endif
  csoundSpinLock(&processOwnerLock);
  claimed = processOwner == NULL || processOwner == csound;
  if (claimed)
    processOwner = csound;
  csoundSpinUnLock(&processOwnerLock);

  return claimed;
}

static void release_process_input(CSOUND *csound)
{
  csoundSpinLock(&processOwnerLock);
  if (processOwner == csound)
    processOwner = NULL;
  csoundSpinUnLock(&processOwnerLock);
}

static READLINE_GLOBALS *get_readline_globals(CSOUND *csound)
{
  READLINE_GLOBALS *globals =
    (READLINE_GLOBALS *) csound->QueryGlobalVariable(
      csound, READLINE_GLOBALS_NAME);

  if (globals == NULL) {
    if (UNLIKELY(csound->CreateGlobalVariable(
                   csound, READLINE_GLOBALS_NAME,
                   sizeof(READLINE_GLOBALS)) != OK))
      return NULL;

    globals = (READLINE_GLOBALS *) csound->QueryGlobalVariable(
      csound, READLINE_GLOBALS_NAME);
  }

  if (globals != NULL && !globals->resetRegistered) {
    if (UNLIKELY(csound->RegisterResetCallback(
                   csound, globals, readline_reset) != OK))
      return NULL;
    globals->resetRegistered = 1;
  }

  return globals;
}

static int32_t grow_buffer(CSOUND *csound, READLINE_STATE *state,
                           size_t required)
{
  size_t capacity = state->capacity;
  char *buffer;

  if (required <= capacity)
    return OK;

  while (capacity < required)
    capacity *= 2;

  buffer = (char *) csound->ReAlloc(csound, state->buffer, capacity);
  if (UNLIKELY(buffer == NULL))
    return NOTOK;

  state->buffer = buffer;
  state->capacity = capacity;
  return OK;
}

static int32_t copy_line_to_output(CSOUND *csound, READLINE_OPCODE *p)
{
  READLINE_STATE *state = (READLINE_STATE *) p->state;
  size_t required = state->length + 1;

  if (required > p->line->size) {
    char *data = (char *) csound->ReAlloc(csound, p->line->data,
                                          required);
    if (UNLIKELY(data == NULL))
      return NOTOK;
    p->line->data = data;
    p->line->size = required;
  }

  memcpy(p->line->data, state->buffer, required);
  return OK;
}

static uintptr_t terminal_writer(void *userData)
{
  READLINE_STATE *state = (READLINE_STATE *) userData;
  char output[READLINE_OUTPUT_CHUNK];

  for (;;) {
    int32_t count = state->csound->ReadCircularBuffer(
      state->csound, state->outputBuffer, output, READLINE_OUTPUT_CHUNK);

    if (count == 0) {
      if (ATOMIC_GET(state->writerStop))
        break;
      state->csound->Sleep(1);
      continue;
    }

    size_t offset = 0;
    while (offset < (size_t) count) {
      size_t written = fwrite(output + offset, 1,
                              (size_t) count - offset, stdout);

      if (written > 0) {
        offset += written;
      }
      else if (ferror(stdout) && errno == EINTR) {
        clearerr(stdout);
      }
      else {
        ATOMIC_SET(state->outputError, 1);
        return 0;
      }
    }

    if (fflush(stdout) != 0) {
      ATOMIC_SET(state->outputError, 1);
      return 0;
    }
    ATOMIC_ADD(state->outputCompleted, count);
  }

  return 0;
}

static int32_t start_terminal_writer(CSOUND *csound, READLINE_STATE *state)
{
  if (!state->interactive)
    return OK;

  state->outputBuffer = csound->CreateCircularBuffer(
    csound, READLINE_OUTPUT_CAPACITY, sizeof(char));
  if (UNLIKELY(state->outputBuffer == NULL))
    return NOTOK;

  state->writerThread = csound->CreateThread(terminal_writer, state);
  if (UNLIKELY(state->writerThread == NULL)) {
    csound->DestroyCircularBuffer(csound, state->outputBuffer);
    state->outputBuffer = NULL;
    return NOTOK;
  }
  return OK;
}

static void stop_terminal_writer(READLINE_STATE *state)
{
  if (state->writerThread != NULL) {
    ATOMIC_SET(state->writerStop, 1);
    state->csound->JoinThread(state->writerThread);
    state->writerThread = NULL;
  }

  if (state->outputBuffer != NULL) {
    state->csound->DestroyCircularBuffer(state->csound, state->outputBuffer);
    state->outputBuffer = NULL;
  }
}

static int32_t queue_terminal_output(READLINE_STATE *state,
                                     const READLINE_OUTPUT_PART *parts,
                                     size_t partCount)
{
  size_t total = 0;

  if (!state->interactive)
    return OK;

  for (size_t index = 0; index < partCount; index++) {
    if (parts[index].length > (size_t) INT32_MAX - total)
      return NOTOK;
    total += parts[index].length;
  }

  if (total == 0)
    return OK;
  if (state->outputBuffer == NULL ||
      state->csound->CheckCircularBuffer(
        state->csound, state->outputBuffer, 1) < (int32_t) total)
    return NOTOK;

  for (size_t index = 0; index < partCount; index++) {
    int32_t length = (int32_t) parts[index].length;

    if (length == 0)
      continue;
    if (state->csound->WriteCircularBuffer(
          state->csound, state->outputBuffer,
          parts[index].data, length) != length)
      return NOTOK;
    ATOMIC_ADD(state->outputQueued, length);
  }
  return OK;
}

static int32_t write_prompt(READLINE_OPCODE *p)
{
  READLINE_STATE *state = (READLINE_STATE *) p->state;

  if (state->interactive && p->prompt->data != NULL &&
      p->prompt->data[0] != '\0') {
    READLINE_OUTPUT_PART part = {
      p->prompt->data, strlen(p->prompt->data)
    };

    if (UNLIKELY(queue_terminal_output(state, &part, 1) != OK))
      return NOTOK;
    state->lineOpen = 1;
  }
  state->promptPending = 0;
  return OK;
}

static int32_t is_utf8_continuation(char value)
{
  return ((unsigned char) value & 0xc0U) == 0x80U;
}

static size_t previous_character(const READLINE_STATE *state, size_t position)
{
  if (position == 0)
    return 0;

  position--;
  while (position > 0 && is_utf8_continuation(state->buffer[position]))
    position--;
  return position;
}

static size_t next_character(const READLINE_STATE *state, size_t position)
{
  if (position >= state->length)
    return state->length;

  position++;
  while (position < state->length &&
         is_utf8_continuation(state->buffer[position]))
    position++;
  return position;
}

static size_t count_characters(const READLINE_STATE *state, size_t start)
{
  size_t count = 0;

  for (size_t index = start; index < state->length; index++) {
    if (!is_utf8_continuation(state->buffer[index]))
      count++;
  }
  return count;
}

static int32_t format_cursor_sequence(char *buffer, size_t capacity,
                                      size_t count, char direction,
                                      size_t *length)
{
  int32_t result = snprintf(buffer, capacity, "\033[%zu%c",
                            count, direction);

  if (UNLIKELY(result < 0 || (size_t) result >= capacity))
    return NOTOK;
  *length = (size_t) result;
  return OK;
}

static int32_t move_terminal_cursor(READLINE_STATE *state, size_t count,
                                    char direction)
{
  char sequence[64];
  size_t length;
  READLINE_OUTPUT_PART part;

  if (!state->interactive || count == 0)
    return OK;
  if (UNLIKELY(format_cursor_sequence(sequence, sizeof(sequence),
                                      count, direction, &length) != OK))
    return NOTOK;

  part.data = sequence;
  part.length = length;
  return queue_terminal_output(state, &part, 1);
}

static int32_t redraw_after_insert(READLINE_STATE *state,
                                   size_t insertPosition,
                                   int32_t fromTerminal)
{
  char cursorSequence[64];
  size_t cursorLength = 0;
  size_t cursorCount;
  READLINE_OUTPUT_PART parts[2];

  if (!state->interactive || !fromTerminal)
    return OK;

  cursorCount = count_characters(state, state->cursor);
  if (cursorCount > 0 &&
      UNLIKELY(format_cursor_sequence(
                 cursorSequence, sizeof(cursorSequence), cursorCount,
                 'D', &cursorLength) != OK))
    return NOTOK;

  parts[0].data = state->buffer + insertPosition;
  parts[0].length = state->length - insertPosition;
  parts[1].data = cursorSequence;
  parts[1].length = cursorLength;
  if (UNLIKELY(queue_terminal_output(state, parts, 2) != OK))
    return NOTOK;
  state->lineOpen = 1;
  return OK;
}

static int32_t redraw_after_backspace(READLINE_STATE *state,
                                      int32_t fromTerminal)
{
  char leftSequence[64];
  char rightSequence[64];
  size_t leftLength;
  size_t rightLength;
  size_t tailLength;
  READLINE_OUTPUT_PART parts[4];

  if (!state->interactive || !fromTerminal)
    return OK;

  if (UNLIKELY(format_cursor_sequence(
                 leftSequence, sizeof(leftSequence), 1,
                 'D', &leftLength) != OK))
    return NOTOK;

  tailLength = state->length - state->cursor;
  if (UNLIKELY(format_cursor_sequence(
                 rightSequence, sizeof(rightSequence),
                 count_characters(state, state->cursor) + 1,
                 'D', &rightLength) != OK))
    return NOTOK;

  parts[0].data = leftSequence;
  parts[0].length = leftLength;
  parts[1].data = state->buffer + state->cursor;
  parts[1].length = tailLength;
  parts[2].data = " ";
  parts[2].length = 1;
  parts[3].data = rightSequence;
  parts[3].length = rightLength;
  return queue_terminal_output(state, parts, 4);
}

static int32_t move_cursor_left(READLINE_STATE *state, int32_t fromTerminal)
{
  size_t position = previous_character(state, state->cursor);

  if (position != state->cursor) {
    state->cursor = position;
    if (fromTerminal)
      return move_terminal_cursor(state, 1, 'D');
  }
  return OK;
}

static int32_t move_cursor_right(READLINE_STATE *state, int32_t fromTerminal)
{
  size_t position = next_character(state, state->cursor);

  if (position != state->cursor) {
    state->cursor = position;
    if (fromTerminal)
      return move_terminal_cursor(state, 1, 'C');
  }
  return OK;
}

static int32_t finish_terminal_line(READLINE_STATE *state)
{
  if (state->interactive) {
    READLINE_OUTPUT_PART part = { "\r\n", 2 };

    if (UNLIKELY(queue_terminal_output(state, &part, 1) != OK))
      return NOTOK;
  }
  state->lineOpen = 0;
  return OK;
}

static int32_t configure_terminal(CSOUND *csound, READLINE_STATE *state)
{
#if defined(WIN32)
  state->interactive = _isatty(_fileno(stdin)) &&
                       _isatty(_fileno(stdout));

  if (state->interactive) {
    DWORD mode;

    state->outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (UNLIKELY(state->outputHandle == NULL ||
                 state->outputHandle == INVALID_HANDLE_VALUE ||
                 !GetConsoleMode(state->outputHandle, &mode)))
      return csound->InitError(
        csound, "%s", Str("readline: failed to read console mode"));

    state->savedOutputMode = mode;
    if (!(mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
      if (UNLIKELY(!SetConsoleMode(
                     state->outputHandle,
                     mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)))
        return csound->InitError(
          csound, "%s",
          Str("readline: failed to enable virtual terminal mode"));
      state->outputModeSet = 1;
    }
  }
#elif defined(CSOUND_READLINE_POSIX)
  state->interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);

  if (state->interactive) {
    struct termios mode;

    if (UNLIKELY(tcgetattr(STDIN_FILENO, &state->savedTerminalMode) != 0))
      return csound->InitError(csound, "%s",
                               Str("readline: failed to read terminal mode"));

    mode = state->savedTerminalMode;
    mode.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    mode.c_cc[VMIN] = 0;
    mode.c_cc[VTIME] = 0;
    if (UNLIKELY(tcsetattr(STDIN_FILENO, TCSAFLUSH, &mode) != 0))
      return csound->InitError(csound, "%s",
                               Str("readline: failed to set terminal mode"));
    state->terminalModeSet = 1;
  }
#else
  IGN(csound);
  state->interactive = 0;
#endif
  return OK;
}

static void restore_terminal_modes(READLINE_STATE *state)
{
  if (state == NULL)
    return;

#if defined(WIN32)
  if (state->outputModeSet) {
    SetConsoleMode(state->outputHandle, state->savedOutputMode);
    state->outputModeSet = 0;
  }
#elif defined(CSOUND_READLINE_POSIX)
  if (state->terminalModeSet) {
    tcsetattr(STDIN_FILENO, TCSANOW, &state->savedTerminalMode);
    state->terminalModeSet = 0;
  }
#else
  state->terminalModeSet = 0;
#endif
}

static void release_readline(CSOUND *csound, READLINE_OPCODE *p)
{
  READLINE_STATE *state;

  if (p == NULL || p->state == NULL)
    return;

  state = (READLINE_STATE *) p->state;
  state->lineOpen = 0;
  restore_terminal_modes(state);

  if (state->globals != NULL && state->globals->active == p)
    state->globals->active = NULL;

  if (state->ownsProcessInput) {
    release_process_input(csound);
    state->ownsProcessInput = 0;
  }
}

static int32_t readline_perf_error(CSOUND *csound, READLINE_OPCODE *p,
                                   const char *message)
{
  if (p != NULL && p->state != NULL) {
    READLINE_STATE *state = (READLINE_STATE *) p->state;
    ATOMIC_SET(state->writerStop, 1);
  }
  release_readline(csound, p);
  return csound->PerfError(csound, &p->h, "%s", message);
}

static int32_t readline_reset(CSOUND *csound, void *userData)
{
  READLINE_GLOBALS *globals = (READLINE_GLOBALS *) userData;

  if (globals != NULL && globals->active != NULL)
    readline_deinit(csound, globals->active);
  return OK;
}

static int32_t poll_terminal(CSOUND *csound, int32_t *key,
                             int32_t *fromTerminal)
{
  int32_t callbackKey = 0;
  int32_t callbackResult = csound->doCsoundCallback(
    csound, &callbackKey, CSOUND_CALLBACK_KBD_TEXT);

  if (UNLIKELY(callbackResult < 0))
    return READLINE_POLL_ERROR;
  if (callbackKey > 0) {
    *key = callbackKey;
    *fromTerminal = 0;
    return READLINE_POLL_CHARACTER;
  }

  if (csound->inChar_ > 0) {
    *key = csound->inChar_;
    csound->inChar_ = 0;
    *fromTerminal = 0;
    return READLINE_POLL_CHARACTER;
  }

#if defined(WIN32)
  if (_kbhit()) {
    int32_t input = _getch();

    if (input == 0 || input == 0xe0) {
      switch (_getch()) {
      case 75:
        input = READLINE_KEY_LEFT;
        break;
      case 77:
        input = READLINE_KEY_RIGHT;
        break;
      default:
        return READLINE_POLL_NONE;
      }
    }
    *key = input;
    *fromTerminal = 1;
    return READLINE_POLL_CHARACTER;
  }
#elif defined(CSOUND_READLINE_POSIX)
  {
    fd_set readfds;
    struct timeval timeout = { 0, 0 };
    unsigned char input;
    int32_t selected;
    ssize_t count;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    selected = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
    if (selected == 0)
      return READLINE_POLL_NONE;
    if (selected < 0)
      return errno == EINTR ? READLINE_POLL_NONE : READLINE_POLL_ERROR;

    count = read(STDIN_FILENO, &input, 1);
    if (count == 0)
      return READLINE_POLL_EOF;
    if (count < 0)
      return errno == EAGAIN || errno == EINTR ?
        READLINE_POLL_NONE : READLINE_POLL_ERROR;

    *key = (int32_t) input;
    *fromTerminal = 1;
    return READLINE_POLL_CHARACTER;
  }
#endif

  return READLINE_POLL_NONE;
}

static int32_t insert_character(CSOUND *csound, READLINE_STATE *state,
                                int32_t key)
{
  if (UNLIKELY(grow_buffer(csound, state, state->length + 2) != OK))
    return NOTOK;

  memmove(state->buffer + state->cursor + 1,
          state->buffer + state->cursor,
          state->length - state->cursor + 1);
  state->buffer[state->cursor++] = (char) key;
  state->length++;
  return OK;
}

static int32_t remove_character(READLINE_STATE *state)
{
  size_t position;
  size_t removed;

  if (state->cursor == 0)
    return 0;

  position = previous_character(state, state->cursor);
  removed = state->cursor - position;
  memmove(state->buffer + position, state->buffer + state->cursor,
          state->length - state->cursor + 1);
  state->cursor = position;
  state->length -= removed;
  return 1;
}

static int32_t consume_escape(READLINE_STATE *state, int32_t key)
{
  if (key == READLINE_KEY_LEFT || key == READLINE_KEY_RIGHT) {
    state->escapeState = 0;
    return key == READLINE_KEY_LEFT ?
      READLINE_ESCAPE_LEFT : READLINE_ESCAPE_RIGHT;
  }

  if (state->escapeState == 0) {
    if (key == 27) {
      state->escapeState = 1;
      return READLINE_ESCAPE_CONSUMED;
    }
    return READLINE_ESCAPE_NONE;
  }

  if (state->escapeState == 1 && (key == '[' || key == 'O')) {
    state->escapeState = 2;
    return READLINE_ESCAPE_CONSUMED;
  }

  if (state->escapeState == 2 && key >= 0x40 && key <= 0x7e) {
    state->escapeState = 0;
    if (key == 'D')
      return READLINE_ESCAPE_LEFT;
    if (key == 'C')
      return READLINE_ESCAPE_RIGHT;
  }
  else if (state->escapeState == 1) {
    state->escapeState = 0;
  }
  return READLINE_ESCAPE_CONSUMED;
}

int32_t readline_init(CSOUND *csound, READLINE_OPCODE *p)
{
  READLINE_GLOBALS *globals;
  READLINE_STATE *state;

  p->state = NULL;
  *p->status = FL(0.0);
  if (p->line->data != NULL)
    p->line->data[0] = '\0';

  globals = get_readline_globals(csound);
  if (UNLIKELY(globals == NULL))
    return csound->InitError(csound, "%s",
                             Str("readline: failed to allocate global state"));
  if (UNLIKELY(globals->active != NULL))
    return csound->InitError(csound, "%s",
                             Str("readline: another prompt is already active"));
  if (UNLIKELY(csound->stdin_assign_flg != 0))
    return csound->InitError(csound, "%s",
                             Str("readline: stdin is already in use"));
  if (UNLIKELY(!claim_process_input(csound)))
    return csound->InitError(csound, "%s",
                             Str("readline: stdin is owned by another Csound instance"));

  state = (READLINE_STATE *) csound->Calloc(csound,
                                             sizeof(READLINE_STATE));
  if (UNLIKELY(state == NULL)) {
    release_process_input(csound);
    return csound->InitError(csound, "%s",
                             Str("readline: memory allocation failure"));
  }

  state->buffer = (char *) csound->Calloc(csound,
                                           READLINE_INITIAL_CAPACITY);
  if (UNLIKELY(state->buffer == NULL)) {
    csound->Free(csound, state);
    release_process_input(csound);
    return csound->InitError(csound, "%s",
                             Str("readline: memory allocation failure"));
  }

  state->globals = globals;
  state->csound = csound;
  state->capacity = READLINE_INITIAL_CAPACITY;
  state->promptPending = 1;
  state->ownsProcessInput = 1;
  p->state = state;
  globals->active = p;

  if (UNLIKELY(configure_terminal(csound, state) != OK)) {
    readline_deinit(csound, p);
    return NOTOK;
  }
  if (UNLIKELY(start_terminal_writer(csound, state) != OK)) {
    readline_deinit(csound, p);
    return csound->InitError(
      csound, "%s", Str("readline: failed to start terminal writer"));
  }
  return OK;
}

int32_t readline_perf(CSOUND *csound, READLINE_OPCODE *p)
{
  READLINE_STATE *state = (READLINE_STATE *) p->state;
  int32_t key = 0;
  int32_t fromTerminal = 0;
  int32_t pollResult;
  int32_t escapeAction;

  if (UNLIKELY(state == NULL))
    return readline_perf_error(csound, p,
                               Str("readline: not initialized"));

  *p->status = FL(0.0);
  if (UNLIKELY(ATOMIC_GET(state->outputError)))
    return readline_perf_error(csound, p,
                               Str("readline: terminal output error"));

  if (state->pendingStatus != READLINE_PENDING_NONE) {
    int32_t pendingStatus;

    if (ATOMIC_GET(state->outputCompleted) != state->pendingTarget)
      return OK;

    pendingStatus = state->pendingStatus;
    state->pendingStatus = READLINE_PENDING_NONE;
    if (pendingStatus == READLINE_PENDING_EOF) {
      state->eof = 1;
      *p->status = FL(-1.0);
      release_readline(csound, p);
    }
    else {
      *p->status = FL(1.0);
    }
    return OK;
  }

  if (state->eof) {
    *p->status = FL(-1.0);
    return OK;
  }

  if (state->promptPending &&
      UNLIKELY(write_prompt(p) != OK))
    return readline_perf_error(
      csound, p, Str("readline: terminal output queue full"));

  pollResult = poll_terminal(csound, &key, &fromTerminal);
  if (pollResult == READLINE_POLL_NONE)
    return OK;
  if (pollResult == READLINE_POLL_ERROR)
    return readline_perf_error(csound, p, Str("readline: input error"));
  if (pollResult == READLINE_POLL_EOF ||
      ((key == 4 || key == 26) && state->length == 0)) {
    if (UNLIKELY(finish_terminal_line(state) != OK))
      return readline_perf_error(
        csound, p, Str("readline: terminal output queue full"));
    state->pendingStatus = READLINE_PENDING_EOF;
    state->pendingTarget = ATOMIC_GET(state->outputQueued);
    ATOMIC_SET(state->writerStop, 1);

    if (ATOMIC_GET(state->outputCompleted) == state->pendingTarget) {
      state->pendingStatus = READLINE_PENDING_NONE;
      state->eof = 1;
      *p->status = FL(-1.0);
      release_readline(csound, p);
    }
    return OK;
  }

  escapeAction = consume_escape(state, key);
  if (escapeAction == READLINE_ESCAPE_LEFT) {
    if (UNLIKELY(move_cursor_left(state, fromTerminal) != OK))
      return readline_perf_error(
        csound, p, Str("readline: terminal output queue full"));
    return OK;
  }
  if (escapeAction == READLINE_ESCAPE_RIGHT) {
    if (UNLIKELY(move_cursor_right(state, fromTerminal) != OK))
      return readline_perf_error(
        csound, p, Str("readline: terminal output queue full"));
    return OK;
  }
  if (escapeAction == READLINE_ESCAPE_CONSUMED)
    return OK;

  if (key == '\n' || key == '\r') {
    if (UNLIKELY(copy_line_to_output(csound, p) != OK))
      return readline_perf_error(
        csound, p, Str("readline: memory allocation failure"));
    if (UNLIKELY(finish_terminal_line(state) != OK))
      return readline_perf_error(
        csound, p, Str("readline: terminal output queue full"));
    state->length = 0;
    state->cursor = 0;
    state->buffer[0] = '\0';
    state->promptPending = 1;
    state->pendingStatus = READLINE_PENDING_LINE;
    state->pendingTarget = ATOMIC_GET(state->outputQueued);

    if (ATOMIC_GET(state->outputCompleted) == state->pendingTarget) {
      state->pendingStatus = READLINE_PENDING_NONE;
      *p->status = FL(1.0);
    }
    return OK;
  }

  if (key == 8 || key == 127) {
    if (remove_character(state) &&
        UNLIKELY(redraw_after_backspace(state, fromTerminal) != OK))
      return readline_perf_error(
        csound, p, Str("readline: terminal output queue full"));
    return OK;
  }

  if (key == '\t' || key >= 32) {
    size_t insertPosition = state->cursor;

    if (UNLIKELY(insert_character(csound, state, key) != OK))
      return readline_perf_error(
        csound, p, Str("readline: memory allocation failure"));
    if (UNLIKELY(redraw_after_insert(
                   state, insertPosition, fromTerminal) != OK))
      return readline_perf_error(
        csound, p, Str("readline: terminal output queue full"));
  }
  return OK;
}

int32_t readline_deinit(CSOUND *csound, READLINE_OPCODE *p)
{
  READLINE_STATE *state;

  if (p == NULL || p->state == NULL)
    return OK;

  state = (READLINE_STATE *) p->state;
  if (state->lineOpen && !ATOMIC_GET(state->outputError))
    (void) finish_terminal_line(state);
  stop_terminal_writer(state);
  release_readline(csound, p);
  csound->Free(csound, state->buffer);
  csound->Free(csound, state);
  p->state = NULL;
  return OK;
}
