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
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <cstdarg>
#include <vector>
#include <string>
#include <cstdio>
#include "System.hpp"
#include "CsoundFile.hpp"

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
		fprintf(stdout, text);
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

	void ThreadLock::wait(size_t milliseconds)
	{
		if(lock)
		{
			System::waitThreadLock(lock, milliseconds);
		}
	}

	void ThreadLock::notify()
	{
		if(lock)
		{
			System::notifyThreadLock(lock);
		}
	}

	int System::messageLevel = ERROR_LEVEL | WARNING_LEVEL | INFORMATION_LEVEL | DEBUGGING_LEVEL;

	void (*System::messageCallback)(const char *format, va_list valist) = 0;

	void System::message(int level, const char *format,...)
	{
		if((level & messageLevel) == level)
		{
			va_list marker;
			va_start(marker, format);
			message(format, marker);
			va_end(marker);
		}
	}

	void System::error(const char *format,...)
	{
		if((ERROR_LEVEL & messageLevel) == ERROR_LEVEL)
		{
			va_list marker;
			va_start(marker, format);
			message(format, marker);
			va_end(marker);
		}
	}

	void System::warn(const char *format,...)
	{
		if((WARNING_LEVEL & messageLevel) == WARNING_LEVEL)
		{
			va_list marker;
			va_start(marker, format);
			message(format, marker);
			va_end(marker);
		}
	}

	void System::inform(const char *format,...)
	{
		if((INFORMATION_LEVEL & messageLevel) == INFORMATION_LEVEL)
		{
			va_list marker;
			va_start(marker, format);
			message(format, marker);
			va_end(marker);
		}
	}

	void System::debug(const char *format,...)
	{
		if((DEBUGGING_LEVEL & messageLevel) == DEBUGGING_LEVEL)
		{
			va_list marker;
			va_start(marker, format);
			message(format, marker);
			va_end(marker);
		}
	}

	void System::message(const char *format,...)
	{
		va_list marker;
		va_start(marker, format);
		message(format, marker);
		va_end(marker);
	}

	void System::message(const char *format, va_list valist)
	{
		if(messageCallback)
		{
			messageCallback(format, valist);
		}
		else
		{
			vfprintf(stdout, format, valist);
		}
	}

	void System::message(void *userdata, const char *format, va_list valist)
	{
		if(messageCallback)
		{
			messageCallback(format, valist);
		}
		else
		{
			vfprintf(stdout, format, valist);
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

	void System::setMessageCallback(void (*messageCallback_)(const char *format, va_list valist))
	{
		messageCallback = messageCallback_;
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


#if defined(WIN32) && !defined(__CYGWIN__)

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

	void *System::openLibrary(std::string filename)
	{
		return (void *) LoadLibrary(filename.c_str());
	}

	void *System::getSymbol(void *library, std::string name)
	{
		return (void *) GetProcAddress((HMODULE) library, name.c_str());
	}

	void System::closeLibrary(void *library)
	{
		FreeLibrary((HMODULE) library);
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
		Sleep((int) milliseconds);
	}

	void System::beep()
	{
		Beep(880, 1000);
	}

#elif defined(LINUX) || defined(__CYGWIN__)

#include <dlfcn.h>
#include <dirent.h>
#if !defined(__CYGWIN__)
#include <libgen.h>
#else

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

	char *basename(const char *path)
	{
		static char bname[MAXPATHLEN];
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

		if (endp - startp + 2 > sizeof(bname)) {
			//errno = ENAMETOOLONG;
			return(NULL);
		}
		strlcpy(bname, startp, endp - startp + 2);
		return(bname);
	}

	char *dirname(const char *path)
	{
		static char bname[MAXPATHLEN];
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

		if (endp - path + 2 > sizeof(bname)) {
			//errno = ENAMETOOLONG;
			return(NULL);
		}
		strlcpy(bname, path, endp - path + 2);
		return(bname);
	}

#endif

	int System::shellOpen(const char *filename, const char *command)
	{
		int argc;
		char **argv;
		int returnValue = fork();
		if(!returnValue)
		{
			std::string buffer = filename;
			buffer += " ";
			buffer += command;
			scatterArgs(buffer, &argc, &argv);
			execv(filename, argv);
			deleteArgs(argc, argv);
		}
		return returnValue;
	}

	int System::execute(const char *command)
	{
		return ::system(command);
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
		char *dirTemp = strdup(pathname.c_str());
		directory = dirname(dirTemp);
		char *fileTemp = strdup(pathname.c_str());
		file = basename(fileTemp);
		int periodPosition = pathname.find_last_of(".");   
		if(periodPosition != -1)
		{
			extension = pathname.substr(periodPosition + 1);
		}
		free(dirTemp);
		free(fileTemp);
	}

	void *System::openLibrary(std::string filename)
	{
		void *library = 0;
		library = dlopen(filename.c_str(), RTLD_NOW | RTLD_GLOBAL);
		if(!library)
		{
			System::error("Error in dlopen(): '%s'\n", dlerror());
		}
		return library;
	}

	void *System::getSymbol(void *library, std::string name)
	{
		void *procedureAddress = 0;
		procedureAddress = dlsym(library, name.c_str());
		return procedureAddress;
	}

	void System::closeLibrary(void *library)
	{
		void *returnValue = 0;
		returnValue = (void *)dlclose(library);
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
			return 0;
		}
	}

	void *System::createThreadLock()
	{
		pthread_mutex_t *pthread_mutex = new pthread_mutex_t;
		if(pthread_mutex_init(pthread_mutex, 0) == 0)
		{
			return pthread_mutex;
		}
		else
		{
			return 0;
		}
	}

	void System::waitThreadLock(void *lock, size_t milliseconds)
	{
		pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
		int returnValue = pthread_mutex_lock(pthread_mutex);
	}

	void System::notifyThreadLock(void *lock)
	{
		pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
		int returnValue = pthread_mutex_unlock(pthread_mutex);
	}

	void System::destroyThreadLock(void *lock)
	{
		pthread_mutex_t *pthread_mutex = (pthread_mutex_t *)lock;
		int returnValue = pthread_mutex_destroy(pthread_mutex);
		delete pthread_mutex;
		pthread_mutex = 0;
	}

	std::string System::getSharedLibraryExtension()
	{
		return "so";
	}

	void System::sleep(double milliseconds)
	{
		sleep((int) milliseconds);
	}

	void System::beep()
	{
		beep();
	}

#endif
}


