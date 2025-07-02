/*
    smf_conv.c

    Copyright (C) 2025 Gleb Rogozinski

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

/*******************************************************\
*   smf_conv.c                                          *
*   Converts MIDI SMF0 file to SMF1                     *
*   Gleb Rogozinski 11 Jan 2025                         *
\*******************************************************/


/* Notes:
 *     The results were tested in Ableton Live 11.
 *     You can exclude some MIDI channels with masking flag -m, e.g.
 *     smf_conv -mFFFF in.mid out.mid keeps all existing channels
 *     smf_conv -mF in.mid out.mid  keeps only first 4 channels
 *     smf_conv -m8888 in.mid out.mid keeps only channels ## 4, 8, 12, 16
 */

#include "std_util.h"
#include <ctype.h>   
#include <errno.h>   

#define HEADER_SIZE 14
#define TRACK_HEADER_SIZE 8
#define MAX_TRACKS 16 

typedef struct {
    uint32_t delta_time;      
    uint32_t cumulative_time;  
    uint8_t status;           
    uint8_t data[2];         
    uint8_t data_size;        
} MidiEvent;

int is_valid_mask(CSOUND *csound, const char *mask_str, uint16_t *mask) {
    if (mask_str[0] == '\0') {
        fprintf(stderr, "Error: Mask is empty.\n");
        return 0; // Возвращаем 0 вместо false
    }

    for (int i = 0; mask_str[i] != '\0'; i++) {
        if (!isxdigit(mask_str[i])) {
            csound->Message(csound, Str("Error: Invalid character in mask: %c (only 0-9, A-F, a-f are allowed).\n"), mask_str[i]);
            return 0; 
        }
    }

    errno = 0; 
    char *end_ptr;
    long mask_long = strtol(mask_str, &end_ptr, 16);

    if (errno != 0 || *end_ptr != '\0') {
        csound->Message(csound, "%s", Str("Error: Invalid mask format.\n"));
        return 0;
    }

    if (mask_long > 0xFFFF) {
        csound->Message(csound, "%s", Str("Error: Mask value is too large (max FFFF).\n"));
        return 0; 
    }

    if (mask_long == 0) {
        csound->Message(csound, "%s", Str("Error: Mask is 0, no channels will be written.\n"));
        return 0; 
    }
    *mask = (uint16_t)mask_long;
    return 1; 
}

int count_tracks_to_write(uint32_t *channel_sizes, uint16_t track_mask) {
    int count = 0;
    for (int i = 0; i < MAX_TRACKS; i++) {
        if (channel_sizes[i] > 0 && (track_mask & (1 << i))) {
            count++;
        }
    }
    return count;
}

uint32_t encode_vlq(uint32_t value, uint8_t *buffer) {
    uint32_t size = 0;
    uint32_t shifted_value = value;

    uint32_t num_bytes = 0;
    uint32_t temp = value;
    do {
        temp >>= 7;
        num_bytes++;
    } while (temp != 0);

    for (uint32_t i = num_bytes; i > 0; i--) {
        buffer[size] = (shifted_value >> (7 * (i - 1))) & 0x7F; 
        if (i > 1) {
            buffer[size] |= 0x80; 
        }
        size++;
    }
    return size; 
}

