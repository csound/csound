/* Deterministic native plugin for DSSI host integration tests.
   Use the system ABI, when installed, independently of the host headers. */
#include <alsa/asoundlib.h>
#include <dssi.h>
#include <stdlib.h>
#include <string.h>

#define EXPORT __attribute__((visibility("default")))
static int live, cleanups, failures, group_calls, largest_group;
static int active[4];
typedef struct {
    LADSPA_Data *ports[8];
    float voice;
    int held, sustain;
    int kind;
    unsigned int events, program;
} Instance;

EXPORT int dssi_test_stat(int which)
{
    switch (which) {
    case 0: return live;
    case 1: return cleanups;
    case 2: return failures;
    case 3: return group_calls;
    case 4: return largest_group;
    default: return -1;
    }
}
EXPORT void dssi_test_reset(void)
{
    if (live) ++failures;
    cleanups = failures = group_calls = largest_group = 0;
    memset(active, 0, sizeof(active));
}

static LADSPA_Handle instantiate(const LADSPA_Descriptor *d, unsigned long sr)
{
    (void)sr;
    Instance *p = calloc(1, sizeof(*p));
    p->kind = (int)(d->UniqueID - 900001);
    ++live;
    return p;
}
static void connect_port(LADSPA_Handle h, unsigned long port, LADSPA_Data *data)
{ ((Instance *)h)->ports[port] = data; }
static void activate(LADSPA_Handle h)
{ Instance *p = h; p->voice = 0; ++active[p->kind]; }
static void deactivate(LADSPA_Handle h)
{ Instance *p = h; p->voice = 0; --active[p->kind]; }
static void cleanup(LADSPA_Handle h)
{ --live; ++cleanups; free(h); }

static void run_synth(LADSPA_Handle h, unsigned long n, snd_seq_event_t *events,
                      unsigned long count)
{
    Instance *p = h;
    unsigned long e = 0;
    for (unsigned long i = 0; i < count; ++i)
        if (events[i].time.tick >= n || (i && events[i].time.tick < events[i-1].time.tick)) ++failures;
    for (unsigned long i = 0; i < n; ++i) {
        while (e < count && events[e].time.tick == i) {
            snd_seq_event_t *event = &events[e++];
            ++p->events;
            *p->ports[7] = event->time.tick;
            if (event->type == SND_SEQ_EVENT_NOTEON) {
                if (!event->data.note.velocity) ++failures;
                ++p->held;
                p->voice = event->data.note.velocity / 127.f;
            }
            else if (event->type == SND_SEQ_EVENT_NOTEOFF) {
                if (p->held) --p->held;
                if (!p->held && !p->sustain) p->voice = 0;
            }
            else if (event->type == SND_SEQ_EVENT_CONTROLLER && event->data.control.param == 64) {
                p->sustain = event->data.control.value >= 64;
                if (!p->sustain && !p->held) p->voice = 0;
            }
            else if (event->type == SND_SEQ_EVENT_CONTROLLER &&
                     (event->data.control.param == 120 || event->data.control.param == 123)) {
                p->held = 0;
                p->voice = 0;
            }
            else if (event->type == SND_SEQ_EVENT_PGMCHANGE || event->type == SND_SEQ_EVENT_NOTE)
                ++failures;
            else if (event->type == SND_SEQ_EVENT_CONTROLLER &&
                     (event->data.control.param == 0 || event->data.control.param == 32 ||
                      event->data.control.param == 7)) ++failures;
        }
        float input = p->kind == 1 ? 0 : p->ports[3][i];
#ifdef DSSI_TEST_LADSPA
        p->voice = 0;
#endif
        p->ports[0][i] = (input + p->voice) * *p->ports[1] + *p->ports[4];
        p->ports[5][i] = p->ports[0][i];
    }
    *p->ports[2] = p->events;
    *p->ports[6] = p->program;
}
#ifdef DSSI_TEST_LADSPA
static void run(LADSPA_Handle h, unsigned long n) { run_synth(h, n, NULL, 0); }
#endif
static void run_multiple(unsigned long count, LADSPA_Handle *handles,
                         unsigned long n, snd_seq_event_t **events,
                         unsigned long *counts)
{
    ++group_calls;
    if ((int)count > largest_group) largest_group = (int)count;
    if ((int)count != active[1]) ++failures;
    for (unsigned long i = 0; i < count; ++i) run_synth(handles[i], n, events[i], counts[i]);
}
static int midi_mapping(LADSPA_Handle h, unsigned long port)
{ (void)h; return port == 1 ? DSSI_CC(7) | DSSI_NRPN(42) : DSSI_NONE; }
static const DSSI_Program_Descriptor *get_program(LADSPA_Handle h, unsigned long index)
{
    (void)h;
    static DSSI_Program_Descriptor p;
    static char name[32];
    if (index > 1) return NULL;
    strcpy(name, index ? "second" : "first");
    p.Bank = index ? 130 : 0;
    p.Program = index ? 5 : 0;
    p.Name = name;
    return &p;
}
static void select_program(LADSPA_Handle h, unsigned long bank, unsigned long program)
{
    Instance *p = h;
    p->program = (unsigned int)(bank * 128 + program);
    *p->ports[1] = program == 5 ? .5f : 1.f;
}
static char *configure(LADSPA_Handle h, const char *key, const char *value)
{
    Instance *p = h;
    if (!strcmp(key, "bias") || !strcmp(key, "GLOBAL:bias")) {
        *p->ports[4] = atof(value);
        return NULL;
    }
    return strdup("unknown fixture key");
}

