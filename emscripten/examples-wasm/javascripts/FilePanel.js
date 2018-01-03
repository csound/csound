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

define('FilePanel', ["libcsound"], function() {

	function FilePanel(fileManager, listOnClickFunction) {

		var that = this;
		var listElement = document.getElementById('FileList');
		var fileButtonElement = document.getElementById('FileButton');
		var fileListObject = new FileList("/");
		var selectedFileName = null;
		this.fileLinks = null;

		var onClickFunction = function() {

			listOnClickFunction(this);

			for (var i = 0; i < that.fileLinks.length; ++i) {

				if (this === that.fileLinks[i]) {

					selectedFileName = that.fileLinks[i].fileName;
					that.populateList();
				}
			}
		}


		this.fileNameDiv = document.createElement('div');
		this.fileNameDiv.id = "FileNames";
		this.populateList = function() {

			that.fileNameDiv.innerHTML = "";
			var fileList = fileListObject.getFileList();
			that.fileLinks = [];
			for (var i = 0; i < fileList.length; ++i) {

				var fileExtension = fileList[i].split('.').pop();

				var link = document.createElement('a');
				link.innerHTML = fileList[i];
				link.fileName = fileList[i];
				link.fileExtension = fileExtension;

				if (link.fileName.localeCompare(selectedFileName) === 0) {


					link.className = "list-group-item active";
				}
				else {

					link.className = "list-group-item";
				}
				link.onclick = onClickFunction;
				that.fileNameDiv.appendChild(link);
				that.fileLinks.push(link);
			}

			listElement.appendChild(that.fileNameDiv);
		};



		function handleFileSelect(e) {


			fileManager.fileUploadFromClient(e.target.files[0], that.populateList);
		}

		fileButtonElement.addEventListener('change', handleFileSelect, false);



		var filePanelDiv = document.getElementById('FilePanel');
		filePanelDiv.ondragover = function(e) {

			this.className = 'panel panel-warning';
			e.dataTransfer.dropAllowed = 'copy'
			e.dataTransfer.dropEffect = 'copy'
			e.preventDefault();	
		};	
		filePanelDiv.ondragend = function(e)	{

			this.className = 'panel panel-primary';
			e.preventDefault();	
		};	

		filePanelDiv.ondragleave = function(e)	{

			this.className = 'panel panel-primary';
			e.preventDefault();	
		};	

		filePanelDiv.ondrop = function (e) {
			this.className = 'panel panel-primary';
			e.preventDefault();
			fileManager.fileUploadFromClient(e.dataTransfer.files[0], that.populateList);
		};
	};

	return FilePanel;

});	
