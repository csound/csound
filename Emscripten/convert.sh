#!/bin/bash

rm libcsound-worklet.base64.js

echo "AudioWorkletGlobalScope.WAM = { ENVIRONMENT: \"WEB\" };" >> libcsound-worklet.base64.js

echo "AudioWorkletGlobalScope.WAM.wasmBinary = \"$(base64 -w 0 libcsound-worklet.wasm)\";" >> libcsound-worklet.base64.js

cat << EOF >> libcsound-worklet.base64.js
AudioWorkletGlobalScope.WAM.atob = function (sBase64, nBlocksSize)
{
        
    var sB64Enc = sBase64.replace(/[^A-Za-z0-9+/]/g, "");
    var nInLen = sB64Enc.length;
    var nOutLen = nBlocksSize ? Math.ceil((nInLen * 3 + 1 >> 2) / nBlocksSize) * nBlocksSize : nInLen * 3 + 1 >> 2;
    var taBytes = new Uint8Array(nOutLen);
    
    for (var nMod3, nMod4, nUint24 = 0, nOutIdx = 0, nInIdx = 0; nInIdx < nInLen; nInIdx++) {
        nMod4 = nInIdx & 3;
        nUint24 |= AudioWorkletGlobalScope.WAM.b64ToUint6(sB64Enc.charCodeAt(nInIdx)) << 18 - 6 * nMod4;
        if (nMod4 === 3 || nInLen - nInIdx === 1) {
            for (nMod3 = 0; nMod3 < 3 && nOutIdx < nOutLen; nMod3++, nOutIdx++) {
                taBytes[nOutIdx] = nUint24 >>> (16 >>> nMod3 & 24) & 255;
            }
            nUint24 = 0;
        }
    }
    return taBytes.buffer;
}

AudioWorkletGlobalScope.WAM.b64ToUint6 = function (nChr)
{
    return nChr > 64 && nChr < 91 ?
        nChr - 65
        : nChr > 96 && nChr < 123 ?
        nChr - 71
        : nChr > 47 && nChr < 58 ?
        nChr + 4
        : nChr === 43 ?
        62
        : nChr === 47 ?
        63
        :
        0;
}

AudioWorkletGlobalScope.WAM.wasmBinary = AudioWorkletGlobalScope.WAM.atob(AudioWorkletGlobalScope.WAM.wasmBinary);
EOF
