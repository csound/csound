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

goog.provide("csound.filesystem.constants");

if (typeof goog.global === "undefined") {
  goog.global = {};
}

if (typeof goog.global.BigInt === "undefined") {
  if (typeof BigInt !== "undefined") {
    goog.global.BigInt = BigInt;
  } else {
    goog.global.BigInt = Number;
  }
}

csound.filesystem.constants.WASI_ESUCCESS = 0;
csound.filesystem.constants.WASI_E2BIG = 1;
csound.filesystem.constants.WASI_EACCES = 2;
csound.filesystem.constants.WASI_EADDRINUSE = 3;
csound.filesystem.constants.WASI_EADDRNOTAVAIL = 4;
csound.filesystem.constants.WASI_EAFNOSUPPORT = 5;
csound.filesystem.constants.WASI_EAGAIN = 6;
csound.filesystem.constants.WASI_EALREADY = 7;
csound.filesystem.constants.WASI_EBADF = 8;
csound.filesystem.constants.WASI_EBADMSG = 9;
csound.filesystem.constants.WASI_EBUSY = 10;
csound.filesystem.constants.WASI_ECANCELED = 11;
csound.filesystem.constants.WASI_ECHILD = 12;
csound.filesystem.constants.WASI_ECONNABORTED = 13;
csound.filesystem.constants.WASI_ECONNREFUSED = 14;
csound.filesystem.constants.WASI_ECONNRESET = 15;
csound.filesystem.constants.WASI_EDEADLK = 16;
csound.filesystem.constants.WASI_EDESTADDRREQ = 17;
csound.filesystem.constants.WASI_EDOM = 18;
csound.filesystem.constants.WASI_EDQUOT = 19;
csound.filesystem.constants.WASI_EEXIST = 20;
csound.filesystem.constants.WASI_EFAULT = 21;
csound.filesystem.constants.WASI_EFBIG = 22;
csound.filesystem.constants.WASI_EHOSTUNREACH = 23;
csound.filesystem.constants.WASI_EIDRM = 24;
csound.filesystem.constants.WASI_EILSEQ = 25;
csound.filesystem.constants.WASI_EINPROGRESS = 26;
csound.filesystem.constants.WASI_EINTR = 27;
csound.filesystem.constants.WASI_EINVAL = 28;
csound.filesystem.constants.WASI_EIO = 29;
csound.filesystem.constants.WASI_EISCONN = 30;
csound.filesystem.constants.WASI_EISDIR = 31;
csound.filesystem.constants.WASI_ELOOP = 32;
csound.filesystem.constants.WASI_EMFILE = 33;
csound.filesystem.constants.WASI_EMLINK = 34;
csound.filesystem.constants.WASI_EMSGSIZE = 35;
csound.filesystem.constants.WASI_EMULTIHOP = 36;
csound.filesystem.constants.WASI_ENAMETOOLONG = 37;
csound.filesystem.constants.WASI_ENETDOWN = 38;
csound.filesystem.constants.WASI_ENETRESET = 39;
csound.filesystem.constants.WASI_ENETUNREACH = 40;
csound.filesystem.constants.WASI_ENFILE = 41;
csound.filesystem.constants.WASI_ENOBUFS = 42;
csound.filesystem.constants.WASI_ENODEV = 43;
csound.filesystem.constants.WASI_ENOENT = 44;
csound.filesystem.constants.WASI_ENOEXEC = 45;
csound.filesystem.constants.WASI_ENOLCK = 46;
csound.filesystem.constants.WASI_ENOLINK = 47;
csound.filesystem.constants.WASI_ENOMEM = 48;
csound.filesystem.constants.WASI_ENOMSG = 49;
csound.filesystem.constants.WASI_ENOPROTOOPT = 50;
csound.filesystem.constants.WASI_ENOSPC = 51;
csound.filesystem.constants.WASI_ENOSYS = 52;
csound.filesystem.constants.WASI_ENOTCONN = 53;
csound.filesystem.constants.WASI_ENOTDIR = 54;
csound.filesystem.constants.WASI_ENOTEMPTY = 55;
csound.filesystem.constants.WASI_ENOTRECOVERABLE = 56;
csound.filesystem.constants.WASI_ENOTSOCK = 57;
csound.filesystem.constants.WASI_ENOTSUP = 58;
csound.filesystem.constants.WASI_ENOTTY = 59;
csound.filesystem.constants.WASI_ENXIO = 60;
csound.filesystem.constants.WASI_EOVERFLOW = 61;
csound.filesystem.constants.WASI_EOWNERDEAD = 62;
csound.filesystem.constants.WASI_EPERM = 63;
csound.filesystem.constants.WASI_EPIPE = 64;
csound.filesystem.constants.WASI_EPROTO = 65;
csound.filesystem.constants.WASI_EPROTONOSUPPORT = 66;
csound.filesystem.constants.WASI_EPROTOTYPE = 67;
csound.filesystem.constants.WASI_ERANGE = 68;
csound.filesystem.constants.WASI_EROFS = 69;
csound.filesystem.constants.WASI_ESPIPE = 70;
csound.filesystem.constants.WASI_ESRCH = 71;
csound.filesystem.constants.WASI_ESTALE = 72;
csound.filesystem.constants.WASI_ETIMEDOUT = 73;
csound.filesystem.constants.WASI_ETXTBSY = 74;
csound.filesystem.constants.WASI_EXDEV = 75;
csound.filesystem.constants.WASI_ENOTCAPABLE = 76;

