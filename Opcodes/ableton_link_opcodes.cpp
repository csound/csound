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
#include <cstdint>
#include <type_traits>
#include <byteswap.h>
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
 * The Link tempo is independent of the Csound score tempo. Performances that 
 * need to synchronize the score tempo with the Link tempo may use the tempo 
 * opcode to set the score tempo from the Link tempo; or conversely, set the 
 * Link tempo from the score tempo using the tempoval opcode.
 * 
 * Build for testing with something like: 
 *
 * g++ ableton_link_opcodes.cpp -std=gnu++11 -DLINK_PLATFORM_LINUX=1 -O2 -g -lcsound64 
 -I/home/mkg/link/include -I/home/mkg/link/modules/asio-standalone/asio/include -I../include -I../H -shared -oableton_link_opcodes.dll
 */
 
extern "C" {
    static uint64_t htonll(uint64_t x) { return bswap_64(x); }
}

typedef union {
    MYFLT float;
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
public:
    int init(CSOUND *csound) {
        if (*p0_bpm == FL(-1.0)) {
            bpm = 60;
        } else {
            bpm = *p0_bpm;
        }
        link.pointer = new Link(bpm);
        *r0_link = link.float;
        return OK;
    }
};

// link_enable i_link [, p_enabled = 1]

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
        link = *p0_link;
        link->enable(*p1_enabled);
        prior_enabled = *p1_enabled;
        return OK;
    }
    int kontrol(CSOUND *csound) {
        if (prior_enabled != *p1_enabled) {
            link->enable(*p1_enabled);
            prior_enabled = *p1_enabled;
        }
        return OK;
    }
};

// link_disable i_link

class link_disable_t : public OpcodeBase<link_disable_t> 
{
    // Pfields out:
    // Pfields in:
    MYFLT *p0_link;
    // State:
    link_cast_t link;
public:
    int init(CSOUND *csound) {
        link = p0_link;
        link->disable();
        return OK;
    }
    int kontrol(CSOUND *csound) {
        if (link->isEnabled()) {
            link->disable();
        }
        return OK;
    }
};

// link_tempo_set i_link, k_bpm [, O_at_time_seconds = current time]

class link_tempo_set_t : public OpcodeBase<link_tempo_set_t> 
{
    // Pfields out:
    // Pfields in:
    MYFLT *p0_link;
    MYFLT *p1_bpm;
    MPFLT *p2_at_time_seconds;
    // State:
    link_cast_t link;
    MYFLT prior_bpm;
    MYFLT prior_at_time_seconds;
public:
    int init(CSOUND *csound) {
        link = po_link;
        TODO: Fix (needs clock time). Timeline timeline = link->captureAudioTimeline();
        prior_bpm = *p1_bpm;
        prior_at_time_seconds = *p2_at_time_seconds;
        timeline.setTempo(prior_bpm, prior_at_time_seconds);
        link->commitAudioTimeline(timeline);
        return OK;
    }
    int kontrol(CSOUND *csound) {
        if ((prior_bpm != *p1_bpm) || (prior_at_time_seconds != *p2_at_time_seconds)) {
            Timeline timeline = link->captureAudioTimeline();
            prior_bpm = *p1_bpm;
            prior_at_time_seconds = *p2_at_time_seconds;
            timeline.setTempo(prior_bpm, prior_at_time_seconds);
            link->commitAudioTimeline(timeline);
        }
        return OK;
    }
};

// k_bpm link_tempo_get i_link

