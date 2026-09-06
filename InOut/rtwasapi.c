/*
  rtwasapi.c:

  Copyright (C) 2026 The Csound Developers

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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#ifdef _WIN32

/* Must include initguid.h before windows.h to define GUIDs */
#include <initguid.h>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "csdl.h"
#include "soundio.h"

/* Define the COM GUIDs we need for WASAPI */
/* MinGW headers already define these when initguid.h is included, but MSVC doesn't */
#ifdef _MSC_VER
/* MMDevice API GUIDs */
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C,
            0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35,
            0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4c32,
            0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IAudioRenderClient, 0xF294ACFC, 0x3146, 0x4483,
            0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);
DEFINE_GUID(IID_IAudioCaptureClient, 0xC8ADBD64, 0xE71E, 0x48a0,
            0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17);
#endif /* _MSC_VER */

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->lpVtbl->Release(punk); (punk) = NULL; }

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

typedef struct csdata_ {
  IMMDevice *pInDevice;
  IMMDevice *pOutDevice;
  IAudioClient *pInAudioClient;
  IAudioClient *pOutAudioClient;
  IAudioRenderClient *pRenderClient;
  IAudioCaptureClient *pCaptureClient;
  HANDLE hInEvent;
  HANDLE hOutEvent;
  HANDLE hInThread;
  HANDLE hOutThread;
  WAVEFORMATEX *pwfxIn;
  WAVEFORMATEX *pwfxOut;
  MYFLT       *inputBuffer;
  MYFLT       *outputBuffer;
  csRtAudioParams *inParm;
  csRtAudioParams *outParm;
  int32_t onchnls, inchnls;
  CSOUND *csound;
  int32_t disp;
  void *incb;
  void *outcb;
  MYFLT sr;
  volatile int32_t inRunning;
  volatile int32_t outRunning;
  volatile int32_t outDraining;
  UINT32 inBufferFrames;
  UINT32 outBufferFrames;
} csdata;

static void DAC_channels(CSOUND *csound, int32_t chans){
    int32_t *dachans = (int32_t *) csound->QueryGlobalVariable(csound, "_DAC_CHANNELS_");
    if (dachans == NULL) {
      if (csound->CreateGlobalVariable(csound, "_DAC_CHANNELS_",
                                       sizeof(int32_t)) != 0)
        return;
      dachans = (int32_t *) csound->QueryGlobalVariable(csound, "_DAC_CHANNELS_");
      *dachans = chans;
    }
}

static void ADC_channels(CSOUND *csound, int32_t chans){
    int32_t *dachans = (int32_t *) csound->QueryGlobalVariable(csound, "_ADC_CHANNELS_");
    if (dachans == NULL) {
      if (csound->CreateGlobalVariable(csound, "_ADC_CHANNELS_",
                                       sizeof(int32_t)) != 0)
        return;
      dachans = (int32_t *) csound->QueryGlobalVariable(csound, "_ADC_CHANNELS_");
      *dachans = chans;
    }
}

static DWORD WINAPI InputThread(LPVOID lpParam)
{
    csdata *cdata = (csdata *)lpParam;
    CSOUND *csound = cdata->csound;
    IAudioCaptureClient *pCaptureClient = cdata->pCaptureClient;
    HANDLE hEvent = cdata->hInEvent;
    UINT32 packetLength = 0;
    BYTE *pData;
    UINT32 numFramesAvailable;
    DWORD flags;
    HRESULT hr;
    int32_t i, j;
    MYFLT *inputBuffer = cdata->inputBuffer;
    int32_t inchnls = cdata->inchnls;

    while (cdata->inRunning) {
        WaitForSingleObject(hEvent, INFINITE);

        hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
        if (FAILED(hr)) {
            continue;
        }
        while (packetLength != 0 && cdata->inRunning) {
            hr = pCaptureClient->lpVtbl->GetBuffer(pCaptureClient, &pData,
                                                    &numFramesAvailable, &flags, NULL, NULL);
            if (FAILED(hr)) {
                break;
            }

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                memset(inputBuffer, 0, numFramesAvailable * inchnls * sizeof(MYFLT));
            } else {
                float *pFloatData = (float *)pData;
                for (i = 0; i < (int32_t)numFramesAvailable; i++) {
                    for (j = 0; j < inchnls; j++) {
                        inputBuffer[i * inchnls + j] = (MYFLT)pFloatData[i * inchnls + j];
                    }
                }
            }

            csound->WriteCircularBuffer(csound, cdata->incb, inputBuffer,
                                        numFramesAvailable * inchnls);

            hr = pCaptureClient->lpVtbl->ReleaseBuffer(pCaptureClient, numFramesAvailable);
            if (FAILED(hr)) {
                break;
            }

            hr = pCaptureClient->lpVtbl->GetNextPacketSize(pCaptureClient, &packetLength);
            if (FAILED(hr)) {
                break;
            }
        }
    }

    return 0;
}