csound.filesystem.constants.WASI_SIGABRT = 0;
csound.filesystem.constants.WASI_SIGALRM = 1;
csound.filesystem.constants.WASI_SIGBUS = 2;
csound.filesystem.constants.WASI_SIGCHLD = 3;
csound.filesystem.constants.WASI_SIGCONT = 4;
csound.filesystem.constants.WASI_SIGFPE = 5;
csound.filesystem.constants.WASI_SIGHUP = 6;
csound.filesystem.constants.WASI_SIGILL = 7;
csound.filesystem.constants.WASI_SIGINT = 8;
csound.filesystem.constants.WASI_SIGKILL = 9;
csound.filesystem.constants.WASI_SIGPIPE = 10;
csound.filesystem.constants.WASI_SIGQUIT = 11;
csound.filesystem.constants.WASI_SIGSEGV = 12;
csound.filesystem.constants.WASI_SIGSTOP = 13;
csound.filesystem.constants.WASI_SIGTERM = 14;
csound.filesystem.constants.WASI_SIGTRAP = 15;
csound.filesystem.constants.WASI_SIGTSTP = 16;
csound.filesystem.constants.WASI_SIGTTIN = 17;
csound.filesystem.constants.WASI_SIGTTOU = 18;
csound.filesystem.constants.WASI_SIGURG = 19;
csound.filesystem.constants.WASI_SIGUSR1 = 20;
csound.filesystem.constants.WASI_SIGUSR2 = 21;
csound.filesystem.constants.WASI_SIGVTALRM = 22;
csound.filesystem.constants.WASI_SIGXCPU = 23;
csound.filesystem.constants.WASI_SIGXFSZ = 24;

csound.filesystem.constants.WASI_FILETYPE_UNKNOWN = 0;
csound.filesystem.constants.WASI_FILETYPE_BLOCK_DEVICE = 1;
csound.filesystem.constants.WASI_FILETYPE_CHARACTER_DEVICE = 2;
csound.filesystem.constants.WASI_FILETYPE_DIRECTORY = 3;
csound.filesystem.constants.WASI_FILETYPE_REGULAR_FILE = 4;
csound.filesystem.constants.WASI_FILETYPE_SOCKET_DGRAM = 5;
csound.filesystem.constants.WASI_FILETYPE_SOCKET_STREAM = 6;
csound.filesystem.constants.WASI_FILETYPE_SYMBOLIC_LINK = 7;

csound.filesystem.constants.WASI_FDFLAG_APPEND = 0x0001;
csound.filesystem.constants.WASI_FDFLAG_DSYNC = 0x0002;
csound.filesystem.constants.WASI_FDFLAG_NONBLOCK = 0x0004;
csound.filesystem.constants.WASI_FDFLAG_RSYNC = 0x0008;
csound.filesystem.constants.WASI_FDFLAG_SYNC = 0x0010;

