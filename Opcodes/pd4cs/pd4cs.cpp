#include "OpcodeBase.hpp"
#include <string>
#include <vector>
#include <boost/thread/thread.hpp>
#include <boost/tokenizer.hpp>


extern "C" int sys_main(int argc, char **argv);
extern "C" int sys_bail(int exitcode);
extern "C" int sys_lock();
extern "C" int sys_unlock();

// If this is possible at all: 
// Call sys_main in a separate thread to start PD.
// Before each tick, write audio using adc_dsp, or perhaps using send.
// Before each tick, enqueue MIDI using sys_midibytein.
// After each tick, read audio from sys_soundout if it can be accessed,
// perhaps using recv,
// or create a new external to do that.
// Schedule ticks using thread locking calls: sys_lock before PD does it,
// then do stuff, sys_unlock, sys_lock, etc.

class PDPatch : public OpcodeBase<PDPatch>
{
public:
    // Outputs.
    MYFLT *ipd;
    // Inputs.
    MYFLT *ipdcommand;
    // State.
    std::string pdcommand;
    std::vector<const char *> tokens;
    float *sys_soundin_;
    float *sys_soundout_;
    int init()
    {
 		if (*ipdcommand == SSTRCOD) {
			pdcommand = STRARG;          
		}
        boost::tokenizer<> tokenizer_ = pdcommand;
        for(boost::tokenizer<>::iterator it = tokenizer_.begin(); it != tokenizer_.end(); ++it) {
            tokens.push_back(it->c_str());
        }
        ipd = (MYFLT *)this;
        return OK;
    }
    int deinit()
    {
        sys_bail(0);
        return OK;
    }
    void startThread()
    {
        boost::thread thread_(*this);
    }
    void operator()()
    {
        sys_main(tokens.size(), const_cast<char **>(&tokens.front()));
    }
};

class PDMidi : public OpcodeBase<PDMidi>
{
public:
    // Inputs.
    MYFLT *ipdpatch;
    MYFLT *kinstatus;
    MYFLT *kinchannel;
    MYFLT *kindata1;
    MYFLT *kindata2;
    // No outputs.
    // State.
    MYFLT oldkinstatus;
    MYFLT oldkinchannel;
    MYFLT oldkindata1;
    MYFLT oldkindata2;
    PDPatch *pdpatch;
    int init()
    {
        pdpatch = (PDPatch *)ipdpatch;
        oldkinstatus = 0;
        oldkinchannel = 0;
        oldkindata1 = 0;
        oldkindata2 = 0;
        return OK;
    }
    int kontrol()
    {
        return OK;
    }
};

class PDNote : public OpcodeBase<PDNote>
{
public:
    // Inputs.
    MYFLT *ipdpatch;
    MYFLT *kinchannel;
    MYFLT *kinkey;
    MYFLT *kinvelocity;
    MYFLT *kinduration;
    // No outputs.
    // State.
    PDPatch *pdpatch;
    MYFLT oldkinchannel;
    MYFLT oldkinkey;
    MYFLT oldkinvelocity;
    MYFLT oldkinduration;
    size_t framesRemaining;
    int init()
    {
        pdpatch = (PDPatch *)ipdpatch;
        oldkinchannel = 0;
        oldkinkey = 0;
        oldkinvelocity = 0;
        oldkinduration = 0;
        return OK;
    }
    int kontrol()
    {
        sys_lock();
        sys_unlock();
        return OK;
    }
};

class PDAin : public OpcodeBase<PDAin>
{
public:
    // No outputs.
    // Inputs.
    MYFLT *ipdpatch;
    MYFLT *ain1;
    MYFLT *ain2;
    // State.
    PDPatch *pdpatch;
    std::vector<MYFLT> input1;
    std::vector<MYFLT> input2;
    int initialize()
    {
        pdpatch = (PDPatch *)ipdpatch;
        return OK;
    }
    int audio()
    {
        return OK;
    }
};

class PDAout : public OpcodeBase<PDAout>
{
public:
    // Outputs.
    MYFLT *aout1;
    MYFLT *aout2;
    // Inputs.
    MYFLT *ipdpatch;
    // State.
    PDPatch *pdpatch;
    int init()
    {
        pdpatch = (PDPatch *)ipdpatch;
        return OK;
    }
    int audio()
    {
        return OK;
    }
};

OENTRY oentries[] = {
    {"pdpatch", sizeof(PDPatch), 1, "i", "S", &PDPatch::init_, 0, 0, &PDPatch::deinit_},
    {"pdmidi", sizeof(PDMidi), 5, "", "ikkjj", &PDMidi::init_, &PDMidi::kontrol_, 0, 0},
    {"pdnote", sizeof(PDNote), 5, "", "ikkkk", &PDNote::init_, &PDNote::kontrol_, 0, 0},
    {"pdain", sizeof(PDAin), 5, "", "iaa", &PDAin::init_, 0, &PDAin::audio_, 0},
    {"pdaout", sizeof(PDAout), 5, "aa", "i", &PDAout::init_, 0, &PDAout::audio_, 0},
};


int opcode_size()
{
    return sizeof(oentries);
}

OENTRY *opcode_init(ENVIRON *csound)
{
    return oentries;
}