static DWORD WINAPI OutputThread(LPVOID lpParam)
{
    csdata *cdata = (csdata *)lpParam;
    CSOUND *csound = cdata->csound;
    IAudioRenderClient *pRenderClient = cdata->pRenderClient;
    IAudioClient *pAudioClient = cdata->pOutAudioClient;
    HANDLE hEvent = cdata->hOutEvent;
    UINT32 bufferFrameCount = cdata->outBufferFrames;
    BYTE *pData;
    UINT32 numFramesAvailable;
    UINT32 numFramesPadding;
    HRESULT hr;
    int32_t i, j;
    MYFLT *outputBuffer = cdata->outputBuffer;
    int32_t onchnls = cdata->onchnls;
    int32_t n;

    while (cdata->outRunning) {
        WaitForSingleObject(hEvent, INFINITE);

        hr = pAudioClient->lpVtbl->GetCurrentPadding(pAudioClient, &numFramesPadding);
        if (FAILED(hr)) {
            continue;
        }

        numFramesAvailable = bufferFrameCount - numFramesPadding;

        hr = pRenderClient->lpVtbl->GetBuffer(pRenderClient, numFramesAvailable, &pData);
        if (FAILED(hr)) {
            continue;
        }

        n = csound->ReadCircularBuffer(csound, cdata->outcb, outputBuffer,
                                       numFramesAvailable * onchnls);

        if (n < (int32_t)(numFramesAvailable * onchnls)) {
            /* Not enough data in buffer, fill remainder with silence */
            memset(&outputBuffer[n], 0,
                   ((numFramesAvailable * onchnls) - n) * sizeof(MYFLT));
        }

        float *pFloatData = (float *)pData;
        for (i = 0; i < (int32_t)numFramesAvailable; i++) {
            for (j = 0; j < onchnls; j++) {
                pFloatData[i * onchnls + j] = (float)outputBuffer[i * onchnls + j];
            }
        }

        hr = pRenderClient->lpVtbl->ReleaseBuffer(pRenderClient, numFramesAvailable, 0);
        if (FAILED(hr)) {
            continue;
        }
        if (cdata->outDraining &&
            csound->CheckCircularBuffer(csound, cdata->outcb, 0) == 0) {
            cdata->outRunning = 0;
        }
    }

    return 0;
}