csound.filesystem.constants.WASI_RIGHT_FD_DATASYNC = goog.global.BigInt(0x0000000000000001);
csound.filesystem.constants.WASI_RIGHT_FD_READ = goog.global.BigInt(0x0000000000000002);
csound.filesystem.constants.WASI_RIGHT_FD_SEEK = goog.global.BigInt(0x0000000000000004);
csound.filesystem.constants.WASI_RIGHT_FD_FDSTAT_SET_FLAGS = goog.global.BigInt(0x0000000000000008);
csound.filesystem.constants.WASI_RIGHT_FD_SYNC = goog.global.BigInt(0x0000000000000010);
csound.filesystem.constants.WASI_RIGHT_FD_TELL = goog.global.BigInt(0x0000000000000020);
csound.filesystem.constants.WASI_RIGHT_FD_WRITE = goog.global.BigInt(0x0000000000000040);
csound.filesystem.constants.WASI_RIGHT_FD_ADVISE = goog.global.BigInt(0x0000000000000080);
csound.filesystem.constants.WASI_RIGHT_FD_ALLOCATE = goog.global.BigInt(0x0000000000000100);
csound.filesystem.constants.WASI_RIGHT_PATH_CREATE_DIRECTORY = goog.global.BigInt(
  0x0000000000000200
);
csound.filesystem.constants.WASI_RIGHT_PATH_CREATE_FILE = goog.global.BigInt(0x0000000000000400);
csound.filesystem.constants.WASI_RIGHT_PATH_LINK_SOURCE = goog.global.BigInt(0x0000000000000800);
csound.filesystem.constants.WASI_RIGHT_PATH_LINK_TARGET = goog.global.BigInt(0x0000000000001000);
csound.filesystem.constants.WASI_RIGHT_PATH_OPEN = goog.global.BigInt(0x0000000000002000);
csound.filesystem.constants.WASI_RIGHT_FD_READDIR = goog.global.BigInt(0x0000000000004000);
csound.filesystem.constants.WASI_RIGHT_PATH_READLINK = goog.global.BigInt(0x0000000000008000);
csound.filesystem.constants.WASI_RIGHT_PATH_RENAME_SOURCE = goog.global.BigInt(0x0000000000010000);
csound.filesystem.constants.WASI_RIGHT_PATH_RENAME_TARGET = goog.global.BigInt(0x0000000000020000);
csound.filesystem.constants.WASI_RIGHT_PATH_FILESTAT_GET = goog.global.BigInt(0x0000000000040000);
csound.filesystem.constants.WASI_RIGHT_PATH_FILESTAT_SET_SIZE = goog.global.BigInt(
  0x0000000000080000
);
csound.filesystem.constants.WASI_RIGHT_PATH_FILESTAT_SET_TIMES = goog.global.BigInt(
  0x0000000000100000
);
csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_GET = goog.global.BigInt(0x0000000000200000);
csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_SET_SIZE = goog.global.BigInt(
  0x0000000000400000
);
csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_SET_TIMES = goog.global.BigInt(
  0x0000000000800000
);
csound.filesystem.constants.WASI_RIGHT_PATH_SYMLINK = goog.global.BigInt(0x0000000001000000);
csound.filesystem.constants.WASI_RIGHT_PATH_REMOVE_DIRECTORY = goog.global.BigInt(
  0x0000000002000000
);
csound.filesystem.constants.WASI_RIGHT_PATH_UNLINK_FILE = goog.global.BigInt(0x0000000004000000);
csound.filesystem.constants.WASI_RIGHT_POLL_FD_READWRITE = goog.global.BigInt(0x0000000008000000);
csound.filesystem.constants.WASI_RIGHT_SOCK_SHUTDOWN = goog.global.BigInt(0x0000000010000000);

