/*
 *  Copyright (C) 2005 Andres Cabrera
 *
 *  The dssi4cs library is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  The dssi4cs library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with The dssi4cs library; if not, write to the Free Software
 *  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
 *
 * Uses code by Richard W.E. Furse from the ladspa sdk
 */

#include "dssi4cs.h"
#include "utils.h"
#include <dlfcn.h>
#include <dirent.h>
#include <limits.h>
#include <float.h>
#include <math.h>

/* State belongs to one Csound engine, and survives until reset. */
static DSSI_HOST *host(CSOUND *csound)
{
    return csound->QueryGlobalVariable(csound, "$DSSI4CS");
}

static int integer(cs_float value, int64_t maximum)
{
    /* Compare as integers so float builds do not round UINT32_MAX up. */
    return value >= 0 && value < 0x1p63 && floor(value) == value &&
           (int64_t)value <= maximum;
}

static DSSI_PLUGIN *lookup(CSOUND *csound, cs_float id)
{
    DSSI_HOST *h = host(csound);
    return h && h->count && integer(id, h->count - 1) ?
           h->plugins[(unsigned int)id] : NULL;
}

static int64_t block_start(OPDS *p)
{
    return ((int64_t)GetLocalKcounter(p) - 1) * GetLocalKsmps(p);
}

static int64_t event_time(OPDS *p)
{
    return block_start(p) + GetKsmpsOffset(p);
}

static void set_active(DSSI_PLUGIN *p, int active)
{
    if (active == p->active) return;
    if (active) {
        if (p->ladspa->activate) p->ladspa->activate(p->handle);
    }
    else {
        if (p->ladspa->deactivate) p->ladspa->deactivate(p->handle);
        p->queued = 0;
        p->panic = 1;
    }
    p->active = active;
    p->last_block = -1;
}

static void destroy_plugin(CSOUND *csound, DSSI_PLUGIN *p)
{
    if (!p) return;
    if (p->handle) {
        set_active(p, 0);
        p->ladspa->cleanup(p->handle);
    }
    if (p->audio) {
        for (unsigned long i = 0; i < p->ladspa->PortCount; ++i)
            if (p->audio[i]) csound->Free(csound, p->audio[i]);
        csound->Free(csound, p->audio);
    }
    if (p->control) csound->Free(csound, p->control);
    if (p->mapping) csound->Free(csound, p->mapping);
    if (p->inputs) csound->Free(csound, p->inputs);
    if (p->outputs) csound->Free(csound, p->outputs);
    if (p->library) dlclose(p->library);
    csound->Free(csound, p);
}

static int32_t reset(CSOUND *csound, void *data)
{
    DSSI_HOST *h = data;
    for (unsigned int i = 0; i < h->count; ++i)
        destroy_plugin(csound, h->plugins[i]);
    csound->DestroyGlobalVariable(csound, "$DSSI4CS");
    return OK;
}

static cs_double bound(DSSI_PLUGIN *p, unsigned long port, int upper)
{
    const LADSPA_PortRangeHint *hint = &p->ladspa->PortRangeHints[port];
    return (upper ? hint->UpperBound : hint->LowerBound) *
           (LADSPA_IS_HINT_SAMPLE_RATE(hint->HintDescriptor) ? p->sample_rate : 1);
}

static cs_double interpolate(DSSI_PLUGIN *p, unsigned long port, cs_double fraction)
{
    cs_double lo = bound(p, port, 0), hi = bound(p, port, 1);
    LADSPA_PortRangeHintDescriptor flags = p->ladspa->PortRangeHints[port].HintDescriptor;
    if (LADSPA_IS_HINT_LOGARITHMIC(flags) && lo > 0 && hi > 0)
        return exp(log(lo) * (1 - fraction) + log(hi) * fraction);
    return lo * (1 - fraction) + hi * fraction;
}

static LADSPA_Data default_control(DSSI_PLUGIN *p, unsigned long port)
{
    LADSPA_PortRangeHintDescriptor flags = p->ladspa->PortRangeHints[port].HintDescriptor;
    cs_double value = 0;
    switch (flags & LADSPA_HINT_DEFAULT_MASK) {
    case LADSPA_HINT_DEFAULT_MINIMUM: value = bound(p, port, 0); break;
    case LADSPA_HINT_DEFAULT_LOW: value = interpolate(p, port, .25); break;
    case LADSPA_HINT_DEFAULT_MIDDLE: value = interpolate(p, port, .5); break;
    case LADSPA_HINT_DEFAULT_HIGH: value = interpolate(p, port, .75); break;
    case LADSPA_HINT_DEFAULT_MAXIMUM: value = bound(p, port, 1); break;
    case LADSPA_HINT_DEFAULT_1: value = 1; break;
    case LADSPA_HINT_DEFAULT_100: value = 100; break;
    case LADSPA_HINT_DEFAULT_440: value = 440; break;
    default:
        if (LADSPA_IS_HINT_BOUNDED_BELOW(flags)) value = fmax(value, bound(p, port, 0));
        if (LADSPA_IS_HINT_BOUNDED_ABOVE(flags)) value = fmin(value, bound(p, port, 1));
    }
    if (LADSPA_IS_HINT_INTEGER(flags)) value = round(value);
    return (LADSPA_Data)value;
}