static int32_t WASAPI_open(CSOUND *csound, const csRtAudioParams *parm,
                           csdata *cdata, int32_t isInput)
{
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pDevice = NULL;
    IAudioClient *pAudioClient = NULL;
    WAVEFORMATEX *pwfx = NULL;
    REFERENCE_TIME hnsRequestedDuration;
    UINT32 bufferFrameCount;
    UINT32 nchnls;
    const OPARMS *O = csound->GetOParms(csound);
    DWORD devnum = 0;

    /* Calculate requested buffer duration from -B flag (bufSamp_HW) */
    /* WASAPI duration is in 100-nanosecond units (REFERENCE_TIME) */
    /* Duration = (bufSamp_HW / sampleRate) * 10,000,000 */
    if (parm->bufSamp_HW > 0 && parm->sampleRate > 0) {
        hnsRequestedDuration = (REFERENCE_TIME)((double)parm->bufSamp_HW / 
                                                 parm->sampleRate * REFTIMES_PER_SEC);
    } else {
        /* Default to 100ms if not specified */
        hnsRequestedDuration = REFTIMES_PER_SEC / 10;
    }

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        csound->ErrorMsg(csound, Str("WASAPI: Failed to initialize COM"));
        return -1;
    }

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                          &IID_IMMDeviceEnumerator, (void **)&pEnumerator);
    if (FAILED(hr)) {
        csound->ErrorMsg(csound, Str("WASAPI: Failed to create device enumerator"));
        return -1;
    }

    if (parm->devName != NULL) {
        devnum = atoi(parm->devName);
    } else {
        devnum = parm->devNum;
    }

    /* Use default device if devnum is 0 or out of valid range (>= 1024) */
    if (devnum == 0 || devnum >= 1024) {
        hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(
            pEnumerator, isInput ? eCapture : eRender, eConsole, &pDevice);
        if (FAILED(hr)) {
            SAFE_RELEASE(pEnumerator);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to get default device"));
            return -1;
        }
    } else {
        IMMDeviceCollection *pCollection = NULL;
        UINT count = 0;

        hr = pEnumerator->lpVtbl->EnumAudioEndpoints(
            pEnumerator, isInput ? eCapture : eRender, DEVICE_STATE_ACTIVE, &pCollection);
        if (FAILED(hr)) {
            SAFE_RELEASE(pEnumerator);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to enumerate devices"));
            return -1;
        }

        hr = pCollection->lpVtbl->GetCount(pCollection, &count);
        if (FAILED(hr)) {
            SAFE_RELEASE(pCollection);
            SAFE_RELEASE(pEnumerator);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to enumerate devices"));
            return -1;
        }
        
        if (devnum > count) {
            SAFE_RELEASE(pCollection);
            if (O->msglevel || O->odebug)
                csound->Warning(csound,
                                Str("WASAPI: Requested device %d out of range, using default"),
                                devnum);
            /* Fall back to default device */
            hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(
                pEnumerator, isInput ? eCapture : eRender, eConsole, &pDevice);
            if (FAILED(hr)) {
                SAFE_RELEASE(pEnumerator);
                csound->ErrorMsg(csound, Str("WASAPI: Failed to get default device"));
                return -1;
            }
        } else {
            hr = pCollection->lpVtbl->Item(pCollection, devnum - 1, &pDevice);
            SAFE_RELEASE(pCollection);

            if (FAILED(hr)) {
                SAFE_RELEASE(pEnumerator);
                csound->ErrorMsg(csound, Str("WASAPI: Failed to get device"));
                return -1;
            }
        }
    }

    hr = pDevice->lpVtbl->Activate(pDevice, &IID_IAudioClient, CLSCTX_ALL,
                                    NULL, (void **)&pAudioClient);
    if (FAILED(hr)) {
        SAFE_RELEASE(pDevice);
        SAFE_RELEASE(pEnumerator);
        csound->ErrorMsg(csound, Str("WASAPI: Failed to activate audio client"));
        return -1;
    }

    hr = pAudioClient->lpVtbl->GetMixFormat(pAudioClient, &pwfx);
    if (FAILED(hr)) {
        SAFE_RELEASE(pAudioClient);
        SAFE_RELEASE(pDevice);
        SAFE_RELEASE(pEnumerator);
        csound->ErrorMsg(csound, Str("WASAPI: Failed to get mix format"));
        return -1;
    }

    /* Set up our desired format as WAVEFORMATEX (for shared mode) */
    WAVEFORMATEX desiredFormat;
    desiredFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    desiredFormat.nChannels = (WORD)parm->nChannels;
    desiredFormat.nSamplesPerSec = (DWORD)parm->sampleRate;
    desiredFormat.wBitsPerSample = 32;
    desiredFormat.nBlockAlign = (desiredFormat.nChannels * desiredFormat.wBitsPerSample) / 8;
    desiredFormat.nAvgBytesPerSec = desiredFormat.nSamplesPerSec * desiredFormat.nBlockAlign;
    desiredFormat.cbSize = 0;

    /* For exclusive mode: use WAVEFORMATEXTENSIBLE with KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
     * Most devices require WAVE_FORMAT_EXTENSIBLE rather than WAVE_FORMAT_IEEE_FLOAT
     * in exclusive mode. KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {00000003-0000-0010-8000-00aa00389b71} */
    static const GUID ksdataFormatSubtypeIeeeFloat = {
        0x00000003, 0x0000, 0x0010,
        {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}
    };
    WAVEFORMATEXTENSIBLE desiredFormatEx;
    memset(&desiredFormatEx, 0, sizeof(WAVEFORMATEXTENSIBLE));
    desiredFormatEx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    desiredFormatEx.Format.nChannels = (WORD)parm->nChannels;
    desiredFormatEx.Format.nSamplesPerSec = (DWORD)parm->sampleRate;
    desiredFormatEx.Format.wBitsPerSample = 32;
    desiredFormatEx.Format.nBlockAlign = desiredFormatEx.Format.nChannels * 4;
    desiredFormatEx.Format.nAvgBytesPerSec =
        desiredFormatEx.Format.nSamplesPerSec * desiredFormatEx.Format.nBlockAlign;
    desiredFormatEx.Format.cbSize = 22; /* sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) */
    desiredFormatEx.Samples.wValidBitsPerSample = 32;
    desiredFormatEx.dwChannelMask = (parm->nChannels == 1) ?
        SPEAKER_FRONT_CENTER : (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);
    desiredFormatEx.SubFormat = ksdataFormatSubtypeIeeeFloat;

    /* Try exclusive mode first for lower latency */
    WAVEFORMATEX *pFormatToUse = NULL;
    REFERENCE_TIME hnsBufferDuration = hnsRequestedDuration;
    WAVEFORMATEX *exclusiveFmts[2];
    size_t exclusiveFmtSizes[2];
    int fi;

    if (O->msglevel || O->odebug)
        csound->Message(csound, Str("WASAPI: Attempting exclusive mode initialization...\n"));

    /* Try two formats for exclusive mode in order:
     * 1. The device's own mix format (most compatible - uses device-native format)
     * 2. Our custom WAVEFORMATEXTENSIBLE 32-bit float format
     * Only float formats are tried since the I/O threads assume 32-bit float data. */
    exclusiveFmts[0] = pwfx;
    exclusiveFmtSizes[0] = (pwfx != NULL) ? (sizeof(WAVEFORMATEX) + pwfx->cbSize) : 0;
    exclusiveFmts[1] = (WAVEFORMATEX *)&desiredFormatEx;
    exclusiveFmtSizes[1] = sizeof(WAVEFORMATEXTENSIBLE);

    /* Reset hr so the shared mode fallback triggers if exclusive mode fails */
    hr = AUDCLNT_E_UNSUPPORTED_FORMAT;

    for (fi = 0; fi < 2; fi++) {
        WAVEFORMATEX *fmt = exclusiveFmts[fi];
        if (fmt == NULL) continue;

        /* Only use 32-bit float formats (I/O threads assume float data) */
        int isFloat32 = 0;
        if (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT && fmt->wBitsPerSample == 32) {
            isFloat32 = 1;
        } else if (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE && fmt->wBitsPerSample == 32) {
            WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE *)fmt;
            isFloat32 = (memcmp(&fmtex->SubFormat, &ksdataFormatSubtypeIeeeFloat,
                                sizeof(GUID)) == 0);
        }
        if (!isFloat32) {
            if (O->msglevel || O->odebug)
                csound->Message(csound,
                                Str("WASAPI: Exclusive format %d skipped (not 32-bit float)\n"),
                                fi);
            continue;
        }

        hr = pAudioClient->lpVtbl->IsFormatSupported(pAudioClient, AUDCLNT_SHAREMODE_EXCLUSIVE,
                                                       fmt, NULL);
        if (hr != S_OK) {
            if (O->msglevel || O->odebug)
                csound->Message(csound,
                                Str("WASAPI: Exclusive format %d not supported "
                                    "(HRESULT: 0x%08lX)\n"), fi, hr);
            continue;
        }

        /* Format is supported - get device period and align buffer duration */
        REFERENCE_TIME hnsDefaultDevicePeriod = 0, hnsMinimumDevicePeriod = 0;
        pAudioClient->lpVtbl->GetDevicePeriod(pAudioClient,
                                               &hnsDefaultDevicePeriod, &hnsMinimumDevicePeriod);
        if (hnsMinimumDevicePeriod > 0 && hnsBufferDuration < hnsMinimumDevicePeriod)
            hnsBufferDuration = hnsMinimumDevicePeriod;

        pFormatToUse = (WAVEFORMATEX *)CoTaskMemAlloc(exclusiveFmtSizes[fi]);
        if (pFormatToUse == NULL) {
            hr = E_OUTOFMEMORY;
            continue;
        }
        memcpy(pFormatToUse, fmt, exclusiveFmtSizes[fi]);

        hr = pAudioClient->lpVtbl->Initialize(pAudioClient, AUDCLNT_SHAREMODE_EXCLUSIVE,
                                               AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                               hnsBufferDuration, hnsBufferDuration,
                                               pFormatToUse, NULL);

        if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED) {
            /* Buffer size must be aligned - re-activate and retry with aligned size */
            UINT32 nFrames = 0;
            pAudioClient->lpVtbl->GetBufferSize(pAudioClient, &nFrames);
            CoTaskMemFree(pFormatToUse);
            pFormatToUse = NULL;
            hnsBufferDuration =
                (REFERENCE_TIME)(10000.0 * 1000 * nFrames / parm->sampleRate + 0.5);
            SAFE_RELEASE(pAudioClient);
            hr = pDevice->lpVtbl->Activate(pDevice, &IID_IAudioClient, CLSCTX_ALL,
                                            NULL, (void **)&pAudioClient);
            if (SUCCEEDED(hr)) {
                pFormatToUse = (WAVEFORMATEX *)CoTaskMemAlloc(exclusiveFmtSizes[fi]);
                if (pFormatToUse != NULL) {
                    memcpy(pFormatToUse, fmt, exclusiveFmtSizes[fi]);
                    hr = pAudioClient->lpVtbl->Initialize(pAudioClient,
                                                           AUDCLNT_SHAREMODE_EXCLUSIVE,
                                                           AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                                           hnsBufferDuration, hnsBufferDuration,
                                                           pFormatToUse, NULL);
                } else {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if (SUCCEEDED(hr)) {
            if (O->msglevel || O->odebug)
                csound->Message(csound, Str("WASAPI: Successfully using exclusive mode "
                                            "(format %d)\n"), fi);
            if (fi == 0 && pwfx != NULL) {
                /* We copied the mix format into pFormatToUse - free the original */
                CoTaskMemFree(pwfx);
                pwfx = NULL;
            }
            break; /* Exclusive mode succeeded */
        } else {
            if (O->msglevel || O->odebug)
                csound->Message(csound,
                                Str("WASAPI: Exclusive Initialize failed (HRESULT: 0x%08lX), "
                                    "trying next format\n"), hr);
            CoTaskMemFree(pFormatToUse);
            pFormatToUse = NULL;
            hnsBufferDuration = hnsRequestedDuration; /* Reset for next attempt */
        }
    }

    /* Free the mix format if exclusive mode succeeded and we didn't use it */
    if (hr == S_OK && pwfx != NULL) {
        CoTaskMemFree(pwfx);
        pwfx = NULL;
    }

    /* If exclusive mode failed, fall back to shared mode */
    if (FAILED(hr) || hr != S_OK) {
        if (O->msglevel || O->odebug)
            csound->Message(csound, Str("WASAPI: All exclusive mode attempts failed, "
                                        "falling back to shared mode\n"));
        /* Ensure we still have the mix format */
        if (pwfx == NULL) {
            hr = pAudioClient->lpVtbl->GetMixFormat(pAudioClient, &pwfx);
            if (FAILED(hr)) {
                if (pFormatToUse != NULL) CoTaskMemFree(pFormatToUse);
                SAFE_RELEASE(pAudioClient);
                SAFE_RELEASE(pDevice);
                SAFE_RELEASE(pEnumerator);
                csound->ErrorMsg(csound, Str("WASAPI: Failed to get mix format"));
                return -1;
            }
        }

        /* Check if our desired format is supported in shared mode */
        WAVEFORMATEX *pClosestMatch = NULL;
        hr = pAudioClient->lpVtbl->IsFormatSupported(pAudioClient, AUDCLNT_SHAREMODE_SHARED,
                                                       &desiredFormat, &pClosestMatch);

        /* Determine which format to use */
        if (hr == S_OK) {
            /* Exact match - use our desired format (allocate new structure) */
            pFormatToUse = (WAVEFORMATEX *)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
            if (pFormatToUse != NULL) {
                memcpy(pFormatToUse, &desiredFormat, sizeof(WAVEFORMATEX));
                CoTaskMemFree(pwfx);
                pwfx = NULL;
            } else {
                pFormatToUse = pwfx;
                pwfx = NULL;
            }
        } else if (hr == S_FALSE && pClosestMatch != NULL) {
            /* Close match suggested - use it */
            CoTaskMemFree(pwfx);
            pwfx = NULL;
            pFormatToUse = pClosestMatch;
        } else {
            /* Use the original mix format from GetMixFormat */
            pFormatToUse = pwfx;
            pwfx = NULL;
        }

        hr = pAudioClient->lpVtbl->Initialize(pAudioClient, AUDCLNT_SHAREMODE_SHARED,
                                               AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                               hnsRequestedDuration, 0, pFormatToUse, NULL);
        if (FAILED(hr)) {
            CoTaskMemFree(pFormatToUse);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to initialize audio client"));
            return -1;
        }

        if (O->msglevel || O->odebug)
            csound->Message(csound, Str("WASAPI: Using shared mode\n"));
    }

    hr = pAudioClient->lpVtbl->GetBufferSize(pAudioClient, &bufferFrameCount);
    if (FAILED(hr)) {
        CoTaskMemFree(pFormatToUse);
        SAFE_RELEASE(pAudioClient);
        SAFE_RELEASE(pDevice);
        SAFE_RELEASE(pEnumerator);
        csound->ErrorMsg(csound, Str("WASAPI: Failed to get buffer size"));
        return -1;
    }

    nchnls = parm->nChannels;

    if (isInput) {
        IAudioCaptureClient *pCaptureClient = NULL;
        hr = pAudioClient->lpVtbl->GetService(pAudioClient, &IID_IAudioCaptureClient,
                                               (void **)&pCaptureClient);
        if (FAILED(hr)) {
            CoTaskMemFree(pFormatToUse);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to get capture client"));
            return -1;
        }

        cdata->hInEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (cdata->hInEvent == NULL) {
            SAFE_RELEASE(pCaptureClient);
            CoTaskMemFree(pFormatToUse);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to create event"));
            return -1;
        }

        hr = pAudioClient->lpVtbl->SetEventHandle(pAudioClient, cdata->hInEvent);
        if (FAILED(hr)) {
            CloseHandle(cdata->hInEvent);
            SAFE_RELEASE(pCaptureClient);
            CoTaskMemFree(pFormatToUse);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to set event handle"));
            return -1;
        }

        cdata->pInDevice = pDevice;
        cdata->pInAudioClient = pAudioClient;
        cdata->pCaptureClient = pCaptureClient;
        cdata->pwfxIn = pFormatToUse;
        cdata->inchnls = nchnls;
        cdata->inBufferFrames = bufferFrameCount;

        ADC_channels(csound, nchnls);

        if (O->msglevel || O->odebug)
            csound->Message(csound,
                            Str("***** WASAPI module: input device open with %d "
                                "buffer frames\n"),
                            bufferFrameCount);
    } else {
        IAudioRenderClient *pRenderClient = NULL;
        hr = pAudioClient->lpVtbl->GetService(pAudioClient, &IID_IAudioRenderClient,
                                               (void **)&pRenderClient);
        if (FAILED(hr)) {
            CoTaskMemFree(pFormatToUse);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to get render client"));
            return -1;
        }

        cdata->hOutEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (cdata->hOutEvent == NULL) {
            SAFE_RELEASE(pRenderClient);
            CoTaskMemFree(pFormatToUse);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to create event"));
            return -1;
        }

        hr = pAudioClient->lpVtbl->SetEventHandle(pAudioClient, cdata->hOutEvent);
        if (FAILED(hr)) {
            CloseHandle(cdata->hOutEvent);
            SAFE_RELEASE(pRenderClient);
            CoTaskMemFree(pFormatToUse);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to set event handle"));
            return -1;
        }

        cdata->pOutDevice = pDevice;
        cdata->pOutAudioClient = pAudioClient;
        cdata->pRenderClient = pRenderClient;
        cdata->pwfxOut = pFormatToUse;
        cdata->onchnls = nchnls;
        cdata->outBufferFrames = bufferFrameCount;

        DAC_channels(csound, nchnls);

        if (O->msglevel || O->odebug)
            csound->Message(csound,
                            Str("***** WASAPI module: output device open with %d "
                                "buffer frames\n"),
                            bufferFrameCount);
    }

    SAFE_RELEASE(pEnumerator);

    cdata->sr = parm->sampleRate;
    cdata->disp = 0;
    return 0;
}

static int32_t listDevices(CSOUND *csound, CS_AUDIODEVICE *list, int32_t isOutput)
{
    HRESULT hr;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDeviceCollection *pCollection = NULL;
    UINT count = 0;
    UINT i;
    int32_t n = 0;
    char tmp[64];
    char *s;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return 0;
    }

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                          &IID_IMMDeviceEnumerator, (void **)&pEnumerator);
    if (FAILED(hr)) {
        return 0;
    }

    hr = pEnumerator->lpVtbl->EnumAudioEndpoints(
        pEnumerator, isOutput ? eRender : eCapture, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr)) {
        SAFE_RELEASE(pEnumerator);
        return 0;
    }

    hr = pCollection->lpVtbl->GetCount(pCollection, &count);
    if (FAILED(hr)) {
        SAFE_RELEASE(pCollection);
        SAFE_RELEASE(pEnumerator);
        return 0;
    }

    if (list == NULL) {
        SAFE_RELEASE(pCollection);
        SAFE_RELEASE(pEnumerator);
        return (int32_t)count;
    }

    if ((s = (char *)csound->QueryGlobalVariable(csound, "_RTAUDIO")) == NULL) {
        SAFE_RELEASE(pCollection);
        SAFE_RELEASE(pEnumerator);
        return 0;
    }

    for (i = 0; i < count; i++) {
        IMMDevice *pDevice = NULL;
        IPropertyStore *pProps = NULL;
        PROPVARIANT varName;

        hr = pCollection->lpVtbl->Item(pCollection, i, &pDevice);
        if (FAILED(hr)) {
            continue;
        }

        hr = pDevice->lpVtbl->OpenPropertyStore(pDevice, STGM_READ, &pProps);
        if (FAILED(hr)) {
            SAFE_RELEASE(pDevice);
            continue;
        }

        PropVariantInit(&varName);
        hr = pProps->lpVtbl->GetValue(pProps, &PKEY_Device_FriendlyName, &varName);
        if (SUCCEEDED(hr)) {
            WideCharToMultiByte(CP_UTF8, 0, varName.pwszVal, -1,
                                list[n].device_name, 64, NULL, NULL);

            snprintf(tmp, 64, "%s%d", isOutput ? "dac" : "adc", i + 1);
            strncpy(list[n].device_id, tmp, 63);
            list[n].device_id[63] = '\0';
            strncpy(list[n].rt_module, s, 63);
            list[n].rt_module[63] = '\0';

            IAudioClient *pAudioClient = NULL;
            hr = pDevice->lpVtbl->Activate(pDevice, &IID_IAudioClient, CLSCTX_ALL,
                                            NULL, (void **)&pAudioClient);
            if (SUCCEEDED(hr)) {
                WAVEFORMATEX *pwfx = NULL;
                hr = pAudioClient->lpVtbl->GetMixFormat(pAudioClient, &pwfx);
                if (SUCCEEDED(hr)) {
                    list[n].max_nchnls = pwfx->nChannels;
                    CoTaskMemFree(pwfx);
                } else {
                    list[n].max_nchnls = 2;
                }
                SAFE_RELEASE(pAudioClient);
            } else {
                list[n].max_nchnls = 2;
            }

            list[n].isOutput = isOutput;
            n++;
        }

        PropVariantClear(&varName);
        SAFE_RELEASE(pProps);
        SAFE_RELEASE(pDevice);
    }

    SAFE_RELEASE(pCollection);
    SAFE_RELEASE(pEnumerator);
    return n;
}