csound.filesystem.constants.RIGHTS_ALL =
  csound.filesystem.constants.WASI_RIGHT_FD_DATASYNC |
  csound.filesystem.constants.WASI_RIGHT_FD_READ |
  csound.filesystem.constants.WASI_RIGHT_FD_SEEK |
  csound.filesystem.constants.WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
  csound.filesystem.constants.WASI_RIGHT_FD_SYNC |
  csound.filesystem.constants.WASI_RIGHT_FD_TELL |
  csound.filesystem.constants.WASI_RIGHT_FD_WRITE |
  csound.filesystem.constants.WASI_RIGHT_FD_ADVISE |
  csound.filesystem.constants.WASI_RIGHT_FD_ALLOCATE |
  csound.filesystem.constants.WASI_RIGHT_PATH_CREATE_DIRECTORY |
  csound.filesystem.constants.WASI_RIGHT_PATH_CREATE_FILE |
  csound.filesystem.constants.WASI_RIGHT_PATH_LINK_SOURCE |
  csound.filesystem.constants.WASI_RIGHT_PATH_LINK_TARGET |
  csound.filesystem.constants.WASI_RIGHT_PATH_OPEN |
  csound.filesystem.constants.WASI_RIGHT_FD_READDIR |
  csound.filesystem.constants.WASI_RIGHT_PATH_READLINK |
  csound.filesystem.constants.WASI_RIGHT_PATH_RENAME_SOURCE |
  csound.filesystem.constants.WASI_RIGHT_PATH_RENAME_TARGET |
  csound.filesystem.constants.WASI_RIGHT_PATH_FILESTAT_GET |
  csound.filesystem.constants.WASI_RIGHT_PATH_FILESTAT_SET_SIZE |
  csound.filesystem.constants.WASI_RIGHT_PATH_FILESTAT_SET_TIMES |
  csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_GET |
  csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_SET_TIMES |
  csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_SET_SIZE |
  csound.filesystem.constants.WASI_RIGHT_PATH_SYMLINK |
  csound.filesystem.constants.WASI_RIGHT_PATH_UNLINK_FILE |
  csound.filesystem.constants.WASI_RIGHT_PATH_REMOVE_DIRECTORY |
  csound.filesystem.constants.WASI_RIGHT_POLL_FD_READWRITE |
  csound.filesystem.constants.WASI_RIGHT_SOCK_SHUTDOWN;

csound.filesystem.constants.RIGHTS_BLOCK_DEVICE_BASE = csound.filesystem.constants.RIGHTS_ALL;
csound.filesystem.constants.RIGHTS_BLOCK_DEVICE_INHERITING = csound.filesystem.constants.RIGHTS_ALL;

csound.filesystem.constants.RIGHTS_CHARACTER_DEVICE_BASE = csound.filesystem.constants.RIGHTS_ALL;
csound.filesystem.constants.RIGHTS_CHARACTER_DEVICE_INHERITING =
  csound.filesystem.constants.RIGHTS_ALL;

csound.filesystem.constants.RIGHTS_REGULAR_FILE_BASE =
  csound.filesystem.constants.WASI_RIGHT_FD_DATASYNC |
  csound.filesystem.constants.WASI_RIGHT_FD_READ |
  csound.filesystem.constants.WASI_RIGHT_FD_SEEK |
  csound.filesystem.constants.WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
  csound.filesystem.constants.WASI_RIGHT_FD_SYNC |
  csound.filesystem.constants.WASI_RIGHT_FD_TELL |
  csound.filesystem.constants.WASI_RIGHT_FD_WRITE |
  csound.filesystem.constants.WASI_RIGHT_FD_ADVISE |
  csound.filesystem.constants.WASI_RIGHT_FD_ALLOCATE |
  csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_GET |
  csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_SET_SIZE |
  csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_SET_TIMES |
  csound.filesystem.constants.WASI_RIGHT_POLL_FD_READWRITE;

csound.filesystem.constants.RIGHTS_REGULAR_FILE_INHERITING = goog.global.BigInt(0);

csound.filesystem.constants.RIGHTS_DIRECTORY_BASE =
  csound.filesystem.constants.WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
  csound.filesystem.constants.WASI_RIGHT_FD_SYNC |
  csound.filesystem.constants.WASI_RIGHT_FD_ADVISE |
  csound.filesystem.constants.WASI_RIGHT_PATH_CREATE_DIRECTORY |
  csound.filesystem.constants.WASI_RIGHT_PATH_CREATE_FILE |
  csound.filesystem.constants.WASI_RIGHT_PATH_LINK_SOURCE |
  csound.filesystem.constants.WASI_RIGHT_PATH_LINK_TARGET |
  csound.filesystem.constants.WASI_RIGHT_PATH_OPEN |
  csound.filesystem.constants.WASI_RIGHT_FD_READDIR |
  csound.filesystem.constants.WASI_RIGHT_PATH_READLINK |
  csound.filesystem.constants.WASI_RIGHT_PATH_RENAME_SOURCE |
  csound.filesystem.constants.WASI_RIGHT_PATH_RENAME_TARGET |
  csound.filesystem.constants.WASI_RIGHT_PATH_FILESTAT_GET |
  csound.filesystem.constants.WASI_RIGHT_PATH_FILESTAT_SET_SIZE |
  csound.filesystem.constants.WASI_RIGHT_PATH_FILESTAT_SET_TIMES |
  csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_GET |
  csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_SET_TIMES |
  csound.filesystem.constants.WASI_RIGHT_PATH_SYMLINK |
  csound.filesystem.constants.WASI_RIGHT_PATH_UNLINK_FILE |
  csound.filesystem.constants.WASI_RIGHT_PATH_REMOVE_DIRECTORY |
  csound.filesystem.constants.WASI_RIGHT_POLL_FD_READWRITE;

