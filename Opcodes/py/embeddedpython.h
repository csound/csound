/*
  * embeddedpython.h
  *
  * Copyright (C) 2002 Maurizio Umberto Puxeddu
  *
  * This software is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This software is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public
  * License along with this software; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#ifndef _pycsound_embeddedpython_h_
#define _pycsound_embeddedpython_h_

extern void python_startup(int argc, char *argv[]);
extern void python_read_orchestra(char *filename);
extern int python_add_cmdline_definition(char *s);
extern void python_shutdown(void);
extern void python_print_version(void);
extern void python_enable_alternate_streams();

typedef int (*FileWriteCallbackType)(const char *data, int size);

extern FileWriteCallbackType python_set_stdout_write_callback(FileWriteCallbackType callback);
extern FileWriteCallbackType python_set_stderr_write_callback(FileWriteCallbackType callback);

extern int python_stdout_write(const char *data, int size);
extern int python_stderr_write(const char *data, int size);

#endif /* _pycsound_embeddedpython_h_ */