/* open for audio input */
static int32_t recopen_(CSOUND *csound, const csRtAudioParams *parm)
{
    csdata *cdata;
    void **recordata = csound->GetRtRecordUserData(csound);

    if (*(csound->GetRtRecordUserData(csound)) != NULL)
        return 0;

    if (*(csound->GetRtPlayUserData(csound)) != NULL)
        cdata = (csdata *)*(csound->GetRtPlayUserData(csound));
    else {
        cdata = (csdata *)csound->Calloc(csound, sizeof(csdata));
        cdata->disp = 1;
    }

    *recordata = (void *)cdata;
    cdata->inParm = (csRtAudioParams *)parm;
    cdata->csound = csound;
    cdata->incb = csound->CreateCircularBuffer(csound,
                                               parm->bufSamp_HW * parm->nChannels, sizeof(MYFLT));

    int32_t ret = WASAPI_open(csound, parm, cdata, 1);
    if (ret == 0) {
        /* Allocate inputBuffer based on the actual buffer size from WASAPI */
        cdata->inputBuffer = (MYFLT *)csound->Calloc(csound,
                                                      cdata->inBufferFrames * cdata->inchnls * sizeof(MYFLT));
        if (cdata->inputBuffer == NULL) {
            csound->ErrorMsg(csound, Str("WASAPI: Failed to allocate input buffer"));
            return -1;
        }
        
        cdata->inRunning = 1;
        cdata->pInAudioClient->lpVtbl->Start(cdata->pInAudioClient);
        cdata->hInThread = CreateThread(NULL, 0, InputThread, cdata, 0, NULL);
        if (cdata->hInThread == NULL) {
            cdata->inRunning = 0;
            cdata->pInAudioClient->lpVtbl->Stop(cdata->pInAudioClient);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to create input thread"));
            return -1;
        }
    }
    return ret;
}

