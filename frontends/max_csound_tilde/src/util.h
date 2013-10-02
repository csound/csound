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
#include <string>

#ifndef _UTIL_H
#define _UTIL_H

void change_directory(t_object *o, const char *path);

void to_lower(char *str);
void to_lower(std::string & str);

// Reverses size bytes in src.
void reverseBytes(byte *b, int size);

void reverseNumber(byte *b, int size, bool reverse);

int nextPowOf2(int n);
int isPowOf2(int n);
bool hasSpace(const char *str);
bool isQuoted(const std::string & s);
bool isQuoted(const char *str);
void removeQuotes(std::string & s);
void removeQuotes(char *str);
void addQuotes(const char *src, char *dst, int capacity); // Make sure dst has enough room for 2 more chars.
void openFile(t_object *o, const char *path);
bool isAbsolutePath(const char *path);
bool isAbsoluteMaxPath(std::string & s);
bool isAbsoluteMaxPath(const char *path);

// Convert a Max style absolute path (e.g. Macintosh HD:/Users/George/hello.wav) to POSIX style path.
// If src is not a Max style absolute path, do nothing.
void convertMaxPathToPosixPath(std::string & s);
void convertMaxPathToPosixPath(const char *src, char *dst, int arraySize);

// Returns 0 on failure.
int CreateAtomListFromString(t_object *o, const char *str, t_atom *atomList, int capacity);

void PrintAtoms(t_symbol *s, long argc, t_atom *argv, char *dst, int dstSize);

#endif // _UTIL_H