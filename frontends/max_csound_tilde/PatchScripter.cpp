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

#include "PatchScripter.h"
#include "jpatcher_api.h"

using namespace dvx::Patcher;
using namespace std;

Element::Element(t_object *o) : m_obj(o), m_name(), m_script(), m_new(false)
{
	t_symbol *sym = NULL;
	assert(o);
	sym = object_attr_getsym(m_obj, gensym("varname"));
	if(sym) m_name = sym->s_name;
	object_attr_get_rect(m_obj, gensym("patching_rect"), (t_rect*)&m_rect);
	m_rect.r = m_rect.x + m_rect.width;
	m_rect.b = m_rect.y + m_rect.height;
}

Element::Element(t_object *patcher, string script) : m_obj(NULL), m_name(), m_script(script), m_new(true)
{
	t_symbol *sym = NULL;
	m_obj = newobject_sprintf(patcher, (char*)m_script.c_str());
	if(NULL == m_obj) throw eksepshun("Element::Element() : newobject_sprintf() failed to allocate.");
	sym = object_attr_getsym(m_obj, gensym("varname"));
	if(sym) m_name = sym->s_name;
	object_attr_get_rect(m_obj, gensym("patching_rect"), (t_rect*)&m_rect);
	m_rect.r = m_rect.x + m_rect.width;
	m_rect.b = m_rect.y + m_rect.height;
}

bool Element::collision(const Element & e)
{
	return !(e.m_rect.r < m_rect.x || e.m_rect.x > m_rect.r) && 
	       !(e.m_rect.b < m_rect.y || e.m_rect.y > m_rect.b);
}

void Element::move(double x, double y)
{ 
	m_rect.x+=x; m_rect.y+=y; m_rect.r+=x; m_rect.b+=y;
	object_attr_set_rect(m_obj, gensym("patching_rect"), (t_rect*)&m_rect);
}

void Element::move_abs(double x, double y)
{ 
	m_rect.x=x; m_rect.y=y; m_rect.r = m_rect.x + m_rect.width; m_rect.b = m_rect.y + m_rect.height;
	object_attr_set_rect(m_obj, gensym("patching_rect"), (t_rect*)&m_rect);
}

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

Scripter::Scripter(t_object *o, dvx::ChannelGroup & cg) :
	m_obj(o), m_patcher(NULL), m_subpatcher(NULL), m_cg(cg)
{
	if(MAX_ERR_NONE != object_obex_lookup(m_obj, gensym("#P"), &m_patcher))
		object_error(m_obj, "Scripter::Scripter() : object_obex_lookup() failed to find patcher.");
}

void Scripter::AddElement(t_object *patcher, t_object *o)
{
	m_elements.push_back(new Element(o));
}

Element* Scripter::AddElement(t_object *patcher, const std::string & s, bool addCompanions)
{
	Element *e = new Element(patcher, s);
	t_rect parent_patch_rect;
	object_attr_get_rect(patcher, gensym("rect"), &parent_patch_rect);

	// If we're creating elements in csound~'s patcher, and no patching_rect was specified
	// in the script, check for collisions.
	if(s.find("@patching_rect") == string::npos)
	{
		// Don't add x-axis offset to first element being created.
		if(m_prev_rect.width != 0.0)
			e->move(m_prev_rect.r + 5.0, 0.0); 
	}

	// Insert newly created element into m_elements.
	m_elements.push_back(e);

	// Save current element's patching_rect to make future collision detection less costly.
	memcpy(&m_prev_rect, &e->m_rect, sizeof(t_rectex));

	if(addCompanions)
	{
		Element *prepend_elem = NULL, *pattrforward_elem = NULL;
		t_symbol *sym = NULL, *csound_sym = NULL;
		char script[MAX_STRING_LENGTH];
	
		sym = object_attr_getsym(e->m_obj, gensym("varname"));
		if(sym)
		{
			csound_sym = object_attr_getsym(m_obj, gensym("varname"));
			if(!csound_sym) csound_sym = gensym("csound~");

			snprintf(script, MAX_STRING_LENGTH-1, "@maxclass newobj @text \"prepend c %s\" @patching_rect %f %f %f %f",
				sym->s_name, e->m_rect.x, e->m_rect.b + 5.0, 100.0, 20.0);

			prepend_elem = AddElement(patcher, script, false);

			snprintf(script, MAX_STRING_LENGTH-1, "@maxclass newobj @text \"pattrforward %s\" @patching_rect %f %f %f %f",
				csound_sym->s_name, e->m_rect.x, e->m_rect.b + 30.0, 100.0, 20.0);

			pattrforward_elem = AddElement(patcher, script, false);

			connect(patcher, e->m_obj, 0, prepend_elem->m_obj, 0);
			connect(patcher, prepend_elem->m_obj, 0, pattrforward_elem->m_obj, 0);
		}
	}

	return e;
}

