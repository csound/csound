/*
    plugin_metadata.h:

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

#ifndef CSOUND_PLUGIN_METADATA_H
#define CSOUND_PLUGIN_METADATA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A plugin's metadata record, as read from its library file: the payload
   holds the version, its NUL, then the text and its NUL. */
typedef struct {
  unsigned char *payload;
  uint32_t      length;          /* payload bytes */
  uint32_t      versionLength;   /* version bytes, without the NUL */
} PLUGIN_METADATA;

/* Read the record of the library at 'path' without loading it. Return a
   CSOUND_PLUGIN_METADATA_STATUS; on CSOUND_PLUGIN_METADATA_OK the caller
   frees the record with pluginMetadataFree(). */
int32_t pluginMetadataRead(const char *path, PLUGIN_METADATA *rec);
void    pluginMetadataFree(PLUGIN_METADATA *rec);

#ifdef __cplusplus
}
#endif

#endif /* CSOUND_PLUGIN_METADATA_H */
