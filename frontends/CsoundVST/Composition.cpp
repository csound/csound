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
#if defined(_MSC_VER) && !defined(__GNUC__)
#pragma warning (disable:4786) 
#endif
#include "Composition.hpp"
#include "System.hpp"

namespace csound 
{
	Composition::Composition() : 
        tonesPerOctave(12.0),
        conformPitches(false),
		cppSound(&cppSound_)
	{
	}

	Composition::~Composition()
	{
	}

	void Composition::render()
	{
		generate();
		perform();
	}

	void Composition::perform()
	{
		cppSound->removeScore();
		score.sort();
		for(Score::iterator it = score.begin(); it != score.end(); ++it)
		{
			if(getConformPitches())
			{
				csound::Event e = *it;
				e.conformToPitchClassSet();
				cppSound->addScoreLine(e.toCsoundIStatement(tonesPerOctave));
			}
			else
			{
				cppSound->addScoreLine(it->toCsoundIStatement(tonesPerOctave));
			}
		}
		cppSound->exportForPerformance();
		cppSound->perform();
	}

	void Composition::generate()
	{
	}

	void Composition::clear()
	{
		score.clear();
		cppSound->removeScore();
	}

	Score &Composition::getScore()
	{
		return score;
	}

	void Composition::setCppSound(CppSound *cppSound)
	{
		if(!cppSound)
		{
			this->cppSound = &cppSound_;
		}
		else
		{
			this->cppSound = cppSound;
		}
	}

	CppSound *Composition::getCppSound()
	{
		return cppSound;
	}

	void Composition::write(const char *text)
	{
		System::message(text);
	}

	void Composition::setTonesPerOctave(double tonesPerOctave)
	{
		this->tonesPerOctave = tonesPerOctave;
	}

	double Composition::getTonesPerOctave() const
	{
		return tonesPerOctave;
	}

	void Composition::setConformPitches(bool conformPitches)
	{
		this->conformPitches = conformPitches;
	}
	
	bool Composition::getConformPitches() const
	{
		return conformPitches;
	}
}	