// Helper functions for reading and writing big-endian values
uint32_t read_be32(const uint8_t *buffer) {
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

uint16_t read_be16(const uint8_t *buffer) {
    return (buffer[0] << 8) | buffer[1];
}

void write_be32(uint8_t *buffer, uint32_t value) {
    buffer[0] = (value >> 24) & 0xFF;
    buffer[1] = (value >> 16) & 0xFF;
    buffer[2] = (value >> 8) & 0xFF;
    buffer[3] = value & 0xFF;
}

void write_be16(uint8_t *buffer, uint16_t value) {
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = value & 0xFF;
}


typedef struct {
    uint8_t *track_data;
    uint32_t track_size;
    uint16_t division;
} Smf0Data;

int read_smf0_file(CSOUND *csound, const char *input_file, Smf0Data *smf0_data) {
    FILE *in = fopen(input_file, "rb");
    if (!in) {
        csound->Message(csound, "%s", Str("Error opening input file"));
        return -1;
    }

    uint8_t header[HEADER_SIZE];
    if (fread(header, 1, HEADER_SIZE, in) != HEADER_SIZE) {
        csound->Message(csound, "%s", Str("Error reading header"));
        fclose(in);
        return -1;
    }

    if (memcmp(header, "MThd", 4) != 0 || read_be32(header + 4) != 6 || read_be16(header + 8) != 0) {
        csound->Message(csound, "%s", Str("Invalid SMF0 file\n"));
        fclose(in);
        return -1;
    }

    uint16_t division = read_be16(header + 12);

    uint8_t track_header[TRACK_HEADER_SIZE];
    if (fread(track_header, 1, TRACK_HEADER_SIZE, in) != TRACK_HEADER_SIZE) {
        csound->Message(csound, "%s", Str("Error reading track header"));
        fclose(in);
        return -1;
    }

    if (memcmp(track_header, "MTrk", 4) != 0) {
        csound->Message(csound, "%s", Str("Invalid track header\n"));
        fclose(in);
        return -1;
    }

    uint32_t track_size = read_be32(track_header + 4);
    uint8_t *track_data = malloc(track_size);
    if (!track_data || fread(track_data, 1, track_size, in) != track_size) {
        csound->Message(csound, "%s", Str("Error reading track data"));
        free(track_data);
        fclose(in);
        return -1;
    }
    fclose(in);

    smf0_data->track_data = track_data;
    smf0_data->track_size = track_size;
    smf0_data->division = division;
    return 0;
}

typedef struct {
    MidiEvent **channel_buffers;
    uint32_t *channel_sizes;
    uint32_t *channel_capacities;
} ChannelData;

ChannelData parse_track_data(CSOUND *csound, uint8_t *track_data, uint32_t track_size) {
    ChannelData channel_data = {0};
    channel_data.channel_buffers = malloc(MAX_TRACKS * sizeof(MidiEvent*));
    channel_data.channel_sizes = malloc(MAX_TRACKS * sizeof(uint32_t));
    channel_data.channel_capacities = malloc(MAX_TRACKS * sizeof(uint32_t));

    for (int i = 0; i < MAX_TRACKS; i++) {
        channel_data.channel_buffers[i] = NULL;
        channel_data.channel_sizes[i] = 0;
        channel_data.channel_capacities[i] = 0;
    }

    uint32_t pos = 0;
    uint32_t cumulative_times[MAX_TRACKS] = {0};

    while (pos < track_size) {
        uint32_t delta_time = 0;
        do {
            delta_time = (delta_time << 7) | (track_data[pos] & 0x7F);
        } while (track_data[pos++] & 0x80);

        uint8_t status = track_data[pos++];
        uint8_t channel = status & 0x0F;

        for (int i = 0; i < MAX_TRACKS; i++) {
            cumulative_times[i] += delta_time;
        }

        if ((status & 0xF0) >= 0x80 && (status & 0xF0) <= 0xE0) {
            // Channel event
            uint32_t event_size = ((status & 0xF0) == 0xC0 || (status & 0xF0) == 0xD0) ? 1 : 2;

            MidiEvent event;
            event.delta_time = delta_time;
            event.cumulative_time = cumulative_times[channel];
            event.status = status;
            event.data_size = event_size;
            memcpy(event.data, &track_data[pos], event_size);

            cumulative_times[channel] = 0;

            if (channel_data.channel_capacities[channel] < channel_data.channel_sizes[channel] + 1) {
                channel_data.channel_capacities[channel] = (channel_data.channel_capacities[channel] + 1) * 2;
                channel_data.channel_buffers[channel] = realloc(channel_data.channel_buffers[channel], channel_data.channel_capacities[channel] * sizeof(MidiEvent));
            }
            channel_data.channel_buffers[channel][channel_data.channel_sizes[channel]++] = event;
            pos += event_size;
        } else if (status == 0xFF) {
          // uint8_t meta_type = track_data[pos++]; not used
            uint32_t meta_size = 0;

            do {
                meta_size = (meta_size << 7) | (track_data[pos] & 0x7F);
            } while (track_data[pos++] & 0x80);

            pos += meta_size;
        } else {
            // System events are not ignored
            csound->Message(csound, Str("System Event, Delta: 0x%X\n"), delta_time);
        }
    }

    return channel_data;
}

int write_smf1_file(CSOUND *csound, const char *output_file, uint16_t track_mask, uint16_t division, ChannelData channel_data) {
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        csound->Message(csound, "%s", Str("Error opening output file"));
        for (int i = 0; i < MAX_TRACKS; i++) {
            free(channel_data.channel_buffers[i]); 
        }
        return -1;
    }

    int num_tracks_to_write = count_tracks_to_write(channel_data.channel_sizes, track_mask);

    uint8_t smf1_header[HEADER_SIZE];
    memcpy(smf1_header, "MThd", 4);
    write_be32(smf1_header + 4, 6);
    write_be16(smf1_header + 8, 1);
    write_be16(smf1_header + 10, num_tracks_to_write + 1);
    write_be16(smf1_header + 12, division);
    fwrite(smf1_header, 1, HEADER_SIZE, out);

    uint8_t tempo_track[] = {
        'M', 'T', 'r', 'k', 0, 0, 0, 12,
        0x00, 0xFF, 0x58, 0x04, 0x04, 0x02, 0x18, 0x08,
        0x00, 0xFF, 0x2F, 0x00
    };
    fwrite(tempo_track, 1, sizeof(tempo_track), out);

    // Write each channel track
    for (int i = 0; i < MAX_TRACKS; i++) {
        if (channel_data.channel_sizes[i] > 0 && (track_mask & (1 << i))) {
            char track_name[20];
            snprintf(track_name, sizeof(track_name), "channel%d", i + 1);
            uint8_t track_name_length = (uint8_t)strlen(track_name);

            uint32_t new_track_size = 0;
            for (uint32_t j = 0; j < channel_data.channel_sizes[i]; j++) {
                uint8_t vlq_buffer[4];
                uint32_t vlq_size = encode_vlq(channel_data.channel_buffers[i][j].cumulative_time, vlq_buffer);
                new_track_size += vlq_size + 1 + channel_data.channel_buffers[i][j].data_size;
            }
            new_track_size += 5 + track_name_length + 3;

            uint8_t *new_track_data = malloc(new_track_size);
            if (!new_track_data) {
                csound->Message(csound, "%s", Str("Error allocating memory for new track data"));
                free(channel_data.channel_buffers[i]);
                continue;
            }

            uint32_t pos = 0;
            new_track_data[pos++] = 0x00;
            new_track_data[pos++] = 0xFF;
            new_track_data[pos++] = 0x03;
            new_track_data[pos++] = track_name_length;
            memcpy(new_track_data + pos, track_name, track_name_length);
            pos += track_name_length;

            for (uint32_t j = 0; j < channel_data.channel_sizes[i]; j++) {
                uint8_t vlq_buffer[4];
                uint32_t vlq_size = encode_vlq(channel_data.channel_buffers[i][j].cumulative_time, vlq_buffer);
                memcpy(new_track_data + pos, vlq_buffer, vlq_size);
                pos += vlq_size;

                new_track_data[pos++] = channel_data.channel_buffers[i][j].status;
                memcpy(new_track_data + pos, channel_data.channel_buffers[i][j].data, channel_data.channel_buffers[i][j].data_size);
                pos += channel_data.channel_buffers[i][j].data_size;
            }

            new_track_data[pos++] = 0x00;
            new_track_data[pos++] = 0xFF;
            new_track_data[pos++] = 0x2F;

            uint8_t track_header[TRACK_HEADER_SIZE];
            memcpy(track_header, "MTrk", 4);
            write_be32(track_header + 4, new_track_size);
            fwrite(track_header, 1, TRACK_HEADER_SIZE, out);
            fwrite(new_track_data, 1, new_track_size, out);

            free(new_track_data);
        }
        free(channel_data.channel_buffers[i]);
    }
    fclose(out);
    return 0;
}