/* open for audio output */
static int32_t playopen_(CSOUND *csound, const csRtAudioParams *parm)
{
    csdata *cdata;
    void **playdata = csound->GetRtPlayUserData(csound);

    if (*(csound->GetRtRecordUserData(csound)) != NULL)
        cdata = (csdata *)*(csound->GetRtRecordUserData(csound));
    else {
        cdata = (csdata *)csound->Calloc(csound, sizeof(csdata));
        cdata->disp = 1;
    }

    *playdata = (void *)cdata;
    cdata->outParm = (csRtAudioParams *)parm;
    cdata->csound = csound;
    cdata->outcb = csound->CreateCircularBuffer(csound,
                                                parm->bufSamp_HW * parm->nChannels, sizeof(MYFLT));

    int32_t ret = WASAPI_open(csound, parm, cdata, 0);
    if (ret == 0) {
        /* Allocate outputBuffer based on the actual buffer size from WASAPI */
        cdata->outputBuffer = (MYFLT *)csound->Calloc(csound,
                                                       cdata->outBufferFrames * cdata->onchnls * sizeof(MYFLT));
        if (cdata->outputBuffer == NULL) {
            csound->ErrorMsg(csound, Str("WASAPI: Failed to allocate output buffer"));
            return -1;
        }
        
        cdata->outDraining = 0;
        cdata->outRunning = 1;
        cdata->pOutAudioClient->lpVtbl->Start(cdata->pOutAudioClient);
        cdata->hOutThread = CreateThread(NULL, 0, OutputThread, cdata, 0, NULL);
        if (cdata->hOutThread == NULL) {
            cdata->outRunning = 0;
            cdata->pOutAudioClient->lpVtbl->Stop(cdata->pOutAudioClient);
            csound->ErrorMsg(csound, Str("WASAPI: Failed to create output thread"));
            return -1;
        }
    }
    return ret;
}

