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

  const cxxPluginTest = `
<CsoundSynthesizer>
<CsOptions>
 -odac
</CsOptions>
<CsInstruments>
  0dbfs=1
instr 1
 kcone_lengths[] fillarray 0.0316, 0.051, .3, 0.2
 kradii_in[] fillarray 0.0055, 0.00635, 0.0075, 0.0075
 kradii_out[]  fillarray 0.0055, 0.0075, 0.0075, 0.0275
 kcurve_type[] fillarray 1, 1, 1, 2
 kLength linseg 0.2, 2, 0.3
 kPick_Pos = 1.0
 kEndReflection init 1.0
 kEndReflection = 1.0
 kDensity = 1.0
 kComputeVisco = 0
 aImpulse mpulse .5, .1
 aFeedback, aSound resontube 0.005*aImpulse, kLength, kcone_lengths, kradii_in, kradii_out, kcurve_type, kEndReflection, kDensity, kPick_Pos, kComputeVisco
 out aSound
endin
</CsInstruments>
<CsScore>
i1 0 2
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

  mocha.setup({ ui: "bdd", timeout: 10000 }).fullTrace();

  if (isCI) {
    MochaWebdriverClient.install(mocha);
  }

  const csoundVariations = [
    { useWorker: false, useSPN: false, name: "SINGLE THREAD, AW" },
    { useWorker: false, useSPN: true, name: "SINGLE THREAD, SPN" },
    { useWorker: true, useSAB: true, name: "WORKER, AW, SAB" },
    { useWorker: true, useSAB: false, name: "WORKER, AW, Messageport" },
    { useWorker: true, useSAB: false, useSPN: true, name: "WORKER, SPN, MessagePort" },
  ];

  csoundVariations.forEach((test) => {
    describe(`@csound/browser : ${test.name}`, async function () {
      this.timeout(10000);
      it("can be started", async function () {
        const cs = await Csound(test);
        console.log(`Csound version: ${cs.name}`);
        const startReturn = await cs.start();
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

      // DISABLED due to cxx plugin using source not updated for CS7
      //   it("can load and run c++ plugins", async function () {
      //     const testWithPlugin = Object.assign(
      //       {
      //         withPlugins: ["./plugin_example_cxx.wasm"],
      //       },
      //       test,
      //     );
      //     const cs = await Csound(testWithPlugin);

      //     assert.equal(0, await cs.compileCSD(cxxPluginTest));
      //     await cs.start();
      //     await cs.stop();
      //     await cs.terminateInstance();
      //   });

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

        // assert that the values got delivered
        assert.equal(
          float64array[0],
          await csoundObj.tableGet(1, 0),
          "The first index from table1 matches the first index of the copied array",
        );
        assert.equal(
          float64array[1],
          await csoundObj.tableGet(1, 1),
          "The second index from table1 matches the second index of the copied array",
        );
        assert.equal(
          float64array[2],
          await csoundObj.tableGet(1, 2),
          "The third index from table1 matches the third index of the copied array",
        );
        assert.equal(
          float64array[3],
          await csoundObj.tableGet(1, 3),
          "The fourth index from table1 matches the fourth index of the copied array",
        );

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

     it("can #include a file from parent directory", async function () {
        const csoundObj = await Csound(test);
        const csdPath = "/folder1/test.csd";

        await csoundObj.fs.writeFile("/test.orc", "i1 0 .1");
        await csoundObj.fs.mkdir("/folder1");
        await csoundObj.fs.writeFile(csdPath, `
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
        `);

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

    });
  });

  const triggerEvent = "ontouchstart" in document.documentElement ? "touchend" : "click";
  document.querySelector("#all_tests").addEventListener(triggerEvent, async function () {
    mocha.fullTrace(true);
    mocha.checkLeaks(false); // worker + spn defenitely leaks
    mocha.cleanReferencesAfterRun(true);
    mocha.run();
  });
  if (isCI) {
    mocha.cleanReferencesAfterRun(true);
    mocha.run();
  }
})();