static void describe(CSOUND *csound, DSSI_PLUGIN *p)
{
    csound->Message(csound, Str("DSSI4CS: %s: %s (%s), %lu audio inputs, %lu outputs\n"),
                    p->dssi ? "DSSI" : "LADSPA", p->ladspa->Name,
                    p->ladspa->Label, p->input_count, p->output_count);
    for (unsigned long i = 0; i < p->ladspa->PortCount; ++i) {
        LADSPA_PortDescriptor flags = p->ladspa->PortDescriptors[i];
        csound->Message(csound, "  %lu: %s, %s %s", i, p->ladspa->PortNames[i],
                        LADSPA_IS_PORT_INPUT(flags) ? "input" : "output",
                        LADSPA_IS_PORT_AUDIO(flags) ? "audio" : "control");
        if (LADSPA_IS_PORT_CONTROL(flags))
            csound->Message(csound, Str(", value %g, MIDI mapping %d"),
                            (cs_double)p->control[i], p->mapping[i]);
        csound->Message(csound, "\n");
    }
}

static int32_t dssiinit(CSOUND *csound, DSSIINIT *op)
{
    *op->result = -1;
    if (!integer(*op->index, UINT32_MAX))
        return csound->InitError(csound, Str("DSSI4CS: invalid plugin index"));
    if (csound->GetOParms(csound)->numThreads > 1)
        return csound->InitError(csound, Str("DSSI4CS: use one Csound performance thread"));
    DSSI_HOST *h = host(csound);
    if (!h) {
        if (csound->CreateGlobalVariable(csound, "$DSSI4CS", sizeof(DSSI_HOST)))
            return csound->InitError(csound, Str("DSSI4CS: cannot create host state"));
        h = host(csound);
        csound->RegisterResetCallback(csound, h, reset);
    }
    if (h->count == DSSI4CS_INSTANCES)
        return csound->InitError(csound, Str("DSSI4CS: instance limit reached"));
    char legacy_name[MAXNAME];
    const char *name;
    if (!strcmp(GetTypeForArg(op->filename)->varTypeName, "S"))
        name = ((STRINGDAT *)op->filename)->data;
    else {
        csound->StringArg2Name(csound, legacy_name,
            IsStringCode(*op->filename) ? csound->GetArgString(csound, *op->filename) :
            (char *)op->filename, "dssiinit.", IsStringCode(*op->filename));
        name = legacy_name;
    }
    DSSI_PLUGIN *p = csound->Calloc(csound, sizeof(*p));
    p->last_block = p->rendered_until = -1;
    p->capacity = GetLocalKsmps(&op->h);
    p->sample_rate = csound->GetEngineSr(csound);
    p->library = dlopenLADSPA(csound, name, RTLD_NOW);
    const char *error = Str_noop("cannot load library");
    if (!p->library) goto fail;
    DSSI_Descriptor_Function dssi = (DSSI_Descriptor_Function)dlsym(p->library, "dssi_descriptor");
    LADSPA_Descriptor_Function ladspa = (LADSPA_Descriptor_Function)dlsym(p->library, "ladspa_descriptor");
    error = Str_noop("missing descriptor or plugin index");
    if (dssi) {
        p->dssi = dssi((unsigned long)*op->index);
        if (!p->dssi) goto fail;
        error = Str_noop("unsupported DSSI API version");
        if (p->dssi->DSSI_API_Version != 1) goto fail;
        p->ladspa = p->dssi->LADSPA_Plugin;
    }
    else if (ladspa) p->ladspa = ladspa((unsigned long)*op->index);
    error = Str_noop("invalid LADSPA descriptor");
    const LADSPA_Descriptor *d = p->ladspa;
    if (!d || !d->instantiate || !d->connect_port || !d->cleanup ||
        !d->Label || !d->Name || d->PortCount > 65536 ||
        (d->PortCount && (!d->PortDescriptors || !d->PortNames || !d->PortRangeHints))) goto fail;
    error = Str_noop("plugin has no render callback");
    if (!d->run && !(p->dssi && (p->dssi->run_synth || p->dssi->run_multiple_synths))) goto fail;
    error = Str_noop("invalid port descriptor");
    for (unsigned long i = 0; i < d->PortCount; ++i) {
        LADSPA_PortDescriptor flags = d->PortDescriptors[i];
        if (!!LADSPA_IS_PORT_AUDIO(flags) == !!LADSPA_IS_PORT_CONTROL(flags) ||
            !!LADSPA_IS_PORT_INPUT(flags) == !!LADSPA_IS_PORT_OUTPUT(flags) ||
            !d->PortNames[i]) goto fail;
    }
    p->handle = d->instantiate(d, (unsigned long)p->sample_rate);
    error = Str_noop("cannot instantiate plugin");
    if (!p->handle) goto fail;
    size_t ports = d->PortCount ? d->PortCount : 1;
    p->audio = csound->Calloc(csound, ports * sizeof(*p->audio));
    p->control = csound->Calloc(csound, ports * sizeof(*p->control));
    p->mapping = csound->Malloc(csound, ports * sizeof(*p->mapping));
    p->inputs = csound->Malloc(csound, ports * sizeof(*p->inputs));
    p->outputs = csound->Malloc(csound, ports * sizeof(*p->outputs));
    for (unsigned long i = 0; i < d->PortCount; ++i) {
        LADSPA_PortDescriptor flags = d->PortDescriptors[i];
        p->mapping[i] = DSSI_NONE;
        if (LADSPA_IS_PORT_AUDIO(flags)) {
            p->audio[i] = csound->Calloc(csound, p->capacity * sizeof(LADSPA_Data));
            d->connect_port(p->handle, i, p->audio[i]);
            if (LADSPA_IS_PORT_INPUT(flags)) p->inputs[p->input_count++] = i;
            else p->outputs[p->output_count++] = i;
        }
        else {
            if (LADSPA_IS_PORT_INPUT(flags)) {
                p->control[i] = default_control(p, i);
                if (p->dssi && p->dssi->get_midi_controller_for_port)
                    p->mapping[i] = p->dssi->get_midi_controller_for_port(p->handle, i);
            }
            d->connect_port(p->handle, i, &p->control[i]);
        }
    }
    h->plugins[h->count] = p;
    *op->result = h->count++;
    if (*op->verbose != 0) describe(csound, p);
    return OK;
fail:
    destroy_plugin(csound, p);
    return csound->InitError(csound, "DSSI4CS: %s: %s", name, Str(error));
}

