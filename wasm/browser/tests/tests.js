/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

(async () => {
  const isCI = ["8081", "8082"].includes(location.port) && location.search.includes("ci=true");
  const url = "/dist/csound.js"; // isCI ? "/csound.esm.js" : "/csound.dev.esm.js";
  const resp = await import(url);
  // console.log("RESP", resp);
  const { Csound } = resp;

  const helloWorld = `
<CsoundSynthesizer>
<CsOptions>
    -odac
</CsOptions>
<CsInstruments>
    instr 1
    prints "Hello World!\\n"
    endin
</CsInstruments>
<CsScore>
    i 1 0 0
</CsScore>
</CsoundSynthesizer>
`;

  const shortTone = `
<CsoundSynthesizer>
<CsOptions>
    -odac
</CsOptions>
<CsInstruments>

    chnset(1, "test1")
    chnset(2, "test2")

    instr 1
    out poscil(0dbfs/3, 440) * linen:a(1, .01, p3, .01)
    endin
</CsInstruments>
<CsScore>
    i 1 0 2
</CsScore>
</CsoundSynthesizer>
`;

  const shortTone2 = `
<CsoundSynthesizer>
<CsOptions>
    -odac
</CsOptions>
<CsInstruments>
    0dbfs = 1

    chnset(440, "freq")

    instr 1
    out poscil(0dbfs/3, chnget:k("freq")) * linen:a(1, .01, p3, .01)
    endin
</CsInstruments>
<CsScore>
    i 1 0 1
</CsScore>
</CsoundSynthesizer>
`;

  const stringChannelTest = `
<CsoundSynthesizer>
<CsOptions>
    -odac
</CsOptions>
<CsInstruments>
    0dbfs = 1

    instr 1
      chnset("test0", "strChannel")
      turnoff
    endin

</CsInstruments>
<CsScore>
    i 1 0 2
    e 2 0
</CsScore>
</CsoundSynthesizer>
`;

  const pluginTest = `
<CsoundSynthesizer>
<CsOptions>
 -odac
</CsOptions>
<CsInstruments>
  0dbfs=1
  instr 1
    i1 = 2
    i2 = 2
    i3 mult i1, i2
    print i3
  endin
  instr 2
    k1 = 2
    k2 = 2
    k3 mult k1, k2
    printk2 k3
  endin
  instr 3
    a1 oscili 0dbfs, 440
    a2 oscili 0dbfs, 356
    a3 mult a1, a2
    out a3
  endin
</CsInstruments>
<CsScore>
  i1 0 0
  i2 0 1
  i3 0 2
  e 0 0
</CsScore>
</CsoundSynthesizer>
`;

  const cppPluginTest = `
<CsoundSynthesizer>
<CsOptions>
 -odac
</CsOptions>
<CsInstruments>
  sr = 44100
  ksmps = 64
  nchnls = 1
  0dbfs = 1

  instr 1
    a1 hello440
  endin
</CsInstruments>
<CsScore>
  i1 0 0.2
</CsScore>
</CsoundSynthesizer>
`;

  const ftableTest = `
<CsoundSynthesizer>
<CsOptions>
    -odac
</CsOptions>
<CsInstruments>
    instr 1
    prints "Hello Fibonnaci!\\n"
    prints "Table length %d\\n", tableng:i(1)
    endin
</CsInstruments>
<CsScore>
    f 1 0 8 -2 0 1 1 2 3 5 8 13
    i 1 0 -1
</CsScore>
</CsoundSynthesizer>
`;

  const samplesTest = `
<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
 Ssample = "tiny_test_sample.wav"
 aRead[] diskin Ssample, 1, 0, 0
 out aRead[0], aRead[0]
endin

instr 2
  aSig monitor
  fout "monitor_out.wav", 4, aSig
endin

</CsInstruments>
<CsScore>
i 2 0 0.3
i 1 0 0.1
i 1 + .
i 1 + .
e
</CsScore>
</CsoundSynthesizer>
`;

  const mp3DiskinTest = `
<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  a1 diskin2 "sine.mp3", 1, 0, 0
  out a1
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
`;

  const missingMp3DiskinTest = `
<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  a1 diskin2 "missing.mp3", 1, 0, 0
  out a1
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
`;

  const collectCsoundMessages = (csoundObj) => {
    const messages = [];
    const listener = (message) => {
      messages.push(String(message));
    };
    csoundObj.on("message", listener);
    return {
      messages,
      stop: () => csoundObj.off("message", listener),
    };
  };

  const findRuntimeAudioOpenErrors = (messages) =>
    messages.filter((message) =>
      /(INIT ERROR|failed to open file|unimplemented format|note deleted)/i.test(message),
    );

  const waitForPerformanceEnd = (csoundObj) =>
    new Promise((resolve) => {
      const onRealtimeEnded = () => {
        csoundObj.off("realtimePerformanceEnded", onRealtimeEnded);
        csoundObj.off("renderEnded", onRenderEnded);
        resolve("realtimePerformanceEnded");
      };
      const onRenderEnded = () => {
        csoundObj.off("realtimePerformanceEnded", onRealtimeEnded);
        csoundObj.off("renderEnded", onRenderEnded);
        resolve("renderEnded");
      };
      csoundObj.on("realtimePerformanceEnded", onRealtimeEnded);
      csoundObj.on("renderEnded", onRenderEnded);
    });

  mocha.setup({ ui: "bdd", timeout: 10000 }).fullTrace();

  if (isCI) {
    MochaWebdriverClient.install(mocha);
  }

  const csoundVariations = [
    { useWorker: false, name: "SINGLE THREAD, AW" },
    { useWorker: true, useSAB: true, name: "WORKER, AW, SAB" },
    { useWorker: true, useSAB: false, name: "WORKER, AW, Messageport" },
  ];

  csoundVariations.forEach((test) => {
    describe(`@csound/browser : ${test.name}`, async function () {
      this.timeout(10000);
      it("can be started", async function () {
        console.log("initialising Csound object");
        const cs = await Csound(test);
        console.log(`Csound version: ${cs.name}`);
        console.log("calling start");
        const startReturn = await cs.start();
        console.log("done calling start");
        assert.equal(startReturn, 0);
        await cs.stop();
        cs.terminateInstance && (await cs.terminateInstance());
      });

      it("has expected methods", async function () {
        const cs = await Csound(test);
        assert.property(cs, "getAudioContext", "has .getAudioContext() method");
        assert.property(cs, "start", "has .start() method");
        assert.property(cs, "stop", "has .stop() method");
        assert.property(cs, "pause", "has .pause() method");
        await cs.stop();
        await cs.terminateInstance();
      });

      it("can use run using just compileOrc", async function () {
        const cs = await Csound(test);
        await cs.compileOrc(`
        ksmps=64
        instr 1
          out oscili(.25, 110)
        endin
        schedule(1,0,1)
      `);
        const startReturn = await cs.start();
        assert.equal(startReturn, 0);
        await cs.stop();
        await cs.terminateInstance();
      });

      it("can play tone and get channel values", async function () {
        const cs = await Csound(test);
        const compileReturn = await cs.compileCSD(shortTone);
        assert.equal(compileReturn, 0);
        const startReturn = await cs.start();
        assert.equal(startReturn, 0);
        assert.equal(1, await cs.getControlChannel("test1"));
        assert.equal(2, await cs.getControlChannel("test2"));
        await cs.stop();
        await cs.terminateInstance();
      });

      it("can play tone and send channel values", async function () {
        const cs = await Csound(test);
        const compileReturn = await cs.compileCSD(shortTone2);
        assert.equal(compileReturn, 0);
        const startReturn = await cs.start();
        assert.equal(startReturn, 0);
        await cs.setControlChannel("freq", 880);
        assert.equal(880, await cs.getControlChannel("freq"));
        await cs.stop();
        await cs.terminateInstance();
      });

      it("can send and receive string channel values", async function () {
        const cs = await Csound(test);
        const compileReturn = await cs.compileCSD(stringChannelTest);
        assert.equal(compileReturn, 0);
        const startReturn = await cs.start();
        assert.equal(startReturn, 0);
        assert.equal("test0", await cs.getStringChannel("strChannel"));
        await cs.setStringChannel("strChannel", "test1");
        assert.equal("test1", await cs.getStringChannel("strChannel"));
        await cs.stop();
        await cs.terminateInstance();
      });

      it("can load and run plugins", async function () {
        const testWithPlugin = Object.assign(
          {
            withPlugins: ["./plugin_example.wasm"],
          },
          test,
        );
        const cs = await Csound(testWithPlugin);
        assert.equal(0, await cs.compileCSD(pluginTest));
        await cs.start();
        await cs.stop();
        await cs.terminateInstance();
      });

      it("can load and run c++ plugins", async function () {
        const testWithPlugin = Object.assign(
          {
            withPlugins: ["./plugin_example_cpp.wasm"],
          },
          test,
        );
        const cs = await Csound(testWithPlugin);

        assert.equal(0, await cs.compileCSD(cppPluginTest));
        await cs.start();
        await cs.stop();
        await cs.terminateInstance();
      });

      it("emits public events in realtime performance", async function () {
        if (test.name !== "WORKER, AW, SAB") {
          const eventPlaySpy = sinon.spy();
          const eventPauseSpy = sinon.spy();
          const eventStopSpy = sinon.spy();
          const eventOnAudioNodeCreatedSpy = sinon.spy();

          const csoundObj = await Csound(test);

          csoundObj.on("play", eventPlaySpy);
          csoundObj.on("pause", eventPauseSpy);
          csoundObj.on("stop", eventStopSpy);
          csoundObj.on("onAudioNodeCreated", eventOnAudioNodeCreatedSpy);

          await csoundObj.setOption("-odac");
          await csoundObj.compileCSD(shortTone);
          await csoundObj.start();
          await csoundObj.pause();
          await csoundObj.resume();
          await csoundObj.stop();

          assert(eventPlaySpy.calledTwice, 'The "play" event was emitted twice');
          assert(eventPauseSpy.calledOnce, 'The "pause" event was emitted once');
          assert(eventStopSpy.calledOnce, 'The "stop" event was emitted once');
          assert(
            eventOnAudioNodeCreatedSpy.calledOnce,
            'The "onAudioNodeCreated" event was emitted once',
          );
          assert(
            eventOnAudioNodeCreatedSpy.calledWith(sinon.match.instanceOf(AudioNode)),
            'The argument provided to the callback of "onAudioNodeCreated" was an AudioNode',
          );
          await csoundObj.terminateInstance();
        }
      });

      it("keeps multi-instance worklets isolated on shared AudioContext", async function () {
        const backendConfig = {
          useWorker: Boolean(test.useWorker),
          useSAB: test.useSAB === true,
        };

        const sharedAudioContext = new (window.AudioContext || window.webkitAudioContext)({
          latencyHint: "interactive",
        });

        let csound1;
        let csound2;
        let node1;
        let node2;

        try {
          csound1 = await Csound({
            useWorker: backendConfig.useWorker,
            useSAB: backendConfig.useSAB,
            audioContext: sharedAudioContext,
            autoConnect: false,
          });
          csound2 = await Csound({
            useWorker: backendConfig.useWorker,
            useSAB: backendConfig.useSAB,
            audioContext: sharedAudioContext,
            autoConnect: false,
          });

          const realtimeOrc = `
          instr 1
            chnset(p4, "last")
            out oscili(.25, p5)
          endin
        `;

          await csound1.setOption("-odac");
          await csound2.setOption("-odac");
          assert.equal(await csound1.compileOrc(realtimeOrc), 0);
          assert.equal(await csound2.compileOrc(realtimeOrc), 0);

          // In worker mode, the AudioWorkletNode is created lazily during start(),
          // so we must initiate start() before awaiting getNode().
          // getNode() returns immediately in singlethread mode, or waits for the
          // onAudioNodeCreated event in worker mode.
          if (backendConfig.useWorker) {
            const [startResult1, startResult2] = await Promise.all([
              csound1.start(),
              csound2.start(),
            ]);

            node1 = await csound1.getNode();
            node2 = await csound2.getNode();
            node1.connect(sharedAudioContext.destination);
            node2.connect(sharedAudioContext.destination);
          } else {
            node1 = await csound1.getNode();
            node2 = await csound2.getNode();
            node1.connect(sharedAudioContext.destination);
            node2.connect(sharedAudioContext.destination);

            await csound1.start();
            await csound2.start();
          }

          await csound1.inputMessage("i1 0 0.2 111 440");
          await csound2.inputMessage("i1 0 0.2 222 550");
          await new Promise((resolve) => setTimeout(resolve, 250));

          assert.closeTo(
            await csound1.getControlChannel("last"),
            111,
            0.001,
            `${test.name}: csound1 should keep its own channel/message state`,
          );
          assert.closeTo(
            await csound2.getControlChannel("last"),
            222,
            0.001,
            `${test.name}: csound2 should keep its own channel/message state`,
          );
        } finally {
          if (node1) {
            node1.disconnect();
          }
          if (node2) {
            node2.disconnect();
          }
          if (csound1) {
            try {
              await csound1.stop();
            } catch {}
          }
          if (csound2) {
            try {
              await csound2.stop();
            } catch {}
          }
          if (csound2 && csound2.terminateInstance) {
            await csound2.terminateInstance();
          }
          if (csound1 && csound1.terminateInstance) {
            await csound1.terminateInstance();
          }
          if (sharedAudioContext && sharedAudioContext.state !== "closed") {
            await sharedAudioContext.close();
          }
        }
      });

      it("does not close shared AudioContext when terminating one instance", async function () {
        const backendConfig = {
          useWorker: Boolean(test.useWorker),
          useSAB: test.useSAB === true,
        };

        const sharedAudioContext = new (window.AudioContext || window.webkitAudioContext)({
          latencyHint: "interactive",
        });

        let csound1;
        let csound2;

        try {
          csound1 = await Csound({
            useWorker: backendConfig.useWorker,
            useSAB: backendConfig.useSAB,
            audioContext: sharedAudioContext,
            autoConnect: false,
          });
          csound2 = await Csound({
            useWorker: backendConfig.useWorker,
            useSAB: backendConfig.useSAB,
            audioContext: sharedAudioContext,
            autoConnect: false,
          });

          assert.notEqual(
            sharedAudioContext.state,
            "closed",
            `${test.name}: shared AudioContext should start open`,
          );

          await csound1.terminateInstance();

          assert.notEqual(
            sharedAudioContext.state,
            "closed",
            `${test.name}: terminating one instance must not close shared AudioContext`,
          );

          await csound2.terminateInstance();

          assert.notEqual(
            sharedAudioContext.state,
            "closed",
            `${test.name}: terminating second instance must not close shared AudioContext`,
          );
        } finally {
          if (csound2 && csound2.terminateInstance) {
            try {
              await csound2.terminateInstance();
            } catch {}
          }
          if (csound1 && csound1.terminateInstance) {
            try {
              await csound1.terminateInstance();
            } catch {}
          }
          if (sharedAudioContext && sharedAudioContext.state !== "closed") {
            await sharedAudioContext.close();
          }
        }
      });

      /* DISABLED due to removed API functions in CS7
      it("can read and write ftables in realtime", async function () {
        const csoundObj = await Csound(test);
        await csoundObj.setOption("-odac");
        await csoundObj.compileCSD(ftableTest);
        await csoundObj.start();
        // assert few indicies
        assert.equal(8, await csoundObj.tableLength(1), "The length of the table counts as 8");
        assert.equal(0, await csoundObj.tableGet(1, 0, "The first index is 0"));
        assert.equal(1, await csoundObj.tableGet(1, 1, "The second index is 1"));
        assert.equal(1, await csoundObj.tableGet(1, 2, "The third index is 2"));
        assert.equal(2, await csoundObj.tableGet(1, 3, "The fourth index is 3"));

        await csoundObj.tableSet(1, 0, 123);
        await csoundObj.tableSet(1, 1, 666);

        assert.equal(123, await csoundObj.tableGet(1, 0, "The first index was modified to 123"));
        assert.equal(666, await csoundObj.tableGet(1, 1, "The second index was modified to 666"));

        await csoundObj.stop();
        await csoundObj.terminateInstance();
      });
    */
      it("can read and write arraybuffers to/from ftables in realtime", async function () {
        const csoundObj = await Csound(test);
        await csoundObj.setOption("-odac");
        await csoundObj.compileCSD(ftableTest);
        await csoundObj.start();

        const tableLength = await csoundObj.tableLength(1);

        // we initialize a float64 typed array
        // using the length of the original csound table
        const float64array = new Float64Array(tableLength);
        // we then fill the arrays with test values
        float64array.set([1, 1.1, 1.01, 1.001]);

        // then we copy the the array from js into csound's runtime onto table 1
        await csoundObj.tableCopyIn(1, float64array);

        const csoundTableOneFloat64 = await csoundObj.tableCopyOut(1);
        // we convert it to normal Array for readability
        const csoundTableOneArray = Array.from(csoundTableOneFloat64);
        assert.deepEqual(
          csoundTableOneArray,
          [1, 1.1, 1.01, 1.001, 0, 0, 0, 0],
          "The current csound table matches the 4 numbers we copied into it followed by 4 empty values (0)",
        );
        await csoundObj.stop();
        await csoundObj.terminateInstance();
      });

      it("can stop() and reset() without start()", async function () {
        const csoundObj = await Csound(test);
        await csoundObj.stop();
        await csoundObj.reset();
        await csoundObj.start();
        await csoundObj.stop();
        await csoundObj.terminateInstance();
      });

      it("can start() -> stop() -> reset() and start again", async function () {
        const csoundObj = await Csound(test);
        await csoundObj.compileCSD(helloWorld);
        await csoundObj.start();
        await csoundObj.stop();
        await csoundObj.reset();
        await csoundObj.compileCSD(helloWorld);
        await csoundObj.start();
        await csoundObj.stop();
        await csoundObj.terminateInstance();
      });

      it("can play a sample, write a sample and read the output file", async function () {
        const csoundObj = await Csound(test);
        const response = await fetch("tiny_test_sample.wav");
        const testSampleArrayBuffer = await response.arrayBuffer();
        const testSample = new Uint8Array(testSampleArrayBuffer);
        await csoundObj.fs.writeFile("tiny_test_sample.wav", testSample);

        // allow the example to play until the end
        const waitUntilEnd = waitForPerformanceEnd(csoundObj);

        assert.include(
          await csoundObj.fs.readdir("/"),
          "tiny_test_sample.wav",
          "The sample was written into the root dir",
        );

        assert.equal(0, await csoundObj.compileCSD(samplesTest), "The test string is valid");
        assert.equal(
          0,
          await csoundObj.start(),
          "Csounds starts normally, indicating the sample was found",
        );

        await waitUntilEnd;
        assert.include(
          await csoundObj.fs.readdir("/"),
          "monitor_out.wav",
          "The sample which csound wrote with fout, is accessible after the end of performance",
        );
        await csoundObj.terminateInstance();
      });

      it("can play an mp3 sample with diskin2", async function () {
        const csoundObj = await Csound(test);
        const response = await fetch("sine.mp3");
        const mp3ArrayBuffer = await response.arrayBuffer();
        const mp3Sample = new Uint8Array(mp3ArrayBuffer);
        await csoundObj.fs.writeFile("sine.mp3", mp3Sample);
        const messageCollector = collectCsoundMessages(csoundObj);

        const waitUntilEnd = waitForPerformanceEnd(csoundObj);

        assert.include(
          await csoundObj.fs.readdir("/"),
          "sine.mp3",
          "The MP3 sample was written into the root dir",
        );

        assert.equal(
          0,
          await csoundObj.compileCSD(mp3DiskinTest),
          "The MP3 diskin2 CSD is valid",
        );
        assert.equal(
          0,
          await csoundObj.start(),
          "Csound starts and can open sine.mp3 with diskin2",
        );
        await waitUntilEnd;
        const runtimeErrors = findRuntimeAudioOpenErrors(messageCollector.messages);
        messageCollector.stop();
        assert.equal(
          runtimeErrors.length,
          0,
          `No runtime diskin2/open-file errors expected.\n${runtimeErrors.join("\n")}`,
        );
        await csoundObj.terminateInstance();
      });

      it("reports runtime diskin2 open errors even when start() returns 0", async function () {
        const csoundObj = await Csound(test);
        const messageCollector = collectCsoundMessages(csoundObj);

        const waitUntilEnd = waitForPerformanceEnd(csoundObj);

        assert.equal(
          0,
          await csoundObj.compileCSD(missingMp3DiskinTest),
          "Missing-file diskin2 CSD should compile",
        );

        const startReturn = await csoundObj.start();
        await waitUntilEnd;
        const runtimeErrors = findRuntimeAudioOpenErrors(messageCollector.messages);
        messageCollector.stop();

        assert(
          startReturn !== 0 || runtimeErrors.length > 0,
          `Expected start failure or runtime init errors.\nstart() returned: ${startReturn}\n` +
            `messages:\n${messageCollector.messages.join("\n")}`,
        );
        assert(
          runtimeErrors.length > 0,
          `Expected a runtime diskin2 open error for missing.mp3.\n` +
            `messages:\n${messageCollector.messages.join("\n")}`,
        );
        await csoundObj.terminateInstance();
      });

      it("can play a csd from a nested filesystem directory, with code requiring a sample", async function () {
        const csoundObj = await Csound(test);
        const response = await fetch("/tiny_test_sample.wav");
        const testSampleArrayBuffer = await response.arrayBuffer();
        const testSample = new Uint8Array(testSampleArrayBuffer);

        // Writing the csd to disk
        const csdPath = "/anycsd.csd";
        await csoundObj.fs.mkdir("/somedir");
        await csoundObj.fs.writeFile("tiny_test_sample.wav", testSample);
        await csoundObj.fs.writeFile(csdPath, samplesTest);

        // allow the example to play until the end
        let endResolver;
        const waitUntilEnd = new Promise((resolve) => {
          endResolver = resolve;
        });
        csoundObj.on("realtimePerformanceEnded", endResolver);

        assert.include(
          await csoundObj.fs.readdir("/"),
          "tiny_test_sample.wav",
          "The sample was written into the root dir",
        );

        assert.equal(0, await csoundObj.compileCSD(csdPath, 0), "The test Csd is valid");
        assert.equal(
          0,
          await csoundObj.start(),
          "Csounds starts normally, indicating the sample was found",
        );

        await waitUntilEnd;
        assert.include(
          await csoundObj.fs.readdir("/"),
          "monitor_out.wav",
          "The sample which csound wrote with fout, is accessible after the end of performance",
        );
        await csoundObj.terminateInstance();
      });

      it("can use fs.stat() to get file information", async function () {
        const csoundObj = await Csound(test);
        const response = await fetch("tiny_test_sample.wav");
        const testSampleArrayBuffer = await response.arrayBuffer();
        const testSample = new Uint8Array(testSampleArrayBuffer);

        // Write a test file
        await csoundObj.fs.writeFile("test_file.txt", new TextEncoder().encode("Hello World"));
        await csoundObj.fs.writeFile("tiny_test_sample.wav", testSample);

        // Test stat on existing file
        const fileStat = await csoundObj.fs.stat("test_file.txt");
        assert.isObject(fileStat, "stat returns an object for existing file");
        assert.property(fileStat, "size", "stat object has size property");
        assert.property(fileStat, "isFile", "stat object has isFile property");
        assert.property(fileStat, "isDirectory", "stat object has isDirectory property");
        assert.equal(fileStat.size, 11, "file size matches expected length");
        assert.isTrue(fileStat.isFile, "isFile returns true for file");
        assert.isFalse(fileStat.isDirectory, "isDirectory returns false for file");

        // Test stat on binary file
        const binaryStat = await csoundObj.fs.stat("tiny_test_sample.wav");
        assert.isObject(binaryStat, "stat returns an object for binary file");
        assert.equal(binaryStat.size, testSample.length, "binary file size matches");

        // Test stat on non-existing file
        const nonExistentStat = await csoundObj.fs.stat("non_existent_file.txt");
        assert.isUndefined(nonExistentStat, "stat returns undefined for non-existent file");

        await csoundObj.terminateInstance();
      });

      it("can use fs.pathExists() to check file existence", async function () {
        const csoundObj = await Csound(test);

        // Test non-existing file
        const nonExistentExists = await csoundObj.fs.pathExists("non_existent_file.txt");
        assert.isFalse(nonExistentExists, "pathExists returns false for non-existent file");

        // Write a test file
        await csoundObj.fs.writeFile("test_file.txt", new TextEncoder().encode("Hello World"));

        // Test existing file
        const existingFileExists = await csoundObj.fs.pathExists("test_file.txt");
        assert.isTrue(existingFileExists, "pathExists returns true for existing file");

        // Test with directory
        await csoundObj.fs.mkdir("test_directory");
        const directoryExists = await csoundObj.fs.pathExists("test_directory");
        assert.isTrue(directoryExists, "pathExists returns true for existing directory");

        // Test with nested path
        await csoundObj.fs.writeFile(
          "test_directory/nested_file.txt",
          new TextEncoder().encode("Nested"),
        );
        const nestedFileExists = await csoundObj.fs.pathExists("test_directory/nested_file.txt");
        assert.isTrue(nestedFileExists, "pathExists returns true for nested file");

        await csoundObj.terminateInstance();
      });

      it("can use fs.stat() and fs.pathExists() together for file operations", async function () {
        const csoundObj = await Csound(test);

        const fileName = "combined_test_file.txt";
        const fileContent = "This is a test file for combined operations";

        // Initially file should not exist
        assert.isFalse(await csoundObj.fs.pathExists(fileName), "File does not exist initially");
        assert.isUndefined(
          await csoundObj.fs.stat(fileName),
          "stat returns undefined for non-existent file",
        );

        // Write file
        await csoundObj.fs.writeFile(fileName, new TextEncoder().encode(fileContent));

        // Now file should exist
        assert.isTrue(await csoundObj.fs.pathExists(fileName), "File exists after writing");

        const stat = await csoundObj.fs.stat(fileName);
        assert.isObject(stat, "stat returns object after file creation");
        assert.equal(stat.size, fileContent.length, "file size matches content length");
        assert.isTrue(stat.isFile, "stat correctly identifies file");
        assert.isFalse(stat.isDirectory, "stat correctly identifies not directory");

        // Remove file
        await csoundObj.fs.unlink(fileName);

        // File should no longer exist
        assert.isFalse(
          await csoundObj.fs.pathExists(fileName),
          "File does not exist after unlinking",
        );
        assert.isUndefined(
          await csoundObj.fs.stat(fileName),
          "stat returns undefined after unlinking",
        );

        await csoundObj.terminateInstance();
      });

      it("does not leave stale entries after unlink and repeated writeFile", async function () {
        const csoundObj = await Csound(test);

        const deletePath = "/deleteme";
        const testPath = "/test";

        if (await csoundObj.fs.pathExists(deletePath)) {
          await csoundObj.fs.unlink(deletePath);
        }
        if (await csoundObj.fs.pathExists(testPath)) {
          await csoundObj.fs.unlink(testPath);
        }

        await csoundObj.fs.writeFile(deletePath, new Uint8Array([0]));
        await csoundObj.fs.unlink(deletePath);

        const rootAfterUnlink = await csoundObj.fs.readdir("/");
        assert.notInclude(
          rootAfterUnlink,
          "deleteme",
          `unlink should remove ${deletePath}; got ${JSON.stringify(rootAfterUnlink)}`,
        );

        for (let x = 0; x < 6; x += 1) {
          await csoundObj.fs.writeFile(testPath, new Uint8Array([99 + x]));
        }

        const rootAfterRepeatedWrites = await csoundObj.fs.readdir("/");
        const testEntries = rootAfterRepeatedWrites.filter((entry) => entry === "test").length;
        assert.equal(
          testEntries,
          1,
          `repeated writeFile should not duplicate entries; got ${JSON.stringify(rootAfterRepeatedWrites)}`,
        );

        const finalContent = await csoundObj.fs.readFile(testPath);
        assert.equal(
          finalContent[0],
          104,
          `expected final file content from last write (104); got ${finalContent[0]}`,
        );

        await csoundObj.terminateInstance();
      });

      it("can #include a file from parent directory", async function () {
        const csoundObj = await Csound(test);
        const csdPath = "/folder1/test.csd";

        await csoundObj.fs.writeFile("/test.orc", "i1 0 .1");
        await csoundObj.fs.mkdir("/folder1");
        await csoundObj.fs.writeFile(
          csdPath,
          `
<CsoundSynthesizer>
<CsOptions>
    -odac
</CsOptions>
<CsInstruments>
    sr=44100
    ksmps=64
    nchnls=1
    0dbfs=1

    instr 1
        out(oscili(0.25, 440))
    endin

</CsInstruments>
<CsScore>
    #include "../test.orc"
</CsScore>
</CsoundSynthesizer>
        `,
        );

        // allow the example to play until the end
        let endResolver;
        const waitUntilEnd = new Promise((resolve) => {
          endResolver = resolve;
        });
        csoundObj.on("realtimePerformanceEnded", endResolver);

        assert.equal(0, await csoundObj.compileCSD(csdPath, 0), "The test Csd is valid");
        assert.equal(
          0,
          await csoundObj.start(),
          "Csounds starts normally, indicating the sample was found",
        );

        await waitUntilEnd;
        await csoundObj.terminateInstance();
      });

      it("fs operations work after unlink creates holes in fd table", async function () {
        const csoundObj = await Csound(test);

        // Create files, then unlink one to create a hole (undefined entry) in the fd array
        await csoundObj.fs.writeFile("file_a.txt", new TextEncoder().encode("AAA"));
        await csoundObj.fs.writeFile("file_b.txt", new TextEncoder().encode("BBB"));
        await csoundObj.fs.writeFile("file_c.txt", new TextEncoder().encode("CCC"));

        // Unlink the middle file — this creates an undefined slot in this.fd
        await csoundObj.fs.unlink("file_b.txt");

        // All of these iterate Object.values(this.fd) and must handle the hole:

        // readdir should still work
        const files = await csoundObj.fs.readdir("/");
        assert.include(files, "file_a.txt", "readdir lists file_a.txt after unlink");
        assert.include(files, "file_c.txt", "readdir lists file_c.txt after unlink");
        assert.notInclude(files, "file_b.txt", "readdir does not list unlinked file_b.txt");

        // writeFile should still work (its find for existing files must not crash)
        await csoundObj.fs.writeFile("file_d.txt", new TextEncoder().encode("DDD"));
        assert.include(
          await csoundObj.fs.readdir("/"),
          "file_d.txt",
          "writeFile works after unlink",
        );

        // stat should still work
        const statA = await csoundObj.fs.stat("file_a.txt");
        assert.isObject(statA, "stat works for file_a.txt after unlink");
        assert.equal(statA.size, 3, "stat returns correct size");
        const statB = await csoundObj.fs.stat("file_b.txt");
        assert.isUndefined(statB, "stat returns undefined for unlinked file");

        // pathExists should still work
        assert.isTrue(
          await csoundObj.fs.pathExists("file_a.txt"),
          "pathExists returns true for existing file after unlink",
        );
        assert.isFalse(
          await csoundObj.fs.pathExists("file_b.txt"),
          "pathExists returns false for unlinked file",
        );

        // mkdir should still work
        await csoundObj.fs.mkdir("new_dir");
        assert.isTrue(
          await csoundObj.fs.pathExists("new_dir"),
          "mkdir works after unlink creates holes in fd table",
        );

        await csoundObj.terminateInstance();
      });

      it("it fails with error when #include references a non-existent file", async function () {
        const csoundObj = await Csound(test);
        await csoundObj.fs.writeFile(
          "/test.csd",
          `
<CsoundSynthesizer>
<CsOptions>
    -odac
</CsOptions>
<CsInstruments>
    sr=44100
    ksmps=64
    nchnls=1
    0dbfs=1

    instr 1
        out(oscili(0.25, 440))
    endin

</CsInstruments>
<CsScore>
    #include "../test.orc"
    i1 0 0.01
</CsScore>
</CsoundSynthesizer>
        `,
        );

        // Attempting to compile should fail with a normal Csound error code
        // and must not crash the WASM runtime.
        let compileResult = 0;
        let thrownError = null;
        try {
          compileResult = await csoundObj.compileCSD("/test.csd", 0);
        } catch (e) {
          thrownError = e;
        }

        assert.equal(
          null,
          thrownError,
          `compileCSD should not crash when include file is missing: ${thrownError?.message}`,
        );
        assert.notEqual(0, compileResult, "compileCSD should fail for missing #include file");
        await csoundObj.terminateInstance();
      });
    });
  });

  /* ================================================================
   * libcsound + UGEN API tests (no WebAudio node)
   * ================================================================ */
  describe.only("libcsound (no WebAudio)", function () {
    this.timeout(30000);

    /** Shared libcsound API instance (one wasm load for all tests) */
    let cs;

    before(async function () {
      console.log("[libcsound tests] before: importing module…");
      const { libcsound } = await import(url);
      console.log("[libcsound tests] before: calling libcsound()…");
      cs = await libcsound();
      console.log("[libcsound tests] before: done, cs =", cs);
    });

    /** Helper: create a started csound instance with common options */
    function makeStartedCsound() {
      const csound = cs.csoundCreate();
      cs.csoundSetOption(csound, "-d");
      cs.csoundSetOption(csound, "-n");
      cs.csoundSetOption(csound, "--nchnls=1");
      cs.csoundSetOption(csound, "--0dbfs=1");
      cs.csoundStart(csound);
      return csound;
    }

    it("can instantiate via libcsound()", function () {
      assert.isObject(cs, "libcsound() returns an object");
      assert.property(cs, "csoundCreate");
      assert.property(cs, "csoundDestroy");
      assert.property(cs, "csoundSetOption");
      assert.property(cs, "csoundCompileOrc");
      assert.property(cs, "csoundStart");
      assert.property(cs, "csoundPerformKsmps");
      assert.property(cs, "csoundStop");
      assert.property(cs, "csoundReset");
      assert.property(cs, "wasm");
      assert.property(cs, "getMemory");
    });

    it("can create and run a csound instance", function () {
      const csound = cs.csoundCreate();
      assert.notEqual(csound, 0, "csoundCreate returns non-zero pointer");
      assert.equal(0, cs.csoundSetOption(csound, "-d"));
      assert.equal(0, cs.csoundSetOption(csound, "-n"));
      assert.equal(0, cs.csoundSetOption(csound, "--nchnls=1"));
      assert.equal(0, cs.csoundSetOption(csound, "--0dbfs=1"));
      assert.equal(
        0,
        cs.csoundCompileOrc(
          csound,
          "instr 1\n  out oscili(0.5, 440)\nendin\nschedule(1,0,0.01)",
        ),
      );
      assert.equal(0, cs.csoundStart(csound));
      // Perform a few k-cycles
      for (let i = 0; i < 10; i++) {
        cs.csoundPerformKsmps(csound);
      }
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("exposes UGEN_ARG_TYPE enum", function () {
      assert.isObject(cs.UGEN_ARG_TYPE, "UGEN_ARG_TYPE is an object");
      assert.equal(cs.UGEN_ARG_TYPE.I, 0);
      assert.equal(cs.UGEN_ARG_TYPE.K, 1);
      assert.equal(cs.UGEN_ARG_TYPE.A, 2);
      assert.equal(cs.UGEN_ARG_TYPE.S, 3);
      assert.equal(cs.UGEN_ARG_TYPE.F, 4);
      assert.equal(cs.UGEN_ARG_TYPE.UNKNOWN, 5);
    });

    it("can create and delete a UGEN factory", function () {
      const csound = makeStartedCsound();

      const factory = cs.csoundUgenFactoryNew(csound);
      assert.notEqual(factory, 0, "factory is non-zero");

      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("can list opcodes", function () {
      const csound = makeStartedCsound();

      const factory = cs.csoundUgenFactoryNew(csound);
      const opcodes = cs.csoundUgenListOpcodes(factory);
      assert.isArray(opcodes, "listOpcodes returns an array");
      assert.isAbove(opcodes.length, 0, "opcodes list is non-empty");

      // Each entry should have opname, outypes, intypes
      const first = opcodes[0];
      assert.property(first, "opname");
      assert.property(first, "outypes");
      assert.property(first, "intypes");

      // Check that we can find well-known opcodes.
      // Note: polymorphic opcodes appear with suffixes (e.g. "oscili.a")
      const oscNames = opcodes.map((o) => o.opname);
      assert.isTrue(
        oscNames.some((n) => n === "oscils"),
        "oscils is in opcode list",
      );
      assert.isTrue(
        oscNames.some((n) => n === "vco2"),
        "vco2 is in opcode list",
      );

      // Verify oscils has correct types
      const oscils = opcodes.find((o) => o.opname === "oscils");
      assert.equal(oscils.outypes, "a", "oscils outypes");
      assert.equal(oscils.intypes, "iiio", "oscils intypes");

      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("can find a specific opcode", function () {
      const csound = makeStartedCsound();

      const factory = cs.csoundUgenFactoryNew(csound);
      // csoundUgenFindOpcode returns bool: true(1)=found, false(0)=not found
      // Must use exact OENTRY types: oscils has outypes="a", intypes="iiio"
      const found = cs.csoundUgenFindOpcode(factory, "oscils", "a", "iiio");
      assert.notEqual(found, 0, "findOpcode returns true for oscils/a/iiio");

      const notFound = cs.csoundUgenFindOpcode(factory, "not_an_opcode", "a", "kk");
      assert.equal(notFound, 0, "findOpcode returns false for unknown opcode");

      // Wrong types for an existing opcode should also return false
      const wrongTypes = cs.csoundUgenFindOpcode(factory, "oscils", "k", "kk");
      assert.equal(wrongTypes, 0, "findOpcode returns false for wrong types");

      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("can create a UGEN and query argument counts/types", function () {
      const csound = makeStartedCsound();

      const factory = cs.csoundUgenFactoryNew(csound);
      // oscils: a oscils iAmp, iFreq, iPhs [, iOpt]
      // Exact OENTRY types: outypes="a", intypes="iiio"
      const osc = cs.csoundUgenNew(factory, "oscils", "a", "iiio");
      assert.notEqual(osc, 0, "csoundUgenNew returns non-zero for valid opcode");

      assert.equal(cs.csoundUgenGetOutCount(osc), 1, "oscils has 1 output");
      assert.equal(cs.csoundUgenGetInCount(osc), 4, "oscils has 4 inputs (iiio)");

      assert.equal(cs.csoundUgenGetOutType(osc, 0), cs.UGEN_ARG_TYPE.A, "output 0 is A-rate");
      assert.equal(cs.csoundUgenGetInType(osc, 0), cs.UGEN_ARG_TYPE.I, "input 0 is I-rate");
      assert.equal(cs.csoundUgenGetInType(osc, 1), cs.UGEN_ARG_TYPE.I, "input 1 is I-rate");
      assert.equal(cs.csoundUgenGetInType(osc, 2), cs.UGEN_ARG_TYPE.I, "input 2 is I-rate");
      assert.equal(cs.csoundUgenGetInType(osc, 3), cs.UGEN_ARG_TYPE.I, "input 3 is I-rate (optional)");

      cs.csoundUgenDelete(osc);
      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("can set/get scalar values on UGEN inputs/outputs", function () {
      const csound = makeStartedCsound();

      const factory = cs.csoundUgenFactoryNew(csound);
      // oscils: outypes="a", intypes="iiio"
      const osc = cs.csoundUgenNew(factory, "oscils", "a", "iiio");

      // Set amplitude, frequency, phase
      cs.csoundUgenSetValue(osc, 0, 0.5);
      cs.csoundUgenSetValue(osc, 1, 440.0);
      cs.csoundUgenSetValue(osc, 2, 0.0);

      // csoundUgenGetValue reads OUTPUTS; to read back INPUTS use csoundUgenGetInVar + csoundUgenVarGetValue
      const inVar0 = cs.csoundUgenGetInVar(osc, 0);
      const inVar1 = cs.csoundUgenGetInVar(osc, 1);
      const inVar2 = cs.csoundUgenGetInVar(osc, 2);
      assert.notEqual(inVar0, 0, "inVar0 pointer valid");
      assert.notEqual(inVar1, 0, "inVar1 pointer valid");
      assert.notEqual(inVar2, 0, "inVar2 pointer valid");
      assert.closeTo(cs.csoundUgenVarGetValue(inVar0), 0.5, 1e-6, "amplitude read back");
      assert.closeTo(cs.csoundUgenVarGetValue(inVar1), 440.0, 1e-6, "frequency read back");
      assert.closeTo(cs.csoundUgenVarGetValue(inVar2), 0.0, 1e-6, "phase read back");

      cs.csoundUgenDelete(osc);
      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("csoundUgenGetValue reads output after init/perform", function () {
      const csound = makeStartedCsound();
      const factory = cs.csoundUgenFactoryNew(csound);

      // --- oscils (a-rate output): GetValue returns first audio sample ---
      const osc = cs.csoundUgenNew(factory, "oscils", "a", "iiio");
      assert.notEqual(osc, 0, "oscils created");

      cs.csoundUgenSetValue(osc, 0, 1.0);    // amp
      cs.csoundUgenSetValue(osc, 1, 1000.0); // freq
      cs.csoundUgenSetValue(osc, 2, 0.25);   // phase — quarter-cycle so sin(π/2)≈1

      assert.equal(0, cs.csoundUgenInit(osc));
      assert.equal(0, cs.csoundUgenPerform(osc));

      // GetValue on output 0 should return first sample near 1.0
      const outA = cs.csoundUgenGetValue(osc, 0);
      assert.closeTo(outA, 1.0, 0.05, "oscils first sample with phase=0.25");

      // Consistency: same as UgenVar
      const outVar = cs.csoundUgenGetOutVar(osc, 0);
      assert.notEqual(outVar, 0);
      assert.closeTo(
        cs.csoundUgenGetValue(osc, 0),
        cs.csoundUgenVarGetValue(outVar),
        1e-10,
        "GetValue == VarGetValue"
      );

      // --- line (k-rate output): GetValue returns a scalar ---
      const ln = cs.csoundUgenNew(factory, "line", "k", "iii");
      assert.notEqual(ln, 0, "line created");

      // Start at 1.0, ramp to 0.0 — first output should be near 1.0
      cs.csoundUgenSetValue(ln, 0, 1.0);  // ia
      cs.csoundUgenSetValue(ln, 1, 1.0);  // dur
      cs.csoundUgenSetValue(ln, 2, 0.0);  // ib

      assert.equal(0, cs.csoundUgenInit(ln));
      assert.equal(0, cs.csoundUgenPerform(ln));

      const outK = cs.csoundUgenGetValue(ln, 0);
      assert.isAbove(outK, 0.0, "line output > 0 after one k-cycle");
      assert.isAtMost(outK, 1.0, "line output <= 1.0");

      // --- edge cases ---
      // Out-of-range index returns 0
      assert.equal(cs.csoundUgenGetValue(osc, 99), 0.0, "out-of-range returns 0");

      // Before perform: output is zero
      const osc2 = cs.csoundUgenNew(factory, "oscils", "a", "iiio");
      cs.csoundUgenSetValue(osc2, 0, 1.0);
      cs.csoundUgenSetValue(osc2, 1, 440.0);
      cs.csoundUgenSetValue(osc2, 2, 0.0);
      assert.equal(cs.csoundUgenGetValue(osc2, 0), 0.0, "before perform output is 0");

      cs.csoundUgenDelete(osc2);
      cs.csoundUgenDelete(osc);
      cs.csoundUgenDelete(ln);
      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("can init and perform a UGEN", function () {
      const csound = makeStartedCsound();

      const factory = cs.csoundUgenFactoryNew(csound);
      // oscils: outypes="a", intypes="iiio"
      const osc = cs.csoundUgenNew(factory, "oscils", "a", "iiio");

      cs.csoundUgenSetValue(osc, 0, 0.5);   // amp
      cs.csoundUgenSetValue(osc, 1, 440.0);  // freq
      cs.csoundUgenSetValue(osc, 2, 0.0);   // phase

      assert.equal(0, cs.csoundUgenInit(osc), "init returns 0");
      assert.equal(0, cs.csoundUgenPerform(osc), "perform returns 0");

      // Get audio output as Float64Array
      const outVar = cs.csoundUgenGetOutVar(osc, 0);
      assert.notEqual(outVar, 0, "output var pointer is valid");

      const samples = cs.csoundUgenVarGetFloat64Array(outVar);
      assert.instanceOf(samples, Float64Array, "returns Float64Array");
      assert.isAbove(samples.length, 0, "samples array is non-empty");

      // After perform, some samples should be non-zero (oscillator output)
      const hasNonZero = samples.some((s) => Math.abs(s) > 1e-10);
      assert.isTrue(hasNonZero, "oscillator produces non-zero output");

      cs.csoundUgenDelete(osc);
      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("can wire UGENs together via UGEN_VAR", function () {
      const csound = makeStartedCsound();

      const factory = cs.csoundUgenFactoryNew(csound);

      // Create oscillator: a oscils iAmp, iFreq, iPhs [, iOpt]
      const osc = cs.csoundUgenNew(factory, "oscils", "a", "iiio");
      cs.csoundUgenSetValue(osc, 0, 0.5);   // amp
      cs.csoundUgenSetValue(osc, 1, 440.0); // freq
      cs.csoundUgenSetValue(osc, 2, 0.0);   // phase

      // Create gain multiplier: a product y (y = variable number of a-rate args)
      const gain = cs.csoundUgenNew(factory, "product", "a", "y");

      // Wire oscillator output -> gain input
      const oscOut = cs.csoundUgenGetOutVar(osc, 0);
      cs.csoundUgenSetInputVar(gain, 0, oscOut);

      // Set gain factor via a standalone a-rate var
      const gainVar = cs.csoundUgenVarNew(factory, cs.UGEN_ARG_TYPE.A);
      assert.notEqual(gainVar, 0, "standalone var is valid");
      cs.csoundUgenVarSetValue(gainVar, 0.25);
      cs.csoundUgenSetInputVar(gain, 1, gainVar);

      // Init and perform both
      assert.equal(0, cs.csoundUgenInit(osc));
      assert.equal(0, cs.csoundUgenInit(gain));
      assert.equal(0, cs.csoundUgenPerform(osc));
      assert.equal(0, cs.csoundUgenPerform(gain));

      // Check output of gain
      const gainOutVar = cs.csoundUgenGetOutVar(gain, 0);
      const gainSamples = cs.csoundUgenVarGetFloat64Array(gainOutVar);
      assert.instanceOf(gainSamples, Float64Array);
      assert.isAbove(gainSamples.length, 0);

      cs.csoundUgenVarDelete(gainVar);
      cs.csoundUgenDelete(osc);
      cs.csoundUgenDelete(gain);
      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("can use UGEN_VAR get/set for i/k scalars", function () {
      const csound = makeStartedCsound();

      const factory = cs.csoundUgenFactoryNew(csound);

      // Create a standalone K-rate var and set/get its value
      const kVar = cs.csoundUgenVarNew(factory, cs.UGEN_ARG_TYPE.K);
      assert.notEqual(kVar, 0, "kVar is valid");
      assert.equal(cs.csoundUgenVarGetType(kVar), cs.UGEN_ARG_TYPE.K, "type is K");

      cs.csoundUgenVarSetValue(kVar, 123.456);
      assert.closeTo(cs.csoundUgenVarGetValue(kVar), 123.456, 1e-3, "value read-back");

      // Create an I-rate var
      const iVar = cs.csoundUgenVarNew(factory, cs.UGEN_ARG_TYPE.I);
      assert.equal(cs.csoundUgenVarGetType(iVar), cs.UGEN_ARG_TYPE.I, "type is I");
      cs.csoundUgenVarSetValue(iVar, 42.0);
      assert.closeTo(cs.csoundUgenVarGetValue(iVar), 42.0, 1e-6, "iVar value");

      cs.csoundUgenVarDelete(kVar);
      cs.csoundUgenVarDelete(iVar);
      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("can use UGEN context for instrument-like state", function () {
      const csound = makeStartedCsound();

      const factory = cs.csoundUgenFactoryNew(csound);

      const ctx = cs.csoundUgenContextNew(factory);
      assert.notEqual(ctx, 0, "context is valid");

      const osc = cs.csoundUgenNew(factory, "oscils", "a", "iiio");
      cs.csoundUgenSetContext(osc, ctx);
      cs.csoundUgenSetValue(osc, 0, 0.5);   // amp
      cs.csoundUgenSetValue(osc, 1, 440.0); // freq
      cs.csoundUgenSetValue(osc, 2, 0.0);   // phase
      assert.equal(0, cs.csoundUgenInit(osc));
      assert.equal(0, cs.csoundUgenPerform(osc));

      cs.csoundUgenDelete(osc);
      cs.csoundUgenContextDelete(ctx);
      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("can use UGEN graph for ordered init/perform", function () {
      const csound = makeStartedCsound();

      const factory = cs.csoundUgenFactoryNew(csound);

      // Build a simple graph: oscils -> product (gain control)
      const osc = cs.csoundUgenNew(factory, "oscils", "a", "iiio");
      cs.csoundUgenSetValue(osc, 0, 0.5);   // amp
      cs.csoundUgenSetValue(osc, 1, 440.0); // freq
      cs.csoundUgenSetValue(osc, 2, 0.0);   // phase

      const gain = cs.csoundUgenNew(factory, "product", "a", "y");
      const oscOut = cs.csoundUgenGetOutVar(osc, 0);
      cs.csoundUgenSetInputVar(gain, 0, oscOut);

      // Create a constant gain var
      const gainVar = cs.csoundUgenVarNew(factory, cs.UGEN_ARG_TYPE.A);
      cs.csoundUgenVarSetValue(gainVar, 0.5);
      cs.csoundUgenSetInputVar(gain, 1, gainVar);

      // Build graph
      const graph = cs.csoundUgenGraphNew(factory);
      assert.notEqual(graph, 0, "graph is valid");
      cs.csoundUgenGraphAdd(graph, osc);
      cs.csoundUgenGraphAdd(graph, gain);

      // Init and perform the entire graph at once
      assert.equal(0, cs.csoundUgenGraphInit(graph));
      assert.equal(0, cs.csoundUgenGraphPerform(graph));

      // Verify oscils output has non-zero samples after graph perform
      // (product with A-rate gainVar set via scalar won't produce non-zero
      // because csoundUgenVarSetValue only sets data[0] and sin(0)=0)
      const oscOutSamples = cs.csoundUgenVarGetFloat64Array(oscOut);
      assert.instanceOf(oscOutSamples, Float64Array);
      assert.isAbove(oscOutSamples.length, 0);
      const hasNonZero = oscOutSamples.some((s) => Math.abs(s) > 1e-10);
      assert.isTrue(hasNonZero, "graph produces non-zero output from oscils");

      // Perform a second block
      assert.equal(0, cs.csoundUgenGraphPerform(graph));

      cs.csoundUgenVarDelete(gainVar);
      cs.csoundUgenGraphDeleteAll(graph);
      cs.csoundUgenFactoryDelete(factory);
      cs.csoundStop(csound);
      cs.csoundDestroy(csound);
    });

    it("supports multiple independent libcsound instances", async function () {
      // This test needs its own, second wasm instance
      const { libcsound } = await import(url);
      const cs2 = await libcsound();

      const csound1 = cs.csoundCreate();
      const csound2 = cs2.csoundCreate();

      cs.csoundSetOption(csound1, "-d");
      cs.csoundSetOption(csound1, "-n");
      cs.csoundSetOption(csound1, "--0dbfs=1");
      cs2.csoundSetOption(csound2, "-d");
      cs2.csoundSetOption(csound2, "-n");
      cs2.csoundSetOption(csound2, "--0dbfs=1");

      cs.csoundStart(csound1);
      cs2.csoundStart(csound2);

      // Each instance should have its own UGEN factory
      const factory1 = cs.csoundUgenFactoryNew(csound1);
      const factory2 = cs2.csoundUgenFactoryNew(csound2);
      assert.notEqual(factory1, 0);
      assert.notEqual(factory2, 0);

      // Create a UGEN in each and verify isolation
      // oscils: outypes="a", intypes="iiio"
      const osc1 = cs.csoundUgenNew(factory1, "oscils", "a", "iiio");
      const osc2 = cs2.csoundUgenNew(factory2, "oscils", "a", "iiio");
      cs.csoundUgenSetValue(osc1, 0, 0.5);
      cs.csoundUgenSetValue(osc1, 1, 440.0);
      cs.csoundUgenSetValue(osc1, 2, 0.0);
      cs2.csoundUgenSetValue(osc2, 0, 0.25);
      cs2.csoundUgenSetValue(osc2, 1, 880.0);
      cs2.csoundUgenSetValue(osc2, 2, 0.0);

      // Values should be independent — read back inputs via csoundUgenGetInVar
      const in1_0 = cs.csoundUgenGetInVar(osc1, 0);
      const in2_0 = cs2.csoundUgenGetInVar(osc2, 0);
      const in1_1 = cs.csoundUgenGetInVar(osc1, 1);
      const in2_1 = cs2.csoundUgenGetInVar(osc2, 1);
      assert.closeTo(cs.csoundUgenVarGetValue(in1_0), 0.5, 1e-6);
      assert.closeTo(cs2.csoundUgenVarGetValue(in2_0), 0.25, 1e-6);
      assert.closeTo(cs.csoundUgenVarGetValue(in1_1), 440.0, 1e-6);
      assert.closeTo(cs2.csoundUgenVarGetValue(in2_1), 880.0, 1e-6);

      cs.csoundUgenDelete(osc1);
      cs2.csoundUgenDelete(osc2);
      cs.csoundUgenFactoryDelete(factory1);
      cs2.csoundUgenFactoryDelete(factory2);
      cs.csoundStop(csound1);
      cs2.csoundStop(csound2);
      cs.csoundDestroy(csound1);
      cs2.csoundDestroy(csound2);
    });
  });

  const triggerEvent = "ontouchstart" in document.documentElement ? "touchend" : "click";
  document.querySelector("#all_tests").addEventListener(triggerEvent, async function () {
    mocha.fullTrace(true);
    mocha.checkLeaks(false); // worker definitely leaks
    mocha.cleanReferencesAfterRun(true);
    mocha.run();
  });
  if (isCI) {
    mocha.cleanReferencesAfterRun(true);
    mocha.run();
  }
})();
