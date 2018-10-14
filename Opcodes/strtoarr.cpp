/*
    This file is part of Csound.

        Copyright (C) 2018 Rory Walsh

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
 */

#include <plugin.h>
#include <string>
#include <vector>

//opcode version of the StrToArr UDO written by Joachim Heintz
struct StrToArr : csnd::Plugin<2, 2> {
  
  int init() 
  {
    return parseStringAndFillStruct(this);
  }

  int kperf() 
  {
    return parseStringAndFillStruct(this);
  }

  int parseStringAndFillStruct(Plugin* opcodeData)
  {
    csnd::Vector<STRINGDAT> &out = opcodeData->outargs.vector_data<STRINGDAT>(0);
    
    char* inString = opcodeData->inargs.str_data(0).data;
    char* inStringDeLimiter = opcodeData->inargs.str_data(1).data;
    

    std::string input(inString);
    std::string delimiter(inStringDeLimiter);
    std::vector<std::string> tokens;

    size_t pos = 0;

    while ((pos = input.find(delimiter)) != std::string::npos) 
    {
        tokens.push_back(input.substr(0, pos));
        input.erase(0, pos + delimiter.length());
    }

    if(input.size()>0)
      tokens.push_back(input);

    out.init(csound, (int)tokens.size());
    
    for ( int i = 0 ; i < tokens.size() ; i++)
    {
      csound->message(tokens[i].c_str());
      out[i].data = csound->strdup((char*)tokens[i].c_str());
    }

    outargs[1] = (int)tokens.size(); 
    tokens.clear();
    return OK;
  }
};

//removes a number of occurrences of one string from another
struct StrStrip : csnd::Plugin<1, 3> {
  
  int init() 
  {
    return parseStringAndFillStruct(this);
  }

  int kperf() 
  {
    return parseStringAndFillStruct(this);
  }

  int parseStringAndFillStruct(Plugin* opcodeData)
  {
    int occurrences = -1;
    int strsRemoved = 0;
    char* inString = opcodeData->inargs.str_data(0).data;
    char* inStripString = opcodeData->inargs.str_data(1).data;

    if(opcodeData->in_count()>2)
      occurrences = opcodeData->inargs[2];

    std::string input(inString);
    std::string stripString(inStripString);
    std::string::size_type index = input.find(stripString);

    while (index != std::string::npos) 
    {
      input.erase(index, stripString.length());
      index = input.find(stripString, index);
      strsRemoved+=1;
      if(strsRemoved == occurrences)
        break;
    }

    opcodeData->outargs.str_data(0).data = csound->strdup((char*)input.c_str());

    return OK;
  }
};


#include <modload.h>
  
void csnd::on_load(Csound *csound) {
  csnd::plugin<StrToArr>(csound, "strtoarr.kk", "S[]k", "SS", csnd::thread::k);
  csnd::plugin<StrToArr>(csound, "strtoarr.ii", "S[]i", "SS", csnd::thread::i);
  csnd::plugin<StrStrip>(csound, "strstrip.ii", "S", "SSo", csnd::thread::i);
}

