/*
  This file is part of Csound.

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
#if defined(__MINGW64__)
#include <stdint.h>
#include <stdlib.h>

extern "C" {
    uint64_t htonll(uint64_t value)
    {
        _byteswap_uint64(value);
    }
    uint64_t ntohll(uint64_t value)
    {
        _byteswap_uint64(value);
    }
}
#endif

#include <ableton/Link.hpp>
#include <OpcodeBase.hpp>

/**
 * A B L E T O N   L I N K   O P C O D E S
 *
 * Author: Michael Gogins
 * January 2017
 *
 * These opcodes implement the Ableton Link protocol from:
 *
 * https://github.com/Ableton/link
 *
 * The purpose of Ableton Link is to sychronize musical time, beat, and phase
 * between musical applications performing in real time from separate
 * programs, processes, and network addresses. This is useful e.g. for laptop
 * orchestras.
 *
 * There is one global, peer-to-peer Link session that maintains a global time
 * and beat on the local area network. Any peer may set the global tempo,
 * and thereafter all peers in the session share that tempo. A process may
 * have any number of peers (i.e., any number of Link objects). Each peer
 * may also define its own "quantum" i.e. some multiple of the beat, e.g. a
 * quantum of 4 might imply 4/4 time. The phase of the time is defined w.r.t
 * the quantum, e.g. phase 0.5 of a quantum of 4 would be the second beat of
 * the measure. Peers may read and write timelines with local time, beat, and
 * phase, counting from when the peer is enabled, but the tempo and beat on
 * all timelines for all peers in the session will coincide.
 *
 * The first peer to enable a session determines the initial tempo. After
 * that, the tempo is changed only, and whenever, any peer explicity calls
 * the set tempo functon (link_tempo_set, in Csound).
 *
 * The Link tempo is independent of the Csound score tempo. Performances that
 * need to synchronize the score tempo with the Link tempo may use the tempo
 * opcode to set the score tempo from the Link tempo; or conversely, set the
 * Link tempo from the score tempo using the tempoval opcode.
 *
 * Please note, the phase and beat obtained or set by these opcodes is only as
 * precise as allowed by the duration of Csound's kperiod, the audio driver
 * used by Csound, the network stability, and the system's most precise clock.
 *
 * Build for testing with something like:
 *
 * g++ ableton_link_opcodes.cpp -std=gnu++11 -DLINK_PLATFORM_WINDOWS=1 -Werror -Wno-multichar -O2 -g -lcsound64 -I/home/restore/link/include -I/home/restore/link/modules/asio-standalone/asio/include -I../include -I../H -shared -oableton_link_opcodes.dll
 * g++ ableton_link_opcodes.cpp -std=gnu++11 -DLINK_PLATFORM_LINUX=1 -Werror -Wno-multichar -O2 -g -fPIC -lcsound64 -I/home/mkg/link/include -I/home/mkg/link/modules/asio-standalone/asio/include -I/usr/local/include/csound -I/home/mkg/csound/csound/include -shared -oableton_link_opcodes.so
 */
using namespace csound;

static CSOUND *csound_;

void * operator new(std::size_t n) throw(std::bad_alloc)
{
    return csound_->Malloc(csound_, n);
}

void operator delete(void * p) throw()
{
  csound_->Free(csound_, p);
}

static bool enable_debug = 0;

