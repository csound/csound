# Desktop Java bindings for Csound 7

The optional SWIG build produces `csnd7.jar` and the JNI library
`_jcsound7`. It wraps the Csound 7 C API, `Csound` C++ class,
`CsoundPerformanceThread`, and the Java helper classes. It does not load
the Csound 6 JNI library.

The desktop build stays off by default. The separate Android build uses
its own interface file. Work on a replacement based on
[CsoundJNI](https://github.com/kunstmusik/CsoundJNI) can continue separately.

## Build

Install a JDK (11 or later), SWIG 4, and Csound's usual build dependencies.
Set `JAVA_HOME` to the JDK if CMake cannot find its JNI headers. The generated
classes target Java 8; the native library must match the runtime's platform
and CPU architecture.

```sh
cmake -S . -B build -G Ninja -DBUILD_JAVA_INTERFACE=ON
ninja -C build csnd7 csnd7-smoke
ctest --test-dir build -R '^java-api$' --output-on-failure
```

The bindings require `BUILD_PERFTHREAD_CLASS=ON`, which is the default.
SWIG uses the selected `USE_DOUBLE`/`USE_FLOAT` settings so its numeric
types match the native library. Rebuild the JAR and JNI library together
when changing precision. `JAVA_MODULE_INSTALL_DIR` sets their install
directory and defaults to Csound's library install directory.

## Run a CSD

Save this as `CsoundCommand.java`:

```java
import csnd7.Csound;
import csnd7.csnd7;

public final class CsoundCommand {
    public static void main(String[] args) {
        Csound cs = new Csound();
        try {
            String[] argv = new String[args.length + 1];
            argv[0] = "csound";
            System.arraycopy(args, 0, argv, 1, args.length);
            if (cs.Compile(argv.length, argv) != 0 || cs.Start() != 0) {
                throw new IllegalStateException("Could not start Csound");
            }
            int result;
            do {
                result = cs.PerformKsmps();
            } while (result == 0);
            if (result < 0 || csnd7.csoundErrCnt(cs.GetCsound()) != 0) {
                throw new IllegalStateException("Csound performance failed");
            }
        } finally {
            cs.delete();
        }
    }
}
```

On Linux or macOS, from the repository root:

```sh
javac -cp build/csnd7.jar CsoundCommand.java
java -Djava.library.path=build -cp build/csnd7.jar:. \
  CsoundCommand example.csd
```

Use `;` as the classpath separator on Windows. Some generators place the
JNI library in a configuration subdirectory, such as `build/Release`;
point `java.library.path` there. The loader must also find the matching
Csound shared library. On JDK 24 and later, add
`--enable-native-access=ALL-UNNAMED` to allow JNI access without a warning.

`Start()` is required after compilation. Csound 7 replaces `Perform()`
with the `PerformKsmps()` loop and removes `Cleanup()`. Call `delete()`
explicitly to release the native instance. Stop, join, and delete a
`CsoundPerformanceThread` before deleting its Csound instance. Keep Java
callback objects alive until Csound can no longer call them.

Use `SetStringChannel()` to write string channels. The old
`CsoundMYFLTArray.SetStringValue()` helper is not exposed: it wrote raw
bytes into Csound's string structure rather than setting the string.
