/*
  Copyright (C) 2007 Rory Walsh, Victor Lazzarini


  csLADSPA is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.


  csLADSPA is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.


  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
      02111-1307 USA

*/


#include <dirent.h>
#include <string>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <cstring>
#include "csound.hpp"
#include "ladspa.h"
using namespace std;

#define MAXLINESIZE 4098
#define MAXPORTS 64

//to remove leading and trailing spaces
string trim(string s)
{
  //trim spaces at start
  s.erase( 0, s.find_first_not_of( " \t\n" ) );
  //trim spaces at end
  s.erase( s.find_last_not_of( " \t\n" ) + 1);
  return s;
}

struct AuxData {
  string *portnames;
  int ksmps;
};

// Csound plugin class
// holds the Csound instancec
// and audio/control port pointers
struct CsoundPlugin {

  LADSPA_Data *ctl[MAXPORTS];
  LADSPA_Data **inp;
  LADSPA_Data **outp;
  string *ctlchn;
  int ctlports;
  Csound* csound;
  int result;
  MYFLT *spout, *spin;
  int chans;
  int frames;

  CsoundPlugin(const char *csd, int chns, int ports, AuxData* paux,
               unsigned long rate);
  ~CsoundPlugin();
  void Process(unsigned long cnt);
};



// constructor
CsoundPlugin::CsoundPlugin(const char *csd,
                           int chns, int ports, AuxData *paux,
                           unsigned long rate){
  char** cmdl;
  string sr_override, kr_override;
  char *sr, *kr;
  int ksmps = paux->ksmps;
  ctlchn = paux->portnames;
  ctlports = ports;

  chans = chns;
  frames = ksmps;
  inp = new LADSPA_Data*[chans];
  outp = new LADSPA_Data*[chans];


  // csound parameters
  cmdl = new char*[5];
  cmdl[0] = (char*)"csound";
  cmdl[1] =  (char *)csd;
  cmdl[2] = (char*)"-n";

  // sampling-rate override
  sr = new char[32];
  sprintf(sr,"%d", (int)rate);
  sr_override.append("--sample-rate= ");
  sr_override.append(sr);
  cmdl[3] = (char *) sr_override.c_str();

  // ksmps override
  kr = new char[32];
  sprintf(kr,"%f",(float)rate/ksmps);
  kr_override.append("-k ");
  kr_override.append(kr);
  cmdl[4] = (char *) kr_override.c_str();

  csound =  new Csound;
  result = csound->Compile(5,cmdl);
  spout = csound->GetSpout();
  spin  = csound->GetSpin();
  memset(ctl, 0, sizeof(LADSPA_Data *)*MAXPORTS);

  delete[] cmdl;
  delete[]  sr;
  delete[]  kr;
  delete paux;
}

CsoundPlugin::~CsoundPlugin(){
  delete csound;
  delete[] ctlchn;
  delete[] inp;
  delete[] outp;
}


// Processing function
void CsoundPlugin::Process(unsigned long cnt){
  int pos, i, j, ksmps = csound->GetKsmps(),n = cnt;
  MYFLT scale = csound->Get0dBFS();

  for(i=0;i<ctlports;i++)
    csound->SetChannel(ctlchn[i].c_str(),
                       *(ctl[i]));

  if(!result){
    for(i=0; i < n; i++, frames++){
      if(frames == ksmps){

        result = csound->PerformKsmps();
        frames = 0;
      }
      for(j=0; j < chans; j++)
        if(!result){
          pos = frames*chans;
          spin[j+pos] =  inp[j][i]*scale;
          outp[j][i] = (LADSPA_Data) (spout[j+pos]/scale);
        } else outp[j][i] = 0;
     }
  }
}


