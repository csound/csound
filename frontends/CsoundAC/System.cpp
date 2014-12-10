/*
 * C S O U N D
 *
 * L I C E N S E
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if defined(HAVE_UNISTD_H) || defined(MACOSX)
#include <unistd.h>
#endif

#include <cstdarg>
#include <vector>
#include <string>
#include <cstdio>
#include <csound.h>
#include "System.hpp"
#include "CsoundFile.hpp"
#include "Conversions.hpp"
#include <csound.h>

namespace csound
{

  Logger::Logger()
  {
  }

  Logger::~Logger()
  {
  }

  void Logger::write(const char *text)
  {
    fprintf(stderr, "%s", text);
  }

  ThreadLock::ThreadLock() : lock(0)
  {
  }

  ThreadLock::~ThreadLock()
  {
    close();
  }

  void ThreadLock::open()
  {
    lock = System::createThreadLock();
  }

  void ThreadLock::close()
  {
    if(lock)
      {
        System::destroyThreadLock(lock);
        lock = 0;
      }
  }

  bool ThreadLock::isOpen()
  {
    return (lock != 0);
  }

  void ThreadLock::startWait(size_t milliseconds)
  {
    if(lock)
      {
        System::waitThreadLock(lock, milliseconds);
      }
  }

  void ThreadLock::endWait()
  {
    if(lock)
      {
        System::notifyThreadLock(lock);
      }
  }

  FILE *System::logfile = 0;

  int System::messageLevel = ERROR_LEVEL | WARNING_LEVEL | INFORMATION_LEVEL;

  void * System::userdata_ = 0;

  void (*System::messageCallback)(CSOUND *csound, int attribute, const char *format, va_list valist) = 0;

  void System::setUserdata(void *userdata)
  {
    userdata_ = userdata;
  }

  void *System::getUserdata()
  {
    return userdata_;
  }

  void System::message(CSOUND *csound, int level, const char *format,...)
  {
    if((level & messageLevel) == level)
      {
        va_list marker;
        va_start(marker, format);
        message(csound, level, format, marker);
        va_end(marker);
      }
  }

  void System::error(CSOUND *csound, const char *format,...)
  {
    if((ERROR_LEVEL & messageLevel) == ERROR_LEVEL)
      {
        va_list marker;
        va_start(marker, format);
        message(csound, ERROR_LEVEL, format, marker);
        va_end(marker);
      }
  }

  void System::error(const char *format,...)
  {
    if((ERROR_LEVEL & messageLevel) == ERROR_LEVEL)
      {
        va_list marker;
        va_start(marker, format);
        message((CSOUND*) userdata_, ERROR_LEVEL, format, marker);
        va_end(marker);
      }
  }

  void System::warn(CSOUND *csound, const char *format,...)
  {
    if((WARNING_LEVEL & messageLevel) == WARNING_LEVEL)
      {
        va_list marker;
        va_start(marker, format);
        message(csound, WARNING_LEVEL, format, marker);
        va_end(marker);
      }
  }

  void System::warn(const char *format,...)
  {
    if((WARNING_LEVEL & messageLevel) == WARNING_LEVEL)
      {
        va_list marker;
        va_start(marker, format);
        message((CSOUND*) userdata_, WARNING_LEVEL, format, marker);
        va_end(marker);
      }
  }

  void System::inform(CSOUND *csound, const char *format,...)
  {
    if((INFORMATION_LEVEL & messageLevel) == INFORMATION_LEVEL)
      {
        va_list marker;
        va_start(marker, format);
        message(csound, INFORMATION_LEVEL, format, marker);
        va_end(marker);
      }
  }

  void System::inform(const char *format,...)
  {
    if((INFORMATION_LEVEL & messageLevel) == INFORMATION_LEVEL)
      {
        va_list marker;
        va_start(marker, format);
        message((CSOUND*) userdata_, INFORMATION_LEVEL, format, marker);
        va_end(marker);
      }
  }

  void System::debug(CSOUND *csound, const char *format,...)
  {
    if((DEBUGGING_LEVEL & messageLevel) == DEBUGGING_LEVEL)
      {
        va_list marker;
        va_start(marker, format);
        message(csound, DEBUGGING_LEVEL, format, marker);
        va_end(marker);
      }
  }

  void System::debug(const char *format,...)
  {
    if((DEBUGGING_LEVEL & messageLevel) == DEBUGGING_LEVEL)
      {
        va_list marker;
        va_start(marker, format);
        message((CSOUND*) userdata_, DEBUGGING_LEVEL, format, marker);
        va_end(marker);
      }
  }

  void System::message(CSOUND *csound, const char *format,...)
  {
    va_list marker;
    va_start(marker, format);
    message(csound, messageLevel, format, marker);
    va_end(marker);
  }

  void System::message(const char *format,...)
  {
    va_list marker;
    va_start(marker, format);
    message((CSOUND*) userdata_, messageLevel, format, marker);
    va_end(marker);
  }

  void System::message(const char *format, va_list valist)
  {
    message((CSOUND*) userdata_, messageLevel, format, valist);
  }

  void System::message(CSOUND *csound, const char *format, va_list valist)
  {
    if (logfile) {
      vfprintf(logfile, format, valist);
      fflush(logfile);
    }
    if(messageCallback) {
      messageCallback(csound, messageLevel, format, valist);
    }
    else {
      vfprintf(stderr, format, valist);
    }
  }

  void System::message(CSOUND *csound, int attribute, const char *format, va_list valist)
  {
    if (logfile) {
      vfprintf(logfile, format, valist);
      fflush(logfile);
    }
    if(messageCallback)
      {
        messageCallback(csound, attribute, format, valist);
      }
    else
      {
        vfprintf(stderr, format, valist);
      }
  }

  int System::setMessageLevel(int messageLevel_)
  {
    int returnValue = messageLevel;
    messageLevel = messageLevel_;
    return returnValue;
  }

  int System::getMessageLevel()
  {
    return messageLevel;
  }

  void System::setMessageCallback(MessageCallbackType messageCallback_)
  {
    messageCallback = messageCallback_;
  }

  MessageCallbackType System::getMessageCallback()
  {
    return messageCallback;
  }

  clock_t System::startTiming()
  {
    return clock();
  }

  double System::stopTiming(clock_t beganAt)
  {
    clock_t endedAt = clock();
    clock_t elapsed = endedAt - beganAt;
    return double(elapsed) / double(CLOCKS_PER_SEC);
  }

  void System::yieldThread()
  {
  }

  int System::openLibrary(void **library, std::string filename)
  {
    return csoundOpenLibrary(library, filename.c_str());
  }

  void *System::getSymbol(void *library, std::string name)
  {
    void *procedureAddress = 0;
    procedureAddress = csoundGetLibrarySymbol(library, name.c_str());
    return procedureAddress;
  }

  void System::closeLibrary(void *library)
  {
    csoundCloseLibrary(library);
  }

  void System::setLogfile(FILE *logfile_)
  {
    logfile = logfile_;
  }

  FILE *System::getLogfile()
  {
    return logfile;
  }

#if defined(WIN32)

#include <process.h>
#include <windows.h>
#include <stdlib.h>
#include <io.h>

  int System::execute(const char *command)
  {
    STARTUPINFO startupInfo;
    memset(&startupInfo, 0, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInformation;
    memset(&processInformation, 0, sizeof(processInformation));
    return CreateProcess(0,
                         const_cast<char *>(command),
                         0,
                         0,
                         0,
                         DETACHED_PROCESS,
                         0,
                         0,
                         &startupInfo,
                         &processInformation);
  }

  int System::shellOpen(const char *filename, const char *command)
  {
    int returnValue = 0;
    int hInstance = (int) ShellExecute(0,
                                       command,
                                       filename,
                                       0,
                                       0,
                                       SW_SHOWNORMAL);
    returnValue = !(hInstance > 32);
    return returnValue;
  }

  void System::parsePathname(const std::string pathname,
                             std::string &drive,
                             std::string &base,
                             std::string &file,
                             std::string &extension)
  {
    char drive_[_MAX_DRIVE];
    char base_[_MAX_DIR];
    char file_[_MAX_FNAME];
    char extension_[_MAX_EXT];
    _splitpath(pathname.c_str(), drive_, base_, file_, extension_);
    drive = drive_;
    base = base_;
    file = file_;
    extension = extension_;
  }

  std::vector<std::string> System::getFilenames(std::string path)
  {
    std::vector<std::string> names;
    struct _finddata_t finddata;
    int intptr = _findfirst(path.c_str(), &finddata);
    if(intptr != -1)
      {
        if((finddata.attrib & _A_SUBDIR) != _A_SUBDIR)
          {
            names.push_back(finddata.name);
          }
        while(_findnext(intptr, &finddata) != -1)
          {
            if((finddata.attrib & _A_SUBDIR) != _A_SUBDIR)
              {
                names.push_back(finddata.name);
              }
          }
        _findclose(intptr);
      }
    return names;
  }

  std::vector<std::string> System::getDirectoryNames(std::string path)
  {
    std::vector<std::string> names;
    struct _finddata_t finddata;
    int intptr = _findfirst(path.c_str(), &finddata);
    if(intptr != -1)
      {
        if((finddata.attrib & _A_SUBDIR) == _A_SUBDIR)
          {
            if(strcmp(finddata.name, ".") == 0)
              {
              }
            else if(strcmp(finddata.name, "..") == 0)
              {
              }
            else
              {
                names.push_back(finddata.name);
              }
          }
        while(_findnext(intptr, &finddata) != -1)
          {
            if((finddata.attrib & _A_SUBDIR) == _A_SUBDIR)
              {
                if(strcmp(finddata.name, ".") == 0)
                  {
                  }
                else if(strcmp(finddata.name, "..") == 0)
                  {
                  }
                else
                  {
                    names.push_back(finddata.name);
                  }
              }
          }
        _findclose(intptr);
      }
    return names;
  }

  void *System::createThread(void (*threadRoutine)(void *threadData), void *data, int priority)
  {
    return (void *) _beginthread(threadRoutine, (unsigned int)0, data);
  }

  void *System::createThreadLock()
  {
    return (void *)CreateEvent(0, false, false, 0);
  }

  void System::waitThreadLock(void *lock, size_t milliseconds)
  {
    WaitForSingleObject((HANDLE) lock, milliseconds);
  }

  void System::notifyThreadLock(void *lock)
  {
    SetEvent((HANDLE) lock);
  }

  void System::destroyThreadLock(void *lock)
  {
    CloseHandle((HANDLE) lock);
  }

  std::string System::getSharedLibraryExtension()
  {
    return "dll";
  }

  void System::sleep(double milliseconds)
  {
    Sleep((int) (milliseconds + 0.999));
  }

  void System::beep()
  {
    Beep(880, 1000);
  }

#elif defined(LINUX) || defined(MACOSX)

#include <dlfcn.h>
#include <dirent.h>
#include <libgen.h>

  std::string System::getSharedLibraryExtension()
  {
    return "so";
  }

  size_t strlcpy(char *d, const char *s, size_t bufsize)
  {
    size_t len = strlen(s);
    size_t ret = len;
    if (bufsize <= 0) return 0;
    if (len >= bufsize) len = bufsize-1;
    memcpy(d, s, len);
    d[len] = 0;
    return ret;
  }

#ifndef MACOSX
  char *basename(const char *path)
#else
  char *basename_(const char *path)
#endif
  {
    static char bname[NAME_MAX + 1];
    register const char *endp, *startp;

    /* Empty or NULL string gets treated as "." */
    if (path == NULL || *path == '\0') {
      (void)strlcpy(bname, ".", sizeof bname);
      return(bname);
    }

    /* Strip trailing slashes */
    endp = path + strlen(path) - 1;
    while (endp > path && *endp == '/')
      endp--;

    /* All slashes become "/" */
    if (endp == path && *endp == '/') {
      (void)strlcpy(bname, "/", sizeof bname);
      return(bname);
    }

    /* Find the start of the base */
    startp = endp;
    while (startp > path && *(startp - 1) != '/')
      startp--;

    if ((int) (endp - startp) + 2 > (int) sizeof(bname)) {
      //errno = ENAMETOOLONG;
      return(NULL);
    }
    strlcpy(bname, startp, endp - startp + 2);
    return(bname);
  }

  char *dirname(const char *path)
  {
    static char bname[NAME_MAX +1];
    register const char *endp;

    /* Empty or NULL string gets treated as "." */
    if (path == NULL || *path == '\0') {
      (void)strlcpy(bname, ".", sizeof(bname));
      return(bname);
    }

    /* Strip trailing slashes */
    endp = path + strlen(path) - 1;
    while (endp > path && *endp == '/')
      endp--;

    /* Find the start of the dir */
    while (endp > path && *endp != '/')
      endp--;

    /* Either the dir is "/" or there are no slashes */
    if (endp == path) {
      (void)strlcpy(bname, *endp == '/' ? "/" : ".", sizeof bname);
      return(bname);
    } else {
      do {
        endp--;
      } while (endp > path && *endp == '/');
    }

    if ((int) (endp - path) + 2 > (int) sizeof(bname)) {
      //errno = ENAMETOOLONG;
      return(NULL);
    }
    strlcpy(bname, path, endp - path + 2);
    return(bname);
  }

  int System::execute(const char *command)
  {
    int returnValue = fork();
    if(!returnValue)
      {
        std::vector<std::string> args;
        std::vector<char *> argv;
        std::string buffer = command;
        scatterArgs(buffer, args, argv);
        argv.push_back((char*) 0);      // argv[] should be null-terminated
        execvp(argv[0], &argv.front());
      }
    return returnValue;
  }

  int System::shellOpen(const char *filename, const char *command)
  {
    std::string buffer = filename;
    buffer += " ";
    buffer += command;
    return System::execute(buffer.c_str());
  }

  void System::parsePathname(const std::string pathname,
                             std::string &drive,
                             std::string &directory,
                             std::string &file,
                             std::string &extension)
  {
    drive.erase();
    directory.erase();
    file.erase();
    extension.erase();
    char *dirTemp = Conversions::dupstr(pathname.c_str());
    directory = dirname(dirTemp);
    char *fileTemp = Conversions::dupstr(pathname.c_str());
#ifndef MACOSX
    file = basename(fileTemp);
#else
     file = basename_(fileTemp);
#endif

    int periodPosition = pathname.find_last_of(".");
    if(periodPosition != -1)
      {
        extension = pathname.substr(periodPosition + 1);
      }
    free(dirTemp);
    free(fileTemp);
  }

  std::vector<std::string> System::getFilenames(std::string path)
  {
    std::vector<std::string> names;
    return names;
  }

  std::vector<std::string> System::getDirectoryNames(std::string path)
  {
    std::vector<std::string> names;
    return names;
  }

  void *System::createThread(void (*threadRoutine)(void *threadData), void *data, int priority)
  {
    pthread_t *pthread = new pthread_t;
    if(pthread_create(pthread,
                      0,
                      (void *(*) (void*)) threadRoutine,
                      data) == 0)
      {
        return pthread;
      }
    else
      {
        delete pthread;
        return 0;
      }
  }

  void *System::createThreadLock()
  {
    pthread_mutex_t *pthread_mutex = new pthread_mutex_t;
    if(pthread_mutex_init(pthread_mutex, 0) == 0)
      {
        // for consistency with Win32 version
        pthread_mutex_trylock(pthread_mutex);
        return pthread_mutex;
      }
    else
      {
        delete pthread_mutex;
        return 0;
      }
  }

  void System::waitThreadLock(void *lock, size_t milliseconds)
  {
    pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
    /* int returnValue = */ pthread_mutex_lock(pthread_mutex);
  }

  void System::notifyThreadLock(void *lock)
  {
    pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
    /* int returnValue = */ pthread_mutex_unlock(pthread_mutex);
  }

  void System::destroyThreadLock(void *lock)
  {
    pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
    /* int returnValue = */ pthread_mutex_destroy(pthread_mutex);
    delete pthread_mutex;
    /* pthread_mutex = 0; */
  }

  void System::sleep(double milliseconds)
  {
    csoundSleep((int) (milliseconds + 0.999));
  }

  void System::beep()
  {
    beep();
  }

#endif
}

