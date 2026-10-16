/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

const allowed = new Set([
  "csoundSupportsRealtime",
  "csoundIsRealtime",
  "csoundEvent",
  "csoundGetCurrentTimeSamples",
  "csoundGetSr",
  "csoundGetKsmps",
  "csoundGetNchnls",
]);

export function assertRealtimeCallAllowed(apiKey, arguments_) {
  // Only numeric note events enter the realtime init queue. Table generation
  // and score-end events keep the generic API's short forms when stopped.
  if (!allowed.has(apiKey) || (apiKey === "csoundEvent" && arguments_[0] !== 0))
    throw new Error(`${apiKey} requires stopped realtime performance`);
  if (
    apiKey === "csoundEvent" &&
    (!Array.isArray(arguments_[1]) ||
      arguments_[1].length > 64 ||
      arguments_[1].length < 3 ||
      ![...arguments_[1]].every(Number.isFinite))
  )
    throw new TypeError("Instrument events require 3 to 64 finite numeric p-fields");
}
