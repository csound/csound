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

#include "util.h"

#if defined(__MACH__)
#include <LaunchServices.h>
#endif

using namespace std;

void change_directory(t_object *o, const char *path)
{
	#ifdef _WINDOWS
		if(-1 == _chdir(path))
		{
			object_post(o, "WARNING: Could not chdir to current directory: %s.", path);
	#elif MACOSX
		if(-1 == chdir(path))
		{
			object_post(o, "WARNING: Could not chdir to current directory: %s.", path);
			switch(errno)
			{
			case ENOTDIR:
				object_post(o, "ENOTDIR: A component of the path prefix is not a directory.");
				break;
			case ENAMETOOLONG:
				object_post(o, "ENAMETOOLONG: A component of a pathname exceeded {NAME_MAX} characters, or an entire path name exceeded {PATH_MAX} characters.");
				break;
			case ENOENT:
				object_post(o, "ENOENT: The named directory does not exist.");
				break;
			case ELOOP:
				object_post(o, "ELOOP: Too many symbolic links were encountered in translating the pathname.");
				break;
			case EACCES:
				object_post(o, "EACCES: Search permission is denied for any component of the path name.");
				break;
			case EFAULT:
				object_post(o, "EFAULT: Path points outside the process's allocated address space.");
				break;
			case EIO:
				object_post(o, "EIO: An I/O error occurred while reading from or writing to the file system.");
				break;						
			}

	#endif
		}
}

void to_lower(char *str)
{
	int i=0, len=0;
	len = strlen(str);
	for(i = 0; i < len; i++)
		str[i] = tolower(str[i]);
}

void to_lower(std::string & str)
{
	for(unsigned int i=0; i < str.size(); i++)
		str[i] = tolower(str[i]);
}

// Reverses size bytes in b.
void reverseBytes(byte *b, int size)
{
	int i, j, limit = size >> 1;
	
	--size;
	
	for(i=0; i < limit; ++i)
	{
		j = size - i;
		b[i] ^= b[j];
		b[j] ^= b[i];
		b[i] ^= b[j];
	}
}

void reverseNumber(byte *b, int size, bool reverse)
{	
	if(reverse && size > 1)	reverseBytes(b, size);
}

int nextPowOf2(int n)
{
	int next = 1;
	while(next < n)	next <<= 1;
	return next;
}

int isPowOf2(int n)
{
	return n > 0 && (n & (n-1)) == 0;
}

bool hasSpace(const char *str)
{
	return NULL != strchr(str,' ');
}

bool isQuoted(const std::string & s)
{
	return (s[0] == '"') && (s[s.size()-1] == '"');
}

bool isQuoted(const char *str)
{
	int len = strlen(str);
	if(str[0] == '"' && str[len-1] == '"') 
		return true;
	return false;
}

void removeQuotes(std::string & s)
{
	size_t pos;
	while((pos = s.find_first_of('"')) != std::string::npos)
	{
		s.erase(pos);
	}
}

void removeQuotes(char *str)
{
	int len = strlen(str);
	if(len < 2) return;
	
	for(int i=0; i<(len-1); i++)
		str[i] = str[i+1];
	
	str[len-2] = '\0';
}

void addQuotes(const char *src, char *dst, int capacity)
{
	snprintf(dst, capacity-1, "\"%s\"", src);	
}

void openFile(t_object *o, const char *path)
{
#ifdef MACOSX
	char  tmpPath[MAX_STRING_LENGTH];
	FSRef fsRef;
	
	snprintf(tmpPath, MAX_STRING_LENGTH-1, "file://%s", path);

	if(FSPathMakeRef((UInt8*)path, &fsRef, false) != noErr)
	{
		object_error(o, "Could not convert %s to FSRef.", path);
		return;
	}
	
	if(LSOpenFSRef(&fsRef, NULL) != noErr)
	{
		object_error(o, "Could not open file: %s", path);
		return;
	}
#elif _WINDOWS
	int result;
	result = (int) ShellExecute(NULL, NULL, path, NULL, NULL, SW_SHOWNORMAL);
	if(result <= 32)
		object_error(o, "Could not open file: %s", path);
#endif
}

bool isAbsolutePath(const char *path)
{
#ifdef MACOSX
	return path[0] == '/'; 
#elif _WINDOWS
	return path[1] == ':' && (path[2] == '\\' || path[2] == '/');
#endif
}

bool isAbsoluteMaxPath(std::string & s)
{
	return std::string::npos != s.find(":/");
}

bool isAbsoluteMaxPath(const char *path)
{
	return (strstr(path, ":/") != NULL);
}

void convertMaxPathToPosixPath(std::string & s)
{
#ifdef _WINDOWS
	return;
#else
	string tmp = s;
	size_t colon_pos = tmp.find_first_of(':');
	tmp.erase(colon_pos,1);
	s = "/Volumes/";
	s += tmp;
#endif
}

void convertMaxPathToPosixPath(const char *src, char *dst, int arraySize)
{
#ifdef _WINDOWS
	strncpy(dst, src, arraySize - 1);
	return;
#else
	char *colonPtr = NULL;
	char src_cpy[arraySize], volume[arraySize], path[arraySize];
	
	strncpy(src_cpy, src, arraySize - 1);
	colonPtr = strchr(src_cpy, ':');
	
	if(colonPtr)
	{
		// It's a max style path.
		*colonPtr = '\0';
		strncpy(volume, src_cpy, arraySize - 1);
		*colonPtr = ':';
		strncpy(path, colonPtr + 1, arraySize - 1);
				
		// Form and output a Unix style path.
		snprintf(dst, arraySize - 1, "/Volumes/%s%s", volume, path);
	} 
#endif
}

int CreateAtomListFromString(t_object *o, const char *str, t_atom *atomList, int capacity)
{
	int i=0;
	char *p, *s;
	char whiteSpace[] = " ";

	if(str == NULL || strlen(str) == 0) return 0;

	// strtok() messes the string that it works with, so
	// work on a duplicate of str.

	s = strdup(str); 
	if(s == NULL) return 0;
	p = strtok(s, whiteSpace);
	atom_setsym(&atomList[i], gensym(p));
	++i;
	while((p = strtok(NULL, whiteSpace)) != NULL)
	{
		atom_setsym(&atomList[i], gensym(p));
		++i;
		if(i == capacity)
		{
			object_error(o, "CreateAtomListFromString(): Too many atoms in string.");
			break;
		}
	}
	free(s);
	return i;
}

void PrintAtoms(t_symbol *s, long argc, t_atom *argv, char *dst, int dstSize)
{
    int i, maxLen, wrote;
	char *b = NULL;
	t_atom *ap;

	dst[dstSize-1] = '\0';

	b = dst;
	maxLen = dstSize-1;

	wrote = snprintf(b, maxLen, "%s ", s->s_name);

	if(wrote < 0) return;
	b += wrote;
	maxLen -= wrote;
	if(maxLen == 0) return;

    for (i = 0, ap = argv; i < argc; i++, ap++) {       // increment ap each time to get to the next atom
        switch (atom_gettype(ap)) {
            case A_LONG:
				wrote = snprintf(b, maxLen, "%lld ", atom_getlong(ap));
                break;
            case A_FLOAT:
				wrote = snprintf(b, maxLen, "%f ", atom_getfloat(ap));
                break;
            case A_SYM:
				wrote = snprintf(b, maxLen, "%s ", atom_getsym(ap)->s_name);
                break;
            default:
                wrote = snprintf(b, maxLen, "UNKNOWN_ATOM_TYPE ");
                break;
        }
		if(wrote < 0) return;
		b += wrote;
		maxLen -= wrote;
		if(maxLen == 0) return;
    }
}
