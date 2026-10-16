/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

import { assertNumericEvent } from "./numeric-events.js";

const allowed = new Set([
  "csoundSupportsRealtime",
  "csoundIsRealtime",
  "csoundEvent",
  "csoundGetControlChannel",
  "csoundSetControlChannel",
  "csoundGetCurrentTimeSamples",
  "csoundGetSr",
  "csoundGetKsmps",
  "csoundGetNchnls",
]);

export function assertRealtimeCallAllowed(apiKey, arguments_) {
  if (!allowed.has(apiKey)) throw new Error(`${apiKey} requires stopped realtime performance`);
  if (apiKey === "csoundEvent") {
    assertNumericEvent(arguments_[0], arguments_[1]);
    // The score queue does not move GEN work off the render thread:
    // process_score_event still calls csoundFTCreate there for table events.
    if (arguments_[0] === 1)
      throw new Error("Table generation requires stopped realtime performance");
  }
  if (apiKey === "csoundGetControlChannel" || apiKey === "csoundSetControlChannel") {
    if (typeof arguments_[0] !== "string" || !arguments_[0] || arguments_[0].includes("\0"))
      throw new TypeError("Control channel names must be nonempty strings without NUL");
    if (apiKey === "csoundSetControlChannel" && !Number.isFinite(arguments_[1]))
      throw new TypeError("Control channel values must be finite numbers");
  }
}
