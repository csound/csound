/*
  csound_server.h: Csound UDP server API

  Copyright (C) 2024 V Lazzarini

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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#ifndef UDP_SERVER_H
#define UDP_SERVER_H


#ifdef __cplusplus
extern "C" {
#endif
  
  /**
   * Starts the UDP server on a supplied port number
   * returns CSOUND_SUCCESS if server has been started successfully,
   * otherwise, CSOUND_ERROR.
   */
  PUBLIC int32_t csoundUDPServerStart(CSOUND *csound, uint32_t port);

  /** returns the port number on which the server is running, or
   *  CSOUND_ERROR if the server is not running.
   */
  PUBLIC int32_t csoundUDPServerStatus(CSOUND *csound);

  /**
   * Closes the UDP server, returning CSOUND_SUCCESS if the
   * running server was successfully closed, CSOUND_ERROR otherwise.
   */
  PUBLIC int32_t csoundUDPServerClose(CSOUND *csound);

  /**
   * Turns on the transmission of console messages to UDP on address addr
   * port port. If mirror is one, the messages will continue to be
   * sent to the usual destination (see csoundSetMessaggeCallback())
   * as well as to UDP.
   * returns CSOUND_SUCCESS or CSOUND_ERROR if the UDP transmission
   * could not be set up.
   */
  PUBLIC int32_t csoundUDPConsole(CSOUND *csound, const char *addr,
                              int32_t port, int32_t mirror);

  /**
   * Stop transmitting console messages via UDP
   */
  PUBLIC void csoundStopUDPConsole(CSOUND *csound);

#ifdef __cplusplus
}
#endif
 
#endif
