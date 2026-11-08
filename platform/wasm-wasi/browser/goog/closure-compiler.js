import googClosureCompiler from "google-closure-compiler";
import JarPath from "google-closure-compiler-java";

const ClosureCompiler = googClosureCompiler.compiler || googClosureCompiler;

export function runClosureCompiler(config, { failOnWarnings = true } = {}) {
  // Read structured diagnostics so every reported warning fails production
  // builds without enabling extra checks or matching compiler log text.
  const compiler = new ClosureCompiler({ ...config, error_format: "JSON" });
  compiler.javaPath = process.env.JAVA_HOME ? `${process.env.JAVA_HOME}/bin/java` : "java";
  compiler.JAR_PATH = JarPath;
  return new Promise((resolve, reject) => {
    compiler.run((exitCode, _stdout, stderr) => {
      if (exitCode !== 0) {
        // The npm wrapper prefixes failed compilations with the Java command.
        console.error(stderr);
        reject(new Error(`Closure compilation failed (exit ${exitCode})`));
        return;
      }
      try {
        const diagnostics = JSON.parse(stderr || "[]");
        for (const diagnostic of diagnostics) {
          const location = diagnostic.source
            ? `${diagnostic.source}:${diagnostic.line}:${diagnostic.column}: `
            : "";
          const key = diagnostic.key ? `[${diagnostic.key}] ` : "";
          console.log(
            `${location}${diagnostic.level.toUpperCase()} ${key}${diagnostic.description}`,
          );
          if (diagnostic.context) console.log(diagnostic.context);
        }
        const errors = diagnostics.some(({ level }) => level === "error");
        const warnings = diagnostics.some(({ level }) => level === "warning");
        if (errors || (failOnWarnings && warnings))
          throw new Error(`Closure compilation failed${warnings ? " (warnings are fatal)" : ""}`);
        resolve();
      } catch (error) {
        if (error instanceof SyntaxError && stderr) console.error(stderr);
        reject(error);
      }
    });
  });
}
