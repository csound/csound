/*
  rtwasapi.c:

  Copyright (C) 2025 Victor Lazzarini, Steven Yi

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

#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <stdint.h>
#include <stdio.h>
#include "csdl.h"
#include "soundio.h"

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
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    UINT32 bufferFrameCount;
    UINT32 nchnls;
    const OPARMS *O = csound->GetOParms(csound);
    DWORD devnum = 0;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return csound->InitError(csound,
                                 Str("WASAPI: Failed to initialize COM"));
    }

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                          &IID_IMMDeviceEnumerator, (void **)&pEnumerator);
    if (FAILED(hr)) {
        return csound->InitError(csound,
                                 Str("WASAPI: Failed to create device enumerator"));
    }

    if (parm->devName != NULL) {
        devnum = atoi(parm->devName);
    } else {
        devnum = parm->devNum;
    }

    if (devnum == 0) {
        hr = pEnumerator->lpVtbl->GetDefaultAudioEndpoint(
            pEnumerator, isInput ? eCapture : eRender, eConsole, &pDevice);
        if (FAILED(hr)) {
            SAFE_RELEASE(pEnumerator);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to get default device"));
        }
    } else {
        IMMDeviceCollection *pCollection = NULL;
        UINT count = 0;

        hr = pEnumerator->lpVtbl->EnumAudioEndpoints(
            pEnumerator, isInput ? eCapture : eRender, DEVICE_STATE_ACTIVE, &pCollection);
        if (FAILED(hr)) {
            SAFE_RELEASE(pEnumerator);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to enumerate devices"));
        }

        hr = pCollection->lpVtbl->GetCount(pCollection, &count);
        if (FAILED(hr) || devnum > count) {
            SAFE_RELEASE(pCollection);
            SAFE_RELEASE(pEnumerator);
            return csound->InitError(csound,
                                     Str("WASAPI: Device number out of range"));
        }

        hr = pCollection->lpVtbl->Item(pCollection, devnum - 1, &pDevice);
        SAFE_RELEASE(pCollection);

        if (FAILED(hr)) {
            SAFE_RELEASE(pEnumerator);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to get device"));
        }
    }

    hr = pDevice->lpVtbl->Activate(pDevice, &IID_IAudioClient, CLSCTX_ALL,
                                    NULL, (void **)&pAudioClient);
    if (FAILED(hr)) {
        SAFE_RELEASE(pDevice);
        SAFE_RELEASE(pEnumerator);
        return csound->InitError(csound,
                                 Str("WASAPI: Failed to activate audio client"));
    }

    hr = pAudioClient->lpVtbl->GetMixFormat(pAudioClient, &pwfx);
    if (FAILED(hr)) {
        SAFE_RELEASE(pAudioClient);
        SAFE_RELEASE(pDevice);
        SAFE_RELEASE(pEnumerator);
        return csound->InitError(csound,
                                 Str("WASAPI: Failed to get mix format"));
    }

    pwfx->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    pwfx->nChannels = (WORD)parm->nChannels;
    pwfx->nSamplesPerSec = (DWORD)parm->sampleRate;
    pwfx->wBitsPerSample = 32;
    pwfx->nBlockAlign = (pwfx->nChannels * pwfx->wBitsPerSample) / 8;
    pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
    pwfx->cbSize = 0;

    hr = pAudioClient->lpVtbl->Initialize(pAudioClient, AUDCLNT_SHAREMODE_SHARED,
                                           AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                           hnsRequestedDuration, 0, pwfx, NULL);
    if (FAILED(hr)) {
        CoTaskMemFree(pwfx);
        SAFE_RELEASE(pAudioClient);
        SAFE_RELEASE(pDevice);
        SAFE_RELEASE(pEnumerator);
        return csound->InitError(csound,
                                 Str("WASAPI: Failed to initialize audio client"));
    }

    hr = pAudioClient->lpVtbl->GetBufferSize(pAudioClient, &bufferFrameCount);
    if (FAILED(hr)) {
        CoTaskMemFree(pwfx);
        SAFE_RELEASE(pAudioClient);
        SAFE_RELEASE(pDevice);
        SAFE_RELEASE(pEnumerator);
        return csound->InitError(csound,
                                 Str("WASAPI: Failed to get buffer size"));
    }

    nchnls = parm->nChannels;

    if (isInput) {
        IAudioCaptureClient *pCaptureClient = NULL;
        hr = pAudioClient->lpVtbl->GetService(pAudioClient, &IID_IAudioCaptureClient,
                                               (void **)&pCaptureClient);
        if (FAILED(hr)) {
            CoTaskMemFree(pwfx);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to get capture client"));
        }

        cdata->hInEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (cdata->hInEvent == NULL) {
            SAFE_RELEASE(pCaptureClient);
            CoTaskMemFree(pwfx);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to create event"));
        }

        hr = pAudioClient->lpVtbl->SetEventHandle(pAudioClient, cdata->hInEvent);
        if (FAILED(hr)) {
            CloseHandle(cdata->hInEvent);
            SAFE_RELEASE(pCaptureClient);
            CoTaskMemFree(pwfx);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to set event handle"));
        }

        cdata->pInDevice = pDevice;
        cdata->pInAudioClient = pAudioClient;
        cdata->pCaptureClient = pCaptureClient;
        cdata->pwfxIn = pwfx;
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
            CoTaskMemFree(pwfx);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to get render client"));
        }

        cdata->hOutEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (cdata->hOutEvent == NULL) {
            SAFE_RELEASE(pRenderClient);
            CoTaskMemFree(pwfx);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to create event"));
        }

        hr = pAudioClient->lpVtbl->SetEventHandle(pAudioClient, cdata->hOutEvent);
        if (FAILED(hr)) {
            CloseHandle(cdata->hOutEvent);
            SAFE_RELEASE(pRenderClient);
            CoTaskMemFree(pwfx);
            SAFE_RELEASE(pAudioClient);
            SAFE_RELEASE(pDevice);
            SAFE_RELEASE(pEnumerator);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to set event handle"));
        }

        cdata->pOutDevice = pDevice;
        cdata->pOutAudioClient = pAudioClient;
        cdata->pRenderClient = pRenderClient;
        cdata->pwfxOut = pwfx;
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
    cdata->inputBuffer = (MYFLT *)csound->Calloc(csound,
                                                  csound->GetInputBufferSize(csound) * sizeof(MYFLT));
    cdata->incb = csound->CreateCircularBuffer(csound,
                                               parm->bufSamp_HW * parm->nChannels, sizeof(MYFLT));

    int32_t ret = WASAPI_open(csound, parm, cdata, 1);
    if (ret == 0) {
        cdata->inRunning = 1;
        cdata->pInAudioClient->lpVtbl->Start(cdata->pInAudioClient);
        cdata->hInThread = CreateThread(NULL, 0, InputThread, cdata, 0, NULL);
        if (cdata->hInThread == NULL) {
            cdata->inRunning = 0;
            cdata->pInAudioClient->lpVtbl->Stop(cdata->pInAudioClient);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to create input thread"));
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
    cdata->outputBuffer = (MYFLT *)csound->Calloc(csound,
                                                   csound->GetOutputBufferSize(csound) * sizeof(MYFLT));
    memset(cdata->outputBuffer, 0, csound->GetOutputBufferSize(csound) * sizeof(MYFLT));
    cdata->outcb = csound->CreateCircularBuffer(csound,
                                                parm->bufSamp_HW * parm->nChannels, sizeof(MYFLT));

    int32_t ret = WASAPI_open(csound, parm, cdata, 0);
    if (ret == 0) {
        cdata->outRunning = 1;
        cdata->pOutAudioClient->lpVtbl->Start(cdata->pOutAudioClient);
        cdata->hOutThread = CreateThread(NULL, 0, OutputThread, cdata, 0, NULL);
        if (cdata->hOutThread == NULL) {
            cdata->outRunning = 0;
            cdata->pOutAudioClient->lpVtbl->Stop(cdata->pOutAudioClient);
            return csound->InitError(csound,
                                     Str("WASAPI: Failed to create output thread"));
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
    cdata = (csdata *)*(csound->GetRtRecordUserData(csound));
    if (cdata == NULL)
        cdata = (csdata *)*(csound->GetRtPlayUserData(csound));

    if (cdata != NULL) {
        Sleep((DWORD)(1000 * csound->GetOutputBufferSize(csound) /
                      (cdata->sr * csound->GetNchnls(csound))));

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
            cdata->outRunning = 0;
            if (cdata->hOutEvent != NULL) {
                SetEvent(cdata->hOutEvent);
            }
            if (cdata->hOutThread != NULL) {
                WaitForSingleObject(cdata->hOutThread, INFINITE);
                CloseHandle(cdata->hOutThread);
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

        *(csound->GetRtRecordUserData(csound)) = NULL;
        *(csound->GetRtPlayUserData(csound)) = NULL;

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
