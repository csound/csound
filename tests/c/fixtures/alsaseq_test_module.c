/* Exercise the ALSA device-list callback without a sequencer or MIDI hardware. */
#define snd_seq_open csound_test_alsaseq_open
#define strNcpy csound_test_alsaseq_strncpy
#define csoundModuleCreate csound_test_alsaseq_module_create
#define csoundModuleInit csound_test_alsaseq_module_init
#define csoundModuleDestroy csound_test_alsaseq_module_destroy
#define csoundModuleInfo csound_test_alsaseq_module_info
#include "../../../InOut/rtalsa.c"

int snd_seq_open(snd_seq_t **seq, const char *name, int streams, int mode)
{
    (void) name;
    (void) streams;
    (void) mode;
    *seq = NULL;
    return -ENODEV;
}

int32_t csound_test_alsaseq_device_list(CSOUND *csound, CS_MIDIDEVICE *list,
                                        int32_t isOutput)
{
    return listAlsaSeq(csound, list, isOutput);
}
