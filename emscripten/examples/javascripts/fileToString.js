

function fileToString(pathOfFileToRead)
{
	var contentsOfFileAsString = FileHelper.readStringFromFileAtPath(pathOfFileToRead);

	return contentsOfFileAsString;
}

function FileHelper()
{}
{
	FileHelper.readStringFromFileAtPath = function(pathOfFileToReadFrom)
	{
		var request = new XMLHttpRequest();
		request.open("GET", pathOfFileToReadFrom, false);
		request.send(null);
		var returnValue = request.responseText;

		return returnValue;
	}

}

