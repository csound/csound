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
 
#include <exception>
#include <cstring>

using namespace std;

namespace dvx {

class eksepshun : public std::exception
{
public:
	eksepshun(const char * c_str) throw()
	{
		strncpy(m_str, c_str, 127);
	}
	
	const char* what() const throw()
	{
		return m_str;
	}
	
private:	
	char m_str[128];
};

} // namespace dvx