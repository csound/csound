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
#include "Shell.hpp"
#include "System.hpp"
#include <iostream>
#include <fstream>
#include <time.h>
#include <Python.h>

namespace csound
{

	Shell::Shell()
	{
	}

	Shell::~Shell()
	{
	}

	void Shell::open()
	{
		Py_Initialize();
	}

	void Shell::close()
	{
		//if(Py_IsInitialized())
		//{
		//	Py_Finalize();
		//}
	}

	void Shell::main(int argc, char **argv)
	{
		PySys_SetArgv(argc, argv);
	}

	void Shell::initialize()
	{
		clear();
		setFilename(generateFilename());
	}

	void Shell::clear()
	{
		filename.erase();
		script.erase();
	}

	std::string Shell::generateFilename()
	{
		time_t time_ = 0;
		time(&time_);
		struct tm* tm_ = gmtime(&time_);
		char buffer[0x100];
		strftime(buffer, 0x100, "csound[%Y-%m-%d][%H.%M.%S].py", tm_);
		return buffer;
	}

	void Shell::setFilename(std::string filename)
	{
		this->filename = filename;
	}

	std::string Shell::getFilename() const
	{
		return filename;
	}

	std::string Shell::getOutputSoundfileName() const
	{
		std::string outputFilename = getFilename();
		outputFilename.append(".wav");
		return filename;
	}

	std::string Shell::getMidiFilename() const
	{
		std::string midiFilename = getFilename();
		midiFilename.append(".mid");
		return filename;
	}

	std::string Shell::getScript() const
	{
		return script;
	}

	void Shell::setScript(std::string script)
	{
		this->script = script;
	}

	void Shell::load(std::string filename)
	{
		clear();
		loadAppend(filename);
	}

	void Shell::loadAppend(std::string filename)
	{
		std::ifstream stream;
		stream.open(filename.c_str(), std::ios_base::binary);
		char c;
		while(!stream.eof())
		{
			stream.get(c);
			script.push_back(c);
		}
	}

	void Shell::save(std::string filename) const
	{
		std::ofstream stream;
		if(filename.length() > 0)
		{
			stream.open(filename.c_str(), std::ios_base::binary);
			for(std::string::const_iterator it = script.begin(); it != script.end(); ++it)
			{
				stream.put(*it);
			}
		}
	}

	void Shell::save() const
	{
		save(getFilename());
	}

	int Shell::run()
	{
		return run(script);
	}

	int Shell::run(std::string script_)
	{
		csound::System::message("BEGAN Shell::run()...\n");
		int result = 0;
		try
		{
			char *script__ = const_cast<char *>(script_.c_str());
			csound::System::message("==============================================================================================================\n");
			result = PyRun_SimpleString(script__);
			if(result)
			{
				PyErr_Print();
			}
		}
		catch(...)
		{
			csound::System::error("Unidentified exception in silence::Shell::run().\n");
		}
		csound::System::message("==============================================================================================================\n");
		csound::System::message("PyRun_SimpleString returned %d.\n", result);
		csound::System::message("ENDED Shell::run().\n");
		return result;
	}

	void Shell::stop()
	{
		//PyErr_SetString(PyExc_KeyboardInterrupt, "Shell::stop() was called.");
	}
}
