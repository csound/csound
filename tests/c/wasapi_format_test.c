/* Exercise WASAPI_open without an audio device. The fake client records the
 * stream format, then stops at GetBufferSize before any audio threads start. */
#include <initguid.h>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

static HRESULT WINAPI testCreateInstance(REFCLSID clsid, LPUNKNOWN outer,
                                         DWORD context, REFIID iid, LPVOID *out);
#define CoCreateInstance testCreateInstance
#include "../../InOut/rtwasapi.c"
#undef CoCreateInstance

static IMMDeviceEnumerator enumerator;
static IMMDevice device;
static IAudioClient client;
static WAVEFORMATEX mixFormat, streamFormat;
static AUDCLNT_SHAREMODE streamMode;
static DWORD streamFlags;
static int allowExclusive, failExclusiveInit, clientFailed, initialized;

static ULONG STDMETHODCALLTYPE releaseEnumerator(IMMDeviceEnumerator *self)
{
    return 1;
}

static ULONG STDMETHODCALLTYPE releaseDevice(IMMDevice *self)
{
    return 1;
}

static ULONG STDMETHODCALLTYPE releaseClient(IAudioClient *self)
{
    return 1;
}

static HRESULT WINAPI testCreateInstance(REFCLSID clsid, LPUNKNOWN outer,
                                         DWORD context, REFIID iid, LPVOID *out)
{
    *out = &enumerator;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE endpoint(IMMDeviceEnumerator *self,
                                          EDataFlow flow, ERole role,
                                          IMMDevice **out)
{
    *out = &device;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE activate(IMMDevice *self, REFIID iid,
                                          DWORD context, PROPVARIANT *params,
                                          void **out)
{
    clientFailed = 0;
    *out = &client;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE getMixFormat(IAudioClient *self,
                                              WAVEFORMATEX **out)
{
    *out = CoTaskMemAlloc(sizeof(mixFormat));
    if (*out == NULL)
        return E_OUTOFMEMORY;
    **out = mixFormat;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE formatSupported(IAudioClient *self,
                                                  AUDCLNT_SHAREMODE mode,
                                                  const WAVEFORMATEX *format,
                                                  WAVEFORMATEX **closest)
{
    if (mode == AUDCLNT_SHAREMODE_SHARED) {
        /* Reproduce a device that suggests a different stream format. */
        HRESULT hr = getMixFormat(self, closest);
        return FAILED(hr) ? hr : S_FALSE;
    }
    if (allowExclusive && format->nSamplesPerSec == mixFormat.nSamplesPerSec &&
        format->nChannels == mixFormat.nChannels &&
        format->wBitsPerSample == mixFormat.wBitsPerSample)
        return S_OK;
    return AUDCLNT_E_UNSUPPORTED_FORMAT;
}

static HRESULT STDMETHODCALLTYPE devicePeriod(IAudioClient *self,
                                               REFERENCE_TIME *normal,
                                               REFERENCE_TIME *minimum)
{
    *normal = *minimum = 100000;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE initialize(IAudioClient *self,
                                             AUDCLNT_SHAREMODE mode, DWORD flags,
                                             REFERENCE_TIME duration,
                                             REFERENCE_TIME period,
                                             const WAVEFORMATEX *format,
                                             LPCGUID session)
{
    if (clientFailed)
        return AUDCLNT_E_ALREADY_INITIALIZED;
    if (mode == AUDCLNT_SHAREMODE_EXCLUSIVE && failExclusiveInit) {
        clientFailed = 1;
        return AUDCLNT_E_DEVICE_IN_USE;
    }
    streamFormat = *format;
    streamMode = mode;
    streamFlags = flags;
    initialized = 1;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE stopAfterInitialize(IAudioClient *self,
                                                      UINT32 *frames)
{
    /* Deliberately stop here. This test covers format negotiation only. */
    return E_FAIL;
}

static const OPARMS *options(CSOUND *csound)
{
    static const OPARMS value = {0};
    return &value;
}

static char *identity(const char *text)
{
    return (char *)text;
}

static void discardMessage(CSOUND *csound, const char *format, ...)
{
}

static int checkFormat(const char *name, DWORD rate, WORD channels, int input,
                       int exclusive, int failInit, int pcm,
                       AUDCLNT_SHAREMODE expectedMode)
{
    CSOUND csound = {0};
    csdata data = {0};
    csRtAudioParams params = {0};
    csound.GetOParms = options;
    csound.LocalizeString = identity;
    csound.ErrorMsg = discardMessage;
    csound.Message = discardMessage;
    params.devNum = 1024;
    params.sampleRate = rate;
    params.nChannels = channels;
    params.bufSamp_HW = 512;

    /* All cases use a stereo 48 kHz device. */
    memset(&mixFormat, 0, sizeof(mixFormat));
    mixFormat.wFormatTag = pcm ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
    mixFormat.nSamplesPerSec = 48000;
    mixFormat.nChannels = 2;
    mixFormat.wBitsPerSample = pcm ? 16 : 32;
    mixFormat.nBlockAlign = mixFormat.nChannels * mixFormat.wBitsPerSample / 8;
    mixFormat.nAvgBytesPerSec = mixFormat.nSamplesPerSec * mixFormat.nBlockAlign;
    allowExclusive = exclusive;
    failExclusiveInit = failInit;
    initialized = 0;
    memset(&streamFormat, 0, sizeof(streamFormat));
    int result = WASAPI_open(&csound, &params, &data, input);
    CoUninitialize();

    int ok = result == -1 && initialized && streamMode == expectedMode &&
             streamFormat.nSamplesPerSec == rate &&
             streamFormat.nChannels == channels &&
             streamFormat.wBitsPerSample == 32 &&
             streamFormat.nBlockAlign == channels * 4 &&
             streamFormat.nAvgBytesPerSec == rate * channels * 4;
    if (expectedMode == AUDCLNT_SHAREMODE_SHARED) {
        DWORD conversion = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                           AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
        ok = ok && streamFormat.wFormatTag == WAVE_FORMAT_IEEE_FLOAT &&
             (streamFlags & conversion) == conversion;
    }
    printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
    return !ok;
}

int main(void)
{
    /* Set just the COM methods reached during format negotiation. */
    static IMMDeviceEnumeratorVtbl enumeratorMethods = {0};
    static IMMDeviceVtbl deviceMethods = {0};
    static IAudioClientVtbl clientMethods = {0};
    enumeratorMethods.Release = releaseEnumerator;
    enumeratorMethods.GetDefaultAudioEndpoint = endpoint;
    deviceMethods.Release = releaseDevice;
    deviceMethods.Activate = activate;
    clientMethods.Release = releaseClient;
    clientMethods.GetMixFormat = getMixFormat;
    clientMethods.IsFormatSupported = formatSupported;
    clientMethods.GetDevicePeriod = devicePeriod;
    clientMethods.Initialize = initialize;
    clientMethods.GetBufferSize = stopAfterInitialize;
    enumerator.lpVtbl = &enumeratorMethods;
    device.lpVtbl = &deviceMethods;
    client.lpVtbl = &clientMethods;

    int failures = 0;
    failures += checkFormat("44.1 kHz output on a 48 kHz device", 44100, 2, 0,
                            1, 0, 0, AUDCLNT_SHAREMODE_SHARED);
    failures += checkFormat("96 kHz output on a 48 kHz device", 96000, 2, 0,
                            1, 0, 0, AUDCLNT_SHAREMODE_SHARED);
    failures += checkFormat("exclusive mode disabled", 44100, 2, 0,
                            0, 0, 0, AUDCLNT_SHAREMODE_SHARED);
    failures += checkFormat("mono output on a stereo device", 48000, 1, 0,
                            1, 0, 0, AUDCLNT_SHAREMODE_SHARED);
    failures += checkFormat("capture at 44.1 kHz", 44100, 2, 1,
                            1, 0, 0, AUDCLNT_SHAREMODE_SHARED);
    failures += checkFormat("float stream on a PCM device", 44100, 2, 0,
                            1, 0, 1, AUDCLNT_SHAREMODE_SHARED);
    failures += checkFormat("fresh client after exclusive initialization fails",
                            48000, 2, 0, 1, 1, 0, AUDCLNT_SHAREMODE_SHARED);
    failures += checkFormat("matching format keeps exclusive mode", 48000, 2, 0,
                            1, 0, 0, AUDCLNT_SHAREMODE_EXCLUSIVE);
    return failures != 0;
}