static int32_t dssiactivate_init(CSOUND *csound, DSSIACTIVATE *p)
{
    p->plugin = lookup(csound, *p->id);
    return p->plugin ? OK : csound->InitError(csound, Str("DSSI4CS: invalid handle"));
}

static int same_plugin(DSSI_PLUGIN *, DSSI_PLUGIN *);

static int32_t dssiactivate(CSOUND *csound, DSSIACTIVATE *p)
{
    if (!integer(*p->trigger, 1))
        return csound->PerfError(csound, &p->h, Str("DSSI4CS: activation must be 0 or 1"));
    DSSI_PLUGIN *q = p->plugin;
    if (*p->trigger && !q->active && q->dssi &&
        !q->dssi->run_synth && q->dssi->run_multiple_synths) {
        DSSI_HOST *h = host(csound);
        for (unsigned int i = 0; i < h->count; ++i)
            if (same_plugin(q, h->plugins[i]) &&
                h->plugins[i]->rendered_until > block_start(&p->h))
                return csound->PerfError(csound, &p->h,
                    Str("DSSI4CS: activate all grouped instances before rendering their block"));
    }
    set_active(q, *p->trigger != 0);
    return OK;
}

static int has_synth(DSSI_PLUGIN *p)
{
    return p && p->dssi && (p->dssi->run_synth || p->dssi->run_multiple_synths);
}

static int32_t synth_handle(CSOUND *csound, cs_float id, DSSI_PLUGIN **p)
{
    *p = lookup(csound, id);
    return has_synth(*p) ? OK : csound->InitError(csound, Str("DSSI4CS: handle is not a DSSI synth"));
}

/* Stable insertion preserves note-on/note-off order at the same sample. */
static void enqueue(DSSI_PLUGIN *p, int64_t time, snd_seq_event_t event)
{
    unsigned int i = p->queued++;
    while (i && p->queue[i-1].time > time) {
        p->queue[i] = p->queue[i-1];
        --i;
    }
    p->queue[i].time = time;
    p->queue[i].event = event;
}

static int32_t reserve_events(CSOUND *csound, OPDS *op, DSSI_PLUGIN *p,
                              unsigned int count, int64_t time)
{
    if (!p->active)
        return csound->PerfError(csound, op, Str("DSSI4CS: activate the synth before sending events"));
    if (time < p->rendered_until)
        return csound->PerfError(csound, op, Str("DSSI4CS: send events before rendering their block"));
    if (count > DSSI4CS_EVENTS - p->queued)
        return csound->PerfError(csound, op, Str("DSSI4CS: event queue is full"));
    return OK;
}

static int32_t dssinote_init(CSOUND *csound, DSSINOTE *p)
{
    return synth_handle(csound, *p->id, &p->plugin);
}

