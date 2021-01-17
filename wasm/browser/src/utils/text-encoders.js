export const decoder = new TextDecoder("utf-8");
export const encoder = new TextEncoder("utf-8");

export const uint2String = (uint) => decoder.decode(uint);