static int32_t rtrecord_(CSOUND *csound, MYFLT *inbuff_, int32_t nbytes)
{
    csdata *cdata;
    int32_t n = nbytes / sizeof(MYFLT);
    int32_t m = 0, l;

    cdata = (csdata *)*(csound->GetRtRecordUserData(csound));
    do {
        l = csound->ReadCircularBuffer(csound, cdata->incb, &inbuff_[m], n);
        m += l;
        n -= l;
        if (n)
            Sleep(1);
    } while (n);
    return nbytes;
}

static void rtplay_(CSOUND *csound, const MYFLT *outbuff_, int32_t nbytes)
{
    csdata *cdata;
    int32_t n = nbytes / sizeof(MYFLT);
    int32_t m = 0, l;

    cdata = (csdata *)*(csound->GetRtPlayUserData(csound));
    do {
        l = csound->WriteCircularBuffer(csound, cdata->outcb, &outbuff_[m], n);
        m += l;
        n -= l;
        if (n)
            Sleep(1);
    } while (n);
}

/* close the I/O device entirely */
/* called only when both complete */
static void rtclose_(CSOUND *csound)
{
    csdata *cdata;
    UINT32 padding;
    DWORD result;
    cdata = (csdata *)*(csound->GetRtRecordUserData(csound));
    if (cdata == NULL)
        cdata = (csdata *)*(csound->GetRtPlayUserData(csound));

    if (cdata != NULL) {
        *(csound->GetRtRecordUserData(csound)) = NULL;
        *(csound->GetRtPlayUserData(csound)) = NULL;
        if (cdata->pInAudioClient != NULL) {
            cdata->inRunning = 0;
            if (cdata->hInEvent != NULL) {
                SetEvent(cdata->hInEvent);
            }
            if (cdata->hInThread != NULL) {
                WaitForSingleObject(cdata->hInThread, INFINITE);
                CloseHandle(cdata->hInThread);
            }
            cdata->pInAudioClient->lpVtbl->Stop(cdata->pInAudioClient);
            if (cdata->hInEvent != NULL) {
                CloseHandle(cdata->hInEvent);
            }
            SAFE_RELEASE(cdata->pCaptureClient);
            SAFE_RELEASE(cdata->pInAudioClient);
            SAFE_RELEASE(cdata->pInDevice);
            if (cdata->pwfxIn != NULL) {
                CoTaskMemFree(cdata->pwfxIn);
            }
        }

        if (cdata->pOutAudioClient != NULL) {
            cdata->outDraining = 1;
            if (cdata->hOutEvent != NULL) {
                SetEvent(cdata->hOutEvent);
            }
            if (cdata->hOutThread != NULL) {
                result = WaitForSingleObject(cdata->hOutThread, 2000);
                if (result != WAIT_OBJECT_0) {
                    cdata->outRunning = 0;
                    if (cdata->hOutEvent != NULL)
                        SetEvent(cdata->hOutEvent);
                    WaitForSingleObject(cdata->hOutThread, INFINITE);
                }
                CloseHandle(cdata->hOutThread);
            }
            for (result = 0; result < 2000; result++) {
                if (FAILED(cdata->pOutAudioClient->lpVtbl->GetCurrentPadding(
                             cdata->pOutAudioClient, &padding)) || padding == 0)
                    break;
                Sleep(1);
            }
            cdata->pOutAudioClient->lpVtbl->Stop(cdata->pOutAudioClient);
            if (cdata->hOutEvent != NULL) {
                CloseHandle(cdata->hOutEvent);
            }
            SAFE_RELEASE(cdata->pRenderClient);
            SAFE_RELEASE(cdata->pOutAudioClient);
            SAFE_RELEASE(cdata->pOutDevice);
            if (cdata->pwfxOut != NULL) {
                CoTaskMemFree(cdata->pwfxOut);
            }
        }

        if (cdata->outputBuffer != NULL) {
            csound->Free(csound, cdata->outputBuffer);
            cdata->outputBuffer = NULL;
        }
        if (cdata->inputBuffer != NULL) {
            csound->Free(csound, cdata->inputBuffer);
            cdata->inputBuffer = NULL;
        }

        if (cdata->incb != NULL) {
            csound->DestroyCircularBuffer(csound, cdata->incb);
        }
        if (cdata->outcb != NULL) {
            csound->DestroyCircularBuffer(csound, cdata->outcb);
        }
        csound->Free(csound, cdata);
        csound->DebugMsg(csound, "%s", Str("WASAPI module: device closed\n"));
    }

    CoUninitialize();
}

int32_t csoundModuleInit(CSOUND *csound)
{
    char *drv;
    csound->ModuleListAdd(csound, "wasapi", "audio");
    drv = (char *)csound->QueryGlobalVariable(csound, "_RTAUDIO");
    if (drv == NULL)
        return 0;
    if (!(strcmp(drv, "wasapi") == 0 || strcmp(drv, "WASAPI") == 0 ||
          strcmp(drv, "Wasapi") == 0))
        return 0;
    csound->DebugMsg(csound, "%s", Str("rtaudio: WASAPI module enabled\n"));
    csound->SetPlayopenCallback(csound, playopen_);
    csound->SetRecopenCallback(csound, recopen_);
    csound->SetRtplayCallback(csound, rtplay_);
    csound->SetRtrecordCallback(csound, rtrecord_);
    csound->SetRtcloseCallback(csound, rtclose_);
    csound->SetAudioDeviceListCallback(csound, listDevices);
    return 0;
}

int32_t csoundModuleCreate(CSOUND *csound)
{
    IGN(csound);
    return 0;
}

#endif /* _WIN32 */
