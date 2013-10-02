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
#include <iostream>
#include <fstream>
#include <string>
#include <boost/ptr_container/ptr_list.hpp>

#ifndef _PATCH_SCRIPTER_H
#define _PATCH_SCRIPTER_H

using namespace std;

namespace dvx { 

class ChannelGroup;

namespace Patcher {

// Extended t_rect;
typedef struct _rectex
{
	double x;
	double y;
	double width;
	double height;
	double r;
	double b;
} t_rectex;

// A class that's used to keep class of patcher objects.
class Element
{
public:
	Element(t_object *o); // Use this to catalog existing objects.
	Element(t_object *patcher, string script); // Creates a new object in patcher using script.
	~Element() {}

	bool collision(const Element & e); 
	void move(double x, double y);
	void move_abs(double x, double y);

	t_object *m_obj; // Pointer to an existing or newly created object box.
	string m_name;   // The scripting name of m_obj.
	std::string m_script; // The script that was used to create the object box.
	t_rectex m_rect; // The bounding rect of object box.
	bool m_new;      // True if this instance was created using ctor with script.
};

class Scripter
{
	enum increment { X = 50, X_LIMIT = 1024, Y = 50, Y_LIMIT = 768 };

public:
	Scripter(t_object *o, ChannelGroup & cg);
	~Scripter() {}

	/* Add existing object box to m_elements. */
	void AddElement(t_object *patcher, t_object *o);

	/* Create new object box in patcher using script s.
	   If addCompanions is true, add prepend and pattrforward objects for the
	   newly created object box.  By default, prepend prepends the scripting name
	   of the new object and pattrforward forwards to an object named "csound~". */
	Element* AddElement(t_object *patcher, const std::string & s, bool addCompanions = true);
	
	/* Get existing object boxes in m_patcher. */
	void GetExistingElements();

	/* Reads a csd file and creates objects. */
	void ParseCSD(const char * filename, bool tosub);

private:
	/* See if e collides with anything in m_elements. */
	bool collides(const Element & e);

	/* Connect 2 objects. */
	void connect(t_object *patcher, t_object *src, int src_outlet, t_object *dst, int dst_inlet);

	t_object *m_obj;        // Pointer to parent t_csound (cast as t_object*).
	t_object *m_patcher;    // The patcher containing parent t_csound.
	t_object *m_subpatcher; // The patcher where new objects will be created.
	ChannelGroup & m_cg;
	t_rectex m_prev_rect;
	boost::ptr_list<Element> m_elements;
	t_atom m_connect_msg[4];
};

} // namespace Patcher
} // namespace dvx

#endif