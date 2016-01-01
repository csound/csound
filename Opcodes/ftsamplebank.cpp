/*
    This file is part of Csound.

	Copyright (C) 2014 Rory Walsh

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
 */
 
#include "OpcodeBase.hpp"
#include <cmath>
#include <sys/types.h> 
#include <dirent.h> 
#include <iostream>  
#include <algorithm>   
#include <vector>     
#include <string>
#include <sstream>

using namespace std;

/* this function will load all samples of supported types into function tables number 'index' and upwards.
   It return the number of samples loaded */
int loadSamplesToTables(CSOUND *csound, int index, char* directory, int skiptime, int format, int channel);

//-----------------------------------------------------------------
//	i-rate class
//-----------------------------------------------------------------
class iftsamplebank : public OpcodeBase<iftsamplebank>
{
public:
	// Outputs.
	MYFLT* numberOfFiles;
	// Inputs.
	STRINGDAT* sDirectory;
	MYFLT* index;
	MYFLT* trigger;
	MYFLT* skiptime;
	MYFLT* format;
	MYFLT* channel;

	iftsamplebank(){}

	//init-pass
	int init(CSOUND *csound)
	{
		*numberOfFiles = loadSamplesToTables(csound, *index,  (char* )sDirectory->data, *skiptime, *format, *channel);
		return OK;
	}

	int noteoff(CSOUND *)
	{
	return OK;
	}  
};

//-----------------------------------------------------------------
//	k-rate class
//-----------------------------------------------------------------
class kftsamplebank : public OpcodeBase<kftsamplebank>
{
public:
	// Outputs.
	MYFLT* numberOfFiles;
	// Inputs.
	STRINGDAT* sDirectory;
	MYFLT* index;
	MYFLT* trigger;
	MYFLT* skiptime;
	MYFLT* format;
	MYFLT* channel;	
	int internalCounter;
	kftsamplebank():internalCounter(0){}

	//init-pass
	int init(CSOUND *csound)
	{
		*numberOfFiles = 0;//loadSamplesToTables(csound, *index, fileNames, (char* )sDirectory->data, *skiptime, *format, *channel);
		//csound->Message(csound, (char* )sDirectory->data);
		*trigger=0;
		return OK;
	}

	int noteoff(CSOUND *)
	{
		return OK;
	}

	int kontrol(CSOUND* csound)
	{
		//if directry changes update tables..
		if(*trigger==1)
		{
			*numberOfFiles = loadSamplesToTables(csound, *index, (char* )sDirectory->data, *skiptime, *format, *channel);
			*trigger = 0;
		}
		return OK;
	}
  
};

//-----------------------------------------------------------------
//	load samples into functoin tables
//-----------------------------------------------------------------
int loadSamplesToTables(CSOUND *csound, int index, char* directory, int skiptime, int format, int channel)
{
	if(directory)
	{
		DIR *dir = opendir(directory);			
		std::vector<std::string> fileNames;
		std::vector<std::string> fileExtensions;
		int noOfFiles = 0;
		fileExtensions.push_back(".wav");
		fileExtensions.push_back(".aiff");	
		fileExtensions.push_back(".ogg");
		fileExtensions.push_back(".flac");
		
		//check for valid path first
		if(dir) 
		{ 
			struct dirent *ent; 
			while((ent = readdir(dir)) != NULL) 
			{ 			
				std::ostringstream fullFileName;
				//only use supported file types
				for(int i=0;i<fileExtensions.size();i++)
					if(std::string(ent->d_name).find(fileExtensions[i])!=std::string::npos)
					{						
					#if defined(WIN32)
						fullFileName << directory << "\\" << ent->d_name;
					#else
						fullFileName << directory << "/" << ent->d_name;
					#endif
					
					noOfFiles++;
					fileNames.push_back(fullFileName.str());		
					}
			}
			
			// Sort names
			std::sort(fileNames.begin(), fileNames.end() );

			// push statements to score, starting with table number 'index'
			for(int y = 0; y < fileNames.size(); y++)
			{
				std::ostringstream statement; 
				statement << "f" << index+y << " 0 0 1 \"" << fileNames[y] <<  "\" " << skiptime << " " << format << " " << channel << "\n";				
				//csound->MessageS(csound, CSOUNDMSG_ORCH, statement.str().c_str()); 
				csound->InputMessage(csound, statement.str().c_str());
			}
		}	
		else 
			{ 
			csound->Message(csound, "Cannot load file. Error opening directory: %s\n",  directory); 
			} 
			
		//return number of files
		return noOfFiles;
	}
	else
	return 0;
} 	


extern "C" {

  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
      return 0;
  }

  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
	  
      int status = csound->AppendOpcode(csound,
                                        (char*)"ftsamplebank.k",
                                        sizeof(kftsamplebank),
                                        0,
                                        3,
                                        (char*)"k",
                                        (char*)"Skkkkk",
                                        (int(*)(CSOUND*,void*)) kftsamplebank::init_,
                                        (int(*)(CSOUND*,void*)) kftsamplebank::kontrol_,
                                        (int (*)(CSOUND*,void*)) 0);

      status |= csound->AppendOpcode(csound,
                                     (char*)"ftsamplebank.i",
                                     sizeof(iftsamplebank),
                                        0,
                                     1,
                                     (char*)"i",
                                     (char*)"Siiiii",
                                     (int (*)(CSOUND*,void*)) iftsamplebank::init_,
                                     (int (*)(CSOUND*,void*)) 0,
                                     (int (*)(CSOUND*,void*)) 0);
									 
      status |= csound->AppendOpcode(csound,
                                        (char*)"ftsamplebank",
                                        0xffff,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0);
      return status;
  }

  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
      return 0;
  }
}