static const LADSPA_PortDescriptor ports[] = {
    LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT, LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT,
    LADSPA_PORT_CONTROL | LADSPA_PORT_OUTPUT, LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT,
    LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT, LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT,
    LADSPA_PORT_CONTROL | LADSPA_PORT_OUTPUT, LADSPA_PORT_CONTROL | LADSPA_PORT_OUTPUT
};
static const char *names[] = {"left", "gain", "events", "input", "bias", "right", "program", "tick"};
static const LADSPA_PortRangeHint hints[] = {
    {0,0,0}, {LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_1, 0,1},
    {0,0,0}, {0,0,0}, {LADSPA_HINT_DEFAULT_0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}
};
static LADSPA_Descriptor descriptors[4];
static DSSI_Descriptor synths[4];
static LADSPA_PortDescriptor multi_ports[8];
static int initialized;
static void init(void)
{
    if (initialized) return;
    initialized = 1;
    memcpy(multi_ports, ports, sizeof(ports));
    multi_ports[3] = LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT;
    const char *labels[] = {"fixture", "multi", "optional", "invalid"};
    for (int i = 0; i < 4; ++i) {
        descriptors[i] = (LADSPA_Descriptor){
            .UniqueID = 900001 + i, .Label = labels[i], .Name = labels[i],
            .Maker = "Csound", .Copyright = "LGPL-2.1-or-later", .PortCount = 8,
            .PortDescriptors = i == 1 ? multi_ports : ports, .PortNames = names,
            .PortRangeHints = hints, .instantiate = instantiate, .connect_port = connect_port,
            .activate = i == 2 ? NULL : activate, .deactivate = i == 2 ? NULL : deactivate,
            .cleanup = cleanup,
#ifdef DSSI_TEST_LADSPA
            .run = run
#else
            .run = NULL
#endif
        };
        synths[i] = (DSSI_Descriptor){.DSSI_API_Version = 1, .LADSPA_Plugin = &descriptors[i],
            .configure = i == 2 ? NULL : configure,
            .get_program = i == 2 ? NULL : get_program,
            .select_program = i == 2 ? NULL : select_program,
            .get_midi_controller_for_port = i == 2 ? NULL : midi_mapping,
            .run_synth = i == 1 ? NULL : run_synth,
            .run_multiple_synths = i == 1 ? run_multiple : NULL};
    }
    descriptors[3].connect_port = NULL;
}
#ifdef DSSI_TEST_LADSPA
EXPORT const LADSPA_Descriptor *ladspa_descriptor(unsigned long i)
{ init(); return i < 4 ? &descriptors[i] : NULL; }
#else
/* Deliberately export only dssi_descriptor: discovery must handle this. */
EXPORT const DSSI_Descriptor *dssi_descriptor(unsigned long i)
{ init(); return i < 4 ? &synths[i] : NULL; }
#endif
