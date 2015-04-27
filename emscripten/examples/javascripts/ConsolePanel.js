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

define('ConsolePanel', ["ace/ace"], function(ace) {

	function ConsolePanel() {

		var that = this;

		var parentDiv = document.getElementById("ConsoleSection");
		var consoleDiv = document.createElement("div");
		parentDiv.appendChild(consoleDiv);
		consoleDiv.id = "ConsoleOutput";
		var editor = ace.edit(consoleDiv);
		editor.setTheme("ace/theme/monokai");
		editor.getSession().setMode("ace/mode/text");
		editor.setReadOnly(true);
		editor.setShowPrintMargin(false); 
		editor.renderer.setShowGutter(false);
		editor.setHighlightActiveLine(false);
		//editor.$blockScrolling = Infinity;
		editor.renderer.$cursorLayer.element.style.opacity = 0;

		var clearConsoleButton = document.getElementById("ClearConsoleButton");

		this.print = function(text) {

			text = editor.getValue() + "\n" + text;
			editor.setValue(text, 1); 
		}
		clearConsoleButton.onclick = function() {

			editor.setValue("");
		}
	};

	return ConsolePanel;
});
