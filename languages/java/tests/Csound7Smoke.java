import csnd7.Csound;
import csnd7.CsoundCallbackWrapper;
import csnd7.CsoundMYFLTArray;
import csnd7.CsoundPerformanceThread;
import csnd7.csnd7;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public final class Csound7Smoke {
    private static final String ORC =
        "sr=48000\nksmps=32\nnchnls=1\n0dbfs=1\n" +
        "instr 1\n" +
        "kFrequency chnget \"frequency\"\n" +
        "aSignal poscil 0.2, kFrequency\n" +
        "out aSignal\nendin\n";

    private static void require(boolean ok, String message) {
        if (!ok) throw new AssertionError(message);
    }

    private static final class Messages extends CsoundCallbackWrapper {
        volatile int count;
        Messages(Csound cs) { super(cs); }
        @Override public void MessageCallback(int attr, String text) {
            count++;
        }
    }

    private static void renderAndReset() {
        Csound cs = new Csound();
        Messages messages = new Messages(cs);
        CsoundMYFLTArray samples = new CsoundMYFLTArray();
        try {
            messages.SetMessageCallback();
            require(cs.GetVersion() >= 7000, "Csound 7 must load");
            require(cs.SetOption("-n -d -m0") == 0, "set options");
            require(cs.CompileOrc(ORC) == 0, "compile orchestra");
            require(cs.Start() == 0, "start");
            cs.SetControlChannel("frequency", 440);
            require(cs.GetControlChannel("frequency") == 440,
                    "control channel round trip");
            cs.EventString("i 1 0 0.1", 0);
            samples.SetConstPtr(cs.GetSpout());
            double peak = 0;
            for (int block = 0; block < 150; block++) {
                require(cs.PerformKsmps() == 0, "perform block");
                for (int i = 0; i < cs.GetKsmps(); i++) {
                    peak = Math.max(peak, Math.abs(samples.GetConstValue(i)));
                }
            }
            require(peak > 0.19 && peak < 0.21, "nonzero sine output");
            require(csnd7.csoundErrCnt(cs.GetCsound()) == 0, "no errors");
            require(messages.count > 0, "Java message callback");
            cs.Reset();
            require(cs.SetOption("-n") == 0, "reset options");
            require(cs.CompileOrc(ORC) == 0, "compile after reset");
            require(cs.Start() == 0, "start after reset");
        } finally {
            samples.delete();
            cs.delete();
            messages.delete();
        }
    }

    private static void argvAndThread() throws Exception {
        Path file = Files.createTempFile("csnd7-", ".csd");
        String csd = "<CsoundSynthesizer>\n<CsOptions>\n-n -d -m0\n" +
            "</CsOptions>\n<CsInstruments>\n" + ORC +
            "</CsInstruments>\n<CsScore>\ni 1 0 0.1\n" +
            "</CsScore>\n</CsoundSynthesizer>\n";
        Files.write(file, csd.getBytes(StandardCharsets.UTF_8));
        Csound cs = new Csound();
        Messages messages = new Messages(cs);
        CsoundPerformanceThread thread = null;
        try {
            messages.SetMessageCallback();
            String[] args = {"csound", file.toString()};
            require(cs.Compile(args.length, args) == 0, "String[] argv");
            require(cs.Start() == 0, "start score");
            cs.SetControlChannel("frequency", 440);
            thread = new CsoundPerformanceThread(cs);
            int before = messages.count;
            thread.Play();
            require(thread.Join() > 0, "thread finishes score");
            require(messages.count > before, "worker calls Java callback");
            require(csnd7.csoundErrCnt(cs.GetCsound()) == 0, "thread errors");
        } finally {
            if (thread != null) {
                thread.Stop();
                thread.Join();
                thread.delete();
            }
            cs.delete();
            messages.delete();
            Files.deleteIfExists(file);
        }
    }

    private static void compileFailure() {
        Csound cs = new Csound();
        try {
            require(cs.CompileOrc("instr 1\nnot_an_opcode 1\nendin") != 0,
                    "compile error must reach Java");
        } finally {
            cs.delete();
        }
    }

    public static void main(String[] args) throws Exception {
        renderAndReset();
        argvAndThread();
        compileFailure();
    }
}
