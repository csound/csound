/*
    msg_attr.h:

    Copyright (C) 2005 Istvan Varga

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
*/

#ifndef CSOUND_MSG_ATTR_H
#define CSOUND_MSG_ATTR_H

/* message types (only one can be specified) */

/* standard message */
#define CSOUNDMSG_DEFAULT       (0x0000)
/* error message (initerror, perferror, etc.) */
#define CSOUNDMSG_ERROR         (0x1000)
/* orchestra opcodes (e.g. printks) */
#define CSOUNDMSG_ORCH          (0x2000)
/* for progress display and heartbeat characters */
#define CSOUNDMSG_REALTIME      (0x3000)

/* format attributes (colors etc.), use the bitwise OR of any of these: */

#define CSOUNDMSG_FG_BLACK      (0x0100)
#define CSOUNDMSG_FG_RED        (0x0101)
#define CSOUNDMSG_FG_GREEN      (0x0102)
#define CSOUNDMSG_FG_YELLOW     (0x0103)
#define CSOUNDMSG_FG_BLUE       (0x0104)
#define CSOUNDMSG_FG_MAGENTA    (0x0105)
#define CSOUNDMSG_FG_CYAN       (0x0106)
#define CSOUNDMSG_FG_WHITE      (0x0107)

#define CSOUNDMSG_FG_BOLD       (0x0108)

#define CSOUNDMSG_BG_BLACK      (0x0200)
#define CSOUNDMSG_BG_RED        (0x0210)
#define CSOUNDMSG_BG_GREEN      (0x0220)
#define CSOUNDMSG_BG_ORANGE     (0x0230)
#define CSOUNDMSG_BG_BLUE       (0x0240)
#define CSOUNDMSG_BG_MAGENTA    (0x0250)
#define CSOUNDMSG_BG_CYAN       (0x0260)
#define CSOUNDMSG_BG_GREY       (0x0270)

 /* ------------------------------------------------------------------------ */

#define CSOUNDMSG_FG_MASK       (0x010F)
#define CSOUNDMSG_BG_MASK       (0x0270)

#define CS_COLOR_FG_BLACK       "\033[30m"
#define CS_COLOR_FG_RED         "\033[31m"
#define CS_COLOR_FG_GREEN       "\033[32m"
#define CS_COLOR_FG_YELLOW      "\033[33m"
#define CS_COLOR_FG_BLUE        "\033[34m"
#define CS_COLOR_FG_MAGENTA     "\033[35m"
#define CS_COLOR_FG_CYAN        "\033[36m"
#define CS_COLOR_FG_WHITE       "\033[37m"

#define CS_COLOR_FG_BOLD        "\033[1m"

#define CS_COLOR_BG_BLACK       "\033[40m"
#define CS_COLOR_BG_RED         "\033[41m"
#define CS_COLOR_BG_GREEN       "\033[42m"
#define CS_COLOR_BG_ORANGE      "\033[43m"
#define CS_COLOR_BG_BLUE        "\033[44m"
#define CS_COLOR_BG_MAGENTA     "\033[45m"
#define CS_COLOR_BG_CYAN        "\033[46m"
#define CS_COLOR_BG_GREY        "\033[47m"

#define CS_COLOR_OFF            "\033[m"

#endif      /* CSOUND_MSG_ATTR_H */