csound.filesystem.constants.RIGHTS_DIRECTORY_INHERITING =
  csound.filesystem.constants.RIGHTS_DIRECTORY_BASE |
  csound.filesystem.constants.RIGHTS_REGULAR_FILE_BASE;

csound.filesystem.constants.RIGHTS_SOCKET_BASE =
  csound.filesystem.constants.WASI_RIGHT_FD_READ |
  csound.filesystem.constants.WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
  csound.filesystem.constants.WASI_RIGHT_FD_WRITE |
  csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_GET |
  csound.filesystem.constants.WASI_RIGHT_POLL_FD_READWRITE |
  csound.filesystem.constants.WASI_RIGHT_SOCK_SHUTDOWN;

csound.filesystem.constants.RIGHTS_SOCKET_INHERITING = csound.filesystem.constants.RIGHTS_ALL;

csound.filesystem.constants.RIGHTS_TTY_BASE =
  csound.filesystem.constants.WASI_RIGHT_FD_READ |
  csound.filesystem.constants.WASI_RIGHT_FD_FDSTAT_SET_FLAGS |
  csound.filesystem.constants.WASI_RIGHT_FD_WRITE |
  csound.filesystem.constants.WASI_RIGHT_FD_FILESTAT_GET |
  csound.filesystem.constants.WASI_RIGHT_POLL_FD_READWRITE;

csound.filesystem.constants.RIGHTS_TTY_INHERITING = goog.global.BigInt(0);

csound.filesystem.constants.WASI_CLOCK_REALTIME = 0;
csound.filesystem.constants.WASI_CLOCK_MONOTONIC = 1;
csound.filesystem.constants.WASI_CLOCK_PROCESS_CPUTIME_ID = 2;
csound.filesystem.constants.WASI_CLOCK_THREAD_CPUTIME_ID = 3;

csound.filesystem.constants.WASI_EVENTTYPE_CLOCK = 0;
csound.filesystem.constants.WASI_EVENTTYPE_FD_READ = 1;
csound.filesystem.constants.WASI_EVENTTYPE_FD_WRITE = 2;

csound.filesystem.constants.WASI_FILESTAT_SET_ATIM = 1 << 0;
csound.filesystem.constants.WASI_FILESTAT_SET_ATIM_NOW = 1 << 1;
csound.filesystem.constants.WASI_FILESTAT_SET_MTIM = 1 << 2;
csound.filesystem.constants.WASI_FILESTAT_SET_MTIM_NOW = 1 << 3;

csound.filesystem.constants.WASI_O_CREAT = 1 << 0;
csound.filesystem.constants.WASI_O_DIRECTORY = 1 << 1;
csound.filesystem.constants.WASI_O_EXCL = 1 << 2;
csound.filesystem.constants.WASI_O_TRUNC = 1 << 3;

csound.filesystem.constants.WASI_PREOPENTYPE_DIR = 0;

csound.filesystem.constants.WASI_DIRCOOKIE_START = 0;

csound.filesystem.constants.WASI_STDIN_FILENO = 0;
csound.filesystem.constants.WASI_STDOUT_FILENO = 1;
csound.filesystem.constants.WASI_STDERR_FILENO = 2;

csound.filesystem.constants.WASI_WHENCE_SET = 0;
csound.filesystem.constants.WASI_WHENCE_CUR = 1;
csound.filesystem.constants.WASI_WHENCE_END = 2;
