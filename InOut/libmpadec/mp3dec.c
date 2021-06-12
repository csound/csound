/*
 *  mpadec - MPEG audio decoder
 *  Copyright (C) 2002-2004 Dmitriy Startsev (dstartsev@rambler.ru)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* $Id: mp3dec.c,v 1.6 2009/03/01 15:27:05 jpff Exp $ */

#include "csoundCore.h"
#include "mp3dec_internal.h"

mp3dec_t mp3dec_init(void)
{
    register struct mp3dec_t *mp3 =
      (struct mp3dec_t *)malloc(sizeof(struct mp3dec_t));

    if (!mp3) return NULL;
    memset(mp3, 0, sizeof(struct mp3dec_t));
    mp3->size = sizeof(struct mp3dec_t);
    mp3->fd = -1;
    mp3->mpadec = mpadec_init();
    if (!mp3->mpadec) {
      free(mp3);
      return NULL;
    }
    return mp3;
}

int mp3dec_init_file(mp3dec_t mp3dec, int fd, int64_t length, int nogap)
{
    register struct mp3dec_t *mp3 = (struct mp3dec_t *)mp3dec;
    int64_t tmp;
    int r;

    if (!mp3 || (mp3->size != sizeof(struct mp3dec_t)) || !mp3->mpadec)
      return MP3DEC_RETCODE_INVALID_HANDLE;
    if (fd < 0) {
      mp3dec_reset(mp3);
      return MP3DEC_RETCODE_INVALID_PARAMETERS;
    }
    if (mp3->flags & MP3DEC_FLAG_INITIALIZED) close(mp3->fd);
    mp3->fd = fd;
    mp3->flags = MP3DEC_FLAG_SEEKABLE;
    mp3->stream_offset = mp3->stream_size = mp3->stream_position = 0;
    mp3->in_buffer_offset = mp3->in_buffer_used = 0;
    mp3->out_buffer_offset = mp3->out_buffer_used = 0;
    tmp = lseek(fd, (off_t)0, SEEK_CUR);
    if (tmp >= 0) mp3->stream_offset = tmp;
    else mp3->flags &= ~MP3DEC_FLAG_SEEKABLE;
    if (mp3->flags & MP3DEC_FLAG_SEEKABLE) {
      tmp = lseek(fd, (off_t)0, SEEK_END);
      if (tmp >= 0) {
        mp3->stream_size = tmp;
        tmp = lseek(fd, mp3->stream_offset, SEEK_SET);
        if (tmp<0) fprintf(stderr, "seek failure im mp3\n");
      } else mp3->flags &= ~MP3DEC_FLAG_SEEKABLE;
    }
    if (mp3->stream_size > mp3->stream_offset) {
      mp3->stream_size -= mp3->stream_offset;
      if (length && (length < mp3->stream_size)) mp3->stream_size = length;
    } else mp3->stream_size = length;
    // check for ID3 tag
    if (lseek(fd, (off_t)0, SEEK_SET)==0) {
      char hdr[10];
      if (read(fd, &hdr, 10)!= 10) return MP3DEC_RETCODE_NOT_MPEG_STREAM;
      if (hdr[0] == 'I' && hdr[1] == 'D' && hdr[2] == '3') {
        /* A*2^21+B*2^14+C*2^7+D=A*2097152+B*16384+C*128+D*/
        mp3->stream_offset = hdr[6]*2097152+hdr[7]*16384+hdr[8]*128+hdr[9] + 10;
        // fprintf(stderr, "==== found ID3 tag, skipping %lld bytes ==== \n",
        //     mp3->stream_offset);
      }
      (void) lseek(fd, mp3->stream_offset, SEEK_SET);
    }
    r = read(fd, mp3->in_buffer, 4);
    if (r < 4) {
      mp3dec_reset(mp3);
      return ((r < 0) ? MP3DEC_RETCODE_INVALID_PARAMETERS :
              MP3DEC_RETCODE_NOT_MPEG_STREAM);
    } else mp3->in_buffer_used = r;
    if (mp3->flags & MP3DEC_FLAG_SEEKABLE)
      tmp = lseek(fd, mp3->stream_offset, SEEK_SET);
    else tmp = -1;
    if (tmp < 0) {
      int32_t n = sizeof(mp3->in_buffer) - mp3->in_buffer_used;
      mp3->flags &= ~MP3DEC_FLAG_SEEKABLE;
      if (mp3->stream_size && (n > (mp3->stream_size - mp3->in_buffer_used)))
        n = (int32_t)(mp3->stream_size - mp3->in_buffer_used);
      n = read(fd, mp3->in_buffer + mp3->in_buffer_used, n);
      if (n < 0) n = 0;
      mp3->in_buffer_used += n;
      mp3->stream_position = mp3->in_buffer_used;
    }
    else {
      int32_t n = sizeof(mp3->in_buffer);
      if (mp3->stream_size && (n > mp3->stream_size))
        n = (int32_t)mp3->stream_size;
      n = read(fd, mp3->in_buffer, n);
      if (n < 0) n = 0;
      mp3->stream_position = mp3->in_buffer_used = n;
    }
    if (mp3->in_buffer_used < 4) {
      mp3dec_reset(mp3);
      return MP3DEC_RETCODE_NOT_MPEG_STREAM;
    }
    if (nogap) {
      mpadec_decode(mp3->mpadec, mp3->in_buffer, mp3->in_buffer_used,
                    mp3->out_buffer, sizeof(mp3->out_buffer),
                    &mp3->in_buffer_offset, &mp3->out_buffer_used);
      mp3->in_buffer_used -= mp3->in_buffer_offset;
      if (!mp3->out_buffer_used) {
        mpadec_reset(mp3->mpadec);
        mp3->in_buffer_used += mp3->in_buffer_offset;
        mp3->in_buffer_offset = 0;
      }
    } else mpadec_reset(mp3->mpadec);
    if (!mp3->out_buffer_used) {
      r = mpadec_decode(mp3->mpadec, mp3->in_buffer, mp3->in_buffer_used,
                        NULL, 0, &mp3->in_buffer_offset, NULL);
      mp3->in_buffer_used -= mp3->in_buffer_offset;
      if (r != MPADEC_RETCODE_OK) {
        // this is a fix for ID3 tag at the start of a file
        while (r == 7) { /* NO SYNC, read more data */
          int32_t n = sizeof(mp3->in_buffer);
          if (mp3->stream_size && (n > mp3->stream_size))
            n = (int32_t)mp3->stream_size;
          n = read(fd, mp3->in_buffer, n);
          if (n <= 0){ /* n = 0; */ break; } /* EOF */
          mp3->stream_position = mp3->in_buffer_used = n;
          r = mpadec_decode(mp3->mpadec, mp3->in_buffer,
                            mp3->in_buffer_used,
                            NULL, 0, &mp3->in_buffer_offset, NULL);
          mp3->in_buffer_used -= mp3->in_buffer_offset;
        }
        if (r != MPADEC_RETCODE_OK) {
          mp3dec_reset(mp3);
          return MP3DEC_RETCODE_NOT_MPEG_STREAM;
        }
      }
    }
    if ((mpadec_get_info(mp3->mpadec, &mp3->mpainfo,
                         MPADEC_INFO_STREAM) != MPADEC_RETCODE_OK) ||
        (mpadec_get_info(mp3->mpadec, &mp3->taginfo,
                         MPADEC_INFO_TAG) != MPADEC_RETCODE_OK)) {
      mp3dec_reset(mp3);
      return MP3DEC_RETCODE_NOT_MPEG_STREAM;
    }
    if (mp3->taginfo.flags & 2)
      if (!mp3->stream_size || (mp3->stream_size > mp3->taginfo.bytes))
        mp3->stream_size = mp3->taginfo.bytes;
    if (mp3->taginfo.flags & 1) {
      mp3->mpainfo.frames = mp3->taginfo.frames;
      if (mp3->mpainfo.frames && mp3->mpainfo.frame_samples) {
        mp3->mpainfo.bitrate = (int32_t)
          ((MYFLT)(((MYFLT)mp3->stream_size*(MYFLT)mp3->mpainfo.frequency + 0.5)/
                   ((MYFLT)125.0*mp3->mpainfo.frame_samples*mp3->mpainfo.frames)));
      }
    } else if (mp3->mpainfo.bitrate && mp3->mpainfo.frame_samples) {
      mp3->mpainfo.frames = (int32_t)
        ((MYFLT)(((MYFLT)mp3->stream_size*(MYFLT)mp3->mpainfo.frequency + 0.5)/
                 ((MYFLT)125.0*mp3->mpainfo.frame_samples*mp3->mpainfo.bitrate)));
    }
    mp3->mpainfo.duration =
      (mp3->mpainfo.frames*mp3->mpainfo.frame_samples +
       (mp3->mpainfo.frequency >> 1))/mp3->mpainfo.frequency;
    mp3->flags |= MP3DEC_FLAG_INITIALIZED;
    return MP3DEC_RETCODE_OK;
}

