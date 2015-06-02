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

#include "Args.h"
#include "util.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace dvx;

Args::Args(t_object *o) : 
	m_obj(o),
	m_list(),
	m_array_size(0),
	m_synced(false),
	m_argListValid(false),
	m_csdInPath(false),
	m_renderingToFile(false),
	m_orcPath(), m_scoPath(), m_csdPath()
{
	m_list.push_back("-g"); // Just in case "start" is received before any "csound" message.
	memset(m_array,0,MAX_ARGS);
}

Args::~Args()
{
	ClearArray();
}

void Args::Add(const char *str)
{
	m_synced = false;
	m_list.push_back(str);
}

void Args::AddSRandKR(int sr, int ksmps)
{
	stringstream stream;
	string s;

	m_synced = false;

	// Adjust kr to keep ksmps constant.
	double new_kr = (double)sr / (double)ksmps;
	stream << "-r" << sr;
	stream >> s;
	m_list.push_back(s);
	stream.clear();
	stream << "-k" << setprecision(10) << new_kr;
	stream >> s;
	m_list.push_back(s);
}

/*  Possible SR and KR formats:
	--sample-rate=88200
	-r88200
	-r 88200
	--control-rate=333
	-k333
	-k 333
*/
void Args::RemoveSRandKR()
{
	list<string>::iterator it = m_list.begin(), it2;

	m_synced = false;

	while(it != m_list.end())
	{
		if(it->find("--sample-rate") != string::npos)
			it = m_list.erase(it);
		else if(it->find("--control-rate") != string::npos)
			it = m_list.erase(it);
		else if( (it->find("-r") != string::npos) || (it->find("-k") != string::npos) )
		{
			if(it->size() == 2)
			{
				it = m_list.erase(it);
				if(it != m_list.end()) 
					it = m_list.erase(it);
			}
			else
				it = m_list.erase(it);
		}
		else
			++it;
	}
}

void Args::ClearArray()
{
	m_synced = false;
	for(int i=0; i<m_array_size; i++) free(m_array[i]);
	m_array_size = 0;
}

