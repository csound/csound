import { uint2String } from "../utils/text-encoders.js";
import { trimNull } from "../utils/trim-null.js";

/*
   csound general i/o module from <csound.h>
   https://csound.com/docs/api/modules.html
*/

// Note: csoundGetOutputName and csoundGetInputName have been removed from Csound 7 API

// PUBLIC void 	csoundSetFileOpenCallback (CSOUND *p, void(*func)(CSOUND *, const char *, int, int, int))