static int32_t dssinote(CSOUND *csound, DSSINOTE *p)
{
    if (*p->trigger == 0) return OK;
    int64_t now = event_time(&p->h);
    cs_double samples = (cs_double)*p->duration * p->plugin->sample_rate;
    if (!integer(*p->note, 127) || !integer(*p->velocity, 127) ||
        !integer(*p->channel, 15) || !isfinite(samples) || samples < 0 ||
        samples > (cs_double)(INT64_MAX / 2))
        return csound->PerfError(csound, &p->h, Str("DSSI4CS: invalid note, velocity, channel or duration"));
    if (reserve_events(csound, &p->h, p->plugin, *p->velocity ? 2 : 1, now)) return NOTOK;
    snd_seq_event_t event = {0};
    event.type = *p->velocity ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
    event.data.note.channel = (unsigned char)*p->channel;
    event.data.note.note = (unsigned char)*p->note;
    event.data.note.velocity = (unsigned char)*p->velocity;
    enqueue(p->plugin, now, event);
    if (*p->velocity) {
        event.type = SND_SEQ_EVENT_NOTEOFF;
        event.data.note.velocity = 0;
        enqueue(p->plugin, now + (int64_t)llround(samples), event);
    }
    return OK;
}

static int32_t dssinoteon_init(CSOUND *csound, DSSINOTEON *p)
{
    return synth_handle(csound, *p->id, &p->plugin);
}

static int32_t dssinoteon(CSOUND *csound, DSSINOTEON *p)
{
    if (*p->trigger == 0) return OK;
    if (!integer(*p->note, 127) || !integer(*p->velocity, 127))
        return csound->PerfError(csound, &p->h, Str("DSSI4CS: invalid note or velocity"));
    int64_t time = event_time(&p->h);
    if (reserve_events(csound, &p->h, p->plugin, 1, time)) return NOTOK;
    snd_seq_event_t event = {0};
    event.type = *p->velocity ? SND_SEQ_EVENT_NOTEON : SND_SEQ_EVENT_NOTEOFF;
    event.data.note.note = (unsigned char)*p->note;
    event.data.note.velocity = (unsigned char)*p->velocity;
    enqueue(p->plugin, time, event);
    return OK;
}

static int32_t dssievent_init(CSOUND *csound, DSSIEVENT *p)
{
    return synth_handle(csound, *p->id, &p->plugin);
}

static int32_t dssievent(CSOUND *csound, DSSIEVENT *p)
{
    if (*p->trigger == 0) return OK;
    if (!integer(*p->channel, 15) || !integer(*p->status, 0xe0) ||
        !integer(*p->data1, 127) || !integer(*p->data2, 127) ||
        !integer(*p->offset, (int64_t)GetLocalKsmps(&p->h) - GetKsmpsOffset(&p->h) -
                                    GetEarlySmps(&p->h) - 1))
        return csound->PerfError(csound, &p->h, Str("DSSI4CS: invalid MIDI event or sample offset"));
    snd_seq_event_t event = {0};
    int status = (int)*p->status, channel = (int)*p->channel;
    int a = (int)*p->data1, b = (int)*p->data2;
    if (status == 0x80 || status == 0x90 || status == 0xa0) {
        event.type = status == 0xa0 ? SND_SEQ_EVENT_KEYPRESS :
                     status == 0x80 || b == 0 ? SND_SEQ_EVENT_NOTEOFF : SND_SEQ_EVENT_NOTEON;
        event.data.note.channel = channel;
        event.data.note.note = a;
        event.data.note.velocity = b;
    }
    else {
        event.data.control.channel = channel;
        switch (status) {
        case 0xb0: event.type = SND_SEQ_EVENT_CONTROLLER; event.data.control.param = a;
                   event.data.control.value = b; break;
        case 0xc0: event.type = SND_SEQ_EVENT_PGMCHANGE; event.data.control.value = a; break;
        case 0xd0: event.type = SND_SEQ_EVENT_CHANPRESS; event.data.control.value = a; break;
        case 0xe0: event.type = SND_SEQ_EVENT_PITCHBEND; event.data.control.value = a + 128*b - 8192; break;
        default: return csound->PerfError(csound, &p->h, Str("DSSI4CS: unsupported MIDI status"));
        }
    }
    int64_t time = event_time(&p->h) + (int64_t)*p->offset;
    if (reserve_events(csound, &p->h, p->plugin, 1, time)) return NOTOK;
    enqueue(p->plugin, time, event);
    return OK;
}

static int32_t dssinrpn_init(CSOUND *csound, DSSINRPN *p)
{
    return synth_handle(csound, *p->id, &p->plugin);
}

static int32_t dssinrpn(CSOUND *csound, DSSINRPN *p)
{
    if (*p->trigger == 0) return OK;
    if (!integer(*p->channel, 15) || !integer(*p->parameter, 16383) ||
        !integer(*p->value, 16383) ||
        !integer(*p->offset, (int64_t)GetLocalKsmps(&p->h) - GetKsmpsOffset(&p->h) -
                            GetEarlySmps(&p->h) - 1))
        return csound->PerfError(csound, &p->h, Str("DSSI4CS: invalid NRPN event"));
    int64_t time = event_time(&p->h) + (int64_t)*p->offset;
    if (reserve_events(csound, &p->h, p->plugin, 1, time)) return NOTOK;
    snd_seq_event_t event = {0};
    event.type = SND_SEQ_EVENT_NONREGPARAM;
    event.data.control.channel = (unsigned char)*p->channel;
    event.data.control.param = (unsigned int)*p->parameter;
    event.data.control.value = (int)*p->value;
    enqueue(p->plugin, time, event);
    return OK;
}