int mp3dec_uninit(mp3dec_t mp3dec)
{
    register struct mp3dec_t *mp3 = (struct mp3dec_t *)mp3dec;

    if (!mp3 || (mp3->size != sizeof(struct mp3dec_t)) || !mp3->mpadec)
      return MP3DEC_RETCODE_INVALID_HANDLE;
    if (mp3->flags & MP3DEC_FLAG_INITIALIZED) close(mp3->fd);
    mp3->fd = -1;
    mp3->flags = 0;
    mpadec_uninit(mp3->mpadec);
    mp3->size = 0;
    free(mp3);
    return MP3DEC_RETCODE_OK;
}

int mp3dec_reset(mp3dec_t mp3dec)
{
    register struct mp3dec_t *mp3 = (struct mp3dec_t *)mp3dec;

    if (!mp3 || (mp3->size != sizeof(struct mp3dec_t)) || !mp3->mpadec)
      return MP3DEC_RETCODE_INVALID_HANDLE;
    if (mp3->flags & MP3DEC_FLAG_INITIALIZED) close(mp3->fd);
    mp3->fd = -1;
    mp3->flags = 0;
    mpadec_reset(mp3->mpadec);
    mp3->stream_offset = mp3->stream_size = mp3->stream_position = 0;
    mp3->in_buffer_offset = mp3->in_buffer_used = 0;
    mp3->out_buffer_offset = mp3->out_buffer_used = 0;
    memset(&mp3->mpainfo, 0, sizeof(mp3->mpainfo));
    memset(&mp3->taginfo, 0, sizeof(mp3->taginfo));
    return MP3DEC_RETCODE_OK;
}

