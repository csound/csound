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

#include "includes.h" // Must include this file before spirit and config headers.

#ifdef _USE_BOOST_SERIALIZATION
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <iostream>
#include <string>
#include <vector>

#ifndef _PARSER_H
#define _PARSER_H

namespace Parser
{
	using boost::spirit::ascii::space;
	using boost::spirit::qi::phrase_parse;
	using boost::spirit::karma::generate_delimited;

	// Parse vector of integers.
	template <typename Iterator>
	bool parse_integers(Iterator first, Iterator last, std::vector<int>& v)
	{
		using boost::spirit::qi::int_;
		bool r = phrase_parse(first, last, *int_ , space, v);
		if (first != last) return false;
		return r;
	}

	// Parse vector of doubles.
	template <typename Iterator>
	bool parse_doubles(Iterator first, Iterator last, std::vector<double>& v)
	{
		using boost::spirit::qi::double_;
		bool r = phrase_parse(first, last, *double_ , space, v);
		if (first != last) return false;
		return r;
	}

	//  Generate list of integers (from container with forward iterator).
	template <typename OutputIterator, typename Container>
	bool generate_integers(OutputIterator& sink, Container const& v)
	{
		using boost::spirit::karma::int_;
		bool r = generate_delimited(sink, *int_ , space, v);
		return r;
	}

	//  Generate list of doubles (from container with forward iterator).
	template <typename OutputIterator, typename Container>
	bool generate_doubles(OutputIterator& sink, Container const& v)
	{
		using boost::spirit::karma::double_;
		bool r = generate_delimited(sink, *double_ , space, v);
		return r;
	}
} // namespace Parser

#endif // _PARSER_H

#endif // _USE_BOOST_SERIALIZATION