static int mapped(DSSI_PLUGIN *p, snd_seq_event_t *e, int apply)
{
    if (e->type != SND_SEQ_EVENT_CONTROLLER && e->type != SND_SEQ_EVENT_NONREGPARAM) return 0;
    int found = 0;
    for (unsigned long i = 0; i < p->ladspa->PortCount; ++i) {
        int m = p->mapping[i];
        if (m == DSSI_NONE) continue;
        int match = e->type == SND_SEQ_EVENT_CONTROLLER ?
            (DSSI_IS_CC(m) && DSSI_CC_NUMBER(m) == e->data.control.param) :
            (DSSI_IS_NRPN(m) && DSSI_NRPN_NUMBER(m) == e->data.control.param);
        if (!match) continue;
        found = 1;
        if (apply) {
            cs_double f = e->data.control.value /
                       (e->type == SND_SEQ_EVENT_CONTROLLER ? 127. : 16383.);
            LADSPA_PortRangeHintDescriptor flags = p->ladspa->PortRangeHints[i].HintDescriptor;
            cs_double value = interpolate(p, i, f);
            if (LADSPA_IS_HINT_TOGGLED(flags)) value = f >= .5;
            else if (LADSPA_IS_HINT_INTEGER(flags)) value = round(value);
            p->control[i] = (LADSPA_Data)value;
        }
    }
    return found;
}

static int host_event(DSSI_PLUGIN *p, snd_seq_event_t *e)
{
    return e->type == SND_SEQ_EVENT_PGMCHANGE ||
           (e->type == SND_SEQ_EVENT_CONTROLLER &&
            (e->data.control.param == 0 || e->data.control.param == 32)) || mapped(p, e, 0);
}

static void apply_event(DSSI_PLUGIN *p, snd_seq_event_t *e)
{
    unsigned int ch = e->data.control.channel;
    if (e->type == SND_SEQ_EVENT_PGMCHANGE) {
        if (p->dssi->select_program)
            p->dssi->select_program(p->handle, p->bank[ch], e->data.control.value);
    }
    else if (e->type == SND_SEQ_EVENT_CONTROLLER && e->data.control.param == 0)
        p->bank[ch] = (p->bank[ch] & 127) | (e->data.control.value << 7);
    else if (e->type == SND_SEQ_EVENT_CONTROLLER && e->data.control.param == 32)
        p->bank[ch] = (p->bank[ch] & (127 << 7)) | e->data.control.value;
    else mapped(p, e, 1);
}

static int same_plugin(DSSI_PLUGIN *a, DSSI_PLUGIN *b)
{
    return a->library == b->library && !strcmp(a->ladspa->Label, b->ladspa->Label);
}

static void connect_audio(DSSI_PLUGIN *p, uint32_t offset)
{
    for (unsigned long i = 0; i < p->ladspa->PortCount; ++i)
        if (p->audio[i]) p->ladspa->connect_port(p->handle, i, p->audio[i] + offset);
}

/* Split only at host-handled MIDI events, so controls/programs change at the
   requested sample. Other MIDI retains its relative timestamp within a run. */
static void render(DSSI_PLUGIN **group, unsigned long count, int multiple,
                   int64_t start, uint32_t frames, uint32_t offset)
{
    int64_t cursor = start, end = start + frames;
    LADSPA_Handle handles[DSSI4CS_INSTANCES];
    snd_seq_event_t *events[DSSI4CS_INSTANCES];
    unsigned long counts[DSSI4CS_INSTANCES];
    while (cursor < end) {
        int64_t next = end;
        for (unsigned long g = 0; g < count; ++g) {
            DSSI_PLUGIN *p = group[g];
            for (unsigned int i = 0; i < p->queued; ++i)
                if (p->queue[i].time > cursor && p->queue[i].time < next &&
                    host_event(p, &p->queue[i].event)) next = p->queue[i].time;
        }
        for (unsigned long g = 0; g < count; ++g) {
            DSSI_PLUGIN *p = group[g];
            unsigned int used = 0;
            p->event_count = 0;
            if (p->panic) {
                /* A plugin may omit activate/deactivate. End old notes before
                   delivering new notes when that instance resumes. */
                for (int channel = 0; channel < 16; ++channel) {
                    const unsigned int controllers[] = {64, 120, 123};
                    for (unsigned int i = 0; i < 3; ++i) {
                        snd_seq_event_t event = {0};
                        event.type = SND_SEQ_EVENT_CONTROLLER;
                        event.data.control.channel = channel;
                        event.data.control.param = controllers[i];
                        if (!mapped(p, &event, 1)) p->events[p->event_count++] = event;
                    }
                }
                for (int channel = 0; channel < 16; ++channel)
                    for (int note = 0; note < 128; ++note)
                        if (p->notes[channel][note]) {
                            snd_seq_event_t *e = &p->events[p->event_count++];
                            memset(e, 0, sizeof(*e));
                            e->type = SND_SEQ_EVENT_NOTEOFF;
                            e->data.note.channel = channel;
                            e->data.note.note = note;
                        }
                memset(p->notes, 0, sizeof(p->notes));
                p->panic = 0;
            }
            while (used < p->queued && p->queue[used].time < next) {
                DSSI_EVENT *q = &p->queue[used++];
                if (host_event(p, &q->event)) apply_event(p, &q->event);
                else {
                    snd_seq_event_t *e = &p->events[p->event_count++];
                    *e = q->event;
                    if (e->type == SND_SEQ_EVENT_NOTEON || e->type == SND_SEQ_EVENT_NOTEOFF)
                        p->notes[e->data.note.channel][e->data.note.note] = e->type == SND_SEQ_EVENT_NOTEON;
                    e->time.tick = q->time > cursor ? (unsigned int)(q->time - cursor) : 0;
                }
            }
            p->queued -= used;
            memmove(p->queue, p->queue + used, p->queued * sizeof(*p->queue));
            connect_audio(p, offset + (uint32_t)(cursor - start));
            handles[g] = p->handle;
            events[g] = p->events;
            counts[g] = p->event_count;
        }
        DSSI_PLUGIN *p = group[0];
        if (multiple)
            p->dssi->run_multiple_synths(count, handles, (unsigned long)(next-cursor), events, counts);
        else if (has_synth(p))
            p->dssi->run_synth(p->handle, (unsigned long)(next-cursor), events[0], counts[0]);
        else p->ladspa->run(p->handle, (unsigned long)(next-cursor));
        cursor = next;
    }
    for (unsigned long g = 0; g < count; ++g) {
        group[g]->rendered_until = end;
        connect_audio(group[g], 0);
    }
}