int mp3dec_configure(mp3dec_t mp3dec, mpadec_config_t *cfg)
{
    register struct mp3dec_t *mp3 = (struct mp3dec_t *)mp3dec;

    if (!mp3 || (mp3->size != sizeof(struct mp3dec_t)) || !mp3->mpadec)
      return MP3DEC_RETCODE_INVALID_HANDLE;
    mp3->out_buffer_offset = mp3->out_buffer_used = 0;
    if (mpadec_configure(mp3->mpadec, cfg) != MPADEC_RETCODE_OK)
      return MP3DEC_RETCODE_INVALID_PARAMETERS;
    return MP3DEC_RETCODE_OK;
}

int mp3dec_get_info(mp3dec_t mp3dec, void *info, int info_type)
{
    register struct mp3dec_t *mp3 = (struct mp3dec_t *)mp3dec;

    if (!mp3 || (mp3->size != sizeof(struct mp3dec_t)) || !mp3->mpadec)
      return MP3DEC_RETCODE_INVALID_HANDLE;
    if (!info) return MP3DEC_RETCODE_INVALID_PARAMETERS;
    if (mp3->flags & MP3DEC_FLAG_INITIALIZED) {
      switch (info_type) {
      case MPADEC_INFO_STREAM:
        memcpy(info, &mp3->mpainfo, sizeof(mp3->mpainfo)); break;
      case MPADEC_INFO_TAG:
        memcpy(info, &mp3->taginfo, sizeof(mp3->taginfo)); break;
      case MPADEC_INFO_CONFIG:
      default:
        if (mpadec_get_info(mp3->mpadec, info, info_type) != MPADEC_RETCODE_OK)
          return MP3DEC_RETCODE_INVALID_PARAMETERS;
      }
    } else return MP3DEC_RETCODE_BAD_STATE;
    return MP3DEC_RETCODE_OK;
}