bool Scripter::collides(const Element & e)
{
	boost::ptr_list<Element>::iterator it;
	for(it = m_elements.begin(); it != m_elements.end(); it++)
	{
		if(it->collision(e))
			return true;
	}
	return false;
}

void Scripter::connect(t_object *patcher, t_object *src, int src_outlet, t_object *dst, int dst_inlet)
{
	 t_atom rv;

    atom_setobj(m_connect_msg, src);             // source
    atom_setlong(m_connect_msg + 1, src_outlet); // outlet number (0 is leftmost)
    atom_setobj(m_connect_msg + 2, dst);         // destination
    atom_setlong(m_connect_msg + 3, dst_inlet);  // inlet number (0 is leftmost)

    object_method_typed(patcher, gensym("connect"), 4, m_connect_msg, &rv);
}

void Scripter::GetExistingElements()
{
	t_object *patcher, *box, *obj;
	t_max_err result;

    result = object_obex_lookup(m_obj, gensym("#P"), &patcher);
	if(result !=  MAX_ERR_NONE)
	{
		object_error(m_obj, "Scripter::GetExistingElements() : object_obex_lookup() failed to find patcher.");
		return;
	}

    for(box = jpatcher_get_firstobject(patcher); box; box = jbox_get_nextobject(box))
	{
        obj = jbox_get_object(box);
        if(obj)
		{
			if(obj == box)
				AddElement(patcher, obj); // obj is a UI object. Create an element and add it to list.
			else
				AddElement(patcher, box); // obj is a non-UI object. Create an element for box and add it to list.
		}
    }
}

void Scripter::ParseCSD(const char * filename, bool tosub)
{
	ifstream infile;

	try
	{	
		int line = 0;
		bool storing = false;
		size_t pos_start, pos_end;
		string buf, elem;

		if(!tosub)
			m_subpatcher = m_patcher;
		else
		{
			// Create a subpatch.
			t_object *newobj = newobject_sprintf(m_patcher, "@maxclass newobj @text \"p subpatch\"");
			if(NULL == newobj) throw eksepshun("Scripter::ParseCSD() : newobject_sprintf() failed to allocate subpatch.");
			m_subpatcher = jbox_get_object(newobj);
		}

		m_elements.clear();
		memset(&m_prev_rect, 0, sizeof(t_rectex));
		GetExistingElements();
		infile.open(filename, ifstream::in);

		while(infile.good())
		{
			++line;
			getline(infile, buf);
			pos_start = pos_end = string::npos;
			pos_start = buf.find("<~>");
			pos_end = buf.find("</~>");

			if( (pos_start != string::npos && storing) ||
			    (pos_start == string::npos && pos_end != string::npos && !storing) )
				object_error(m_obj, "Scripter::ParseCSD() : mismatched pairs of <~>, </~> detected (line %d).", line);
			
			if(storing)
			{
				if(pos_end == string::npos) // No <~> or </~> in this line. Add entire line to elem.
					elem += buf;
				else                        // </~> occurs in this line. 
				{	
					storing = false;
					elem += buf.substr(0, pos_end);
					elem.erase(0, elem.find_first_not_of("<~> "));
					AddElement(m_subpatcher, elem);
					elem.clear();
				}
			}
			else if(!storing && pos_start != string::npos) // <~> occurs in this line.
			{
				elem += buf.substr(pos_start, pos_end);
				if(pos_end == string::npos)	storing = true;
				else                                       // </~> also occurs in this line.
				{
					elem.erase(0, elem.find_first_not_of("<~> "));
					AddElement(m_subpatcher, elem);
					elem.clear();
				}
			}
		}
		if(storing)
			object_error(m_obj, "Scripter::ParseCSD() : mismatched pairs of <~>, </~> detected (line %d).", line);
		infile.close();
	}
	catch(std::exception & ex)
	{
		object_error(m_obj, "PatchScripter::ParseCSD() exception: %s", ex.what());
		infile.close();
	}
}