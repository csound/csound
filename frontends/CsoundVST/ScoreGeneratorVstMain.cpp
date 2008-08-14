/**
 * S C O R E   G E N E R A T O R   V S T
 *
 * A VST plugin for writing score generators in Python.
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "ScoreGeneratorVst.hpp"
#include "ScoreGeneratorVstFltk.hpp"

#define INIT_CLASS_IID

#include "ipluginbase.h"
#include "pluginfactory.h"
#include "iplugui.h"
#include "midieffect.h"
#include "eventqueue.h"

static AudioEffect *effect = 0;
bool oome = false;
extern bool debug;

#if MAC
#pragma export on
#endif

// prototype of the export function main
#if defined(__GNUC__) && defined(WIN32)
#define main main_plugin
extern "C" __declspec(dllexport) AEffect *main_plugin (audioMasterCallback audioMaster)
#elif defined(LINUX) || defined(MACOSX)
  AEffect *main_plugin (audioMasterCallback audioMaster)
#else
  AEffect *main(audioMasterCallback audioMaster)
#endif
{
  // get vst version
  if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0)) {
    return 0;  // old version
  }
  effect = new ScoreGeneratorVst (audioMaster);
  if (!effect) {
    return 0;
  }
  if (oome) {
    delete effect;
    return 0;
  }
  return effect->getAeffect ();
}

#if MAC
#pragma export off
#endif

#if WIN32
#include <windows.h>

HINSTANCE ghInst = 0;

char gPath[MAX_PATH] = {0};

bool InitModule ()
{
    return true;
}

bool DeinitModule ()
{
    return true;
}

BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
    if(dwReason == DLL_PROCESS_ATTACH) {
        ghInst = hInst;
        GetModuleFileName (ghInst, gPath, MAX_PATH);
        char* bkslash = strrchr (gPath, '\\');
        if(bkslash) {
            gPath[bkslash - gPath + 1] = '\0';
        }
        if(!InitModule ()) {
            return FALSE;
        }
    } else if(dwReason == DLL_PROCESS_DETACH) {
        DeinitModule ();
    }
    return TRUE;
}

/**
* Uses ScoreGeneratorVst to implement IMidiEffect.
*/
class ScoreGenPlugin :
    public CMidiEffect,
    public ScoreGeneratorVst,
    public IEditorFactory,
    public IPlugView
{
protected:
    ITransportInfo *transportInfo;
    IMasterTrackInfo *masterTrackInfo;
    CMidiEventQueue outputQueue;
    unsigned long __funknownRefCount;
public:
    ScoreGenPlugin(char *storageUID) :
        CMidiEffect(storageUID),
        transportInfo(0),
        masterTrackInfo(0),
        __funknownRefCount(0)
    {
        log("BEGAN ScoreGenPlugin::ScoreGenPlugin()...\n");
        flags = kMEIsEditable;
        presetsAreChunks (true);
        //setResourceID (gEditorResourceID);
        log("ENDED ScoreGenPlugin::ScoreGenPlugin().\n");
    }
    static FUnknown* createInstance (void* context)
    {
        ScoreGenPlugin *scoreGenPlugin = new ScoreGenPlugin((char *)context);
        scoreGenPlugin->addRef();
        return (IMidiEffect *)scoreGenPlugin;
    }
    int generate()
    {
        alive = false;
        log("BEGAN ScoreGenPlugin::generate()...\n");
        clearEvents();
        csound::Shell::runScript();
        ScoreGeneratorVst::generate();
        alive = true;
        logv("Generated %d events.\n", scoreGeneratorEvents.size());
        log("ENDED ScoreGenPlugin::generate().\n");
        return kResultOk;
    }
    // VST MIDI Plugin SDK Overrides:
    // FUnknown
    tresult PLUGIN_API queryInterface (const char* iid, void** obj)
    {
        QUERY_INTERFACE (iid, obj, ::FUnknown::iid,                     IMidiEffect)
        QUERY_INTERFACE (iid, obj, IPluginBase::iid,                    IPluginBase)
        QUERY_INTERFACE (iid, obj, IMidiEffect::iid,                    IMidiEffect)
        QUERY_INTERFACE (iid, obj, IPlugController::iid,                IPlugController)
        QUERY_INTERFACE (iid, obj, IGenericPlugController::iid,IGenericPlugController)
        QUERY_INTERFACE (iid, obj, IEditorFactory::iid,         IEditorFactory)
        QUERY_INTERFACE (iid, obj, IPersistentChunk::iid,      IPersistentChunk)
        *obj = 0;
        return kNoInterface;
    }
    virtual unsigned long PLUGIN_API addRef ()
    {
            return ++__funknownRefCount;
    }
    virtual unsigned long PLUGIN_API release ()
    {
            if(--__funknownRefCount == 0) {
                    delete this;
                    return 0;
            }
            return __funknownRefCount;
    }
    // IMidiEffect
    tresult initialize (FUnknown* context)
    {
        tresult result = CMidiEffect::initialize (context);
        if (result != kResultOk) {
            return result;
        }
        result = context->queryInterface (ITransportInfo::iid, (void**)&transportInfo);
        if (result != kResultOk) {
            return result;
        }
        result = context->queryInterface (IMasterTrackInfo::iid, (void**)&masterTrackInfo);
        if (result != kResultOk) {
            return result;
        }
        accessor->setCanPlayInStop(true);
        outputQueue.setAccessor(accessor);
        return kResultOk;
    }
    tresult PLUGIN_API terminate ()
    {
        clear();
        outputQueue.empty();
        outputQueue.setAccessor (0);
        if (transportInfo) {
            transportInfo->release ();
        }
        transportInfo = 0;
        if (masterTrackInfo) {
            masterTrackInfo->release ();
        }
        masterTrackInfo = 0;
        return CMidiEffect::terminate ();
    }
    double ppqToSeconds(long ppq)
    {
        double seconds = masterTrackInfo->ppq2seconds(ppq * 10000);
        return (seconds / 10000.0);
    }
    long secondsToPpq(double seconds)
    {
        seconds = long(seconds * 10000.0);
        double ppq = masterTrackInfo->seconds2ppq(seconds);
        return long(ppq / 10000.0);
    }
    tresult PLUGIN_API playAction (long fromPPQ, long toPPQ, bool immediate)
    {
        if (!immediate) {
            double lowerTime = ppqToSeconds(fromPPQ);
            double upperTime = ppqToSeconds(toPPQ);
            if (debug) {
                logv("CALLED playAction(fromPPQ=%8d %12.4f seconds, toPPQ=%8d %12.4f seconds, immediate=%d)\n", fromPPQ, lowerTime, toPPQ, upperTime, immediate);
            }
            std::multimap<double, ScoreGeneratorEvent>::const_iterator lowerI = scoreGeneratorEvents.lower_bound(lowerTime);
            std::multimap<double, ScoreGeneratorEvent>::const_iterator upperI = scoreGeneratorEvents.upper_bound(upperTime);
            for(int i = 0; lowerI != upperI && lowerI != scoreGeneratorEvents.end(); ++lowerI, ++i) {
                long status =   lowerI->second.vstMidiEvent.midiData[0];
                long data1 =    lowerI->second.vstMidiEvent.midiData[1];
                long data2 =    lowerI->second.vstMidiEvent.midiData[2];
                long start =    secondsToPpq(lowerI->second.start);
                long length =   secondsToPpq(lowerI->second.duration);
                IMEObjectID event = accessor->createMidiEvent((MidiStatus) status, data1, data2, length);
                accessor->setStart(event, start);
                accessor->setImmediateEvent(event, false);
                if (debug) {
                    logv("  Event %5d: start=%12.4f (%8d ppq), length=%12.4f (%8d ppq), status=%3d, channel=%3d, data1=%3d, data2=%3d\n",
                        i,
                        ppqToSeconds(accessor->getStart(event)),
                        accessor->getStart(event),
                        ppqToSeconds(accessor->getLength(event)),
                        accessor->getLength(event),
                        accessor->getStatus(event),
                        accessor->getChannel(event),
                        accessor->getData1(event),
                        accessor->getData2(event));
                }
                accessor->passToOutputQueue(event);
                accessor->destroyEvent(event);
            }
        }
        return kResultOk;
    }
    tresult PLUGIN_API positAction (long position)
    {
        if (debug) {
            logv("CALLED positAction(position=%d)\n", position);
        }
        return kResultOk;
    }
    tresult PLUGIN_API swapAction (long position, long length)
    {
        if (debug) {
            logv("CALLED playAction(position=%d, length=%d)\n", position, length);
        }
        return kResultOk;
    }
    tresult PLUGIN_API stopAction (long position)
    {
        if (debug) {
            logv("CALLED stopAction(position=%d)\n", position);
        }
        return kResultOk;
    }
    tresult PLUGIN_API startAction (long position)
    {
        if (debug) {
            logv("CALLED startAction(position=%d)\n", position);
        }
        return kResultOk;
    }
    // IEditorFactory
    //tresult PLUGIN_API getResource (const char* path, char* buffer, long* bufferSize)
    //{
    //    return false;
    //}
    tresult PLUGIN_API getEditorSize (const char* name, ViewRect* rect)
    {
        rect->top = 0;
        rect->left = 0;
        rect->right = ScoreGeneratorVstFltk::kEditorWidth;
        rect->bottom = ScoreGeneratorVstFltk::kEditorHeight;
        return kResultOk;
    }
    tresult PLUGIN_API createEditor (const char* name, ViewRect* rect, IPlugView** editor)
    {
        *editor = this;
        addRef();
        return kResultOk;
    }
    // IPluginView
    tresult PLUGIN_API attached (void* parent)
    {
        scoreGeneratorVstFltk->open(parent);
        return kResultOk;
    }
    tresult PLUGIN_API removed ()
    {
        scoreGeneratorVstFltk->close();
        return kResultOk;
    }
    tresult PLUGIN_API idle ()
    {
        scoreGeneratorVstFltk->idle();
        return kResultOk;
    }
    tresult PLUGIN_API onWheel (float distance)
    {
        return kNotImplemented;
    }
    tresult PLUGIN_API onKey (char asciiKey, long keyMsg, long modifiers)
    {
        return kNotImplemented;
    }
    tresult PLUGIN_API onSize (ViewRect* newSize)
    {
        return kNotImplemented;
    }
    // IPersistentChunk
    tresult PLUGIN_API setChunk (char* chunk, long size)
    {
        log("BEGAN ScoreGenPlugin::setChunk()...\n");
        size = ScoreGeneratorVst::setChunk(chunk, size, true);
        generate();
        log("ENDED ScoreGenPlugin::setChunk().\n");
        return kResultOk;
    }
    tresult PLUGIN_API getChunk (char* chunk, long* size)
    {
        log("BEGAN ScoreGenPlugin::getChunk()...\n");
        void *data = 0;
        if (!chunk) {
            *size = ScoreGeneratorVst::getChunk(&data, true);
            logv("size = %d\n", (int) *size);
        } else {
            *size = ScoreGeneratorVst::getChunk(&data, true);
            memcpy(chunk, data, (int) *size);
            logv("chunk = 0x%x\n", chunk);
        }
        log("ENDED ScoreGenPlugin::getChunk().\n");
        return kResultOk;
    }
};

static PFactoryInfo factoryInfo = {"Csound", "http://csounds.com", "gogins@pipeline.com", 1};

static PClassInfo classInfo = { INLINE_UID (0xe69c1d10, 0x771340b7, 0xafc3c900, 0x7b43406a), PClassInfo::kManyInstances,        kMidiModuleClass, "ScoreGen"};

PLUGIN_API IPluginFactory* GetPluginFactory ()
{
    if(!gPluginFactory) {
        gPluginFactory = new CPluginFactory (factoryInfo);
        gPluginFactory->registerClass (&classInfo, ScoreGenPlugin::createInstance, classInfo.cid);
    } else {
        gPluginFactory->addRef ();
    }
    return gPluginFactory;
}
#endif
