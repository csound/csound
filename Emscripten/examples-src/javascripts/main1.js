/*
 * C S O U N D
 *
 * L I C E N S E
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

require.config({
    shim : {
        "bootstrap" : { "deps" :['jquery']  },
	"libcsound":"libcsound"
	},
    paths: {
	"bootstrap" :  "bootstrap.min" ,
	"jquery"    : "jquery",
        "libcsound" : "libcsound",
        "CsoundObj" : "CsoundObj"
	}
});

require([], main);

var Module = {};

function main() {
	Module['noExitRuntime'] = true;
        Module['_main'] = function() {
	  const csound = new CsoundObj();
          console.log("Welcome to Wasm Csound !");
	};

       require(["jquery","bootstrap", "libcsound", "CsoundObj"], function () {   
	    console.log("Csound loaded");
	});
};