static int32_t dssiaudio_init(CSOUND *csound, DSSIAUDIO *p)
{
    p->plugin = lookup(csound, *p->id);
    DSSI_PLUGIN *q = p->plugin;
    if (!q) return csound->InitError(csound, Str("DSSI4CS: invalid handle"));
    if (q->owner && q->owner != &p->h)
        return csound->InitError(csound, Str("DSSI4CS: each instance needs exactly one audio renderer"));
    if (GetLocalKsmps(&p->h) > q->capacity ||
        GetInputArgCnt(&p->h) - 1 > q->input_count || GetOutputArgCnt(&p->h) > q->output_count)
        return csound->InitError(csound, Str("DSSI4CS: audio port count or ksmps mismatch"));
    q->render_ksmps = GetLocalKsmps(&p->h);
    q->owner = &p->h;
    return OK;
}

static int32_t dssisynth_init(CSOUND *csound, DSSIAUDIO *p)
{
    if (synth_handle(csound, *p->id, &p->plugin)) return NOTOK;
    return dssiaudio_init(csound, p);
}

static int32_t dssiaudio_deinit(CSOUND *csound, DSSIAUDIO *p)
{
    (void)csound;
    if (p->plugin && p->plugin->owner == &p->h) {
        p->plugin->owner = NULL;
        set_active(p->plugin, 0);
    }
    return OK;
}

static int32_t dssiaudio(CSOUND *csound, DSSIAUDIO *p)
{
    DSSI_PLUGIN *q = p->plugin;
    uint32_t size = GetLocalKsmps(&p->h), offset = GetKsmpsOffset(&p->h);
    uint32_t end = size - GetEarlySmps(&p->h);
    int inputs = GetInputArgCnt(&p->h) - 1, outputs = GetOutputArgCnt(&p->h);
    for (int i = 0; i < outputs; ++i) memset(p->out[i], 0, size * sizeof(cs_float));
    if (!q->active || offset >= end) return OK;
    int64_t block = block_start(&p->h);
    if (q->last_block != block) {
        if (block + offset < q->rendered_until)
            return csound->PerfError(csound, &p->h, Str("DSSI4CS: these samples have already rendered"));
        for (unsigned long i = 0; i < q->input_count; ++i) {
            LADSPA_Data *data = q->audio[q->inputs[i]];
            memset(data, 0, size * sizeof(*data));
            if (i < (unsigned long)inputs)
                for (uint32_t n = offset; n < end; ++n)
                    data[n] = (LADSPA_Data)(p->in[i][n] / csound->Get0dBFS(csound));
        }
        DSSI_PLUGIN *group[DSSI4CS_INSTANCES] = {q};
        unsigned long count = 1;
        int multiple = has_synth(q) && !q->dssi->run_synth;
        if (multiple) {
            DSSI_HOST *h = host(csound);
            for (unsigned int i = 0; i < h->count; ++i) {
                DSSI_PLUGIN *other = h->plugins[i];
                if (other != q && other->active && same_plugin(q, other)) {
                    if (other->rendered_until > block + offset)
                        return csound->PerfError(csound, &p->h,
                            Str("DSSI4CS: activate all grouped instances before rendering their block"));
                    group[count++] = other;
                }
            }
            if (count > 1) {
                for (unsigned long i = 0; i < count; ++i)
                    if (group[i]->input_count || group[i]->render_ksmps != size ||
                        !group[i]->owner || GetKsmpsOffset(group[i]->owner) ||
                        GetEarlySmps(group[i]->owner) || offset || end != size)
                        return csound->PerfError(csound, &p->h,
                            Str("DSSI4CS: grouped synths need no audio inputs, matching ksmps and full blocks"));
            }
        }
        render(group, count, multiple, block + offset, end - offset, offset);
        for (unsigned long i = 0; i < count; ++i) group[i]->last_block = block;
    }
    for (int i = 0; i < outputs; ++i)
        for (uint32_t n = offset; n < end; ++n)
            p->out[i][n] = q->audio[q->outputs[i]][n] * csound->Get0dBFS(csound);
    return OK;
}

