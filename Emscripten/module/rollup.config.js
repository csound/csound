import pkg from './package.json';
import url from '@rollup/plugin-url';
//import {terser} from 'rollup-plugin-terser';

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
			})
		]

	}
];
