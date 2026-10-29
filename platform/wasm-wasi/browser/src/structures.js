/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @typedef {Object} CSOUND_PARAMS
 * @property {number} debug_mode
 * @property {number} buffer_frames
 * @property {number} hardware_buffer_frames
 * @property {number} displays
 * @property {number} ascii_graphs
 * @property {number} postscript_graphs
 * @property {number} message_level
 * @property {number} tempo
 * @property {number} ring_bell
 * @property {number} use_cscore Removed Csound option; always zero.
 * @property {number} terminate_on_midi
 * @property {number} heartbeat
 * @property {number} defer_gen01_load
 * @property {number} midi_key
 * @property {number} midi_key_cps
 * @property {number} midi_key_oct
 * @property {number} midi_key_pch
 * @property {number} midi_velocity
 * @property {number} midi_velocity_amp
 * @property {number} no_default_paths
 * @property {number} number_of_threads
 * @property {number} syntax_check_only
 * @property {number} csd_line_counts
 * @property {number} compute_weights Unused compatibility field; always zero.
 * @property {number} realtime_mode
 * @property {number} sample_accurate
 * @property {number} sample_rate_override
 * @property {number} control_rate_override
 * @property {number} nchnls_override
 * @property {number} nchnls_i_override
 * @property {number} e0dbfs_override
 * @property {number} daemon
 * @property {number} ksmps_override
 * @property {number} FFT_library
 */
// Native OPARMS layout from include/csound.h; getParams maps it to the JS API.
export const OPARMS = [
  ["odebug", "int"],
  ["sfread", "int"],
  ["sfwrite", "int"],
  ["filetyp", "int"],
  ["inbufsamps", "int"],
  ["outbufsamps", "int"],
  ["informat", "int"],
  ["outformat", "int"],
  ["sndfileSampleSize", "int"],
  ["displays", "int"],
  ["graphsoff", "int"],
  ["postscript", "int"],
  ["msglevel", "int"],
  ["Beatmode", "int"],
  ["oMaxLag", "int"],
  ["Linein", "int"],
  ["RTevents", "int"],
  ["Midiin", "int"],
  ["FMidiin", "int"],
  ["RMidiin", "int"],
  ["ringbell", "int"],
  ["termifend", "int"],
  ["rewrt_hdr", "int"],
  ["heartbeat", "int"],
  ["gen01defer", "int"],
  ["cmdTempo", "cs_double"],
  ["sr_override", "cs_float"],
  ["kr_override", "cs_float"],
  ["nchnls_override", "int"],
  ["nchnls_i_override", "int"],
  ["infilename", "ptr"],
  ["outfilename", "ptr"],
  ["Linename", "ptr"],
  ["Midiname", "ptr"],
  ["FMidiname", "ptr"],
  ["Midioutname", "ptr"],
  ["FMidioutname", "ptr"],
  ["midiKey", "int"],
  ["midiKeyCps", "int"],
  ["midiKeyOct", "int"],
  ["midiKeyPch", "int"],
  ["midiVelocity", "int"],
  ["midiVelocityAmp", "int"],
  ["noDefaultPaths", "int"],
  ["numThreads", "int"],
  ["syntaxCheckOnly", "int"],
  ["runUnitTests", "int"],
  ["useCsdLineCounts", "int"],
  ["sampleAccurate", "int"],
  ["realtime", "int"],
  ["e0dbfs_override", "cs_float"],
  ["daemon", "int"],
  ["quality", "cs_double"],
  ["ksmps_override", "int"],
  ["fft_lib", "int"],
  ["echo", "int"],
  ["limiter", "cs_float"],
  ["sr_default", "cs_float"],
  ["kr_default", "cs_float"],
  ["mp3_mode", "int"],
  ["redef", "int"],
  ["error_deprecated", "int"],
  ["recursion_depth", "int"],
  ["no_deprecated_warnings", "int"],
];

/**
 * @typedef CS_MIDIDEVICE Array<Array.<string | number>>>
 */
export const CS_MIDIDEVICE = [
  /* debug mode, 0 or 1 */
  ["device_name", "char", 128],
  ["interface_name", "char", 128],
  ["device_id", "char", 128],
  ["midi_module", "char", 128],
  ["isOutput", "int"],
];

export const ORCTOKEN = [
  ["type", "int"],
  ["lexme", "ptr"],
  ["value", "int"],
  ["fvalue", "cs_double"],
  ["optype", "ptr"],
  ["next", "ptr"],
];

export const TREE = [
  ["type", "int"],
  ["value", "ptr"],
  ["rate", "int"],
  ["len", "int"],
  ["line", "int"],
  ["locn", "uint64"],
  ["left", "ptr"],
  ["right", "ptr"],
  ["next", "ptr"],
  ["markup", "ptr"],
];