int mp3dec_decode(mp3dec_t mp3dec, uint8_t *buf, uint32_t bufsize, uint32_t *used)
{
    register struct mp3dec_t *mp3 = (struct mp3dec_t *)mp3dec;
    uint32_t n, src_used, dst_used; int r;

    if (used) *used = 0;
    if (!mp3 || (mp3->size != sizeof(struct mp3dec_t)) || !mp3->mpadec)
      return MP3DEC_RETCODE_INVALID_HANDLE;
    if (!(mp3->flags & MP3DEC_FLAG_INITIALIZED)) return MP3DEC_RETCODE_BAD_STATE;
    if (!buf) return MP3DEC_RETCODE_INVALID_PARAMETERS;
    while (bufsize) {
      if (mp3->out_buffer_used) {
        n = (bufsize < mp3->out_buffer_used) ? bufsize : mp3->out_buffer_used;
        memcpy(buf, mp3->out_buffer + mp3->out_buffer_offset, n);
        mp3->out_buffer_offset += n;
        mp3->out_buffer_used -= n;
        buf += n;
        bufsize -= n;
        if (used) *used += n;
      }
      if (!bufsize) break;
      if (mp3->in_buffer_used > 4) {
        r = mpadec_decode(mp3->mpadec, mp3->in_buffer + mp3->in_buffer_offset,
                          mp3->in_buffer_used, buf, bufsize, &src_used, &dst_used);
        mp3->in_buffer_offset += src_used;
        mp3->in_buffer_used -= src_used;
        buf += dst_used;
        bufsize -= dst_used;
        if (used) *used += dst_used;
        if (!bufsize) break;
        if (r == MPADEC_RETCODE_BUFFER_TOO_SMALL) {
          mp3->out_buffer_offset = mp3->out_buffer_used = 0;
          mpadec_decode(mp3->mpadec, mp3->in_buffer + mp3->in_buffer_offset,
                        mp3->in_buffer_used, mp3->out_buffer,
                        sizeof(mp3->out_buffer), &src_used, &mp3->out_buffer_used);
          mp3->in_buffer_offset += src_used;
          mp3->in_buffer_used -= src_used;
          continue;
        }
      }
      if (mp3->in_buffer_used && mp3->in_buffer_offset)
        memmove(mp3->in_buffer, mp3->in_buffer + mp3->in_buffer_offset,
                mp3->in_buffer_used);
      mp3->in_buffer_offset = 0;
      n = sizeof(mp3->in_buffer) - mp3->in_buffer_used;
      if (mp3->stream_size && (n > (mp3->stream_size - mp3->stream_position)))
        n = (int32_t)(mp3->stream_size - mp3->stream_position);
      if (n) r = read(mp3->fd, mp3->in_buffer + mp3->in_buffer_used, n);
      else r = 0;
      if (r < 0) r = 0;
      mp3->in_buffer_used += r;
      mp3->stream_position += r;
      if (mp3->stream_position > mp3->stream_size)
        mp3->stream_position = mp3->stream_size;
      if (!r) break;
    }
    return MP3DEC_RETCODE_OK;
}

