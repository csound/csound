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
 */


var FileList = function(path) {

	var that = this;
	var _getFileCount = cwrap('FileList_getFileCount', ['number'], ['string']);
	var _getFileNameString = cwrap('FileList_getFileNameString', ['string'], ['string', 'number']);

	this.getFileList = function() {

		var count = _getFileCount(path);

		var fileList = [];

		for (var i = 0; i < count; ++i) {

			var filePointer = _getFileNameString(path, i);
			var buffer = new Uint8Array(Module.HEAP8.buffer, filePointer, _strlen(filePointer));
			var fileName = String.fromCharCode.apply(null, buffer);
			fileList.push(fileName);
		}
		
		return fileList;
	};
};
