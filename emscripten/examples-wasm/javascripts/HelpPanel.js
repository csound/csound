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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

define('HelpPanel', [], function() {

	function HelpPanel(orcEditorDiv, scoEditorDiv, fileNameDiv) {

		var helpTextDiv = document.getElementById("helpText");
		helpTextDiv.innerHTML = "<h4 id=\"HelpText\">HelpPanel</h4><p id=\"HelpText\">Mouse over page elements and the relevant help text will appear in this panel</p>";
		var stack = [helpTextDiv.innerHTML];

		function registerHelpTextWithID(elementID, elementText) {

			var element = document.getElementById(elementID);
			registerHelpTextWithElement(element, elementText);
		};

		function registerHelpTextWithElement(element, elementText) {
			
			elementText = "<h4 id=\"HelpText\">" + element.id + "</h4>" + "<p id=\"HelpText\">" + elementText + "</p>";
			element.onmouseenter = function(e) {

				stack.push(elementText);
				helpTextDiv.innerHTML = stack[stack.length - 1];
			}

			element.onmouseleave = function(e) {
				stack.pop();
				helpTextDiv.innerHTML = stack[stack.length - 1];
			}

		};

		registerHelpTextWithID("CSDEditor", "Use this panel to view and edit .csd files, when the .csd is ready to run press the compile then the perform buttons");
		registerHelpTextWithID("CSDEditorButton", "Show the CSD Editor in the Editor Panel");
		registerHelpTextWithID("ORCSCOEditorButton", "Show the Orchestra and Score Editors in the Editor Panel");
		registerHelpTextWithElement(orcEditorDiv, "Use this panel to view and edit .orc files, send orchestra to running instance of Csound using CMD+ENTER on Mac or CTRL+ENTER on Windows");
		registerHelpTextWithElement(scoEditorDiv, "Use this panel to view and edit .sco files, send score to running instance of Csound using CMD+ENTER on Mac or CTRL+ENTER on Windows");
		registerHelpTextWithID("CompileButton", "Compile the csd file shown in the CSD Editor, if there has been a performance of Csound, the instance needs to be reset using the reset button");
		registerHelpTextWithID("PerformButton", "Perform the csd file shown in the CSD Editor, Csound will only perform after the csd has been compiled, when performance has finised Csound needs to be reset using the Reset Button");
		registerHelpTextWithID("RenderButton", "Render the csd file shown in the CSD Editor, Csound will only render after the csd has been compiled");
		registerHelpTextWithID("ResetButton", "Reset the running Csound instance, this is required in order to compile csd files after a performance");
		registerHelpTextWithID("ConsoleOutput", "This view shows the text output from the running Csound instance");
		registerHelpTextWithID("ClearConsoleButton", "Clear the text output from the Console")
		registerHelpTextWithID("ControlName", "Type the name of the control channel in this text field which can be controlled using the slider below");
		registerHelpTextWithID("ControlSlider", "Change the input value of the control channel with the name specified by the above text field, the slider has a range from 1 to 100");
		registerHelpTextWithID("AudioInputCheckbox", "Enable audio input to Csound, not supported by all browsers");
		registerHelpTextWithID("MidiInputCheckbox", "Enable midi input to Csound, not supported by all browsers");
		registerHelpTextWithID("FileUploadButton", "Upload files to the browser, enables Csound to access files. Allowed file types are .csd, .orc, .sco and .wav");
		registerHelpTextWithID("FilePanel", "Upload files to the browser using drag and drop, enables Csound to access files. Allowed file types are .csd, .orc, .sco and .wav");
		registerHelpTextWithElement(fileNameDiv, "A list of files available to Csound, click on the file names with .csd, .orc or .sco extensions to open them in the respective editors");
	};

	return HelpPanel;
});
