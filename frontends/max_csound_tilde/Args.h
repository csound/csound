/*
    csound~ : A MaxMSP external interface for the Csound API.
    
    Created by Davis Pyon on 2/4/06.
    Copyright 2006-2010 Davis Pyon. All rights reserved.
    
    LICENSE AGREEMENT
    
    This software is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    
    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    
    You should have received a copy of the GNU Lesser General Public
    License along with this software; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "includes.h"
#include "definitions.h"
#include <list>

using namespace std;

namespace dvx {

class Args
{
public:
	Args(t_object *o);
	~Args();

	void Add(const char *str);
	void AddSRandKR(int sr, int ksmps);
	void SetCsoundArguments(short argc, const t_atom *argv, const string & path, const string & defaultPath); 
	string GetArgumentsAsString();
	void RemoveSRandKR();

	char** GetArray(); // Call this AFTER Init(), adding, or removing arguments.
	
	inline bool ArgListValid() const { return m_argListValid; }
	inline bool CsdInPath() const { return m_csdInPath; }
	inline int NumArgs() const { return m_list.size(); }
	inline const string & CsdPath() { return m_csdPath; }
	inline const string & OrcPath() { return m_orcPath; }
	inline const string & ScoPath() { return m_scoPath; }
	inline bool RenderingToFile() const { return m_renderingToFile; }

private:
	void ClearArray();

	// Hide default ctor, copy ctor, and assignment operator.
	Args() {}
	Args(const Args & other) {}
	Args & operator=(const Args & other) { return *this; }

	t_object *m_obj;
	list<string> m_list;
	char *m_array[MAX_ARGS];
	int m_array_size;
	bool m_synced;
	bool m_argListValid;	// If csd/orc/sco file(s) specified in "csound" message can be found, then true.
	bool m_csdInPath;       // If the args[] has a csd file, this will be true. 
	bool m_renderingToFile; // True if -oSomeFilename was in arg list.
	string m_orcPath;       // Path to orchestra file.
	string m_scoPath;       // Path to score file.
	string m_csdPath;       // Path to csd file.
};

} // namespace dvx