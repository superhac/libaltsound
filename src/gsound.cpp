#include <plog/Log.h>
#include "gsound.h"
#include "fileparsers.h"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>


GSound::GSound(DataStructs& ds):m_pds(ds)
{

 
//: AltsoundProcessorBase(_game_name, _vpm_path),

    m_generator.seed(std::random_device{}()); // seed random number generator
    
     if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
         PLOGE << "Failed to initialize SDL Audio: " << SDL_GetError();
         m_sdlAudioinitialized = false;
         return;
      }

      if (!Mix_OpenAudio(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr)) {
        PLOGE << "Failed to initialize SDL Mixer: " << SDL_GetError();
        m_sdlAudioinitialized = false;
      return;
   }

   init();

/* 	
	// perform processor initialization (load samples, etc)
	g_pProcessor->init();

	g_cmdData.cmd_counter = 0;
	g_cmdData.stored_command = -1;
	g_cmdData.cmd_filter = 0;
	std::fill_n(g_cmdData.cmd_buffer, ALT_MAX_CMDS, ~0);

	// Initialize BASS
	int DSidx = -1; // BASS default device

	if (!BASS_Init(DSidx, 44100, 0, NULL, NULL)) {
		ALT_ERROR(0, "BASS initialization error: %s", get_bass_err());
	}

	ALT_DEBUG(0, "END AltsoundInit()"); 

	return true; */
}

void GSound::init() // Superhac - come back and add sound cmd recording
{
    // Superhac - Come back and add this.
    #ifndef ALTSOUND_STANDALONE
	// If recording sound commands, initialize output file
	if (m_record_sound_commands) {
		/* if (!ALT_CALL(startLogging(AltsoundProcessorBase::getGameName()))) {
			ALT_ERROR(0, "FAILED startLogging()");
		}
		else {
			ALT_INFO(1, "SUCCESS startLogging()");
		} */
	}
    #endif

    if (!loadSamples()) {
		PLOGE << "FAILED GSoundProcessor::loadSamples()";
	}
	PLOGI << "SUCCESS: GSoundProcessor::loadSamples()";
}

bool GSound::loadSamples()
{
	
	/* string altsound_path = vpm_path; // in base class
	if (!altsound_path.empty()) {
		altsound_path += "altsound/" + game_name + '/';
	} */
FileParsers fp;

    if (m_pds.m_altsound_format == "g-sound")
    {
        PLOGI << "BEGIN Loading Samples for GSound";
        //GSoundCsvParser csv_parser(altsound_path);

        if (!fp.parseGSoundCVS(m_pds)) {
            PLOGE << "FAILED GSoundCsvParser::parse()";
            PLOGI << "END GSoundProcessor::init()";
            return false;
        }
        PLOGI << "SUCCESS GSoundCsvParser::parse()";
        PLOGI <<  "END GSoundProcessor::init()";
        return true;
    }
	else if (m_pds.m_altsound_format == "altsound") {
/* 		AltsoundCsvParser csv_parser(m_pds.m_altSoundPath);

		if (!csv_parser.parse(samples)) {
			ALT_ERROR(0, "FAILED AltsoundCsvParser::parse()");

			ALT_OUTDENT;
			ALT_DEBUG(0, "END AltsoundProcessor::loadSamples()");
			return false;
		}
		ALT_INFO(0, "SUCCESS AltsoundCsvParser::parse()");
        return true; */
	}
	else if (m_pds.m_altsound_format == "legacy") {
		/* AltsoundFileParser file_parser(altsound_path);
		if (!file_parser.parse(samples)) {
			ALT_ERROR(0, "FAILED AltsoundFileParser::parse()");

			ALT_OUTDENT;
			ALT_DEBUG(0, "END AltsoundProcessor::loadSamples");
			return false;
		}
		ALT_INFO(0, "SUCCESS AltsoundFileParser::parse()");
        return true; */
	}

    PLOGE << "No altsound format match?";
	return false;
}
