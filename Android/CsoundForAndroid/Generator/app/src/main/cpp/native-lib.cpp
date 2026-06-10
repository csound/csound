#include <jni.h>
#include <string>
#include "csound/AndroidCsound.hpp"
static AndroidCsound *csound = nullptr;
static int vol = 100;
static float oct = 4.f;
static const char *code = R"(
0dbfs = 1
nchnls = 2
seed 0
ga1 init 0
ga2 init 0
instr 1
 vol:k = chnget:k("volume")
 fr:i = chnget:i("oct")*45;
 mod:a = oscili(linrand(5)*p5, p5*(1 + gauss(0.5)))
 sig:a = oscili(vol*expon(p4,p3,0.001),p5+mod*expon(1,p3,0.001))
 left:a, right:a pan2 sig, linrand:i(1)
 out(left, right)
 ga1 = left*0.25;
 ga2 = right*0.25;
 schedule(1,0.1 + linrand(0.1), 0.1+linrand(2), linrand(1), fr + gauss(fr))
endin
schedule(1,0,0.1,0.5,500)
instr 2
 left:a, right:a reverbsc2 ga1,ga2, 0.7, 5000
  out(left, right)
  ga1 = 0
  ga2 = 0
endin
schedule(2,0,-1)
        )";

extern "C" JNIEXPORT void JNICALL
Java_com_example_csoundapp_MainActivity_setVolume(JNIEnv* env,jobject MainActivity,
                                                  jint val){
      vol = val;
      if(csound != nullptr)
          csound->SetControlChannel("volume", val / 100.f);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_csoundapp_MainActivity_setOctave(JNIEnv* env,jobject MainActivity,
                                                  jfloat val){
    float octave = powf(2.f, val);
    oct = octave;
    if(csound != nullptr)
        csound->SetControlChannel("oct", octave);

}

extern "C" JNIEXPORT void JNICALL
Java_com_example_csoundapp_MainActivity_startCsound(
        JNIEnv* env, jobject MainActivity) {
    if (csound == nullptr) {
        csound = new AndroidCsound;
        csound->setAAudioCallbacks();
        csound->SetOption("-o dac");
        csound->CompileOrc(code);
        csound->SetControlChannel("volume", vol / 100.f);
        csound->SetControlChannel("oct", oct);
        csound->Start();
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_csoundapp_MainActivity_stopCsound(
        JNIEnv* env, jobject MainActivity) {
    if(csound != nullptr) {
        AndroidCsound *tmp = csound;
        tmp->EventString("e 0 0", 1);
        csound = nullptr;
        delete tmp;
    }
}