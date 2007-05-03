//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Version 2.3 Extension
// © 2003, Steinberg Media Technologies, All Rights Reserved
//-------------------------------------------------------------------------------------------------------

/*
This is a modified version of the original VST SDK file, which is copyright by
Steinberg Media Technologies. This modified file is NOT licensed as a 
software development kit for the development of VST plugins. It is included
here ONLY for the purpose of compiling CsoundVST and vst4cs. If you
wish to develop VST plugins, you must apply to Steinberg for permission
and a copy of the VST SDK.
*/

#ifndef __AEffEditor__
#define __AEffEditor__

#include "Platform.hpp"

class AudioEffect;

//----------------------------------------------------------------------
struct ERect
{
        short top;
        short left;
        short bottom;
        short right;
};

#ifndef __aeffectx__
#include "aeffectx.h"
#endif

#define VST_2_1_EXTENSIONS 1

//----------------------------------------------------------------------
// class AEffEditor Declaration
//----------------------------------------------------------------------
class SILENCE_PUBLIC AEffEditor
{
public:
        AEffEditor (AudioEffect *effect) { this->effect = effect; updateFlag = 0; }
        virtual ~AEffEditor() {}

        virtual long getRect (ERect **rect) { *rect = 0; return 0; }
        virtual long open (void *ptr) { systemWindow = ptr; return 0; }
        virtual void close () {}
        virtual void idle () { if(updateFlag) { updateFlag = 0; update ();} }

        #if MAC
        virtual void draw (ERect *rect) { rect = rect; }
        virtual long mouse (long x, long y) { x = x; y = y; return 0; }
        virtual long key (long keyCode) { keyCode = keyCode; return 0; }
        virtual void top () {}
        virtual void sleep () {}
        #endif

        virtual void update () {}
        virtual void postUpdate () { updateFlag = 1; }

        #if VST_2_1_EXTENSIONS
        virtual long onKeyDown (VstKeyCode &keyCode) { keyCode = keyCode; return -1; }
        virtual long onKeyUp (VstKeyCode &keyCode) { keyCode = keyCode; return -1; }
        virtual long setKnobMode (int val) { return 0; };

        virtual bool onWheel (float distance) { return false; };
        #endif

protected:
        AEffEditor () {};

        AudioEffect *effect;
        void *systemWindow;
        long updateFlag;
};

#endif // __AEffEditor__
//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

