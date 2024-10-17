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

#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "speak_lib.h"

static int gSamplerate = 0;

class eSpeakNGWorker {
   public:
	eSpeakNGWorker()
		: rate(espeakRATE_NORMAL), volume(100), pitch(50), range(50), current_voice(NULL) {
		if (!gSamplerate) {
			// gSamplerate = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 100, NULL,
			// espeakINITIALIZE_DONT_EXIT);
			gSamplerate =
				espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 100, NULL,
								  espeakINITIALIZE_PHONEME_EVENTS | espeakINITIALIZE_PHONEME_IPA |
									  espeakINITIALIZE_DONT_EXIT);
		}

		samplerate = gSamplerate;
		voices = espeak_ListVoices(NULL);
	}

	void synth_(const char* aText, void* aCallback) {
		t_espeak_callback* cb = reinterpret_cast<t_espeak_callback*>(aCallback);
		espeak_SetSynthCallback(cb);

		espeak_SetParameter(espeakRATE, rate, 0);
		espeak_SetParameter(espeakVOLUME, volume, 0);
		espeak_SetParameter(espeakPITCH, pitch, 0);
		espeak_SetParameter(espeakRANGE, range, 0);

		/*
		if (current_voice) {
			espeak_SetVoiceByProperties(current_voice);
		} else {
			espeak_SetVoiceByName("default");
		}
		*/

		// espeak_Synth(aText, 0, 0, POS_CHARACTER, 0, 0, NULL, NULL);
		espeak_Synth(aText, 0, 0, POS_CHARACTER, 0, espeakCHARS_UTF8 | espeakSSML | espeakPHONEMES, NULL, NULL);

		// Reset callback so other instances will work too.
		espeak_SetSynthCallback(NULL);
	}

	int synth_ipa_(const char* aText, const char* virtualFileName) {
		/*
		if (current_voice) {
			espeak_SetVoiceByProperties(current_voice);
		} else {
			espeak_SetVoiceByName("default");
		}
		*/

		/* phoneme_mode
		  bit 1:   0=eSpeak's ascii phoneme names, 1= International Phonetic Alphabet (as UTF-8
		  characters). bit 7:   use (bits 8-23) as a tie within multi-letter phonemes names bits
		  8-23:  separator character, between phoneme names
		*/

		espeak_SetSynthCallback(NULL);

		int phoneme_options = (1 << 1);	 // Use IPA
		int use_custom_phoneme_separator = (0 << 7);
		int phonemes_separator = '_';  // Use a default value

		int phoneme_conf = phoneme_options | (phonemes_separator << 8);

		FILE* f_phonemes_out = fopen(virtualFileName, "wb");
		if (!f_phonemes_out) {
			return -1;
		}

		// espeak_ng_InitializeOutput(ENOUTPUT_MODE_SYNCHRONOUS, 0, NULL);
		espeak_SetPhonemeTrace(phoneme_conf, f_phonemes_out);
		espeak_Synth(aText, 0, 0, POS_CHARACTER, 0, 0, NULL, NULL);
		espeak_SetPhonemeTrace(0, NULL);
		fclose(f_phonemes_out);

		return 0;
	}

	int synth_with_phoneme_trace_(const char* aText,
								  const char* phonemeTraceVirtualFileName,
								  void* aCallback) {
		/*
		if (current_voice) {
			espeak_SetVoiceByProperties(current_voice);
		} else {
			espeak_SetVoiceByName("default");
		}
		*/

		/* phoneme_mode
		  bit 1:   0=eSpeak's ascii phoneme names, 1= International Phonetic Alphabet (as UTF-8
		  characters). bit 7:   use (bits 8-23) as a tie within multi-letter phonemes names bits
		  8-23:  separator character, between phoneme names
		*/

		// Initialize phoneme trace
		int phoneme_options = (1 << 1);	 // Use IPA
		int use_custom_phoneme_separator = (0 << 7);
		int phonemes_separator = '_';  // Use a default value

		int phoneme_conf = phoneme_options | (phonemes_separator << 8);
		FILE* f_phonemes_out = fopen(phonemeTraceVirtualFileName, "wb");

		if (!f_phonemes_out) {
			return -1;
		}

		// Initialize synthesis
		t_espeak_callback* cb = reinterpret_cast<t_espeak_callback*>(aCallback);
		espeak_SetSynthCallback(cb);
		espeak_SetPhonemeTrace(phoneme_conf, f_phonemes_out);

		espeak_SetParameter(espeakRATE, rate, 0);
		espeak_SetParameter(espeakVOLUME, volume, 0);
		espeak_SetParameter(espeakPITCH, pitch, 0);
		espeak_SetParameter(espeakRANGE, range, 0);

		/*
		if (current_voice) {
			espeak_SetVoiceByProperties(current_voice);
		} else {
			espeak_SetVoiceByName("default");
		}
		*/

		espeak_Synth(aText, 0, 0, POS_CHARACTER, 0, 0, NULL, NULL);
		// espeak_Synth(aText, 0, 0, POS_CHARACTER, 0, espeakSSML, NULL, NULL);

		// Reset
		// Reset callback so other instances will work too.
		espeak_SetSynthCallback(NULL);

		// Reset phoneme trace
		espeak_SetPhonemeTrace(0, NULL);

		// Close phoneme trace file
		fclose(f_phonemes_out);

		return 0;
	}

	char* text_to_phonemes(const char* aText, int useIpa) {
		/*
		if (current_voice) {
			espeak_SetVoiceByProperties(current_voice);
		} else {
			espeak_SetVoiceByName("default");
		}
		*/

		// Initialize phoneme trace
		int phoneme_options = 0;
		if (useIpa == 1) {
			phoneme_options = (1 << 1); // Use IPA
		}

		int use_custom_phoneme_separator = (0 << 7);
		int phonemes_separator = '_';  // Use a default value
		int phoneme_mode = phoneme_options | (phonemes_separator << 8);

		const void** textPointer = (const void**) &aText;

		std::string result = "";

		while (*textPointer != NULL) {
			char* phrase = (char*) espeak_TextToPhonemes(textPointer, espeakCHARS_AUTO, phoneme_mode);

			result.append(phrase);

			if (*textPointer != NULL) {
				result.append(" | ");
			}
		}

		char* resultCopy = new char[result.length() + 1];
		std::strcpy(resultCopy, result.c_str());

		return resultCopy;
	}

	long set_voice(const char* aName,
				   const char* aLang = NULL,
				   unsigned char aGender = 0,
				   unsigned char aAge = 0,
				   unsigned char aVariant = 0) {
		long result = 0;
		if (aLang || aGender || aAge || aVariant) {
			espeak_VOICE props = {0};
			props.name = aName;
			props.languages = aLang;
			props.gender = aGender;
			props.age = aAge;
			props.variant = aVariant;
			result = espeak_SetVoiceByProperties(&props);
		} else {
			result = espeak_SetVoiceByName(aName);
		}

		// This way we don't need to allocate the name/lang strings to the heap.
		// Instead, we store the actual global voice.
		current_voice = espeak_GetCurrentVoice();

		return result;
	}

	int getSizeOfEventStruct_() { return sizeof(espeak_EVENT); }

	char* getStringIDFromEventStruct_(void* event) {
		espeak_EVENT* ev = (espeak_EVENT*)event;

		if (ev->type == espeakEVENT_PHONEME) {
			return (char*)&ev->id.string;
		} else if (ev->type == espeakEVENT_MARK || ev->type == espeakEVENT_PLAY) {
			return (char*)ev->id.name;
		} else {
			return NULL;
		}
	}

	int getNumericIDFromEventStruct_(void* event) {
		espeak_EVENT* ev = (espeak_EVENT*)event;

		if (ev->type == espeakEVENT_WORD || ev->type == espeakEVENT_SENTENCE) {
			return ev->id.number;
		} else {
			return 0;
		}
	}

	const espeak_VOICE** voices;
	int samplerate;

	int rate;
	int volume;
	int pitch;
	int range;

   private:
	espeak_VOICE* current_voice;
};

#include <glue.cpp>
