import pkg from './package.json';
import url from '@rollup/plugin-url';
import resolve from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import { terser } from 'rollup-plugin-terser';
import nodePolyfills from 'rollup-plugin-node-polyfills';

export default [
	// browser-friendly UMD build
	{
		input: 'src/CsoundObj.js',
		output: [
			{ file: pkg.main, format: 'umd', name:'CsoundObj' },
			//{ file: 'dist/CsoundObj.min.js', format: 'umd', name:'CsoundObj', plugins: [terser()] },
			//{ file: pkg.module, format: 'es' }
		],
		plugins: [
			url({
				include: ["**/CsoundProcessor.js"],
				limit: 9999999999999,
			}),
      nodePolyfills(),
      resolve(),
      commonjs(),
      terser()
		]

	}
];
