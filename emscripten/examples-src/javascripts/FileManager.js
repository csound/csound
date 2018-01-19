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

var FileManager = function(allowedFileExtensions, errorPrintCallback) {


	function checkFileExtension(fileURL) {

		var fileExtension = fileURL.split('.').pop();

		for (var i = 0; i < allowedFileExtensions.length; ++i) {

			if (fileExtension.localeCompare(allowedFileExtensions[i]) === 0) {

				return true;
			}
		}
		
		return false;
	};
	this.fileUploadFromServer = function(fileURL, fileUploadCallback) {

		if (checkFileExtension(fileURL) == false) {

			errorPrintCallback("File extension not allowed for file " + fileURL);
			return;
		}

		var xmlHttpRequest = new XMLHttpRequest();

		xmlHttpRequest.onload = function () {

			var data = new Uint8Array(xmlHttpRequest.response);
			var stream = FS.open(fileURL, 'w+');
			FS.write(stream, data, 0, data.length, 0);
			FS.close(stream);

			if (typeof fileUploadCallback !== 'undefined') {

				fileUploadCallback(data, fileURL);
			}
		};

		xmlHttpRequest.onerror = function () { 

			errorPrintCallback(this.statusText);
		};

		xmlHttpRequest.open("get", fileURL, true);
		xmlHttpRequest.responseType = "arraybuffer";
		xmlHttpRequest.send(null);
	};

	this.fileUploadFromClient = function(fileObject, fileUploadCallback) {


		if (checkFileExtension(fileObject.name) == false) {
			errorPrintCallback("File extension not allowed for file " + fileObject.name);
			return;
		}
		var reader = new FileReader();
		reader.onload = function(e) {


			var data = new Uint8Array(e.target.result);
			var stream = FS.open(fileObject.name, 'w+');
			FS.write(stream, data, 0, data.length, 0);
			FS.close(stream);
			fileUploadCallback();
		}
		reader.readAsArrayBuffer(fileObject);

	};

	this.readFileAsString = function(filePath) {

		var data = FS.readFile(filePath);
		var string = String.fromCharCode.apply(null, data);

		return string;
	};

	this.writeStringToFile = function(filePath, fileString) {

		FS.writeFile(filePath, fileString, {encoding: 'utf8'});
	};
};