static int control_port(DSSI_PLUGIN *p, cs_float port, int input)
{
    if (!p || !p->ladspa->PortCount ||
        !integer(port, (int64_t)(p->ladspa->PortCount - 1))) return 0;
    LADSPA_PortDescriptor flags = p->ladspa->PortDescriptors[(unsigned long)port];
    return LADSPA_IS_PORT_CONTROL(flags) && (!input || LADSPA_IS_PORT_INPUT(flags));
}

static int32_t dssictls_init(CSOUND *csound, DSSICTLS *p)
{
    p->plugin = lookup(csound, *p->id);
    if (!control_port(p->plugin, *p->port, 1))
        return csound->InitError(csound, Str("DSSI4CS: expected an input control port"));
    p->index = (unsigned long)*p->port;
    return OK;
}

static int32_t dssictls(CSOUND *csound, DSSICTLS *p)
{
    if (*p->trigger == 0) return OK;
    cs_double value = *p->value;
    if (LADSPA_IS_HINT_SAMPLE_RATE(p->plugin->ladspa->PortRangeHints[p->index].HintDescriptor))
        value *= p->plugin->sample_rate;
    if (!isfinite(value) || fabs(value) > FLT_MAX)
        return csound->PerfError(csound, &p->h, Str("DSSI4CS: invalid control value"));
    p->plugin->control[p->index] = (LADSPA_Data)value;
    return OK;
}

static int32_t dssiget_init(CSOUND *csound, DSSIGET *p)
{
    p->plugin = lookup(csound, *p->id);
    if (!control_port(p->plugin, *p->port, 0))
        return csound->InitError(csound, Str("DSSI4CS: expected a control port"));
    p->index = (unsigned long)*p->port;
    *p->value = p->plugin->control[p->index];
    return OK;
}

static int32_t dssiget(CSOUND *csound, DSSIGET *p)
{
    (void)csound;
    *p->value = p->plugin->control[p->index];
    return OK;
}

static int32_t dssiprogram_init(CSOUND *csound, DSSIPROGRAM *p)
{
    p->plugin = lookup(csound, *p->id);
    if (!p->plugin || !p->plugin->dssi || !p->plugin->dssi->select_program)
        return csound->InitError(csound, Str("DSSI4CS: plugin does not support programs"));
    return OK;
}

static int32_t dssiprogram(CSOUND *csound, DSSIPROGRAM *p)
{
    if (*p->trigger == 0) return OK;
    if (!integer(*p->bank, UINT32_MAX) || !integer(*p->program, UINT32_MAX))
        return csound->PerfError(csound, &p->h, Str("DSSI4CS: invalid bank or program"));
    p->plugin->dssi->select_program(p->plugin->handle, (unsigned long)*p->bank,
                                   (unsigned long)*p->program);
    return OK;
}

static int32_t dssiconfigure(CSOUND *csound, DSSICONFIGURE *p)
{
    DSSI_PLUGIN *q = lookup(csound, *p->id);
    if (!q || !q->dssi || !q->dssi->configure)
        return csound->InitError(csound, Str("DSSI4CS: plugin does not support configuration"));
    DSSI_HOST *h = host(csound);
    int global = !strncmp(p->key->data, DSSI_GLOBAL_CONFIGURE_PREFIX,
                          strlen(DSSI_GLOBAL_CONFIGURE_PREFIX));
    for (unsigned int i = 0; i < h->count; ++i) {
        DSSI_PLUGIN *target = h->plugins[i];
        if (target != q && !(global && same_plugin(q, target))) continue;
        char *error = target->dssi->configure(target->handle, p->key->data, p->value->data);
        if (error) {
            int32_t result = csound->InitError(csound, "DSSI4CS: configure: %s", error);
            free(error); /* DSSI specifies malloc/free ownership for this string. */
            return result;
        }
    }
    return OK;
}

static int32_t dssiprograminfo(CSOUND *csound, DSSIPROGRAMINFO *p)
{
    DSSI_PLUGIN *q = lookup(csound, *p->id);
    if (!q || !q->dssi || !q->dssi->get_program || !integer(*p->index, UINT32_MAX))
        return csound->InitError(csound, Str("DSSI4CS: cannot enumerate programs"));
    const DSSI_Program_Descriptor *program = q->dssi->get_program(q->handle, (unsigned long)*p->index);
    const char *name = program && program->Name ? program->Name : "";
    size_t size = strlen(name) + 1;
    if (p->name->size < size) {
        p->name->data = csound->ReAlloc(csound, p->name->data, size);
        p->name->size = size;
    }
    memcpy(p->name->data, name, size);
    *p->bank = program ? (cs_float)program->Bank : -1;
    *p->program = program ? (cs_float)program->Program : -1;
    return OK;
}

