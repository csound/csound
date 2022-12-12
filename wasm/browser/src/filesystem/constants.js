/*
This file is based on @wasmerio/wasi-js which in turn is based on
wasi Node implementation made by Gus Caplan.
 * https://github.com/wasmerio/wasmer-js
 * https://github.com/devsnek/node-wasi

Copyright 2017 Syrus <me@syrusakbary.com>
Copyright 2019 Gus Caplan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

if (goog.global !== undefined) {
  goog.global = {};
}

if (!goog.global.BigInt) {
  goog.global.BigInt = BigInt === undefined ? Number : BigInt;
}

export const WASI_ESUCCESS = 0;
export const WASI_E2BIG = 1;
export const WASI_EACCES = 2;
export const WASI_EADDRINUSE = 3;
export const WASI_EADDRNOTAVAIL = 4;
export const WASI_EAFNOSUPPORT = 5;
export const WASI_EAGAIN = 6;
export const WASI_EALREADY = 7;
export const WASI_EBADF = 8;
export const WASI_EBADMSG = 9;
export const WASI_EBUSY = 10;
export const WASI_ECANCELED = 11;
export const WASI_ECHILD = 12;
export const WASI_ECONNABORTED = 13;
export const WASI_ECONNREFUSED = 14;
export const WASI_ECONNRESET = 15;
export const WASI_EDEADLK = 16;
export const WASI_EDESTADDRREQ = 17;
export const WASI_EDOM = 18;
export const WASI_EDQUOT = 19;
export const WASI_EEXIST = 20;
export const WASI_EFAULT = 21;
export const WASI_EFBIG = 22;
export const WASI_EHOSTUNREACH = 23;
export const WASI_EIDRM = 24;
export const WASI_EILSEQ = 25;
export const WASI_EINPROGRESS = 26;
export const WASI_EINTR = 27;
export const WASI_EINVAL = 28;
export const WASI_EIO = 29;
export const WASI_EISCONN = 30;
export const WASI_EISDIR = 31;
export const WASI_ELOOP = 32;
export const WASI_EMFILE = 33;
export const WASI_EMLINK = 34;
export const WASI_EMSGSIZE = 35;
export const WASI_EMULTIHOP = 36;
export const WASI_ENAMETOOLONG = 37;
export const WASI_ENETDOWN = 38;
export const WASI_ENETRESET = 39;
export const WASI_ENETUNREACH = 40;
export const WASI_ENFILE = 41;
export const WASI_ENOBUFS = 42;
export const WASI_ENODEV = 43;
export const WASI_ENOENT = 44;
export const WASI_ENOEXEC = 45;
export const WASI_ENOLCK = 46;
export const WASI_ENOLINK = 47;
export const WASI_ENOMEM = 48;
export const WASI_ENOMSG = 49;
export const WASI_ENOPROTOOPT = 50;
export const WASI_ENOSPC = 51;
export const WASI_ENOSYS = 52;
export const WASI_ENOTCONN = 53;
export const WASI_ENOTDIR = 54;
export const WASI_ENOTEMPTY = 55;
export const WASI_ENOTRECOVERABLE = 56;
export const WASI_ENOTSOCK = 57;
export const WASI_ENOTSUP = 58;
export const WASI_ENOTTY = 59;
export const WASI_ENXIO = 60;
export const WASI_EOVERFLOW = 61;
export const WASI_EOWNERDEAD = 62;
export const WASI_EPERM = 63;
export const WASI_EPIPE = 64;
export const WASI_EPROTO = 65;
export const WASI_EPROTONOSUPPORT = 66;
export const WASI_EPROTOTYPE = 67;
export const WASI_ERANGE = 68;
export const WASI_EROFS = 69;
export const WASI_ESPIPE = 70;
export const WASI_ESRCH = 71;
export const WASI_ESTALE = 72;
export const WASI_ETIMEDOUT = 73;
export const WASI_ETXTBSY = 74;
export const WASI_EXDEV = 75;
export const WASI_ENOTCAPABLE = 76;

export const WASI_SIGABRT = 0;
export const WASI_SIGALRM = 1;
export const WASI_SIGBUS = 2;
export const WASI_SIGCHLD = 3;
export const WASI_SIGCONT = 4;
export const WASI_SIGFPE = 5;
export const WASI_SIGHUP = 6;
export const WASI_SIGILL = 7;
export const WASI_SIGINT = 8;
export const WASI_SIGKILL = 9;
export const WASI_SIGPIPE = 10;
export const WASI_SIGQUIT = 11;
export const WASI_SIGSEGV = 12;
export const WASI_SIGSTOP = 13;
export const WASI_SIGTERM = 14;
export const WASI_SIGTRAP = 15;
export const WASI_SIGTSTP = 16;
export const WASI_SIGTTIN = 17;
export const WASI_SIGTTOU = 18;
export const WASI_SIGURG = 19;
export const WASI_SIGUSR1 = 20;
export const WASI_SIGUSR2 = 21;
export const WASI_SIGVTALRM = 22;
export const WASI_SIGXCPU = 23;
export const WASI_SIGXFSZ = 24;

export const WASI_FILETYPE_UNKNOWN = 0;
export const WASI_FILETYPE_BLOCK_DEVICE = 1;
export const WASI_FILETYPE_CHARACTER_DEVICE = 2;
export const WASI_FILETYPE_DIRECTORY = 3;
export const WASI_FILETYPE_REGULAR_FILE = 4;
export const WASI_FILETYPE_SOCKET_DGRAM = 5;
export const WASI_FILETYPE_SOCKET_STREAM = 6;
export const WASI_FILETYPE_SYMBOLIC_LINK = 7;

export const WASI_FDFLAG_APPEND = 0x0001;
export const WASI_FDFLAG_DSYNC = 0x0002;
export const WASI_FDFLAG_NONBLOCK = 0x0004;
export const WASI_FDFLAG_RSYNC = 0x0008;
export const WASI_FDFLAG_SYNC = 0x0010;

export const WASI_RIGHT_FD_DATASYNC = goog.global.BigInt(0x0000000000000001);
export const WASI_RIGHT_FD_READ = goog.global.BigInt(0x0000000000000002);
export const WASI_RIGHT_FD_SEEK = goog.global.BigInt(0x0000000000000004);
export const WASI_RIGHT_FD_FDSTAT_SET_FLAGS = goog.global.BigInt(0x0000000000000008);
export const WASI_RIGHT_FD_SYNC = goog.global.BigInt(0x0000000000000010);
export const WASI_RIGHT_FD_TELL = goog.global.BigInt(0x0000000000000020);
export const WASI_RIGHT_FD_WRITE = goog.global.BigInt(0x0000000000000040);
export const WASI_RIGHT_FD_ADVISE = goog.global.BigInt(0x0000000000000080);
export const WASI_RIGHT_FD_ALLOCATE = goog.global.BigInt(0x0000000000000100);
export const WASI_RIGHT_PATH_CREATE_DIRECTORY = goog.global.BigInt(0x0000000000000200);
export const WASI_RIGHT_PATH_CREATE_FILE = goog.global.BigInt(0x0000000000000400);
export const WASI_RIGHT_PATH_LINK_SOURCE = goog.global.BigInt(0x0000000000000800);
export const WASI_RIGHT_PATH_LINK_TARGET = goog.global.BigInt(0x0000000000001000);
export const WASI_RIGHT_PATH_OPEN = goog.global.BigInt(0x0000000000002000);
export const WASI_RIGHT_FD_READDIR = goog.global.BigInt(0x0000000000004000);
export const WASI_RIGHT_PATH_READLINK = goog.global.BigInt(0x0000000000008000);
export const WASI_RIGHT_PATH_RENAME_SOURCE = goog.global.BigInt(0x0000000000010000);
export const WASI_RIGHT_PATH_RENAME_TARGET = goog.global.BigInt(0x0000000000020000);
export const WASI_RIGHT_PATH_FILESTAT_GET = goog.global.BigInt(0x0000000000040000);
export const WASI_RIGHT_PATH_FILESTAT_SET_SIZE = goog.global.BigInt(0x0000000000080000);
export const WASI_RIGHT_PATH_FILESTAT_SET_TIMES = goog.global.BigInt(0x0000000000100000);
export const WASI_RIGHT_FD_FILESTAT_GET = goog.global.BigInt(0x0000000000200000);
export const WASI_RIGHT_FD_FILESTAT_SET_SIZE = goog.global.BigInt(0x0000000000400000);
export const WASI_RIGHT_FD_FILESTAT_SET_TIMES = goog.global.BigInt(0x0000000000800000);
export const WASI_RIGHT_PATH_SYMLINK = goog.global.BigInt(0x0000000001000000);
export const WASI_RIGHT_PATH_REMOVE_DIRECTORY = goog.global.BigInt(0x0000000002000000);
export const WASI_RIGHT_PATH_UNLINK_FILE = goog.global.BigInt(0x0000000004000000);
export const WASI_RIGHT_POLL_FD_READWRITE = goog.global.BigInt(0x0000000008000000);
export const WASI_RIGHT_SOCK_SHUTDOWN = goog.global.BigInt(0x0000000010000000);

export const RIGHTS_ALL =
  WASI_RIGHT_FD_DATASYNC |
  WASI_RIGHT_FD_READ |
  WASI_RIGHT_FD_SEEK |
  WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
  WASI_RIGHT_FD_SYNC |
  WASI_RIGHT_FD_TELL |
  WASI_RIGHT_FD_WRITE |
  WASI_RIGHT_FD_ADVISE |
  WASI_RIGHT_FD_ALLOCATE |
  WASI_RIGHT_PATH_CREATE_DIRECTORY |
  WASI_RIGHT_PATH_CREATE_FILE |
  WASI_RIGHT_PATH_LINK_SOURCE |
  WASI_RIGHT_PATH_LINK_TARGET |
  WASI_RIGHT_PATH_OPEN |
  WASI_RIGHT_FD_READDIR |
  WASI_RIGHT_PATH_READLINK |
  WASI_RIGHT_PATH_RENAME_SOURCE |
  WASI_RIGHT_PATH_RENAME_TARGET |
  WASI_RIGHT_PATH_FILESTAT_GET |
  WASI_RIGHT_PATH_FILESTAT_SET_SIZE |
  WASI_RIGHT_PATH_FILESTAT_SET_TIMES |
  WASI_RIGHT_FD_FILESTAT_GET |
  WASI_RIGHT_FD_FILESTAT_SET_TIMES |
  WASI_RIGHT_FD_FILESTAT_SET_SIZE |
  WASI_RIGHT_PATH_SYMLINK |
  WASI_RIGHT_PATH_UNLINK_FILE |
  WASI_RIGHT_PATH_REMOVE_DIRECTORY |
  WASI_RIGHT_POLL_FD_READWRITE |
  WASI_RIGHT_SOCK_SHUTDOWN;

export const RIGHTS_BLOCK_DEVICE_BASE = RIGHTS_ALL;
export const RIGHTS_BLOCK_DEVICE_INHERITING = RIGHTS_ALL;

export const RIGHTS_CHARACTER_DEVICE_BASE = RIGHTS_ALL;
export const RIGHTS_CHARACTER_DEVICE_INHERITING = RIGHTS_ALL;

export const RIGHTS_REGULAR_FILE_BASE =
  WASI_RIGHT_FD_DATASYNC |
  WASI_RIGHT_FD_READ |
  WASI_RIGHT_FD_SEEK |
  WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
  WASI_RIGHT_FD_SYNC |
  WASI_RIGHT_FD_TELL |
  WASI_RIGHT_FD_WRITE |
  WASI_RIGHT_FD_ADVISE |
  WASI_RIGHT_FD_ALLOCATE |
  WASI_RIGHT_FD_FILESTAT_GET |
  WASI_RIGHT_FD_FILESTAT_SET_SIZE |
  WASI_RIGHT_FD_FILESTAT_SET_TIMES |
  WASI_RIGHT_POLL_FD_READWRITE;

export const RIGHTS_REGULAR_FILE_INHERITING = goog.global.BigInt(0);

export const RIGHTS_DIRECTORY_BASE =
  WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
  WASI_RIGHT_FD_SYNC |
  WASI_RIGHT_FD_ADVISE |
  WASI_RIGHT_PATH_CREATE_DIRECTORY |
  WASI_RIGHT_PATH_CREATE_FILE |
  WASI_RIGHT_PATH_LINK_SOURCE |
  WASI_RIGHT_PATH_LINK_TARGET |
  WASI_RIGHT_PATH_OPEN |
  WASI_RIGHT_FD_READDIR |
  WASI_RIGHT_PATH_READLINK |
  WASI_RIGHT_PATH_RENAME_SOURCE |
  WASI_RIGHT_PATH_RENAME_TARGET |
  WASI_RIGHT_PATH_FILESTAT_GET |
  WASI_RIGHT_PATH_FILESTAT_SET_SIZE |
  WASI_RIGHT_PATH_FILESTAT_SET_TIMES |
  WASI_RIGHT_FD_FILESTAT_GET |
  WASI_RIGHT_FD_FILESTAT_SET_TIMES |
  WASI_RIGHT_PATH_SYMLINK |
  WASI_RIGHT_PATH_UNLINK_FILE |
  WASI_RIGHT_PATH_REMOVE_DIRECTORY |
  WASI_RIGHT_POLL_FD_READWRITE;

export const RIGHTS_DIRECTORY_INHERITING = RIGHTS_DIRECTORY_BASE | RIGHTS_REGULAR_FILE_BASE;

export const RIGHTS_SOCKET_BASE =
  WASI_RIGHT_FD_READ |
  WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
  WASI_RIGHT_FD_WRITE |
  WASI_RIGHT_FD_FILESTAT_GET |
  WASI_RIGHT_POLL_FD_READWRITE |
  WASI_RIGHT_SOCK_SHUTDOWN;

export const RIGHTS_SOCKET_INHERITING = RIGHTS_ALL;

export const RIGHTS_TTY_BASE =
  WASI_RIGHT_FD_READ |
  WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
  WASI_RIGHT_FD_WRITE |
  WASI_RIGHT_FD_FILESTAT_GET |
  WASI_RIGHT_POLL_FD_READWRITE;

export const RIGHTS_TTY_INHERITING = goog.global.BigInt(0);

export const WASI_CLOCK_REALTIME = 0;
export const WASI_CLOCK_MONOTONIC = 1;
export const WASI_CLOCK_PROCESS_CPUTIME_ID = 2;
export const WASI_CLOCK_THREAD_CPUTIME_ID = 3;

export const WASI_EVENTTYPE_CLOCK = 0;
export const WASI_EVENTTYPE_FD_READ = 1;
export const WASI_EVENTTYPE_FD_WRITE = 2;

export const WASI_FILESTAT_SET_ATIM = Math.trunc(1);
export const WASI_FILESTAT_SET_ATIM_NOW = 1 << 1;
export const WASI_FILESTAT_SET_MTIM = 1 << 2;
export const WASI_FILESTAT_SET_MTIM_NOW = 1 << 3;

export const WASI_O_CREAT = Math.trunc(1);
export const WASI_O_DIRECTORY = 1 << 1;
export const WASI_O_EXCL = 1 << 2;
export const WASI_O_TRUNC = 1 << 3;

export const WASI_PREOPENTYPE_DIR = 0;

export const WASI_DIRCOOKIE_START = 0;

export const WASI_STDIN_FILENO = 0;
export const WASI_STDOUT_FILENO = 1;
export const WASI_STDERR_FILENO = 2;

export const WASI_WHENCE_SET = 0;
export const WASI_WHENCE_CUR = 1;
export const WASI_WHENCE_END = 2;