int mp3dec_seek(mp3dec_t mp3dec, int64_t pos, int units)
{
    register struct mp3dec_t *mp3 = (struct mp3dec_t *)mp3dec;
    int64_t newpos;

    if (!mp3 || (mp3->size != sizeof(struct mp3dec_t)) || !mp3->mpadec)
      return MP3DEC_RETCODE_INVALID_HANDLE;
    if (!(mp3->flags & MP3DEC_FLAG_INITIALIZED)) return MP3DEC_RETCODE_BAD_STATE;
    if (!(mp3->flags & MP3DEC_FLAG_SEEKABLE)) return MP3DEC_RETCODE_SEEK_FAILED;
    if (units == MP3DEC_SEEK_BYTES) {
      newpos = (pos < mp3->stream_size) ? pos : mp3->stream_size;
      newpos = lseek(mp3->fd, mp3->stream_offset + newpos, SEEK_SET);
      if (newpos < 0) return MP3DEC_RETCODE_SEEK_FAILED;
      mp3->stream_position = newpos - mp3->stream_offset;
      mp3->in_buffer_offset = mp3->in_buffer_used = 0;
      mp3->out_buffer_offset = mp3->out_buffer_used = 0;
    } else if (units == MP3DEC_SEEK_SAMPLES) {
      MYFLT fsize =
        (MYFLT)(125.0*mp3->mpainfo.bitrate*mp3->mpainfo.decoded_frame_samples)/
        (MYFLT)mp3->mpainfo.decoded_frequency;

      newpos = (int64_t)
        ((MYFLT)pos*fsize/(MYFLT)mp3->mpainfo.decoded_frame_samples);
      //printf("seek pos: %d %d\n", newpos, pos);
      if (newpos > mp3->stream_size) newpos = mp3->stream_size;
      pos = (pos%mp3->mpainfo.decoded_frame_samples)*
        mp3->mpainfo.decoded_sample_size;
      newpos = lseek(mp3->fd, mp3->stream_offset + newpos, SEEK_SET);
      if (newpos < 0) return MP3DEC_RETCODE_SEEK_FAILED;
      mp3->stream_position = newpos - mp3->stream_offset;
      mp3->in_buffer_offset = mp3->in_buffer_used = 0;
      mp3->out_buffer_offset = mp3->out_buffer_used = 0;
      {
        uint8_t temp[8*1152];
        mp3dec_decode(mp3, temp, (uint32_t)pos, NULL);
      }
    } else if (units == MP3DEC_SEEK_SECONDS) {
      if (pos > mp3->mpainfo.duration) pos = mp3->mpainfo.duration;
      if (mp3->taginfo.flags & 4) {
        int32_t n = (int32_t)((100*pos + (mp3->mpainfo.duration >> 1))/
                              mp3->mpainfo.duration);
        if (n > 99) newpos = mp3->stream_size;
        else newpos = (mp3->taginfo.toc[n]*mp3->stream_size)/255;
      }
      else newpos =
             (pos*mp3->stream_size + (mp3->mpainfo.duration >> 1))/
             mp3->mpainfo.duration;
      if (newpos > mp3->stream_size) newpos = mp3->stream_size;
      newpos = lseek(mp3->fd, mp3->stream_offset + newpos, SEEK_SET);
      if (newpos < 0) return MP3DEC_RETCODE_SEEK_FAILED;
      mp3->stream_position = newpos - mp3->stream_offset;
      mp3->in_buffer_offset = mp3->in_buffer_used = 0;
      mp3->out_buffer_offset = mp3->out_buffer_used = 0;
    } else return MP3DEC_RETCODE_INVALID_PARAMETERS;
    return MP3DEC_RETCODE_OK;
}

char *mp3dec_error(int code)
{
    static char *mp3_errors[] = { "No error",
                                  "Invalid handle",
                                  "Bad decoder state",
                                  "Invalid parameters",
                                  "Not an MPEG audio stream",
                                  "Seek failed",
                                  "Unknown error" };
    if (code > MP3DEC_RETCODE_UNKNOWN) code = MP3DEC_RETCODE_UNKNOWN;
    return mp3_errors[code];
}
