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
#include "MusicModel.hpp"
#include "Exception.hpp"
#include "Composition.hpp"
#include "System.hpp"

namespace csound
{
	MusicModel::MusicModel()
	{
	}

	MusicModel::~MusicModel()
	{
		//clear();
	}

	void MusicModel::initialize()
	{
	}

	void MusicModel::generate()
	{                         
		Composition::clear();
		traverse(getLocalCoordinates(), score);
		System::message("Generated %d events.\n", score.size());
	}

	void MusicModel::clear()
	{
		Node::clear();
		Composition::clear();
	}

	std::string MusicModel::getFilename() const
	{
		return filename;
	}

	void MusicModel::setFilename(std::string filename)
	{
		this->filename = filename;
	}

	std::string MusicModel::generateFilename()
	{
		time_t time_ = 0;
		time(&time_);
		struct tm* tm_ = gmtime(&time_);
		char buffer[0x100];
		strftime(buffer, 0x100, "silence[%Y-%m-%d][%H.%M.%S].py", tm_);
		return buffer;
	}

	std::string MusicModel::getMidiFilename()
	{
		std::string name = getFilename();
		name.append(".mid");
		return name;
	}

	std::string MusicModel::getOutputSoundfileName()
	{
		std::string name = getFilename();
		name.append(".wav");
		return name;
	}

	long MusicModel::getThis()
	{
		return (long) this;
	}
}

