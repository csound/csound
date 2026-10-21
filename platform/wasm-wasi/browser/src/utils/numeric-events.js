export function assertNumericEvent(type, pfields) {
  if (
    ![0, 1, 2].includes(type) ||
    !Array.isArray(pfields) ||
    pfields.length > 64 ||
    ![...pfields].every(Number.isFinite) ||
    (type === 0 && pfields.length < 3)
  )
    throw new TypeError(
      "event expects type 0, 1 or 2 and up to 64 finite numeric p-fields (at least 3 for notes)",
    );
}
