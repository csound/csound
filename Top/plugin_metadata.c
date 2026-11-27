/*
    plugin_metadata.c:

    Read the metadata record of a plugin library from its file, without
    loading the library.

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

/* The record is written by CSOUND_PLUGIN_METADATA (csdl.h); its layout is
   in csound_types.h. The reader does not parse any object format: it scans
   the whole file for the marker, so ELF, Mach-O (universal binaries
   included), PE and Wasm files are read the same way. The file is untrusted
   input, so every length is checked against the format limit and against
   the bytes left in the file. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "csound.h"
#include "plugin_metadata.h"

enum {
  MARKER_SIZE = 8,
  /* bytes read at a time while looking for the marker */
  SCAN_CHUNK = 64 * 1024,
  /* more markers than this in one file make it invalid: a plugin has one
     copy per architecture */
  MAX_RECORDS = 16
};

/* The marker, each byte XORed with MARKER_KEY, so that libcsound never holds
   the marker itself and finds no record in its own file. */
#define MARKER_KEY 0x5A
static const unsigned char marker_xored[MARKER_SIZE] = {
  'C' ^ MARKER_KEY, 'S' ^ MARKER_KEY, 'P' ^ MARKER_KEY, 'L' ^ MARKER_KEY,
  'M' ^ MARKER_KEY, 'E' ^ MARKER_KEY, 'T' ^ MARKER_KEY, 'A' ^ MARKER_KEY
};

/* volatile, so that the compiler cannot work the marker out and store it
   as a constant */
static volatile unsigned char marker_key = MARKER_KEY;

static void make_marker(unsigned char *marker)
{
  unsigned char key = marker_key;
  int i;
  for (i = 0; i < MARKER_SIZE; i++)
    marker[i] = (unsigned char) (marker_xored[i] ^ key);
}

/* Check a payload: a version of printable ASCII, then a text with no NUL
   and no control character but tab, CR and LF, each ending with a NUL.
   Bytes from 0x80 are taken as they are, as TOML text is UTF-8. */
static int32_t check_payload(const unsigned char *p, uint32_t length,
                             uint32_t *versionLength)
{
  uint32_t i, limit, textStart;
  limit = length < CSOUND_MODULE_VERSION_MAX ?
          length : (uint32_t) CSOUND_MODULE_VERSION_MAX;
  for (i = 0; i < limit && p[i] != '\0'; i++)
    if (p[i] < 0x20 || p[i] > 0x7E)
      return 0;
  if (i == 0 || i == limit)
    return 0;
  *versionLength = i;
  textStart = i + 1;
  if (textStart >= length || p[length - 1] != '\0')
    return 0;
  for (i = textStart; i < length - 1; i++) {
    unsigned char c = p[i];
    if ((c < 0x20 && c != '\t' && c != '\n' && c != '\r') || c == 0x7F)
      return 0;
  }
  return 1;
}

/* Record the offset of each marker in the file, MAX_RECORDS at most.
   Return the number found, MAX_RECORDS + 1 if there are more, or -1 if the
   file cannot be read. */
static int32_t find_markers(FILE *f, const unsigned char *marker,
                            long *offsets)
{
  unsigned char *buf;
  size_t kept = 0, got, i;
  long base = 0;     /* file offset of buf[0] */
  int32_t count = 0;

  buf = (unsigned char *) malloc(SCAN_CHUNK + MARKER_SIZE);
  if (buf == NULL)
    return -1;
  /* keep the last MARKER_SIZE - 1 bytes of each chunk in front of the next,
     so a marker across two chunks is found too */
  while ((got = fread(buf + kept, 1, SCAN_CHUNK, f)) > 0) {
    size_t avail = kept + got;
    for (i = 0; i + MARKER_SIZE <= avail; i++) {
      const unsigned char *p =
          (const unsigned char *) memchr(buf + i, marker[0],
                                         avail - MARKER_SIZE + 1 - i);
      if (p == NULL)
        break;
      i = (size_t) (p - buf);
      if (memcmp(p, marker, MARKER_SIZE) == 0) {
        if (count == MAX_RECORDS) {
          free(buf);
          return MAX_RECORDS + 1;
        }
        offsets[count++] = base + (long) i;
      }
    }
    kept = avail < MARKER_SIZE - 1 ? avail : MARKER_SIZE - 1;
    memmove(buf, buf + avail - kept, kept);
    if ((size_t) (LONG_MAX - base) < avail - kept) {
      free(buf);
      return -1;
    }
    base += (long) (avail - kept);
  }
  i = (size_t) ferror(f);
  free(buf);
  return i ? -1 : count;
}

