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
