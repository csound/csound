#include "CppSound.hpp"
#include "Counterpoint.hpp"
#include <vector>
#include <iostream>
#include <fstream>

int main(int argc, char **argv)
{
  Counterpoint counterpoint;
  counterpoint.FillRhyPat();

  int trial = 1;
  counterpoint.fillCantus(50,53,52,50,55,53,57,55,53,52,50,0,0,0,0);
  counterpoint.vbs[0]=57;
  counterpoint.vbs[1]=62;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Dorian,&counterpoint.vbs[0],1,11,1);            /* 57 62 -- 38,45,57,62,69,53,50 */
  counterpoint.vbs[0]=38;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Dorian,&counterpoint.vbs[0],1,11,1);            /* 38 57 */

  counterpoint.fillCantus(52,48,50,48,45,57,55,52,53,52,0,0,0,0,0);
  counterpoint.vbs[0]=59;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Phrygian,&counterpoint.vbs[0],1,10,1);          /* 59 64 -- 28,59,64,55,71,40,55 */
  counterpoint.vbs[0]=40;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Phrygian,&counterpoint.vbs[0],1,10,1);          /* 40 59 */

  counterpoint.fillCantus(53,55,57,53,50,52,53,60,57,53,55,53,0,0,0);
  counterpoint.vbs[0]=65;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Lydian,&counterpoint.vbs[0],1,12,1);            /* 65 60 -- 41,60,65,72,57,48,69 */
  counterpoint.vbs[0]=41;
  counterpoint.AnySpecies(Counterpoint::Lydian,&counterpoint.vbs[0],1,12,1);            /* 41 60 */

  counterpoint.fillCantus(43,48,47,43,48,52,50,55,52,48,50,47,45,43,0);
  counterpoint.vbs[0]=55;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Mixolydian,&counterpoint.vbs[0],1,14,1);        /* 55 62 -- 31,55,62,50,59,67,71 */
  counterpoint.vbs[0]=43;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Mixolydian,&counterpoint.vbs[0],1,14,1);        /* 31 55 */

  counterpoint.fillCantus(45,48,47,50,48,52,53,52,50,48,47,45,0,0,0);
  counterpoint.vbs[0]=57;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Aeolian,&counterpoint.vbs[0],1,12,1);           /* 57 64 -- 33,64,52,57,69,40,60 */
  counterpoint.vbs[0]=45;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Aeolian,&counterpoint.vbs[0],1,12,1);           /* 45 64 */

  counterpoint.fillCantus(50,53,52,50,55,53,57,55,53,52,50,0,0,0,0);
  counterpoint.vbs[0]=57;
  counterpoint.vbs[1]=62;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Dorian,&counterpoint.vbs[0],2,11,1);            /* 57 62 -- 38,45,57,62,69,53,50 */

  counterpoint.fillCantus(50,53,52,50,55,53,57,55,53,52,50,0,0,0,0);
  counterpoint.vbs[0]=38;
  counterpoint.vbs[1]=57;
  counterpoint.vbs[2]=62;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.AnySpecies(Counterpoint::Dorian,&counterpoint.vbs[0],1,11,1);            /* 57 62 -- 38,45,57,62,69,53,50 */

  std::vector<int> longcantus;
  for(int i = 0; i < 4; i++)
    {
      longcantus.push_back(50);
      longcantus.push_back(53);
      longcantus.push_back(52);
      longcantus.push_back(50);
      longcantus.push_back(55);
      longcantus.push_back(53);
      longcantus.push_back(57);
      longcantus.push_back(55);
      longcantus.push_back(53);
      longcantus.push_back(52);
      longcantus.push_back(50);
    }
  fprintf(stderr,"\n\nCantus length %zu\n", longcantus.size());
  int voicebegs[] = {38, 57,62,70,80};
  fprintf(stderr,"\n\nTesting resizability...");
  double secondsPerPulse = 0.0625;
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.counterpoint(Counterpoint::Dorian,voicebegs,2,longcantus.size(),1, &longcantus[0]);            /* 57 62 -- 38,45,57,62,69,53,50 */
  counterpoint.toCsoundScore("test1.sco", secondsPerPulse);
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.counterpoint(Counterpoint::Dorian,voicebegs,3,longcantus.size(),3, &longcantus[0]);            /* 57 62 -- 38,45,57,62,69,53,50 */
  counterpoint.toCsoundScore("test2.sco", secondsPerPulse);
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.counterpoint(Counterpoint::Dorian,voicebegs,4,longcantus.size(),2, &longcantus[0]);            /* 57 62 -- 38,45,57,62,69,53,50 */
  counterpoint.toCsoundScore("test3.sco", secondsPerPulse);
  fprintf(stderr,"\n\nTrial %d\n", trial++);
  counterpoint.counterpoint(Counterpoint::Dorian,voicebegs,4,longcantus.size(),3, &longcantus[0]);            /* 57 62 -- 38,45,57,62,69,53,50 */
  counterpoint.toCsoundScore("test4.sco", secondsPerPulse);
  return 0;
}