int convert_smf0_to_smf1(CSOUND *csound, const char *input_file, const char *output_file, uint16_t track_mask) {
    Smf0Data smf0_data;
    if (read_smf0_file(csound, input_file, &smf0_data) == -1) {
	csound->Message(csound, "%s", Str("Failed to read SMF0 file\n"));
	return -1;
    }
    ChannelData channel_data = parse_track_data(csound, smf0_data.track_data, smf0_data.track_size);
    free(smf0_data.track_data);
    write_smf1_file(csound, output_file, track_mask, smf0_data.division, channel_data);
    csound->Message(csound, Str("Converted %s to %s\n"), input_file, output_file);
    return 0;
}

static int32_t smf_conv(CSOUND *csound, int32_t argc, char *argv[]) {
    if (UNLIKELY(argc < 3 || argc > 4)) {
        csound->Message(csound, Str("Usage: %s [-m<track_mask>] <input SMF0 file> <output SMF1 file>\n"), argv[0]);
        return 1;
    }

    uint16_t track_mask = 0xFFFF; // all tracks are unmuted by default

    int file_arg_index = 1; 

    if (strncmp(argv[1], "-m", 2) == 0) {
        const char *mask_str = argv[1] + 2; 
        if (!is_valid_mask(csound, mask_str, &track_mask)) {
            return -1;
        }
        file_arg_index = 2; 
    }

    if (argc != file_arg_index + 2) {
        csound->Message(csound, "%s", Str("Error: Invalid number of arguments. Expected 2 files after the flag.\n"));
        return -1;
    }

    const char *input_file = argv[file_arg_index];
    const char *output_file = argv[file_arg_index + 1];

    convert_smf0_to_smf1(csound, input_file, output_file, track_mask);

    return 0;
}

int32_t smf_conv_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "smf_conv", smf_conv);
    if (retval)
        return retval;
    return 
        (csound->GetUtility(csound))->SetUtilityDescription(csound, "smf_conv", Str("Converts MIDI SMF0 to SMF1"));
}
