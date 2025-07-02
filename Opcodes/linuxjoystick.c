/*
  linuxjoystick.c:
  Copyright (C) 2010 Justin Glenn Smith <noisesmith@gmail.com>

  This Csound plugin is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This plugin is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this plugin; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA

  USAGE:
  kresultmask linuxjoystick kdevice ktab

  kdevice = The index of the joystick device, either /dev/js<index> or
            /dev/input/js<index>

  ktab = A table to hold input results, should be at least enough elements
         to store one value for each stick axis and one for each button + 2.
         The first two elements of the table are initialized with the number
         of axes and the number of buttons, respectively, when a joystick is
         opened. If a joystick is unplugged during performance, the opcode
         will repeatedly attempt to reopen the device with a delay between
         attempts.

  kresultmask: A bitmask, with a 1 bit for each table index with a new input
               received.
*/

#include "linuxjoystick.h"
#include <errno.h>

static int32_t linuxjoystick (CSOUND *csound, LINUXJOYSTICK *stick)
{
    static int32_t read_pos = 0;
    struct js_event js;
    int32_t read_size;
    int32_t getmore;
    int32_t evtidx;
    long long evtmask = 0;
    char device[256];

    if (UNLIKELY(stick->initme == 0)) {
      stick->timeout = 0;
      stick->devFD = -10;
      stick->initme = 1;
    }
    if (UNLIKELY(*stick->ktable != stick->table)) {
      if (UNLIKELY((void *)(stick->ftp = csound->FTFind(csound, stick->ktable))
                   == NULL)) {
        csound->Warning(csound, Str("linuxjoystick: No such table %f"),
                        *(float*)(stick->ktable));
        return OK;
      }
      stick->table = *stick->ktable;
    }
    if (stick->devFD < 0 || *stick->kdev != stick->dev) {
      if (stick->timeout > 0 && *stick->kdev == stick->dev) {
        (stick->timeout)--;
        return OK;
      }
      stick->dev = (int32_t)MYFLT2LRND(*stick->kdev);
      snprintf(device, 256, "/dev/js%i", stick->dev);
      if ((stick->devFD = open(device, O_RDONLY, O_NONBLOCK)) < 0) {
        snprintf(device, 256, "/dev/input/js%i", stick->dev);
        stick->devFD = open(device, O_RDONLY, O_NONBLOCK);
      }
      if (LIKELY(stick->devFD > 0)) {
        fcntl(stick->devFD, F_SETFL, fcntl(stick->devFD, F_GETFL, 0)|O_NONBLOCK);
        ioctl(stick->devFD, JSIOCGAXES, &stick->numk);
        ioctl(stick->devFD, JSIOCGBUTTONS, &stick->numb);
        if (UNLIKELY(stick->ftp->flen < 2u+(stick->numk)+(stick->numb))) {
          csound->Warning
            (csound,
             Str("linuxjoystick: table %d of size %d too small for data size %d"),
             (int32_t
              )stick->table, stick->ftp->flen, 2+stick->numk+stick->numb);
          return OK;
        }
        stick->ftp->ftable[ 0 ] = (MYFLT) stick->numk;
        stick->ftp->ftable[ 1 ] = (MYFLT) stick->numb;
        evtmask = 3;
      }
      else {
        stick->timeout = 10000;
        csound->Warning(csound,
                        Str("linuxjoystick: could not open device "
                            "/dev/input/js%d for reason: %s\n"),
                        stick->dev, strerror(errno));
        csound->Warning(csound,
                        Str("linuxjoystick: could not open device "
                            "/dev/js%d for reason: %s\n"),
                        stick->dev, strerror(errno));
        return OK;
      }
    }
    getmore = 1;
    while (getmore) {
      read_size = read(stick->devFD, (void *) &js+read_pos,
                       sizeof(struct js_event)-read_pos);
      if (read_size == -1 && errno == EAGAIN ) {
        getmore = 0;
      }
      else if (UNLIKELY(read_size < 1)) {
        csound->Warning(csound, Str("linuxjoystick: read %d closing joystick"),
                        read_size);
        close(stick->devFD);
        stick->devFD = -1;
        getmore = 0;
      }
      else {
        read_pos += read_size;
        if (read_pos == sizeof(struct js_event)) {
          read_pos = 0;
          if (js.type & JS_EVENT_AXIS) {
            evtidx = 2 + js.number;
          }
          else if (js.type & JS_EVENT_BUTTON) {
            evtidx = 2 + stick->numk + js.number;
          }
          else {
            csound->Warning(csound, Str("unknown joystick event type %i"),
                            js.type);
            return OK;
          }
          evtmask = evtmask | (1 << evtidx);
          stick->ftp->ftable[ evtidx ] = (MYFLT) js.value;
        }
      }
    }
    *stick->kresult = (MYFLT) evtmask;
    return OK;
}

static OENTRY localops[] = {
  { "joystick", sizeof(LINUXJOYSTICK), 0,  "k", "kk",
    NULL, (SUBR) linuxjoystick, NULL
  },
};

LINKAGE
