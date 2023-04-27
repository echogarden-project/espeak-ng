/*
 * Copyright (C) 2014-2017 Eitan Isaacson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>.
 */

eSpeakNGWorker.prototype.list_voices = function () {
	let voices = []
	let i

	for (let voice = this.get_voices(i = 0); voice.ptr != 0; voice = this.get_voices(++i)) {
		const v = {
			name: voice.get_name(),
			identifier: voice.get_identifier(),
			languages: [],
		}

		let ii = 0
		let byte = voice.get_languages(ii)

		function nullTerminatedString(offset) {
			let str = ''
			let index = offset
			let b = voice.get_languages(index++)

			while (b != 0) {
				str += String.fromCharCode(b)
				b = voice.get_languages(index++)
			}

			return str
		}

		while (byte != 0) {
			const lang = { priority: byte, name: nullTerminatedString(++ii) }

			v.languages.push(lang)
			ii += lang.name.length + 1

			byte = voice.get_languages(ii)
		}

		voices.push(v)
	}

	return voices
}

const eventTypes = [
	'list_terminated',
	'word',
	'sentence',
	'mark',
	'play',
	'end',
	'msg_terminated',
	'phoneme',
	'samplerate'
]

eSpeakNGWorker.prototype.synthesize = function (aText, userCallback) {
	const thisObject = this
	const eventStructSize = thisObject.getSizeOfEventStruct_()

	function jsCallback(audioPtr, length, events_pointer) {
		const audioData = Module.HEAP16.slice((audioPtr / 2), (audioPtr / 2) + length)

		let events = []
		let ptr = events_pointer

		for (let ev = wrapPointer(ptr, espeak_EVENT);
			ev.get_type() != Module.espeakEVENT_LIST_TERMINATED;
			ev = wrapPointer((ptr += eventStructSize), espeak_EVENT)) {

			const eventType = eventTypes[ev.get_type()]

			let id

			if (eventType == 'mark' || eventType == 'play' || eventType == 'phoneme') {
				id = thisObject.getStringIDFromEventStruct_(ptr)
			} else if (eventType == 'word' || eventType == 'sentence') {
				id = thisObject.getNumericIDFromEventStruct_(ptr)
			} else {
				id = undefined
			}

			events.push({
				type: eventType,
				text_position: ev.get_text_position(),
				word_length: ev.get_length(),
				audio_position: ev.get_audio_position(),
				id: id
			})
		}

		return userCallback(audioData, events) ? 1 : 0
	}

	const jsCallbackPtr = addFunction(jsCallback)

	this.synth_(aText, jsCallbackPtr)

	removeFunction(jsCallbackPtr)
}

eSpeakNGWorker.prototype.synthesize_ipa = function (aText, aCallback) {
	// Use a unique temp file for the worker. Avoid collisions, just in case.
	const ipaVirtualFileName = "espeak-ng-ipa-tmp-" + Math.random().toString().substring(2);

	let resultIpa = ""

	const resultCode = this.synth_ipa_(aText, ipaVirtualFileName)

	if (resultCode == 0) {
		resultIpa = FS.readFile(ipaVirtualFileName, { encoding: 'utf8' })
	}

	// Clean up the tmp file
	FS.unlink(ipaVirtualFileName)

	const returnValue = {
		code: resultCode,
		ipa: resultIpa
	}

	return returnValue
}

eSpeakNGWorker.prototype.convert_to_phonemes = function (aText, useIpa) {
	return this.text_to_phonemes(aText, useIpa ? 1 : 0)
}

eSpeakNGWorker.prototype.synthesize_and_get_phonemes = function (text, userCallback) {
	const ipaVirtualFileName = "espeak-ng-ipa-temp-" + Math.random().toString().substring(2)

	const thisObject = this
	const eventStructSize = thisObject.getSizeOfEventStruct_()

	function jsCallback(audioPtr, length, events_pointer) {
		const data = Module.HEAP16.slice((audioPtr / 2), (audioPtr / 2) + length)

		let events = []
		let ptr = events_pointer

		for (let ev = wrapPointer(ptr, espeak_EVENT);
			ev.get_type() != Module.espeakEVENT_LIST_TERMINATED;
			ev = wrapPointer((ptr += eventStructSize), espeak_EVENT)) {

			const eventType = eventTypes[ev.get_type()]

			let id

			if (eventType == 'mark' || eventType == 'play' || eventType == 'phoneme') {
				id = thisObject.getStringIDFromEventStruct_(ptr)
			} else if (eventType == 'word' || eventType == 'sentence') {
				id = thisObject.getNumericIDFromEventStruct_(ptr)
			} else {
				id = undefined
			}

			events.push({
				type: eventType,
				text_position: ev.get_text_position(),
				word_length: ev.get_length(),
				audio_position: ev.get_audio_position(),
				id: id
			})
		}

		return userCallback(data, events) ? 1 : 0
	}

	const jsCallbackPtr = addFunction(jsCallback)

	const resultCode = this.synth_with_phoneme_trace_(text, ipaVirtualFileName, jsCallbackPtr)

	removeFunction(jsCallbackPtr)

	let resultText = ""

	if (resultCode == 0) {
		resultText = FS.readFile(ipaVirtualFileName, { encoding: 'utf8' })
	}

	// Clean up the temporary virtual file
	FS.unlink(ipaVirtualFileName)

	return resultText
}
