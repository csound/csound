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
		ace: "ace",
		"jquery" : "jquery",
		"bootstrap" :  "bootstrap.min" ,
		"libcsound":"libcsound"	
	}
});

require(["jquery", "bootstrap", "libcsound", "InputPanel", "ConsolePanel", "FileManager", "FilePanel", "HelpPanel", "EditorPanel"], main);


function main() {

	var ConsolePanel = require('ConsolePanel');
	var HelpPanel = require('HelpPanel');
	var EditorPanel = require('EditorPanel');
	var FilePanel = require('FilePanel');
	var InputPanel = require('InputPanel');
	Module['noExitRuntime'] = true;

	Module['_main'] = function() {

		var consolePanel = new ConsolePanel();
		consolePanel.print("Welcome to Csound Emscripten!");

		Module['print'] = Module['printErr'] = consolePanel.print 
		const csound = new CsoundObj();

		var inputPanel = new InputPanel(csound);
		var allowedFileExtensions = ["csd", "wav", "orc"];
		const fileManager = new FileManager(allowedFileExtensions, Module["print"]);

		var editorPanel = new EditorPanel(csound, fileManager);
		var filePanel = new FilePanel(fileManager, editorPanel.onClickFunction);
		var fileUploadedCallback = function() {

			filePanel.populateList();
		};

		fileManager.fileUploadFromServer("controlInputTest.csd", fileUploadedCallback);
		fileManager.fileUploadFromServer("test.orc", fileUploadedCallback);
		fileManager.fileUploadFromServer("audioInputTest.csd", fileUploadedCallback);
		fileManager.fileUploadFromServer("Boulanger-Trapped_in_Convert.csd", fileUploadedCallback);
		
		fileUploadedCallback = function() {

			filePanel.populateList();
			filePanel.fileLinks[0].click();
		};

		fileManager.fileUploadFromServer("midiInputTest.csd", fileUploadedCallback);


		var helpPanel = new HelpPanel(editorPanel.orcEditorDiv, editorPanel.scoEditorDiv, filePanel.fileNameDiv);
	};
};