class link_tempo_get_t : public OpcodeBase<link_tempo_get_t> 
{
    // Pfields out:
    MYFLT *r1_bpm;
    // Pfields in:
    MYFLT *p0_link;
    // State:
    link_cast_t link;
public:
    int init(CSOUND *csound) {
        link = p0_link;
        Timeline timeline = link->captureAudioTimeline();
        *r0_bpm = timeline.tempo();
        return OK;
    }
    int kontrol(CSOUND *csound) {
        Timeline timeline = link->captureAudioTimeline();
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
    double microseconds;
public:
    int init(CSOUND *csound) {
        link = p0_link;
        Timeline timeline = link->captureAudioTimeline();
        microseconds = link->clock().micros();
        *r0_beat = timeline.beatAtTime(microseconds, *p1_quantum);
        *r1_phase = timeline.phaseAtTime(microseconds, *p1_quantum);
        *r2_seconds = microseconds * 1000000.0;        
        return OK;
    }
    int kontrol(CSOUND *csound) {
        Timeline timeline = link->captureAudioTimeline();
        microseconds = link->clock().micros();
        *r0_beat = timeline.beatAtTime(microseconds, *p1_quantum);
        *r1_phase = timeline.phaseAtTime(microseconds, *p1_quantum);
        *r2_seconds = microseconds * 1000000.0;        
        return OK;
    }
};

// k_trigger, k_beat, k_phase, k_current_time_seconds link_metro i_link [, P_quantum = 1]

class link_metro_t : public OpcodeBase<link_metro_t> 
{
    // Pfields out:
    MYFLT *r0_trigger;
1   MYFLT *r1_beat;
    MYFLT *r2_phase;
    MYFLT *r3_seconds;
    // Pfields in:
    MYFLT *p0_link;
    MYFLT *p1_quantum;
    // State:
    link_cast_t link;
    double microseconds;
    MYFLT prior_phase;
public:
    // The trigger is "on" when the new phase is less than the prior phase, and "off" 
    // when the new phase is greater than the prior phase.
    int init(CSOUND *csound) {
        link = p0_link;
        Timeline timeline = link->captureAudioTimeline();
        microseconds = link->clock().micros();
        *r0_trigger = 0;
        *r1_beat = timeline.beatAtTime(microseconds, *p1_quantum);
        *r2_phase = timeline.phaseAtTime(microseconds, *p1_quantum);
        prior_phase = *r2_phase;
        *r3_seconds = microseconds * 1000000.0;        
        return OK;
    }
    int kontrol(CSOUND *csound) {
        Timeline timeline = link->captureAudioTimeline();
        microseconds = link->clock().micros();
        *r1_beat = timeline.beatAtTime(microseconds, *p1_quantum);
        *r2_phase = timeline.phaseAtTime(microseconds, *p1_quantum);
        if (*r2_phase < prior_phase) {
            *r0_trigger = 1;
        } else {
            *r0_trigger = 0;
        }
        prior_phase = *r2_phase;
        *r3_seconds = microseconds * 1000000.0;        
        return OK;
    }
};

// link_beat_request i_link k_beat, k_at_time_seconds [, P_quantum = 1]

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
public:
    int init(CSOUND *csound) {
        link = p0_link;
        prior_beat = *p1_beat;
        prior_at_time_seconds = *p2_at_time_seconds;
        double milliseconds = prior_at_time_seconds / 1000000.0;
        prior_quantum = *p3_quantum;
        Timeline timeline = link->captureAudioTimeline();
        timeline.requestBeatAtTime(prior_beat, milliseconds, prior_quantum);
        link->commitAudioTimeline(timeline);        
        return OK;
    }
    int kontrol(CSOUND *csound) {
        if ((prior_beat != *p1_beat) || (prior_at_time_seconds != *p2_at_time_seconds) || (prior_quantum != *p3_quantum)) {
            double milliseconds = p2_at_time_seconds / 1000000.0;
            Timeline timeline = link->captureAudioTimeline();
            timeline.requestBeatAtTime(*p1_beat, milliseconds, p3_quantum);
            link->commitAudioTimeline(timeline);        
            prior_beat = *p1_beat;
            prior_at_time_seconds = *p2_at_time_seconds;
            prior_quantum = *p3_quantum;
        }
    }
};

// link_beat_force i_link k_beat, k_at_time_seconds [, P_quantum = 1]

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
public:
    int init(CSOUND *csound) {
        link = p0_link;
        prior_beat = *p1_beat;
        prior_at_time_seconds = *p2_at_time_seconds;
        double milliseconds = prior_at_time_seconds / 1000000.0;
        prior_quantum = *p3_quantum;
        Timeline timeline = link->captureAudioTimeline();
        timeline.forceBeatAtTime(prior_beat, milliseconds, prior_quantum);
        link->commitAudioTimeline(timeline);        
        return OK;
    }
    int kontrol(CSOUND *csound) {
        if ((prior_beat != *p1_beat) || (prior_at_time_seconds != *p2_at_time_seconds) || (prior_quantum != *p3_quantum)) {
            double milliseconds = p2_at_time_seconds / 1000000.0;
            Timeline timeline = link->captureAudioTimeline();
            timeline.forceBeatAtTime(*p1_beat, milliseconds, p3_quantum);
            link->commitAudioTimeline(timeline);        
            prior_beat = *p1_beat;
            prior_at_time_seconds = *p2_at_time_seconds;
            prior_quantum = *p3_quantum;
        }
        return OK;
    }
};

// i_count link_peers i_link

class link_peers_t : public OpcodeBase<link_peers_t> 
{
    // Pfields out:
    MYFLT *r0_peers;
    // Pfields in:
    MYFLT *p1_link;
    // State:
    link_cast_t link;
public:
    int init(CSOUND *csound) {
        link.float = p0_link;
        *r0_peers = link.pointer->numPeers();
        return OK;
    }
};

extern "C" {

    OENTRY oentries[] =
    {
      {
        (char*)"link_create",
        sizeof(link_create_t),
        0,
        1,
        (char*)"",
        (char*)"j",
        (SUBR) link_create_t::init_,
        0,
        0,
      },
      {
        (char*)"link_enable",
        sizeof(link_enable_t),
        0,
        3,
        (char*)"",
        (char*)"ik",
        (SUBR) link_enable_t::init_,
        (SUBR) link_enable_t::kontrol_,
        0,
      },
      {
        (char*)"link_disable",
        sizeof(link_disable_t),
        0,
        3,
        (char*)"",
        (char*)"i",
        (SUBR) link_enable_t::init_,
        (SUBR) link_enable_t::kontrol_,
        0,
      },
      {
        (char*)"link_tempo_set",
        sizeof(link_tempo_set_t),
        0,
        3,
        (char*)"",
        (char*)"ik",
        (SUBR) link_tempo_set_t::init_,
        (SUBR) link_tempo_set_t::kontrol_,
        0,
      },
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
