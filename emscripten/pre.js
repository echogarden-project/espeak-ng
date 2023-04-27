Module['locateFile'] = function (path, prefix) {
	//console.log("locateFile:", path, prefix)
	//console.log("__dirname:", __dirname)

	if (path == "espeak-ng.data") {
		return __dirname + "/" + "espeak-ng.data"
	} else {
		return prefix + path
	}
}
