/*
 * live coding demo for Csound 6
 * Author: Rory Walsh 2013
 */ 

#ifndef LIVE_CODER
#define LIVE_CODER

#include <string>
#include <cstdio>
#include "csound.hpp"
#include <iostream>

using namespace std;

/*
 * sub-class Csound 
 */

class liveCsound: public Csound {
public:
  liveCsound(): Csound(), 
		perf_status(0),
		orchestraHistory(""),
		scoreHistory(""),
		channelsHistory(""){
    CompileOrc("nchnls=2");
  }

  ~liveCsound(){}

  //live code session
  int runLiveCodeSession()
  {
    string str;
    unsigned findString;
    char ch;
    while(ch = getchar()){
      str.push_back(ch);
      if(string::npos != str.find("uorc")){
	unsigned strPos = str.find("uorc");
	string orch = str.substr(0, strPos);
	orchestraHistory +=orch;
	csoundCompileOrc(GetCsound(), orch.c_str());
	return 1;
      }		
      else if(string::npos != str.find("usco")){
	unsigned strPos = str.find("usco");
	string score = str.substr(0, strPos);
	csoundReadScore(GetCsound(),(char *)score.c_str());
	return 1;
      }
      else if(string::npos != str.find("uchan")){
	unsigned strPos = str.find("uchan");
	string channel;
	channel.append("event_i \"i\", 999, 0, .1 \n instr 999");
	channel.append(str.substr(0, strPos));
	channel.append("endin\n");
	cout << channel;
	csoundCompileOrc(GetCsound(), channel.c_str());
	return 1;
      }
      else if(string::npos != str.find("orchist")){
	cout << orchestraHistory;
	return 1;
      }
      else if(string::npos != str.find("scohist")){
	cout << scoreHistory;
	return 1;
      }
      else if(string::npos != str.find("ends")){
	cout << "ending live coding session\n";
	return 0;
      }			
			
			
    }
    return 0;
  }

  int result;
  int perf_status;
  string orchestraHistory;
  string scoreHistory;
  string channelsHistory;

private:		
  Csound* csound;
};

#endif
