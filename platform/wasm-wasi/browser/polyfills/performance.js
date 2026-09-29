const performance = {
  offset: Date.now(),
  now: function now() {
    return Date.now() - this.offset;
  },
};
