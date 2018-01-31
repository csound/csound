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

define('InputPanel', [], function() {

	function InputPanel(csound) {

		var controlSlider = document.getElementById("ControlSlider");
		var controlSliderValue = document.getElementById("ControlSliderValue");
		var controlName = document.getElementById("ControlName");
		var audioInputButton = document.getElementById("AudioInputButton");
		var midiInputButton = document.getElementById("MidiInputButton");

		controlSlider.oninput = function() {

			controlSliderValue.value = controlSlider.value;
			csound.setControlChannel(controlName.value, controlSlider.value);
		};

		audioInputButton.onchange = function() {

			if (audioInputButton.checked === false) {

				csound.disableAudioInput();
				return;
			}

			var audioInputCallback = function(status) {

				if (status === true) {

					audioInputButton.checked = true;
				}
				else {

					audioInputButton.checked = false;
				}
			};

			csound.enableAudioInput(audioInputCallback);
		};

		midiInputButton.onchange = function() {

			var midiInputCallback = function(status) {

				if (status === true) {

					midiInputButton.checked = true;
				}
				else {

					midiInputButton.checked = false;
				}
			};

			csound.enableMidiInput(midiInputCallback);
		};
	}


	return InputPanel;


});