// Plugin instantiation
static LADSPA_Handle createplugin(const LADSPA_Descriptor *pdesc,
                                  unsigned long rate)
{
  CsoundPlugin* p;
  int i, aports=0;
   cerr << "instantiating plugin: " << pdesc->Label << "\n";
  for(i=0; i < (int)pdesc->PortCount; i++)
    if(pdesc->PortDescriptors[i] & LADSPA_PORT_AUDIO) aports++;
  #ifdef DEBUG
  cerr << "instantiating plugin: " << pdesc->Label << "\n";
  #endif
   p = new CsoundPlugin(pdesc->Label, aports/2, pdesc->PortCount-aports,
                    (AuxData *)pdesc->ImplementationData,rate);
  #ifdef DEBUG
  if(!p->result)
    cerr << "plugin instantiated: " << pdesc->Label << "\n";
  else
    cerr << "plugin not instantiated \n";
  #endif
  return p;
}



// Plugin cleanup
static void destroyplugin(LADSPA_Handle inst)
{
  CsoundPlugin* p = (CsoundPlugin*)inst;
  delete p;
}


// Port connections
static void connect(LADSPA_Handle inst, unsigned long port, LADSPA_Data* pdata)
{
  CsoundPlugin *p = (CsoundPlugin *) inst;
  unsigned int chans = p->chans;
  if(port < chans) p->inp[port] = pdata;
  else if(port < chans*2) p->outp[port-chans] = pdata;
  else p->ctl[port-chans*2] = pdata;
}


// Processing entry point
static void runplugin(LADSPA_Handle inst, unsigned long cnt)
{
   ((CsoundPlugin *)inst)->Process(cnt);
}