#define debug(fmt, ...) \
            do { if (enable_debug) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

using floating_point_microseconds = std::chrono::duration<double, std::chrono::microseconds::period>;
using floating_point_seconds = std::chrono::duration<double, std::chrono::seconds::period>;

/**
 * This is used to perform a static type cast in a clear, obvious, and
 * bulletproof way. Here, we want a bit-for-bit cast between a Csound MYFLT
 * (usually a double) and a pointer. This will only work if sizeof(MYFLT) >
 * sizeof(void *). That is always the case on 32 bit CPU architecture, and
 * will be the case on 64 bit CPU architecture if MYFLT is double-precision.
 */
typedef union {
    MYFLT myflt;
    ableton::Link *pointer;
} link_cast_t;

// i_link link_create [j_bpm = 60]

class link_create_t : public OpcodeBase<link_create_t>
{
    // Pfields out:
    MYFLT *r0_link;
    // Pfields in:
    MYFLT *p0_bpm;
    // State:
    link_cast_t link;
    double bpm;
public:
    int init(CSOUND *csound) {
        if (*p0_bpm == FL(-1.0)) {
            bpm = 60;
        } else {
            bpm = *p0_bpm;
        }
        link.pointer = new ableton::Link(bpm);
        debug("link_create: bpm: %f link.pointer: %p link.myflt: %g\n", bpm, link.pointer, link.myflt);
        *r0_link = link.myflt;
        return OK;
    }
};

// link_enable i_link [, P_enabled = 1]

class link_enable_t : public OpcodeBase<link_enable_t>
{
    // Pfields out:
    // Pfields in:
    MYFLT *p0_link;
    MYFLT *p1_enabled;
    // State:
    link_cast_t link;
    MYFLT prior_enabled;
public:
    int init(CSOUND *csound) {
        link.myflt = *p0_link;
        link.pointer->enable(*p1_enabled);
        prior_enabled = *p1_enabled;
        debug("link_enable: link.pointer: %p link.myflt: %g p1_enabled: %f isEnabled: %d\n", link.pointer, link.myflt, *p1_enabled, link.pointer->isEnabled());
        return OK;
    }
    int kontrol(CSOUND *csound) {
        if (prior_enabled != *p1_enabled) {
            link.pointer->enable(*p1_enabled);
            debug("link_enable: link.pointer: %p link.myflt: %g p1_enabled: %f isEnabled: %d\n", link.pointer, link.myflt, *p1_enabled, link.pointer->isEnabled());
            prior_enabled = *p1_enabled;
        }
        return OK;
    }
};

// k_is_enabled link_is_enabled i_link

class link_is_enabled_t : public OpcodeBase<link_is_enabled_t>
{
    // Pfields out:
    MYFLT *r0_is_enabled;
    // Pfields in:
    MYFLT *p0_link;
    // State:
    link_cast_t link;
public:
    int init(CSOUND *csound) {
        link.myflt = *p0_link;
        *r0_is_enabled = link.pointer->isEnabled();
        return OK;
    }
    int kontrol(CSOUND *csound) {
        *r0_is_enabled = link.pointer->isEnabled();
        return OK;
    }
};

// link_tempo_set i_link, k_bpm [, P_at_time_seconds = current time]

class link_tempo_set_t : public OpcodeBase<link_tempo_set_t>
{
    // Pfields out:
    // Pfields in:
    MYFLT *p0_link;
    MYFLT *p1_bpm;
    MYFLT *p2_at_time_seconds;
    // State:
    link_cast_t link;
    MYFLT prior_bpm;
    std::chrono::microseconds at_time_microseconds;
public:
    int init(CSOUND *csound) {
        link.myflt = *p0_link;
        ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
        prior_bpm = *p1_bpm;
        if (*p2_at_time_seconds == FL(-1.0)) {
            at_time_microseconds = link.pointer->clock().micros();
        } else {
            at_time_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(floating_point_seconds(*p2_at_time_seconds));
        }
        timeline.setTempo(prior_bpm, at_time_microseconds);
        link.pointer->commitAudioTimeline(timeline);
        return OK;
    }
    int kontrol(CSOUND *csound) {
        if (prior_bpm != *p1_bpm) {
            ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
            prior_bpm = *p1_bpm;
            if (*p2_at_time_seconds == FL(-1.0)) {
                at_time_microseconds = link.pointer->clock().micros();
            } else {
                at_time_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(floating_point_seconds(*p2_at_time_seconds));
            }
            timeline.setTempo(prior_bpm, at_time_microseconds);
            link.pointer->commitAudioTimeline(timeline);
        }
        return OK;
    }
};

// k_bpm link_tempo_get i_link

class link_tempo_get_t : public OpcodeBase<link_tempo_get_t>
{
    // Pfields out:
    MYFLT *r0_bpm;
    // Pfields in:
    MYFLT *p0_link;
    // State:
    link_cast_t link;
public:
    int init(CSOUND *csound) {
        link.myflt = *p0_link;
        ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
        *r0_bpm = timeline.tempo();
        return OK;
    }
    int kontrol(CSOUND *csound) {
        ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
        *r0_bpm = timeline.tempo();
        return OK;
    }
};

// k_beat_number, k_phase, k_current_time_seconds link_beat_get i_link [, P_quantum = 1]

class link_beat_get_t : public OpcodeBase<link_beat_get_t>
{
    // Pfields out:
    MYFLT *r0_beat;
    MYFLT *r1_phase;
    MYFLT *r2_seconds;
    // Pfields in:
    MYFLT *p0_link;
    MYFLT *p1_quantum;
    // State:
    link_cast_t link;
    std::chrono::microseconds at_time_microseconds;
public:
    int init(CSOUND *csound) {
        link.myflt = *p0_link;
        ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
        at_time_microseconds = link.pointer->clock().micros();
        *r0_beat = timeline.beatAtTime(at_time_microseconds, *p1_quantum);
        *r1_phase = timeline.phaseAtTime(at_time_microseconds, *p1_quantum);
        *r2_seconds = std::chrono::duration_cast<floating_point_seconds>(at_time_microseconds).count();
        return OK;
    }
    int kontrol(CSOUND *csound) {
        ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
        at_time_microseconds = link.pointer->clock().micros();
        *r0_beat = timeline.beatAtTime(at_time_microseconds, *p1_quantum);
        *r1_phase = timeline.phaseAtTime(at_time_microseconds, *p1_quantum);
        *r2_seconds = std::chrono::duration_cast<floating_point_seconds>(at_time_microseconds).count();
    }
};

// k_trigger, k_beat, k_phase, k_current_time_seconds link_metro i_link [, P_quantum = 1]

class link_metro_t : public OpcodeBase<link_metro_t>
{
    // Pfields out:
    MYFLT *r0_trigger;
    MYFLT *r1_beat;
    MYFLT *r2_phase;
    MYFLT *r3_seconds;
    // Pfields in:
    MYFLT *p0_link;
    MYFLT *p1_quantum;
    // State:
    link_cast_t link;
    std::chrono::microseconds at_time_microseconds;
    MYFLT prior_phase;
public:
    // The trigger is "on" when the new phase is less than the prior phase, and "off"
    // when the new phase is greater than the prior phase.
    int init(CSOUND *csound) {
        link.myflt = *p0_link;
        debug("link_metro i: link.pointer: %p link.myflt: %g\n", link.pointer, link.myflt);
        ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
        at_time_microseconds = link.pointer->clock().micros();
        *r0_trigger = 0;
        *r1_beat = timeline.beatAtTime(at_time_microseconds, *p1_quantum);
        *r2_phase = timeline.phaseAtTime(at_time_microseconds, *p1_quantum);
        prior_phase = *r2_phase;
        *r3_seconds = std::chrono::duration_cast<floating_point_seconds>(at_time_microseconds).count();
        debug("link_metro i: r0_trigger: %f r1_beat: %f r2_phase: %f r3_seconds: %f\n", *r0_trigger, *r1_beat, *r2_phase, *r3_seconds);
        return OK;
    }
    int kontrol(CSOUND *csound) {
        ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
        at_time_microseconds = link.pointer->clock().micros();
        *r1_beat = timeline.beatAtTime(at_time_microseconds, *p1_quantum);
        *r2_phase = timeline.phaseAtTime(at_time_microseconds, *p1_quantum);
        if (*r2_phase < prior_phase) {
            *r0_trigger = 1;
            debug("link_metro k: r0_trigger: %f r1_beat: %f r2_phase: %f r3_seconds: %f\n", *r0_trigger, *r1_beat, *r2_phase, *r3_seconds);
        } else {
            *r0_trigger = 0;
        }
        prior_phase = *r2_phase;
        *r3_seconds = std::chrono::duration_cast<floating_point_seconds>(at_time_microseconds).count();
        return OK;
    }
};

// link_beat_request i_link k_beat [, J_at_time_seconds = current time [, P_quantum = 1]]

class link_beat_request_t : public OpcodeBase<link_beat_request_t>
{
    // Pfields out:
    // Pfields in:
    MYFLT *p0_link;
    MYFLT *p1_beat;
    MYFLT *p2_at_time_seconds;
    MYFLT *p3_quantum;
    // State:
    link_cast_t link;
    MYFLT prior_beat;
    MYFLT prior_at_time_seconds;
    MYFLT prior_quantum;
    std::chrono::microseconds at_time_microseconds;
public:
    int init(CSOUND *csound) {
        link.myflt = *p0_link;
        prior_beat = *p1_beat;
        prior_at_time_seconds = *p2_at_time_seconds;
        prior_quantum = *p3_quantum;
        if (*p2_at_time_seconds == FL(-1.0)) {
            at_time_microseconds = link.pointer->clock().micros();
        } else {
            at_time_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(floating_point_seconds(*p2_at_time_seconds));
        }
        ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
        timeline.requestBeatAtTime(prior_beat, at_time_microseconds, prior_quantum);
        link.pointer->commitAudioTimeline(timeline);
        return OK;
    }
    int kontrol(CSOUND *csound) {
        if ((prior_beat != *p1_beat) || (prior_at_time_seconds != *p2_at_time_seconds) || (prior_quantum != *p3_quantum)) {
            if (*p2_at_time_seconds == FL(-1.0)) {
                at_time_microseconds = link.pointer->clock().micros();
            } else {
                at_time_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(floating_point_seconds(*p2_at_time_seconds));
            }
            ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
            timeline.requestBeatAtTime(prior_beat, at_time_microseconds, prior_quantum);
            link.pointer->commitAudioTimeline(timeline);
            prior_beat = *p1_beat;
            prior_at_time_seconds = *p2_at_time_seconds;
            prior_quantum = *p3_quantum;
        }
    }
};

// link_beat_force i_link k_beat [, J_at_time_seconds = current time [, P_quantum = 1]]

class link_beat_force_t : public OpcodeBase<link_beat_force_t>
{
    // Pfields out:
    // Pfields in:
    MYFLT *p0_link;
    MYFLT *p1_beat;
    MYFLT *p2_at_time_seconds;
    MYFLT *p3_quantum;
    // State:
    link_cast_t link;
    MYFLT prior_beat;
    MYFLT prior_at_time_seconds;
    MYFLT prior_quantum;
    std::chrono::microseconds at_time_microseconds;
public:
    int init(CSOUND *csound) {
        link.myflt = *p0_link;
        prior_beat = *p1_beat;
        prior_at_time_seconds = *p2_at_time_seconds;
        prior_quantum = *p3_quantum;
        if (*p2_at_time_seconds == FL(-1.0)) {
            at_time_microseconds = link.pointer->clock().micros();
        } else {
            at_time_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(floating_point_seconds(*p2_at_time_seconds));
        }
        ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
        timeline.forceBeatAtTime(prior_beat, at_time_microseconds, prior_quantum);
        link.pointer->commitAudioTimeline(timeline);
        return OK;
    }
    int kontrol(CSOUND *csound) {
        if ((prior_beat != *p1_beat) || (prior_at_time_seconds != *p2_at_time_seconds) || (prior_quantum != *p3_quantum)) {
            if (*p2_at_time_seconds == FL(-1.0)) {
                at_time_microseconds = link.pointer->clock().micros();
            } else {
                at_time_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(floating_point_seconds(*p2_at_time_seconds));
            }
            ableton::Link::Timeline timeline = link.pointer->captureAudioTimeline();
            timeline.forceBeatAtTime(prior_beat, at_time_microseconds, prior_quantum);
            link.pointer->commitAudioTimeline(timeline);
            prior_beat = *p1_beat;
            prior_at_time_seconds = *p2_at_time_seconds;
            prior_quantum = *p3_quantum;
        }
    }
};

// k_count link_peers i_link

class link_peers_t : public OpcodeBase<link_peers_t>
{
    // Pfields out:
    MYFLT *r0_peers;
    // Pfields in:
    MYFLT *p0_link;
    // State:
    link_cast_t link;
public:
    int init(CSOUND *csound) {
        link.myflt = *p0_link;
        *r0_peers = link.pointer->numPeers();
        return OK;
    }
    int kontrol(CSOUND *csound) {
        *r0_peers = link.pointer->numPeers();
        return OK;
    }
};

extern "C" {

    OENTRY oentries[] =
    {
// i_link link_create [j_bpm = 60]
      {
        (char*)"link_create",
        sizeof(link_create_t),
        0,
        1,
        (char*)"i",
        (char*)"j",
        (SUBR) link_create_t::init_,
        0,
        0,
      },
// link_enable i_link [, P_enabled = 1]
      {
        (char*)"link_enable",
        sizeof(link_enable_t),
        0,
        3,
        (char*)"",
        (char*)"iP",
        (SUBR) link_enable_t::init_,
        (SUBR) link_enable_t::kontrol_,
        0,
      },
// k_is_enabled link_is_enabled i_link
      {
        (char*)"link_is_enabled",
        sizeof(link_is_enabled_t),
        0,
        3,
        (char*)"k",
        (char*)"i",
        (SUBR) link_is_enabled_t::init_,
        (SUBR) link_is_enabled_t::kontrol_,
        0,
      },
 // link_tempo_set i_link, k_bpm [, J_at_time_seconds = current time]
     {
        (char*)"link_tempo_set",
        sizeof(link_tempo_set_t),
        0,
        3,
        (char*)"",
        (char*)"ikJ",
        (SUBR) link_tempo_set_t::init_,
        (SUBR) link_tempo_set_t::kontrol_,
        0,
      },
// k_bpm link_tempo_get i_link
      {
        (char*)"link_tempo_get",
        sizeof(link_tempo_set_t),
        0,
        3,
        (char*)"k",
        (char*)"i",
        (SUBR) link_tempo_get_t::init_,
        (SUBR) link_tempo_get_t::kontrol_,
        0,
      },
// k_beat_number, k_phase, k_current_time_seconds link_beat_get i_link [, P_quantum = 1]
      {
        (char*)"link_beat_get",
        sizeof(link_beat_get_t),
        0,
        3,
        (char*)"kkk",
        (char*)"iP",
        (SUBR) link_beat_get_t::init_,
        (SUBR) link_beat_get_t::kontrol_,
        0,
      },
// k_trigger, k_beat, k_phase, k_current_time_seconds link_metro i_link [, P_quantum = 1]
      {
        (char*)"link_metro",
        sizeof(link_metro_t),
        0,
        3,
        (char*)"kkkk",
        (char*)"iP",
        (SUBR) link_metro_t::init_,
        (SUBR) link_metro_t::kontrol_,
        0,
      },
// link_beat_request i_link k_beat [, J_at_time_seconds = current time [, P_quantum = 1]]
      {
        (char*)"link_beat_request",
        sizeof(link_beat_request_t),
        0,
        3,
        (char*)"",
        (char*)"kJP",
        (SUBR) link_beat_request_t::init_,
        (SUBR) link_beat_request_t::kontrol_,
        0,
      },
// link_beat_force i_link k_beat [, J_at_time_seconds = current time [, P_quantum = 1]]
      {
        (char*)"link_beat_force",
        sizeof(link_beat_force_t),
        0,
        3,
        (char*)"",
        (char*)"kJP",
        (SUBR) link_beat_force_t::init_,
        (SUBR) link_beat_force_t::kontrol_,
        0,
      },
// k_count link_peers i_link
      {
        (char*)"link_peers",
        sizeof(link_peers_t),
        0,
        3,
        (char*)"k",
        (char*)"i",
        (SUBR) link_peers_t::init_,
        (SUBR) link_peers_t::kontrol_,
        0,
      },
      {
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
      }
    };

  PUBLIC int csoundModuleCreate(CSOUND *csound)
  {
    csound_ = csound;
    return 0;
  }

  PUBLIC int csoundModuleInit(CSOUND *csound)
  {
    int status = 0;
    for(OENTRY *oentry = &oentries[0]; oentry->opname; oentry++)
      {
        status |= csound->AppendOpcode(csound, oentry->opname,
                                       oentry->dsblksiz, oentry->flags,
                                       oentry->thread,
                                       oentry->outypes, oentry->intypes,
                                       (int (*)(CSOUND*,void*)) oentry->iopadr,
                                       (int (*)(CSOUND*,void*)) oentry->kopadr,
                                       (int (*)(CSOUND*,void*)) oentry->aopadr);
      }
    return status;
  }

  PUBLIC int csoundModuleDestroy(CSOUND *csound)
  {
    return 0;
  }

}
