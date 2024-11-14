Module['locateFile'] = function (path, prefix) {
	//console.log(`locateFile called, with path: ${path}, prefix: ${prefix}`)

	if (path === "espeak-ng.data") {
		const currentModuleUrl = import.meta.url

		const lastSlashIndex = currentModuleUrl.lastIndexOf('/')

		let directoryPath

		if (currentModuleUrl.startsWith('file://')) {
			directoryPath = currentModuleUrl.substring(7, lastSlashIndex)

			// If it's a Windows path like `/C:/Some/Directory/...`,
			// remove the first / character
			if (/^\/[a-zA-Z]:\//.test(directoryPath)) {
				directoryPath = directoryPath.substring(1)
			}
		} else {
			directoryPath = currentModuleUrl.substring(0, lastSlashIndex)
		}

		//console.log(`directoryPath: ${directoryPath}\n\n`)

		return `${directoryPath}/espeak-ng.data`
	} else {
		return `${prefix}${path}`
	}
}