static int32_t dssiinfo(CSOUND *csound, DSSIINFO *p)
{
    DSSI_PLUGIN *q = lookup(csound, *p->id);
    if (!q) return csound->InitError(csound, Str("DSSI4CS: invalid handle"));
    describe(csound, q);
    return OK;
}

static void list_directory(CSOUND *csound, const char *directory)
{
    DIR *dir = opendir(directory);
    if (!dir) return;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        size_t size = strlen(directory) + strlen(entry->d_name) + 2;
        char *path = csound->Malloc(csound, size);
        snprintf(path, size, "%s/%s", directory, entry->d_name);
        void *library = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
        if (library) {
            DSSI_Descriptor_Function df = (DSSI_Descriptor_Function)dlsym(library, "dssi_descriptor");
            LADSPA_Descriptor_Function lf = (LADSPA_Descriptor_Function)dlsym(library, "ladspa_descriptor");
            for (unsigned long i = 0; df || lf; ++i) {
                const DSSI_Descriptor *d = df ? df(i) : NULL;
                const LADSPA_Descriptor *l = df ? (d ? d->LADSPA_Plugin : NULL) : lf(i);
                if (!l) break;
                csound->Message(csound, "%s: %lu: %s (%s)\n", path, i, l->Name, l->Label);
            }
            dlclose(library);
        }
        csound->Free(csound, path);
    }
    closedir(dir);
}

static int32_t dssilist(CSOUND *csound, DSSILIST *p)
{
    (void)p;
    const char *paths[] = {csound->GetEnv(csound, "DSSI_PATH"),
                          csound->GetEnv(csound, "LADSPA_PATH"), DSSI4CS_DEFAULT_PATH};
    for (unsigned int i = 0; i < sizeof(paths)/sizeof(paths[0]); ++i) {
        const char *start = paths[i];
        while (start && *start) {
            const char *end = strchr(start, ':');
            size_t length = end ? (size_t)(end-start) : strlen(start);
            if (length) {
                char *path = csound->Malloc(csound, length+1);
                memcpy(path, start, length);
                path[length] = '\0';
                list_directory(csound, path);
                csound->Free(csound, path);
            }
            start = end ? end+1 : NULL;
        }
    }
    return OK;
}

static OENTRY dssi_localops[] = {
    {"dssiinit", sizeof(DSSIINIT), 0, "i", "Tip", (SUBR)dssiinit},
    {"dssiactivate", sizeof(DSSIACTIVATE), 0, "", "ik", (SUBR)dssiactivate_init, (SUBR)dssiactivate},
    {"dssiaudio", sizeof(DSSIAUDIO), 0, "mmmmmmmmm", "iMMMMMMMMM", (SUBR)dssiaudio_init, (SUBR)dssiaudio, (SUBR)dssiaudio_deinit},
    {"dssisynth", sizeof(DSSIAUDIO), 0, "mmmmmmmmm", "iMMMMMMMMM", (SUBR)dssisynth_init, (SUBR)dssiaudio, (SUBR)dssiaudio_deinit},
    {"dssictls", sizeof(DSSICTLS), 0, "", "iikk", (SUBR)dssictls_init, (SUBR)dssictls},
    {"dssiget", sizeof(DSSIGET), 0, "k", "ii", (SUBR)dssiget_init, (SUBR)dssiget},
    {"dssinote", sizeof(DSSINOTE), 0, "", "kikkko", (SUBR)dssinote_init, (SUBR)dssinote},
    {"dssievent", sizeof(DSSINOTEON), 0, "", "kikk", (SUBR)dssinoteon_init, (SUBR)dssinoteon},
    {"dssievent", sizeof(DSSIEVENT), 0, "", "kikkkkO", (SUBR)dssievent_init, (SUBR)dssievent},
    {"dssinrpn", sizeof(DSSINRPN), 0, "", "kikkkO", (SUBR)dssinrpn_init, (SUBR)dssinrpn},
    {"dssiprogram", sizeof(DSSIPROGRAM), 0, "", "ikkk", (SUBR)dssiprogram_init, (SUBR)dssiprogram},
    {"dssiconfigure", sizeof(DSSICONFIGURE), 0, "", "iSS", (SUBR)dssiconfigure},
    {"dssiprograminfo", sizeof(DSSIPROGRAMINFO), 0, "Sii", "ii", (SUBR)dssiprograminfo},
    {"dssiinfo", sizeof(DSSIINFO), 0, "", "i", (SUBR)dssiinfo},
    {"dssilist", sizeof(DSSILIST), 0, "", "", (SUBR)dssilist}
};
LINKAGE_BUILTIN(dssi_localops)
