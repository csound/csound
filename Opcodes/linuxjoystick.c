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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA

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
               received, for indices 0 through 63. Higher indices still
               receive values in the table.
*/

#include "linuxjoystick.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

static int32_t linuxjoystick_deinit(CSOUND *csound, LINUXJOYSTICK *stick)
{
    IGN(csound);
    if (stick->initme && stick->devFD >= 0)
      close(stick->devFD);
    stick->devFD = -1;
    return OK;
}

static int32_t linuxjoystick_init(CSOUND *csound, LINUXJOYSTICK *stick)
{
    linuxjoystick_deinit(csound, stick);
    stick->initme = 1;
    stick->timeout = 0;
    stick->dev = -1;
    stick->ftp = NULL;
    stick->table = FL(0.0);
    *stick->kresult = FL(0.0);
    return OK;
}

static int32_t linuxjoystick(CSOUND *csound, LINUXJOYSTICK *stick)
{
    struct js_event js;
    uint64_t evtmask = 0;
    char device[256];
    int32_t dev;
    FUNC *ftp;

    *stick->kresult = FL(0.0);
    if (UNLIKELY(!(*stick->kdev >= FL(0.0) &&
                   (double)*stick->kdev <= INT32_MAX)))
      return csound->PerfError(csound, &stick->h, "%s",
                              Str("joystick: invalid device number"));
    dev = (int32_t)MYFLT2LRND(*stick->kdev);
    /* Resolve ktab each cycle: the table may have been replaced or freed. */
    ftp = csound->FTFind(csound, stick->ktable);
    if (UNLIKELY(ftp == NULL))
      return csound->PerfError(csound, &stick->h, "%s",
                              Str("joystick: no such table"));

    if (dev != stick->dev) {
      linuxjoystick_deinit(csound, stick);
      stick->dev = dev;
      stick->timeout = 0;
    }
    if (stick->devFD < 0) {
      if (stick->timeout > 0) {
        --stick->timeout;
        return OK;
      }
      snprintf(device, sizeof(device), "/dev/js%d", dev);
      stick->devFD = open(device, O_RDONLY | O_NONBLOCK);
      if (stick->devFD < 0) {
        snprintf(device, sizeof(device), "/dev/input/js%d", dev);
        stick->devFD = open(device, O_RDONLY | O_NONBLOCK);
      }
      if (stick->devFD < 0) {
        stick->timeout = 10000;
        csound->Warning(csound, "%s", Str("joystick: could not open device %d: %s"),
                        dev, strerror(errno));
        return OK;
      }
      if (ioctl(stick->devFD, JSIOCGAXES, &stick->numk) < 0 ||
          ioctl(stick->devFD, JSIOCGBUTTONS, &stick->numb) < 0) {
        csound->Warning(csound, "%s", Str("joystick: could not query device %d: %s"),
                        dev, strerror(errno));
        linuxjoystick_deinit(csound, stick);
        stick->timeout = 10000;
        return OK;
      }
      stick->ftp = NULL;
    }

    /* Check the current table even after a successful device open. */
    if (UNLIKELY(ftp->flen < 2u + stick->numk + stick->numb))
      return csound->PerfError(csound, &stick->h, "%s",
                              Str("joystick: table too small for device data"));
    if (ftp != stick->ftp || *stick->ktable != stick->table ||
        ftp->ftable[0] != stick->numk || ftp->ftable[1] != stick->numb) {
      ftp->ftable[0] = (MYFLT)stick->numk;
      ftp->ftable[1] = (MYFLT)stick->numb;
      stick->ftp = ftp;
      stick->table = *stick->ktable;
      evtmask = 3;
    }

    for (;;) {
      uint32_t evtidx;
      /* The Linux joystick API returns whole js_event records. */
      ssize_t read_size = read(stick->devFD, &js, sizeof(js));
      if (read_size < 0 && errno == EINTR) continue;
      if (read_size < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
      if (UNLIKELY(read_size != (ssize_t)sizeof(js))) {
        csound->Warning(csound, "%s", Str("joystick: read failed, closing device %d"),
                        dev);
        linuxjoystick_deinit(csound, stick);
        stick->timeout = 10000;
        break;
      }
      switch (js.type & ~JS_EVENT_INIT) {
      case JS_EVENT_AXIS:
        if (UNLIKELY(js.number >= stick->numk)) continue;
        evtidx = 2u + js.number;
        break;
      case JS_EVENT_BUTTON:
        if (UNLIKELY(js.number >= stick->numb)) continue;
        evtidx = 2u + stick->numk + js.number;
        break;
      default:
        continue;
      }
      /* Higher entries still update the table, beyond the scalar mask. */
      if (evtidx < 64)
        evtmask |= UINT64_C(1) << evtidx;
      ftp->ftable[evtidx] = (MYFLT)js.value;
    }
    *stick->kresult = (MYFLT)evtmask;
    return OK;
}

static OENTRY localops[] = {
  { "joystick", sizeof(LINUXJOYSTICK), 0,  "k", "kk",
    (SUBR) linuxjoystick_init, (SUBR) linuxjoystick,
    (SUBR) linuxjoystick_deinit
  },
};

LINKAGE