void Args::SetCsoundArguments(short argc, const t_atom *argv, const string & path, const string & defaultPath)
{
	int i;
	char *str = NULL, tmp[MAX_STRING_LENGTH], tmp2[MAX_STRING_LENGTH];
	string *strPtr = NULL;
	bool rtmidi_flagPresent = false;
	bool M_flagPresent = false;
	bool d_flagPresent = false;
	bool g_flagPresent = false;
	bool F_flagPresent = false;
	bool T_flagPresent = false;
	bool isCSD = false, isORC = false, isSCO = false;
	FILE *fp = NULL;
	
	if(argc == 0) return;
	
	m_synced = false;

	try
	{
		m_argListValid = true;
		m_renderingToFile = false;
		m_csdInPath = false;
		m_list.clear(); // Clear previous csound command.
		m_list.push_back("csound");

		for(i=0; i<argc; i++)
		{
			if(argv[i].a_type == A_LONG)
			{
				sprintf(tmp2, "%lld", (long long)atom_getlong(&argv[i]));
				m_list.push_back(tmp2);
			}
			else if(argv[i].a_type == A_FLOAT)
			{
				sprintf(tmp2, "%f", atom_getfloat(&argv[i]));
				m_list.push_back(tmp2);
			}
			else if(argv[i].a_type == A_SYM)
			{
				str = argv[i].a_w.w_sym->s_name;
				strncpy(tmp, str, MAX_STRING_LENGTH-1);
			
				if(isQuoted(tmp)) removeQuotes(tmp);
				if(isAbsoluteMaxPath(tmp)) convertMaxPathToPosixPath(tmp, tmp, MAX_STRING_LENGTH);
				if(strstr(tmp, "-+rtmidi")) rtmidi_flagPresent = true;
				if(strstr(tmp, "-M")) M_flagPresent = true;
				if(strstr(tmp, "-d")) d_flagPresent = true;
				if(strstr(tmp, "-g")) g_flagPresent = true;
				if(strstr(tmp, "-F")) F_flagPresent = true;
				if(strstr(tmp, "-T")) T_flagPresent = true;
				if(strstr(tmp, "-o"))
				{
					if(strlen(tmp) == 2)
					{	// There's a space between "-o" and the name of the file.
						if((i+1)<argc)
						{
							strncpy(tmp2, argv[i+1].a_w.w_sym->s_name, MAX_STRING_LENGTH-1);
							if(!strstr(tmp2, "devaudio") && !strstr(tmp2, "dac")) m_renderingToFile = true;
						}
					}
					else if(!strstr(tmp, "devaudio") && !strstr(tmp, "dac")) m_renderingToFile = true;
				}	
			
				// If the current argument is a csd/orc/sco file, try to find it.
				// If not found, use locatefile_extended() to find and extract absolute path.
				isCSD = isORC = isSCO = false;
				if(strstr(tmp, ".csd") || strstr(tmp, ".CSD")) isCSD = true;
				else if(strstr(tmp, ".orc") || strstr(tmp, ".ORC")) isORC = true;
				else if(strstr(tmp, ".sco") || strstr(tmp, ".SCO")) isSCO = true;
				if(!(isCSD || isORC || isSCO))
					m_list.push_back(tmp); // Add current argument to args array.
				else
				{	
					if(path.size()) change_directory(m_obj, path.c_str());
					else if(defaultPath.size()) change_directory(m_obj, defaultPath.c_str());

					fp = fopen(tmp, "r");
					if(fp != NULL)
					{
						m_list.push_back(tmp);
						fclose(fp); 
					}
					else
					{
						short pathID;
						t_fourcc type;
						t_fourcc typelist = 'TEXT';
						int result = locatefile_extended(tmp, &pathID, &type, &typelist, 1);
						if(result == 0)
						{
							path_topathname(pathID, tmp, tmp2); // Get a Max style absolute pathname. 
							#ifdef _WINDOWS
								strcpy(tmp, tmp2);
							#elif MACOSX
							{
								// Use the volume name (e.g. "Macintosh HD:") to form an absolute path.   
								char *colon_loc = NULL;
								sprintf(tmp, "/Volumes/%s", tmp2);
								colon_loc = strchr(tmp, ':');
								if(colon_loc) strcpy(colon_loc, (colon_loc+1));
							}
							#endif
							m_list.push_back(tmp); // Add the path found with locatefile_extended().
						}
						else
						{
							m_argListValid = false;
							m_list.push_back(tmp);
							object_error(m_obj, "Can't find file %s.", tmp);
						}
					}
				
					if(isCSD) { m_csdInPath = true; strPtr = &m_csdPath; }
					else if(isORC) strPtr = &m_orcPath;
					else if(isSCO) strPtr = &m_scoPath;
				
					if(isAbsolutePath(tmp))
						*strPtr = tmp;
					else
					{
						stringstream stream;
						if(path.size())             stream << path << "/" << tmp;	
						else if(defaultPath.size()) stream << defaultPath << "/" << tmp;
						*strPtr = stream.str();
					}
				}
			}
		}
	
		// If neither -d or -g flag is present, then add -g.  Need either -d or -g to prevent crashing.
		if(!d_flagPresent && !g_flagPresent) m_list.push_back("-g"); 
	
		// If we're rendering to a file and -Fsomefilename is present and the -T flag is not present, add it.
		if(m_renderingToFile && F_flagPresent && !T_flagPresent) m_list.push_back("-T"); 
	
		// Need this so that MIDI input is enabled.  We are not going
		// to let Csound accept MIDI data from MaxMSP; below we're going
		// to set the midiReadCallback function pointer to our own function.
		if(!rtmidi_flagPresent) m_list.push_back("-+rtmidi=null"); 
		if(!M_flagPresent) m_list.push_back("-M0");	
	}
	catch(std::exception & ex)
	{
		object_error(m_obj, "%s", ex.what());
		m_argListValid = false;
	}
}

char** Args::GetArray()
{
	ClearArray();

	if(false == m_synced)
	{
		list<string>::iterator it;
		
		for(it = m_list.begin(); it != m_list.end(); it++)
		{
			m_array[m_array_size] = strdup(it->c_str());
			if(NULL == m_array[m_array_size])
			{
				object_error(m_obj, "Args::GetArray() : strdup() failed to allocate memory.");
				break;
			}
			++m_array_size;
		}
		m_synced = true;
	}

	return static_cast<char**>(m_array);
}

string Args::GetArgumentsAsString()
{
	list<string>::iterator it = m_list.begin();
	stringstream stream;

	while(it != m_list.end())
	{
		stream << *it++ << " ";
	}

	return stream.str();
}
