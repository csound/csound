/*              Copyright Â© 1997â2000  Jens Kilian
 *
 *              This program is free software; you can redistribute it and/or modify
 *              it under the terms of the GNU General Public License as published by
 *              the Free Software Foundation; either version 1, or (at your option)
 *              any later version.
 *
 *              This program is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 *              GNU General Public License for more details.
 *
 *              You should have received a copy of the GNU General Public License
 *              along with this program; if not, write to the Free Software
 *              Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *              Be real-time audio interface.  Communicates with server.
 */

#include <OS.h>

#include "BeAudio.h"
#include "CsSvInterface.h"

// Constant data.
static const int32 kPortCapacity = 100;
static const char *kDataPortName[] = {
        "Csound audio input",
        "Csound audio output"
};

// Global variables (ugh).
port_id gADCPort = B_ERROR;
port_id gDACPort = B_ERROR;

static port_id
openConnection(
        int forOutput,
        int nChannels, int sampleFormat, float sampleRate, size_t bufferSize)
{
        port_id serverPort, dataPort;
        struct ServerAudioPort request;
        status_t status;
        const int32 msg = forOutput
                                                 ? kServerNewAudioOutPort
                                                 : kServerNewAudioInPort;

        // Find the server port.
        if ((serverPort = find_port(kServerPortName)) < B_OK) {
                return serverPort;
        }

        // Open the data port.
        if ((dataPort = create_port(kPortCapacity, kDataPortName[forOutput])) < B_OK) {
                return dataPort;
        }

        // Send the request to the server.
        request.mPort = dataPort;
        request.mNumChannels = nChannels;
        request.mSampleFormat = sampleFormat;
        request.mSampleRate = sampleRate;
        request.mBufferSize = bufferSize;
        status = write_port(serverPort, msg, &request, sizeof(request));
        if (status < B_OK) {
                delete_port(dataPort);
                dataPort = (port_id)status;
        }

        return dataPort;
}

port_id
openADCPort(int nChannels, int sampleFormat, float sampleRate,
                           size_t bufferSize)
{
        return gADCPort =
                openConnection(0, nChannels, sampleFormat, sampleRate, bufferSize);
}

port_id
openDACPort(int nChannels, int sampleFormat, float sampleRate,
                           size_t bufferSize)
{
        return gDACPort =
                openConnection(1, nChannels, sampleFormat, sampleRate, bufferSize);
}

void
closeADCPort(void)
{
        if (gADCPort >= B_OK) {
                delete_port(gADCPort);
                gADCPort = B_ERROR;
        }
}

void
closeDACPort(void)
{
        if (gDACPort >= B_OK) {
                delete_port(gDACPort);
                gDACPort = B_ERROR;
        }
}
