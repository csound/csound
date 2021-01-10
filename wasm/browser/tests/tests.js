(async () => {
  const url = "/libcsound.dev.mjs";
  const { Csound } = await import(url);

  const helloWorld = `
<CsoundSynthesizer>
<CsOptions>
    -odac
</CsOptions>
<CsInstruments>
    instr 1
    prints "Hello World!\n"
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
    0dbfs = 1

    chnset(1, "test1")
    chnset(2, "test2")

    instr 1
    out poscil(0dbfs/3, 440) * linen:a(1, .01, p3, .01)
    endin
</CsInstruments>
<CsScore>
    i 1 0 1
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

    chnset("test0", "strChannel")

</CsInstruments>
<CsScore>
    i 1 0 1
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

  mocha.setup("bdd").fullTrace();

  const csoundVariations = [
    { useWorker: false, useSPN: false, name: "SINGLE THREAD, AW" },
    { useWorker: false, useSPN: true, name: "SINGLE THREAD, SPN" },
    { useWorker: true, useSAB: true, name: "WORKER, AW, SAB" },
    { useWorker: true, useSAB: false, name: "WORKER, AW, Messageport" },
    { useWorker: true, useSAB: false, useSPN: true, name: "WORKER, SPN, MessagePort" },
  ];

  csoundVariations.forEach((test) => {
    describe(`@csound/browser : ${test.name}`, async () => {
      it("can be started", async function () {
        this.timeout(10000);
        const cs = await Csound(test);
        console.log(`Csound version: ${cs.name}`);
        const startReturn = await cs.start();
        assert.equal(startReturn, 0);
        await cs.stop();
      });

      it("has expected methods", async function () {
        this.timeout(10000);
        const cs = await Csound(test);
        assert.property(cs, "getAudioContext", "has .getAudioContext() method");
        assert.property(cs, "start", "has .start() method");
        assert.property(cs, "stop", "has .stop() method");
        assert.property(cs, "pause", "has .pause() method");
        await cs.stop();
      });

      it("can use run using just compileOrc", async function () {
        this.timeout(10000);
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
      });

      it("can play tone and get channel values", async function () {
        this.timeout(10000);
        const cs = await Csound(test);
        const compileReturn = await cs.compileCsdText(shortTone);
        assert.equal(compileReturn, 0);
        const startReturn = await cs.start();

        assert.equal(startReturn, 0);
        assert.equal(1, await cs.getControlChannel("test1"));
        assert.equal(2, await cs.getControlChannel("test2"));
        await cs.stop();
      });

      it("can play tone and send channel values", async function () {
        this.timeout(10000);
        const cs = await Csound(test);
        const compileReturn = await cs.compileCsdText(shortTone2);
        assert.equal(compileReturn, 0);
        const startReturn = await cs.start();
        assert.equal(startReturn, 0);
        await cs.setControlChannel("freq", 880);
        assert.equal(880, await cs.getControlChannel("freq"));
        await cs.stop();
      });

      it("can send and receive string channel values", async function () {
        this.timeout(10000);
        const cs = await Csound(test);
        const compileReturn = await cs.compileCsdText(stringChannelTest);
        assert.equal(compileReturn, 0);
        const startReturn = await cs.start();
        assert.equal(startReturn, 0);
        assert.equal("test0", await cs.getStringChannel("strChannel"));
        await cs.setStringChannel("strChannel", "test1");
        assert.equal("test1", await cs.getStringChannel("strChannel"));
        await cs.stop();
      });

      it("can load and run plugins", async function () {
        const testWithPlugin = Object.assign(
          {
            withPlugins: ["./plugin_example.wasm"],
          },
          test,
        );
        const cs = await Csound(testWithPlugin);
        assert.equal(0, await cs.compileCsdText(pluginTest));
        await cs.start();
        await cs.stop();
      });

      it("can load and run c++ plugins", async function () {
        const testWithPlugin = Object.assign(
          {
            withPlugins: ["./plugin_example_cxx.wasm"],
          },
          test,
        );
        const cs = await Csound(testWithPlugin);
        assert.equal(0, await cs.compileCsdText(cxxPluginTest));
        await cs.start();
        await cs.stop();
      });

      it("emits public events in realtime performance", async function () {
        this.timeout(10000);
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
        await csoundObj.compileCsdText(shortTone);
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
      });
    });
  });

  const triggerEvent = "ontouchstart" in document.documentElement ? "touchend" : "click";
  document.querySelector("#all_tests").addEventListener(triggerEvent, async function () {
    mocha.fullTrace(true);
    mocha.checkLeaks(true);
    mocha.cleanReferencesAfterRun(true);
    mocha.run();
  });
})();