// initialise a descriptor for a given CSD file
static LADSPA_Descriptor *init_descriptor(char *csdname)
{
  char *str, *tmp;
  string csddata, temp;
  int portcnt=2, chans=1,i;
  bool plugin_found=false;
  string::size_type indx,indx2,equals=0;
  fstream csdfile(csdname);
  char **PortNames = new char*[MAXPORTS];
  AuxData *paux = new AuxData;
  float upper, lower;
  string *ctlchn = new string[MAXPORTS];
  LADSPA_PortDescriptor *PortDescriptors = new LADSPA_PortDescriptor[MAXPORTS];
  LADSPA_PortRangeHint  *PortRangeHints =  new LADSPA_PortRangeHint[MAXPORTS];
  LADSPA_Descriptor *desc =  new LADSPA_Descriptor;


  // if descriptor was created
  // and csdfile was open properly
  if (desc && csdfile.is_open())
    {
      tmp = new char[strlen(csdname)+1];
      str = new char[MAXLINESIZE+1];
      strcpy(tmp, csdname);
      desc->Label = (const char*) tmp;
      paux->ksmps = 10;
      // check channels
      while(!csdfile.eof()){
        csdfile.getline(str,MAXLINESIZE);
        csddata = str;
#ifdef DEBUG
        cerr << csddata << "\n";
#endif
        // find nchnls header statement
        if(csddata.find("nchnls")!=string::npos)
          {
            indx = csddata.find('=');
            temp = csddata.substr(indx+1,100);
            chans = (int) atoi((char *) temp.c_str());
            portcnt = chans*2;
            break;
          }
        // if we reached the end of header block
        else if(csddata.find("instr")!=string::npos) break;
      }
      csdfile.seekg(0);
      // check ksmps, similar to above
      while(!csdfile.eof()){
        csdfile.getline(str,MAXLINESIZE);
        csddata = str;
#ifdef DEBUG
        cerr << csddata << "\n";
#endif
        if(csddata.find("ksmps")!=string::npos){
          indx = csddata.find('=');
          temp = csddata.substr(indx+1,100);
          paux->ksmps = (int) atoi((char *) temp.c_str());
          break;
        }
        else if(csddata.find("instr")!=string::npos) break;
      }

      csdfile.seekg(0);
      while(!csdfile.eof())
        {
          csdfile.getline(str,MAXLINESIZE);
          csddata = str;
          // check for csLADSPA section
          if(csddata.find("<csLADSPA>")!=string::npos){
            cerr << "cSLADSPA plugin found: \n";
            plugin_found = true;
          }
          // now if we found a plugin header section
          // we proceed to read it
          if (plugin_found) {
            cerr << csddata << "\n";
            if(csddata.find("Name")!=string::npos){
              equals = csddata.find('=');
              temp = csddata.substr(equals+1);
              temp = trim(temp);
              tmp = new char[temp.length()+1];
              strcpy(tmp, (const char*)temp.c_str());
              desc->Name = (const char*) tmp;
            }
            else if(csddata.find("Maker")!=string::npos){
              equals = csddata.find('=');
              temp = csddata.substr(equals+1) + (sizeof(MYFLT) == 4 ?
                         "     (csLADSPA: Lazzarini, Walsh)" :
                         "     (csLADSPA64: Lazzarini, Walsh)") ;
              temp = trim(temp);
              tmp = new char[temp.length()+1];
              strcpy(tmp, (const char*)temp.c_str());
              desc->Maker = (const char*) tmp;
            }
            else if(csddata.find("UniqueID")!=string::npos){
              equals = csddata.find('=');
              temp = csddata.substr(equals+1);
              temp = trim(temp);
              desc->UniqueID = atoi(temp.c_str());
            }
            else if(csddata.find("Copyright")!=string::npos){
              equals = csddata.find('=');
              temp = csddata.substr(equals+1, 100);
              temp = trim(temp);
              tmp = new char[temp.length()+1];
              strcpy(tmp, (const char*)temp.c_str());
              desc->Copyright = (const char*) tmp;
            }
            else if(csddata.find("ControlPort")!=string::npos)
              {
                equals = csddata.find('=');
                if(portcnt < MAXPORTS) {
                  PortDescriptors[portcnt] =
                    LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
                  temp = csddata;
                  indx = temp.find('|');
                  string temp2 = trim(temp.substr(equals+1,indx-(equals+1)));
                  PortNames[portcnt] = new char[temp2.length()+1];
                  strcpy(PortNames[portcnt], (const char*)temp2.c_str());
                  temp2 = trim(temp.substr(indx+1));
                  ctlchn[portcnt-chans*2] =  temp2;//temp.substr(indx+1);
                  csdfile.getline(str,500);
                  csddata = str;
                  temp = csddata.substr(6, 200);
                  indx = temp.find('|');
                  indx2 = temp.find('&');
                  // if &log is found in the range spec, then we have a
                  // LOGARITHMIC response
                  if(indx2<200 &&
                     !strcmp(temp.substr(indx2+1,indx2+3).c_str(), "log")){
                    PortRangeHints[portcnt].HintDescriptor =
                      (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE
                       | LADSPA_HINT_LOGARITHMIC | LADSPA_HINT_DEFAULT_MIDDLE);
                    lower =  atof((char*)temp.substr(0, indx).c_str());
                    upper =  atof((char*)temp.substr(indx+1, indx2).c_str());
                  }
                  else {
                    PortRangeHints[portcnt].HintDescriptor =
                      (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE  );
                    lower =  atoi((char*)temp.substr(0, indx).c_str());
                    upper =  atoi((char*)temp.substr(indx+1, 200).c_str());
                  }
                  PortRangeHints[portcnt].LowerBound =
                    lower < upper ? lower : upper;
                  PortRangeHints[portcnt].UpperBound =
                    upper > lower ? upper : lower;
                  portcnt++;
                  indx2 = 0;
                }
              }
            else if(csddata.find("</csLADSPA>")!=string::npos) break;
          }
        }
      csdfile.close();
      // now if a csLADSPA section was found
      // we proceed to create the plugin descriptor
      if(plugin_found) {
        desc->PortCount = portcnt;
        for(i=0;i<chans;i++){
          PortDescriptors[i] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
          PortDescriptors[i+chans] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
          PortRangeHints[i].HintDescriptor = 0;
          PortRangeHints[i+chans].HintDescriptor = 0;
          PortNames[i] = new char[32];
          sprintf(PortNames[i], "csladspa-audio-in%d", i);
          PortNames[i+chans] = new char[32];
          sprintf(PortNames[i+chans], "csladspa-audio-out%d", i);
        }
        desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
        desc->PortDescriptors = (const LADSPA_PortDescriptor *) PortDescriptors;
        desc->PortNames = (const char **)PortNames;
        desc->PortRangeHints = (const LADSPA_PortRangeHint *) PortRangeHints;
        desc->instantiate = createplugin;
        desc->connect_port = connect;
        desc->activate = NULL;
        desc->run = runplugin;
        desc->run_adding = NULL;
        desc->set_run_adding_gain  = NULL;
        desc->deactivate = NULL;
        desc->cleanup = destroyplugin;
        // add the channel names to the descriptor
        paux->portnames = ctlchn;
        desc->ImplementationData = (void *) paux;
        delete[] str;
        cerr << "PLUGIN LOADED\n";
        return desc;
      }
      delete[] str;
    }
  // otherwise we just delete the empty descriptors
  // and return NULL
  delete desc;
  delete[] PortNames;
  delete paux;
  delete[] ctlchn;
  delete[] PortDescriptors;
  delete[] PortRangeHints;
  cerr << "PLUGIN NOT LOADED: probably missing csLADSPA section\n";
  return NULL;
}


