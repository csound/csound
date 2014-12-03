//Module['noExitRuntime'] = true;
//Module['_main'] = function() {

	const csdFileName = "input.csd";
	const csound = new CsoundObj(true);

	var uploadCSD = function(evt) {

		var fileReader = new FileReader();
		var files = evt.target.files;
		file = files[0];
		fileReader.readAsBinaryString(file);

		fileReader.onload = function(e) {

			FS.createDataFile('/', csdFileName, e.target.result, true, false);
		}
	}

	document.getElementById('files').addEventListener('change', uploadCSD, false);
	document.getElementById('Compile').addEventListener('click', compile, false);

	function compile() {

		csound.compileCSD(csdFileName);
	}
	
	document.getElementById('Start').addEventListener('click', start, false);

	function start() {

		csound.start();
	}

	document.getElementById('Stop').addEventListener('click', stop, false);

	function stop() {

		csound.stop();
	}
//};

