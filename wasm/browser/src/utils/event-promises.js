// utilities which declutter
// promises which hold back releases
// of certain async events
// for internal usecases only
import { equals, reject } from "ramda";
import { clearArray } from "@utils/clear-array";

export class EventPromises {
  constructor() {
    this.timeoutTimers = [];

    this.startPromise = undefined;
    this.startResolver = undefined;

    this.stopPromise = undefined;
    this.stopResolver = undefined;

    this.createStartPromise = this.createStartPromise.bind(this);
    this.releaseStartPromises = this.releaseStartPromises.bind(this);

    this.createStopPromise = this.createStopPromise.bind(this);
    this.releaseStopPromises = this.releaseStopPromises.bind(this);

    this.waitForStart = this.waitForStart.bind(this);
    this.waitForStop = this.waitForStop.bind(this);

    this.isWaitingToStart = this.isWaitingToStart.bind(this);
    this.isWaitingToStop = this.isWaitingToStop.bind(this);
  }

  isWaitingToStart() {
    return typeof this.startPromise !== "undefined";
  }

  isWaitingToStop() {
    return typeof this.stopPromise !== "undefined";
  }

  async waitForStart() {
    return this.startPromise ? await this.startPromise : -1;
  }

  async waitForStop() {
    return this.stopPromise ? this.stopPromise : -1;
  }

  createStartPromise() {
    if (!this.startPromise) {
      this.startPromise = new Promise((resolve) => {
        this.startResolver = resolve;
        const timer = setTimeout(() => {
          this.timeoutTimers = reject(equals(timer), this.timeoutTimers);
          if (this.startPromise) {
            this.startResolver();
            delete this.startResolver;
            this.startPromise && delete this.startPromise;
          }
        }, 2000);
        this.timeoutTimers.push(timer);
      });
    }
  }

  releaseStartPromises() {
    // first timer cleanup
    try {
      this.timeoutTimers.forEach(clearTimeout);
      clearArray(this.timeoutTimers);
    } catch (error) {
      console.error(error);
    }
    // then resolve
    if (this.startResolver) {
      this.startResolver();
      delete this.startResolver;
    }
    if (this.startPromise) {
      delete this.startPromise;
    }
  }

  createStopPromise() {
    if (!this.stopPromise) {
      this.stopPromise = new Promise((resolve) => {
        this.stopResolver = resolve;
        const timer = setTimeout(() => {
          this.timeoutTimers = reject(equals(timer), this.timeoutTimers);
          if (this.stopPromise) {
            this.stopResolver();
            delete this.stopResolver;
            this.stopPromise && delete this.stopPromise;
          }
        }, 2000);
        this.timeoutTimers.push(timer);
      });
    }
  }

  releaseStopPromises() {
    // first timer cleanup
    try {
      this.timeoutTimers.forEach(clearTimeout);
      clearArray(this.timeoutTimers);
    } catch (error) {
      console.error(error);
    }
    // then resolve
    if (this.stopResolver) {
      this.stopResolver();
      delete this.stopResolver;
    }
    if (this.stopPromise) {
      delete this.stopPromise;
    }
  }
}
