/**
 * C S O U N D   V S T
 *
 * A VST plugin version of Csound, with Python scripting.
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
#ifndef CROSSPLATFORM_H
#define CROSSPLATFORM_H

#ifdef SWIG
%module CsoundVST
%{
#include <string>
#include <vector>
#include <cstdarg>
#include <ctime>
  %}
#else
#include <string>
#include <vector>
#include <cstdarg>
#include <ctime>
#endif

namespace csound
{
  typedef void (*MessageCallbackType)(ENVIRON *csound, int attribute, const char *format, va_list valist);

  class Logger
  {
  public:
    Logger();
    virtual ~Logger();
    virtual void write(const char *text);
  };

  /**
   * Abstraction layer for a minimal set of system services.
   */
  class System
  {
    static void *userdata_;
    static int messageLevel;
    static void (*messageCallback)(ENVIRON *csound, int attribute, const char *format, va_list valist);
  public:
    enum Level
      {
        ERROR_LEVEL                     = 1,
        WARNING_LEVEL           = 2,
        INFORMATION_LEVEL       = 4,
        DEBUGGING_LEVEL         = 8,
      };
    /**
     *  Parses a filename into its component parts, which are returned in the arguments.
     *  On Unix and Linux, "drive" is always empty.
     */
    static void parsePathname(const std::string pathname, std::string &drive, std::string &base, std::string &file, std::string &extension);
    /**
     *  Opens a shared library; useful for loading plugins.
     */
    static void *openLibrary(std::string filename);
    /**
     *  Returns the address of a symbol (function or object) in a shared library;
     *  useful for loading plugin functions.
     */
    static void *getSymbol(void *library, std::string name);
    /**
     *  Closes a shared library.
     */
    static void closeLibrary(void *library);
    /**
     *  Lists filenames in a directory;
     *  useful for locating plugins.
     */
    static std::vector<std::string> getFilenames(std::string directoryName);
    /**
     *  Lists directory names in a directory;
     *  useful for locating plugins.
     */
    static std::vector<std::string> getDirectoryNames(std::string directoryName);
    /**
     *  Creates a new thread.
     */
    static void *createThread(void (*threadRoutine)(void *threadData), void *data, int priority);
    /**
     *  Creates a thread lock.
     */
    static void *createThreadLock();
    /**
     *  Waits on a thread lock.
     * Zero timeout means infinite timeout.
     */
    static void waitThreadLock(void *lock, size_t timeoutMilliseconds = 0);
    /**
     *  Releases a thread lock.
     */
    static void notifyThreadLock(void *lock);
    /**
     *  Destroys a thread lock.
     */
    static void destroyThreadLock(void *lock);
    /**
     *  Sets message level, returns old message level.
     */
    static int setMessageLevel(int messageLevel);
    /**
     *  Yields to the next waiting thread.
     */
    static void yieldThread();
    /**
     *  Returns current system message level.
     */
    static int getMessageLevel();
    /**
     *  Sets userdata for message printing.
     */
    static void setUserdata(void *userdata);
    /**
     * Returns userdata for message printing.
     */
    static void *getUserdata();
#ifndef SWIG
    /**
     *  Prints a message if the ERROR_LEVEL flag is set.
     */
    static void error(ENVIRON *csound, const char *format,...);
    /**
     *  Prints a message if the ERROR_LEVEL flag is set.
     */
    static void error(const char *format,...);
    /**
     *  Prints a message if the WARNNING_LEVEL flag is set.
     */
    static void warn(ENVIRON *csound, const char *format,...);
    /**
     *  Prints a message if the WARNNING_LEVEL flag is set.
     */
    static void warn(const char *format,...);
    /**
     *  Prints a message if the INFORMATION_LEVEL flag is set.
     */
    static void inform(ENVIRON *csound, const char *format,...);
    /**
     *  Prints a message if the INFORMATION_LEVEL flag is set.
     */
    static void inform(const char *format,...);
    /**
     *  Prints a message if the DEBUGGING_LEVEL flag is set.
     */
    static void debug(ENVIRON *csound, const char *format,...);
    /**
     *  Prints a message if the DEBUGGING_LEVEL flag is set.
     */
    static void debug(const char *format,...);
    /**
     *  Prints a message.
     */
    static void message(ENVIRON *csound, const char *format,...);
    /**
     *  Prints a message.
     */
    static void message(const char *format,...);
    /**
     *  Prints a message.
     */
    static void message(ENVIRON *csound, const char *format, va_list valist);
    /**
     *  Prints a message.
     */
    static void message(const char *format, va_list valist);
    /**
     *  Prints a message.
     */
    static void message(ENVIRON *csound, int level, const char *format,...);
    /**
     *  Prints a message.
     */
    static void message(ENVIRON *csound, int attribute, const char *format, va_list valist);
    /**
     *  Sets message callback.
     */
    static void setMessageCallback(void (*messageCallback_)(ENVIRON *csound, int attribute, const char *format, va_list marker));
    /**
     *  Return the message callback, or null if none.
     */
    static MessageCallbackType getMessageCallback();
#endif
    /**
     *  Execute a system command or program.
     */
    static int execute(const char *command);
    /**
     *  Open a file using the operating system shell.
     */
    static int shellOpen(const char *filename, const char *command = "open");
    /**
     *  Returns the standard filename extension for a shared library,
     *  such as "dll" or "so".
     */
    static std::string getSharedLibraryExtension();
    /**
     *  Starts timing.
     */
    static clock_t startTiming();
    /**
     *  Stop timing, and return elapsed seonds.
     */
    static double stopTiming(clock_t startedAt);
    /**
     *  Sleep the indicated number of milliseconds.
     */
    static void sleep(double milliseconds);
    /**
     *  Make some sort of noticeable sound.
     */
    static void beep();
  };

  /**
   * Encapsulates a thread monitor, such as a Windows event handle.
   */
  class ThreadLock
  {
    void *lock;
  public:
    ThreadLock();
    virtual ~ThreadLock();
    /**
     * Creates and initializes the monitor.
     * The monitor is in a non-notified or unsignaled state.
     */
    virtual void open();
    /**
     * Destroys the monitor.
     */
    virtual void close();
    /**
     * Returns whether the monitor is open.
     */
    virtual bool isOpen();
    /**
     * Waits until the monitor is notified by another thread.
     * Zero timeout means infinite timeout.
     */
    virtual void startWait(size_t timeoutMilliseconds = 0);
    /**
     * Releases one thread that is waiting on the monitor.
     */
    virtual void endWait();
  };
}
#endif

