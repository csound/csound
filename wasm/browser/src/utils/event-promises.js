// utilities which declutter
// promises which hold back releases
// of certain async events
// for internal usecases only
import { equals, reject } from "rambda/dist/rambda.mjs";
import { clearArray } from "./clear-array";

export class EventPromises {
  constructor() {
    this.timeoutTimers = [];

    this.startPromise = undefined;
    this.startResolver = undefined;

    this.stopPromise = undefined;
    this.stopResolver = undefined;

    this.pausePromise = undefined;
    this.pauseResolver = undefined;

    this.resumePromise = undefined;
    this.resumeResolver = undefined;

    this.createStartPromise = this.createStartPromise.bind(this);
    this.releaseStartPromise = this.releaseStartPromise.bind(this);

    this.createStopPromise = this.createStopPromise.bind(this);
    this.releaseStopPromise = this.releaseStopPromise.bind(this);

    this.createPausePromise = this.createPausePromise.bind(this);
    this.releasePausePromise = this.releasePausePromise.bind(this);

    this.createResumePromise = this.createResumePromise.bind(this);
    this.releaseResumePromise = this.releaseResumePromise.bind(this);

    this.waitForStart = this.waitForStart.bind(this);
    this.waitForStop = this.waitForStop.bind(this);

    this.isWaitingToStart = this.isWaitingToStart.bind(this);
    this.isWaitingToStop = this.isWaitingToStop.bind(this);
  }

  isWaitingToStart() {
    return this.startPromise !== undefined;
  }

  isWaitingToStop() {
    return this.stopPromise !== undefined;
  }

  isWaiting(eventAsking) {
    if (this.startPromise !== undefined) {
      console.error(`cannot ${eventAsking} while starting, did you forget to 'await'?`);
      return true;
    }

    if (this.stopPromise !== undefined) {
      console.error(`cannot ${eventAsking} while stopping, did you forget to 'await'?`);
      return true;
    }

    if (this.pausePromise !== undefined) {
      console.error(`cannot ${eventAsking} while pausing, did you forget to 'await'?`);
      return true;
    }

    if (this.resumePromise !== undefined) {
      console.error(`cannot ${eventAsking} while resuming, did you forget to 'await'?`);
      return true;
    }

    return false;
  }

  async waitForStart() {
    return this.startPromise ? await this.startPromise : -1;
  }

  async waitForStop() {
    return this.stopPromise ?? -1;
  }

  async waitForPause() {
    return this.pausePromise ?? -1;
  }

  async waitForResume() {
    return this.resumePromise ?? -1;
  }

  createStartPromise() {
    if (!this.startPromise) {
      this.startPromise = new Promise((resolve) => {
        this.startResolver = resolve;
        const timer = setTimeout(() => {
          this.timeoutTimers = reject(equals(timer), this.timeoutTimers);
          if (this.startPromise) {
            console.warn("start promise timed out");
            this.startResolver();
            delete this.startResolver;
            this.startPromise && delete this.startPromise;
          }
        }, 2000);
        this.timeoutTimers.push(timer);
      });
    }
  }

  releaseStartPromise() {
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
            console.warn("stop promise timed out");
            this.stopResolver();
            delete this.stopResolver;
            this.stopPromise && delete this.stopPromise;
          }
        }, 2000);
        this.timeoutTimers.push(timer);
      });
    }
  }

  releaseStopPromise() {
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

  createPausePromise() {
    if (!this.pausePromise) {
      this.pausePromise = new Promise((resolve) => {
        this.pauseResolver = resolve;
        const timer = setTimeout(() => {
          this.timeoutTimers = reject(equals(timer), this.timeoutTimers);
          if (this.pausePromise) {
            console.warn("pause promise timed out");
            this.pauseResolver();
            delete this.pauseResolver;
            this.pausePromise && delete this.pausePromise;
          }
        }, 2000);
        this.timeoutTimers.push(timer);
      });
    }
  }

  releasePausePromise() {
    // first timer cleanup
    try {
      this.timeoutTimers.forEach(clearTimeout);
      clearArray(this.timeoutTimers);
    } catch (error) {
      console.error(error);
    }
    // then resolve
    if (this.pauseResolver) {
      this.pauseResolver();
      delete this.pauseResolver;
    }
    if (this.pausePromise) {
      delete this.pausePromise;
    }
  }

  createResumePromise() {
    if (!this.resumePromise) {
      this.resumePromise = new Promise((resolve) => {
        this.resumeResolver = resolve;
        const timer = setTimeout(() => {
          this.timeoutTimers = reject(equals(timer), this.timeoutTimers);
          if (this.resumePromise) {
            console.warn("resume promise timed out");
            this.resumeResolver();
            delete this.resumeResolver;
            this.resumePromise && delete this.resumePromise;
          }
        }, 2000);
        this.timeoutTimers.push(timer);
      });
    }
  }

  releaseResumePromise() {
    // first timer cleanup
    try {
      this.timeoutTimers.forEach(clearTimeout);
      clearArray(this.timeoutTimers);
    } catch (error) {
      console.error(error);
    }
    // then resolve
    if (this.resumeResolver) {
      this.resumeResolver();
      delete this.resumeResolver;
    }
    if (this.resumePromise) {
      delete this.resumePromise;
    }
  }
}
