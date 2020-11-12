import jsdoc2md from "jsdoc-to-markdown";
jsdoc2md.render({ files: "src/**/*.js" }).then(console.log);
