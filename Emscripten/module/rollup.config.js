//import resolve from '@rollup/plugin-node-resolve';
//import commonjs from '@rollup/plugin-commonjs';
import pkg from './package.json';
import url from '@rollup/plugin-url';
import {terser} from 'rollup-plugin-terser';

export default [
	// // browser-friendly UMD build
	// {
	// 	input: 'src/CsoundObj.js',
	// 	output: {
	// 		name: 'csound',
	// 		file: pkg.browser,
	// 		format: 'umd'
	// 	},
	// 	plugins: [
	// 		url()
	// 	]
	// },

	// CommonJS (for Node) and ES module (for bundlers) build.
	// (We could have three entries in the configuration array
	// instead of two, but it's quicker to generate multiple
	// builds from a single configuration where possible, using
	// an array for the `output` option, where we can specify
	// `file` and `format` for each target)
	{
		input: 'src/CsoundObj.js',
		//external: ['ms'],
		output: [
			{ file: pkg.main, format: 'umd', name:'CsoundObj' },
			{ file: 'dist/CsoundObj.min.js', format: 'umd', name:'CsoundObj', plugins: [terser()] },
			{ file: pkg.module, format: 'es' }
		],
		plugins: [
			url({
				include: ["**/CsoundProcessor.js"],
				limit: 9999999999999,
			})
		]

	}
];
