#include <iostream>
#include <cstdlib>
#include "pd4cs.cpp"

using namespace std;

int main(int argc, char *argv[])
{
    std::cout << "Starting pd4cs test." << std::endl;
    PDPatch pdpatch;
    pdpatch.ipdcommand = new float;
    pdpatch.pdcommand = "pd -font 10 -path \"C:/tools/pd/bin\" -path \"C:/tools/pd/doc/vasp\" -lib cyclone -lib ext13 -lib gem -lib maxlib -lib iemlib1 -lib iemlib2 -lib iem_t3_lib -lib iem_mp3 -lib mjLib -lib motex -lib OSC -lib percolate -lib pdogg -lib vasp -lib xeq -lib xsample -lib zexy -lib py -path c:/tools/pd/extra/py/scripts -listdev";
    std::cout << "pd command: " << pdpatch.pdcommand << std::endl;
    pdpatch.init();
    pdpatch.thread_->join();
    std::cout << "Finished." << std::endl;
    return 0;
}