/* Read the record whose marker is at 'offset'. Return 1 with its payload in
   rec, 0 if it is not a valid record, -1 if the file cannot be read. */
static int32_t read_record(FILE *f, long offset, PLUGIN_METADATA *rec)
{
  unsigned char header[CSOUND_PLUGIN_METADATA_HEADER_SIZE - MARKER_SIZE];
  uint32_t length;
  if (fseek(f, offset + MARKER_SIZE, SEEK_SET) != 0)
    return -1;
  if (fread(header, 1, sizeof(header), f) != sizeof(header))
    return ferror(f) ? -1 : 0;    /* the header runs past the end */
  if (header[0] != CSOUND_PLUGIN_METADATA_FORMAT ||
      header[1] != 0 || header[2] != 0 || header[3] != 0)
    return 0;
  length = (uint32_t) header[4] | ((uint32_t) header[5] << 8) |
           ((uint32_t) header[6] << 16) | ((uint32_t) header[7] << 24);
  if (length == 0 || length > CSOUND_PLUGIN_METADATA_MAX)
    return 0;
  rec->payload = (unsigned char *) malloc(length);
  if (rec->payload == NULL)
    return -1;
  rec->length = length;
  if (fread(rec->payload, 1, length, f) != length) {
    int32_t err = ferror(f);
    pluginMetadataFree(rec);
    return err ? -1 : 0;          /* the payload runs past the end */
  }
  if (!check_payload(rec->payload, length, &rec->versionLength)) {
    pluginMetadataFree(rec);
    return 0;
  }
  return 1;
}

void pluginMetadataFree(PLUGIN_METADATA *rec)
{
  free(rec->payload);
  rec->payload = NULL;
  rec->length = rec->versionLength = 0;
}

int32_t pluginMetadataRead(const char *path, PLUGIN_METADATA *rec)
{
  unsigned char marker[MARKER_SIZE];
  long offsets[MAX_RECORDS];
  FILE *f;
  int32_t count, i, status = CSOUND_PLUGIN_METADATA_OK;

  rec->payload = NULL;
  rec->length = rec->versionLength = 0;
  if (path == NULL || (f = fopen(path, "rb")) == NULL)
    return CSOUND_PLUGIN_METADATA_ERROR;
  make_marker(marker);
  count = find_markers(f, marker, offsets);
  if (count < 0) {
    fclose(f);
    return CSOUND_PLUGIN_METADATA_ERROR;
  }
  if (count == 0) {
    fclose(f);
    return CSOUND_PLUGIN_METADATA_NONE;
  }
  if (count > MAX_RECORDS) {
    fclose(f);
    return CSOUND_PLUGIN_METADATA_INVALID;
  }
  /* A universal binary holds one copy per architecture: identical copies
     count once, different ones make the metadata ambiguous. Any invalid
     copy makes it invalid. */
  for (i = 0; i < count; i++) {
    PLUGIN_METADATA copy;
    int32_t r = read_record(f, offsets[i], &copy);
    if (r < 0) {
      status = CSOUND_PLUGIN_METADATA_ERROR;
      break;
    }
    if (r == 0) {
      status = CSOUND_PLUGIN_METADATA_INVALID;
      break;
    }
    if (rec->payload == NULL) {
      *rec = copy;
      continue;
    }
    if (copy.length != rec->length ||
        memcmp(copy.payload, rec->payload, rec->length) != 0)
      status = CSOUND_PLUGIN_METADATA_AMBIGUOUS;
    pluginMetadataFree(&copy);
  }
  fclose(f);
  if (status != CSOUND_PLUGIN_METADATA_OK)
    pluginMetadataFree(rec);
  return status;
}

PUBLIC int32_t csoundGetPluginMetadata(const char *path,
                                       char *version, size_t versionSize,
                                       char *text, size_t *textSize)
{
  PLUGIN_METADATA rec;
  int32_t status = pluginMetadataRead(path, &rec);
  size_t textLength;
  if (status != CSOUND_PLUGIN_METADATA_OK)
    return status;
  if (version != NULL) {
    if (versionSize > rec.versionLength)
      memcpy(version, rec.payload, rec.versionLength + 1);
    else
      status = CSOUND_PLUGIN_METADATA_SHORT_BUFFER;
  }
  if (textSize != NULL) {
    textLength = rec.length - rec.versionLength - 1;
    if (text != NULL && *textSize >= textLength)
      memcpy(text, rec.payload + rec.versionLength + 1, textLength);
    else
      status = CSOUND_PLUGIN_METADATA_SHORT_BUFFER;
    *textSize = textLength;
  }
  pluginMetadataFree(&rec);
  return status;
}