// count CSDs in the current directory
unsigned int CountCSD(char **csdnames)
{
  DIR             *dip = NULL;
  struct dirent   *dit;
  string          temp, name, path;
  int             i = 0;
  size_t    indx = 0;
  char ladspa_path[1024] = "";
  char *src = NULL;

#ifdef MACOSX
  src = strdup("/Library/Audio/Plug-Ins/LADSPA");
#else
  src = getenv("LADSPA_PATH");
  if (src)
    src = strdup(src);
#endif

  if (src) {
    strncpy(ladspa_path, src, 1023);
    ladspa_path[1023] = '\0';
  }

  // if no LADSPA_PATH attempt to open
  // current directory
  if(strlen(ladspa_path) == 0) dip = opendir(".");
  else {
    path = ladspa_path;
#ifdef WIN32
  indx = path.find(";");
#else
  indx = path.find(":");
#endif
    if(indx!=string::npos){
      dip = opendir(path.substr(0,indx).c_str());
      strncpy(ladspa_path, path.substr(0,indx).c_str(), 1023);
      ladspa_path[1023] = '\0';
    }
    else dip = opendir(ladspa_path);
  }
  if (dip == NULL) {
    free(src);
    return 0;
  }
  while ((dit = readdir(dip))!=NULL)
    {
      temp = dit->d_name;
      indx = temp.find(".csd", 0);
      string validExt = trim(temp.substr(indx+1));
      if(!validExt.compare("csd"))
        {
          if(strlen(ladspa_path) != 0) {
            name = ladspa_path;
            name.append("/");
            name.append(temp);
          }
          else name = temp;
          csdnames[i] =  new char[name.length()+1];
          strcpy(csdnames[i], (char*)name.c_str());
          i++;
        }
    }
  closedir(dip);
  free(src);
  return i;
}

// plugin lib entry point
PUBLIC
const LADSPA_Descriptor *ladspa_descriptor(unsigned long Index)
{
  // count CSD files to build the plugin lib
  // and fill in the CSD names list
  LADSPA_Descriptor *descriptor = NULL;
  char **csdnames = new char*[100];
  unsigned int csds;
  csds = CountCSD(csdnames);

  // if the requested index is in the range of CSD numbers
  if(Index<csds)
    {
      cerr << "attempting to load plugin index: " << Index << "\n";
      // initialise the descriptor for a given CSD
      descriptor = init_descriptor(csdnames[Index]);
    }
  // delete the CSD list
  for(unsigned int i=0; i < csds; i++) delete[] csdnames[i];

  delete[] csdnames;
  if(descriptor == NULL)
    cerr << "no more csLADSPA plugins\n";
  return descriptor;

}



