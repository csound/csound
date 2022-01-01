export const csoundApiRename = (apiName) => {
  const minusCsound = apiName.replace(/^csound/i, "");
  return minusCsound.charAt(0).toLowerCase() + minusCsound.slice(1);
};
