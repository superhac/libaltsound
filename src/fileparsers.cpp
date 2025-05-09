// license:GPLv3+
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <algorithm>
#include <sys/stat.h>

#include "fileparsers.h"
#include "soundtypebehaviors.h"

using namespace std;

string FileParsers::extractValue(const string& line) {
	size_t colonPos = line.find(':');
	if (colonPos == string::npos)
		throw std::runtime_error("Value could not be determined");
	size_t valueStart = colonPos + 1;
	while (valueStart < line.size() && line[valueStart] == ' ')
		++valueStart;

	string value = line.substr(valueStart);
	if (!value.empty() && value.back() == '\r')
		value.pop_back();
	return value;
}

bool FileParsers::parseCmdFile(DataStructs& ds)
{
	std::cout << "BEGIN parseCmdFile" << std::endl;

	try {
		std::ifstream inFile(ds.m_init_data.log_path);
		if (!inFile.is_open())
			throw std::runtime_error("Unable to open file: " + ds.m_init_data.log_path);

		string line;

		// Process paths and game name
		if (!std::getline(inFile, line))
			throw std::runtime_error("altsound_path value could not be determined");

		string altsoundPath = extractValue(line);
	 	std::replace(altsoundPath.begin(), altsoundPath.end(), '\\', '/');
		if (altsoundPath.back() != '/')
			altsoundPath += '/';

		size_t altsoundPos = altsoundPath.find("/altsound/");
		if (altsoundPos == string::npos)
			throw std::runtime_error("altsound_path value could not be determined");

        ds.m_init_data.altsound_path = altsoundPath;
		ds.m_init_data.vpm_path = altsoundPath.substr(0, altsoundPos + 1);

		size_t nextSlashPos = altsoundPath.find('/', altsoundPos + 10);
		if (nextSlashPos == string::npos)
			throw std::runtime_error("game name could not be determined");

        ds.m_init_data.game_name = altsoundPath.substr(altsoundPos + 10, nextSlashPos - (altsoundPos + 10));

		// Process hardware_gen
		if (!std::getline(inFile, line))
			throw std::runtime_error("hardware_gen value could not be determined");

		std::string hexString = extractValue(line);
		ds.m_init_data.hardware_gen = (DataStructs::ALTSOUND_HARDWARE_GEN)std::stoull(hexString, nullptr, 16);

		std::cout << "Altsound path: " << ds.m_init_data.altsound_path << std::endl;
		std::cout << "VPinMAME path: " << ds.m_init_data.vpm_path << std::endl;
		std::cout << "Game name: " << ds.m_init_data.game_name << std::endl;
		std::cout << "Hardware Gen: 0x" 
			<< std::setfill('0') << std::setw(13) 
			<< std::hex << ds.m_init_data.hardware_gen << std::endl;

		// The rest of the lines are test data
		while (std::getline(inFile, line)) {
			if (!line.empty() && line.back() == '\r')
				line.pop_back();

			if (line.empty())
				continue;

			std::istringstream ss(line);

			DataStructs::TestData data;
			string temp, command;

			if (!std::getline(ss, temp, ','))
				continue;

			char* end;
			data.msec = std::strtoul(temp.c_str(), &end, 10);
			if (end == temp.c_str())
				throw std::runtime_error("Unable to parse time: " + temp);

			const string HEX_PREFIX = "0x";
			ss >> std::ws;
			if (!std::getline(ss, command, ',')) continue;
			if (command.substr(0, HEX_PREFIX.length()) == HEX_PREFIX)
				command = command.substr(HEX_PREFIX.length());
			else
				throw std::runtime_error("Command value is not in hexadecimal format: " + command);

			data.snd_cmd = std::strtoul(command.c_str(), &end, 16);
			if (end == command.c_str())
				throw std::runtime_error("Unable to parse command: " + command);

            ds.m_init_data.test_data.push_back(data);
		}

		inFile.close();
		std::cout << "END parseCmdFile" << std::endl;
		return true;
	}
	catch (const std::runtime_error& e) {
		std::cout << e.what() << std::endl;
		std::cout << "END parseCmdFile" << std::endl;
		return false;
	}
}

bool FileParsers::altsoundInit(DataStructs& ds)
{
    auto pinmamePath = ds.m_init_data.vpm_path;
    auto gameName = ds.m_init_data.game_name;
	
    //ALT_DEBUG(0, "BEGIN AltsoundInit()");
	//ALT_INDENT;

	/* if (g_pProcessor) {
		ALT_ERROR(0, "Processor already defined");
		ALT_OUTDENT;
		ALT_DEBUG(0, "END AltsoundInit()");
		return false;
	} */

	// initialize channel_stream storage
	//std::fill(channel_stream.begin(), channel_stream.end(), nullptr);
 
	string szPinmamePath = pinmamePath;

	std::replace(szPinmamePath.begin(), szPinmamePath.end(), '\\', '/');

	if (szPinmamePath.back() != '/')
		szPinmamePath += '/';

	//const string szAltSoundPath = szPinmamePath + "altsound/" + gameName + '/'; 
    ds.m_altSoundPath = szPinmamePath + "altsound/" + gameName + '/';

	// parse .ini file
	//AltsoundIniProcessor ini_proc;
	if (!parse_altsound_ini(ds)) {
		// Error message and return
        std::cout << "Failed to parse_altsound_ini(" << ds.m_altSoundPath.c_str() << ")";
		//ALT_ERROR(0, "Failed to parse_altsound_ini(%s)",  ds.m_altSoundPath.c_str());
		//ALT_OUTDENT;
		//ALT_DEBUG(0, "END AltsoundInit()");
		return false;
	}

	string format = ds.m_altsound_format;

     // WORKING HERE! /////////////////////////////

	/* if (format == "g-sound") {
		// G-Sound only supports new CSV format. No need to specify format
		// in the constructor
		g_pProcessor = new GSoundProcessor(gameName, szPinmamePath);
	}
	else if (format == "altsound" || format == "legacy") {
		g_pProcessor = new AltsoundProcessor(gameName, szPinmamePath, format);
	}
	else {
		ALT_ERROR(0, "Unknown AltSound format: %s", format.c_str());
		ALT_OUTDENT;
		ALT_DEBUG(0, "END AltsoundInit()");
		return false;
	}

	if (!g_pProcessor) {
		ALT_ERROR(0, "FAILED: Unable to create AltSound Processor");
		ALT_OUTDENT;
		ALT_DEBUG(0, "END AltsoundInit()");
		return false;
	}
	
	ALT_INFO(0, "%s processor created", format.c_str());

	g_pProcessor->setMasterVol(1.0f);
	g_pProcessor->setGlobalVol(1.0f);
	g_pProcessor->romControlsVol(ini_proc.usingRomVolumeControl());
	g_pProcessor->recordSoundCmds(ini_proc.recordSoundCmds());
	g_pProcessor->setSkipCount(ini_proc.getSkipCount());

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


    */

	return true; 
}

bool FileParsers::parse_altsound_ini(DataStructs& ds)
{

	//ALT_DEBUG(0, "BEGIN AltsoundIniProcessor::parse_altsound_ini()");
	//ALT_INDENT;

	// if altsound.ini does not exist, create it
	string ini_path = ds.getAltSoundPath() + "altsound.ini";
    

	std::ifstream file_in(ini_path);
	if (!file_in.good()) {
        std:cout << "\"altsound.ini\" not found. Creating it." << ")";
		//ALT_INFO(0, "\"altsound.ini\" not found. Creating it.");
		if (!create_altsound_ini(ds)) {
            std::cout << "FAILED AltsoundIniProcessor::create_ini_file()";
			//ALT_ERROR(0, "FAILED AltsoundIniProcessor::create_ini_file()");

			//ALT_OUTDENT;
			//ALT_DEBUG(0, "END AltsoundIniProcessor::parse_altsound_ini()");
			return false;
		}
		//ALT_INFO(0, "SUCCESS AltsoundIniProcessor::create_ini_file()");

		// .ini file is created, open it
		file_in.open(ini_path);
	}

	if (!file_in.good()) {
        std::cout << "Failed to open \"altsound.ini\"";
		//ALT_ERROR(0, "Failed to open \"altsound.ini\"");

		//ALT_OUTDENT;
		//ALT_DEBUG(0, "END parse_altsound_ini()");
		return false;
	}

	// parse ini file
	inipp::Ini<char> ini;
	ini.parse(file_in);

	// ------------------------------------------------------------------------
	// System parsing
	// ------------------------------------------------------------------------

	// get sound command playback flag
	string record_sound_cmds;
	inipp::get_value(ini.sections["system"], "record_sound_cmds", record_sound_cmds);
	ds.m_record_sound_commands = (record_sound_cmds == "1");
    std::cout << "Parsed \"record_sound_cmds\":" << (ds.m_record_sound_commands ? "true" : "false");
	//ALT_INFO(0, "Parsed \"record_sound_cmds\": %s", (ds.m_record_sound_commands ? "true" : "false"));

	// get ROM volume control flag
	string rom_control;
	inipp::get_value(ini.sections["system"], "rom_volume_ctrl", rom_control);
	ds.m_rom_volume_control = (rom_control == "1");
	//ALT_INFO(0, "Parsed \"rom_volume_ctrl\": %s", ds.m_rom_volume_control ? "true" : "false");

	// get skip count
	string skip_count_str;
	inipp::get_value(ini.sections["system"], "cmd_skip_count", skip_count_str);
	try {
		if (!skip_count_str.empty()) {
			const int val = std::stoi(skip_count_str);
			ds.m_skip_count = val;
			//ALT_INFO(0, "Parsed \"cmd_skip_count\": %d", ds.m_skip_count);
		}
	}
	catch (const std::invalid_argument& e) {
		//ALT_ERROR(0, "Invalid number format while parsing cmd_skip_count value: %s\n", skip_count_str.c_str());
		return false;
	}
	catch (const std::out_of_range& e) {
		//ALT_ERROR(0, "Number out of range while parsing cmd_skip_count value: %s\n", skip_count_str.c_str());
		return false;
	}

	// get AltSound format type
	inipp::get_value(ini.sections["format"], "format", ds.m_altsound_format);
	ds.m_altsound_format = normalizeString(ds.m_altsound_format);
	//ALT_INFO(0, "Parsed \"format\": %s", ds.m_altsound_format.c_str());


	// ------------------------------------------------------------------------
	// Logging parsing
	// ------------------------------------------------------------------------

	// parse LOGGING_LEVEL
	string logging;
	inipp::get_value(ini.sections["logging"], "logging_level", logging);
	//ALT_INFO(0, "Parsed \"logging_level\": %s", logging.c_str());
	//const AltsoundLogger::Level level = alog.toLogLevel(logging);
	//if (level == AltsoundLogger::UNDEFINED) {
		//ALT_ERROR(0, "Unknown log level: %s. Defaulting to Error logging", logging.c_str());
		//alog.setLogLevel(AltsoundLogger::Level::Error);
	//}
	//else {
		//alog.setLogLevel(level);
//	}
 
	// ------------------------------------------------------------------------
	// Behavior parsing
	// ------------------------------------------------------------------------

    
    

	//using BB = BehaviorInfo::BehaviorBits;
	bool success = true;

	// ------------------------------------------------------------------------
	// MUSIC behavior parsing
	// ------------------------------------------------------------------------
    SoundTypeBehaviors music_type;
    music_type.m_type = SoundTypeBehaviors::SoundTypes::MUSIC;
	const auto& music_section = ini.sections["music"];

	// parse MUSIC "STOP" behavior
	// MUSIC streams only stop themselves
	
    music_type.m_stopOtherTypesOfSelf = true; // MUSIC stops other MUSIC streams
	success &= parseBehaviorValue(music_section, "stops", music_type);
    success &= parseVolumeValue(music_section, "group_vol", music_type.m_groupVolume);
    ds.m_behaviors.push_back(music_type);

	// DAR@20230823 Leaving this commented code for now in case it becomes necessary
	// to activate this in the near future.  Will remove later, if not needed
	// parse MUSIC "DUCKS" behavior
	// MUSIC streams do not duck other streams
	//success &= parseBehaviorValue(music_section, "ducks", music_behavior.ducks);

	// parse MUSIC "MUSIC_DUCKING_PROFILES"
	// MUSIC streams do not duck other streams
	//auto& music_ducking_section = ini.sections["music_ducking_profiles"];
	//
	//if (!music_ducking_section.empty()) {
	//	success &= parseDuckingProfile(music_ducking_section, music_behavior.ducking_profiles);
	//}
	//else if (music_behavior.ducks != 0) {
	//	// MUSIC behavior specifies stream ducking but no profile defined.
	//	ALT_ERROR(1, "No ducking profiles defined for MUSIC behavior");
	//	success &= false;
	//}
 
	// parse MUSIC "PAUSES" behavior
	// MUSIC streams do not pause other streams
	// success &= parseBehaviorValue(music_section, "pauses", music_behavior.pauses);


	// ------------------------------------------------------------------------
	// CALLOUT behavior parsing
	// ------------------------------------------------------------------------

    SoundTypeBehaviors callout_type;
    callout_type.m_type = SoundTypeBehaviors::SoundTypes::CALLOUT;
	auto& callout_section = ini.sections["callout"];

	// Parse CALLOUT "STOPS" behavior
    callout_type.m_stopOtherTypesOfSelf = true; // CALLOUT stops other CALLOUT streams
	success &= parseBehaviorValue(callout_section, "stops", callout_type);

	// Parse CALLOUT "DUCKS" behavior	
	success &= parseBehaviorValue(callout_section, "ducks", callout_type);

	// parse CALLOUT "CALLOUT_DUCKING_PROFILES"
	auto& callout_ducking_section = ini.sections["callout_ducking_profiles"];

	if (!callout_ducking_section.empty()) {
		success &= parseDuckingProfile(callout_ducking_section, callout_type);
	}
	else {
		// CALLOUT behavior specifies stream ducking but no profile defined.
		//ALT_ERROR(1, "No ducking profiles defined for CALLOUT behavior");
		success &= false;
	}

	// Parse CALLOUT "PAUSES"  behavior
	success &= parseBehaviorValue(callout_section, "pauses", callout_type);

	// Parse CALLOUT "GROUP_VOL" behavior
	success &= parseVolumeValue(callout_section, "group_vol", callout_type.m_groupVolume);

    ds.m_behaviors.push_back(callout_type);

	// ------------------------------------------------------------------------
	// SFX behavior parsing
	// ------------------------------------------------------------------------

    SoundTypeBehaviors sfx_type;
    sfx_type.m_type = SoundTypeBehaviors::SoundTypes::SFX;
	const auto& sfx_section = ini.sections["sfx"];

	// Parse SFX "STOPS" behavior
	// SFX streams do not stop other streams
	//success &= parseBehaviorValue(sfx_section, "stops", sfx_behavior.stops);

	// Parse SFX "DUCKS" behavior
	success &= parseBehaviorValue(sfx_section, "ducks", sfx_type);

	// Parse SFX "SFX_DUCKING_PROFILES"
	const auto& sfx_ducking_section = ini.sections["sfx_ducking_profiles"];

	if (!sfx_ducking_section.empty()) {
		success &= parseDuckingProfile(sfx_ducking_section, sfx_type);
	}
	else {
		// SFX behavior specifies stream ducking but no profile defined.
		//ALT_ERROR(1, "No ducking profiles defined for SFX behavior");
		success &= false;
	}

	// Parse SFX "PAUSES" behavior
	// SFX streams do not pause other streams
	//success &= parseBehaviorValue(sfx_section, "pauses", sfx_behavior.pauses);

	// Parse SFX "GROUP_VOL" behavior
	success &= parseVolumeValue(sfx_section, "group_vol", sfx_type.m_groupVolume);

    ds.m_behaviors.push_back(sfx_type);

	// ------------------------------------------------------------------------
	// SOLO behavior parsing
	// ------------------------------------------------------------------------

    SoundTypeBehaviors solo_type;
    solo_type.m_type = SoundTypeBehaviors::SoundTypes::SOLO;
	const auto& solo_section = ini.sections["solo"];

	// Parse SOLO "STOPS" behavior
    solo_type.m_stopOtherTypesOfSelf = true; // SOLO stops other SOLO streams
	success &= parseBehaviorValue(solo_section, "stops", solo_type);

	// DAR@20230823 Leaving this commented code for now in case it becomes necessary
	// to activate this in the near future.  Will remove later, if not needed
	// Parse SOLO "DUCKS" behavior
	// SOLO streams do not duck other streams
	//success &= parseBehaviorValue(solo_section, "ducks", solo_behavior.ducks);

	// parse SOLO "SOLO_DUCKING_PROFILES"
	// SOLO streams do not duck other streams
	//auto& solo_ducking_section = ini.sections["solo_ducking_profiles"];
	//
	//if (!solo_ducking_section.empty()) {
	//  success &= parseDuckingProfile(solo_ducking_section, solo_behavior.ducking_profiles);
	//}
	//else if (solo_behavior.ducks != 0) {
	//	// SOLO behavior specifies stream ducking but no profile defined.
	//	ALT_ERROR(1, "No ducking profiles defined for SOLO behavior");
	//	success &= false;
	//}

	// Parse SOLO "PAUSES" behavior
	// SOLO streams do not pause other streams
	//success &= parseBehaviorValue(solo_section, "pauses", solo_behavior.pauses);

	// Parse SOLO "GROUP_VOL" behavior
	success &= parseVolumeValue(solo_section, "group_vol", solo_type.m_groupVolume);

    ds.m_behaviors.push_back(solo_type);

	// ------------------------------------------------------------------------
	// OVERLAY behavior parsing
	// ------------------------------------------------------------------------

    SoundTypeBehaviors overlay_type;
    overlay_type.m_type = SoundTypeBehaviors::SoundTypes::OVERLAY;
	const auto& overlay_section = ini.sections["overlay"];

	// Parse OVERLAY "STOPS" behavior
	// OVERLAY streams only stop other OVERLAY streams
    overlay_type.m_stopOtherTypesOfSelf = true; // OVERLAY stops other OVERLAY streams
	//overlay_behavior.stops.set(static_cast<int>(BB::OVERLAY), true); // OVERLAY stops other OVERLAY streams
	//success &= parseBehaviorValue(overlay_section, "stops", overlay_behavior.stops);

	// Parse OVERLAY "DUCKS" behavior
	success &= parseBehaviorValue(overlay_section, "ducks", overlay_type);

	// parse OVERLAY "OVERLAY_DUCKING_PROFILES"
	const auto& overlay_ducking_section = ini.sections["overlay_ducking_profiles"];

	if (!overlay_ducking_section.empty()) {
		success &= parseDuckingProfile(overlay_ducking_section, overlay_type);
	}
	else {
			// OVERLAY behavior specifies stream ducking but no profile defined.
			//ALT_ERROR(1, "No ducking profiles defined for OVERLAY behavior");
			success &= false;
	}

	// Parse OVERLAY "PAUSES" behavior
	// OVERLAY streams do not pause other streams
	//success &= parseBehaviorValue(overlay_section, "pauses", overlay_behavior.pauses);

	// Parse OVERLAY "GROUP_VOL" behavior
	success &= parseVolumeValue(overlay_section, "group_vol", overlay_type.m_groupVolume);

    ds.m_behaviors.push_back(overlay_type);
 
	// ------------------------------------------------------------------------

	//ALT_OUTDENT;
	//ALT_DEBUG(0, "END parse_altsound_ini()"); 
	return success;
}

bool FileParsers::create_altsound_ini(DataStructs& ds)
{
	//ALT_DEBUG(0, "BEGIN AltsoundIniProcessor::create_altsound_ini()");
	//ALT_INDENT;

	const string format = get_altsound_format(ds);

	if (format.empty()) {
		//ALT_ERROR(0, "FAILED AltsoundIniProcessor::get_altsound_format()");

		//ALT_OUTDENT;
		//ALT_DEBUG(0, "END AltsoundIniProcessor::create_altsound_ini()");
		return false;
	}
	//ALT_INFO(0, "SUCCESS AltsoundIniProcessor::get_altsound_format(): %s", format.c_str());

	const string config_str =
		"; ----------------------------------------------------------------------------\n"
		"; altsound.ini - Configuration file for all AltSound formats\n"
		";\n"
		"; If this file does not already exist, it is created automatically the first\n"
		"; time a table configured for AltSound is launched\n"
		"; ----------------------------------------------------------------------------\n"
		"; record_sound_cmds : records all received sound commands and relative\n"
		";                     playback times. This can be useful for testing altsound\n"
		";                     sample combinations without having to recreate them on\n"
		";                     the table. The file can also be edited or created by hand\n"
		";                     to create custom testing scenarios. The\n"
		";                     \"cmdlog.txt\" file is created in the \"tables\"\n"
		";                     folder. This feature is turned off by default\n"
		";\n"
		"; rom_volume_ctrl   : the AltSound processor attempts to recreate original\n"
		";                     playback behavior using commands sent from the ROM.\n"
		";                     This does not work in all cases, resulting in undesirable\n"
		";                     muting of the playback volume. Setting this variable\n"
		";                     to 0 turns this feature off.\n"
		";                     NOTE: This option works for all AltSound formats\n"
		";\n"
		"; cmd_skip_count    : some ROMs send out spurious commands during initialization\n"
		";                     which match valid runtime commands.  In this case, it is not\n"
		";                     desirable to output sound, but want to allow later instances\n"
		";                     to play normally.  This variable allows AltSound authors to\n"
		";                     specify how many initial commands to ignore at startup.\n"
		";                     NOTE:  If the record_sound_cmds flag is set, the skipped\n"
		";                     commands will be included in the recording file.\n"
		"; ----------------------------------------------------------------------------\n"
		"\n"
		"[system]\n"
		"record_sound_cmds = 0\n"
		"rom_volume_ctrl = 1\n"
		"cmd_skip_count = 0\n"
		"\n"
		"; ----------------------------------------------------------------------------\n"
		"; There are three supported AltSound formats:\n"
		";  1. Legacy\n"
		";  2. AltSound\n"
		";  3. G-Sound\n"
		";\n"
		"; Legacy   : the original AltSound format that parses a file/folder structure\n"
		";            similar to the PinSound system. It is no longer used for new\n"
		";            AltSound packages\n"
		";\n"
		"; AltSound : a CSV-based format designed as a replacement for the PinSound\n"
		";            format. This format defines samples according to \"channels\" with\n"
		";            loosely defined behaviors controlled by the associated metadata\n"
		";            \"gain\", \"ducking\", \"stop\", and \"loop\" fields. This is the format\n"
		";            currently used by most AltSound authors\n"
		";\n"
		"; G-Sound  : a new CSV-based format that defines samples according to\n"
		";            contextual types, allowing for more intuitively designed AltSound\n"
		";            packages. General playback behavior is dictated by the assigned\n"
		";            type. Behaviors can evolve without the need for adding\n"
		";            additional CSV fields, and the combinatorial complexity that\n"
		";            comes with it.\n"
		";            NOTE: This option requires the new g-sound.csv format\n"
		"; ----------------------------------------------------------------------------\n"
		"\n"
		"[format]\n"
		"format = " + format + "\n"
		"\n"
		"; ----------------------------------------------------------------------------\n"
		"; To facilitate troubleshooting, the AltSound processor logs events to the\n"
		"; \"altsound.log\" file, which can be found in the \"tables\" folder. There\n"
		"; are 4 logging levels:\n"
		"; 1. Info\n"
		"; 2. Error\n"
		"; 3. Warning\n"
		"; 4. Debug\n"
		";\n"
		"; Setting a logging level will also include lower-ordered logging as well.  For\n"
		"; example, setting logging level to \"Warning\" will also enable \"Info\" and\n"
		"; \"Error\" logging.  By default, logging is set to \"Error\" which will also\n"
		"; include \"Info\" log messages.  The log file will be overwritten each time a\n"
		"; new table is loaded.\n"
		"; ----------------------------------------------------------------------------\n"
		"\n"
		"[logging]\n"
		"logging_level = Error\n"
		"\n"
		"; ----------------------------------------------------------------------------\n"
		"; The section below allows for tailoring of the G-Sound behaviors.\n"
		";\n"
		"; The G-Sound format supports the following sample types:\n"
		";\n"
		"; - MUSIC   : background music. Only one can play at a time\n"
		"; - CALLOUT : voice interludes and callouts. Only one can play at a time\n"
		"; - SFX     : short sounds to supplement table sounds. Multiple can play at a time\n"
		"; - SOLO    : sound played at end-of-ball/game, or tilt. Only one can play at a time\n"
		"; - OVERLAY : sounds played over music/sfx. Only one can play at a time\n"
		";\n"
		"; G - Sound sample types have built-in behaviors. Some can be user-modified.\n"
		"; Where available, the adjustable variables are:\n"
		";\n"
		"; ducks           : specify which sample types are ducked\n"
		"; pauses          : specify which sample types are paused\n"
		"; stops           : specify which sample types are stopped\n"
		"; group_vol       : relative group volume for sample type\n"
		"; ducking_profile : relative ducking volumes for specified sample types\n"
		";\n"
		"; NOTES\n"
		"; - a sample type cannot duck/pause another sample of the same type\n"
		"; - a stopped sample cannot be resumed/restarted\n"
		"; - ducking values are specified as a percentage of the gain of the\n"
		";   affected sample type(s). Values range from 0 to 100 where\n"
		";   0 completely mutes the sample, and 100 effectively negates ducking\n"
		"; - if multiple ducking values apply to a single sample, the lowest\n"
		";   ducking value is used. When the sample with the lowest duck value ends,\n"
		";   the next lowest duck value is used, and so on, until all affecting samples\n"
		";   have ended.\n"
		"; - ducking/pausing ends when the last affecting sample that set it has ended\n"
		"; - If \"ducks\" variable is set, there must be at least one ducking_profile\n"
		";   defined\n"
		"; ----------------------------------------------------------------------------\n"
		"\n"
		"[music]\n"
		"group_vol = 100\n"
		"\n"
		"[callout]\n"
		"ducks = sfx, music, overlay\n"
		"pauses =\n"
		"stops =\n"
		"group_vol = 100\n"
		"\n"
		"[callout_ducking_profiles]\n"
		";profile0 is reserved\n"
		"ducking_profile1 = sfx:65, music:50, overlay:50\n"
		"\n"
		"[sfx]\n"
		"ducks = music\n"
		"group_vol = 100\n"
		"\n"
		"[sfx_ducking_profiles]\n"
		";profile0 is reserved\n"
		"ducking_profile1 = music:50\n"
		"\n"
		"[solo]\n"
		"stops = music, overlay, callout\n"
		"group_vol = 100\n"
		"\n"
		"[overlay]\n"
		"ducks = music, sfx\n"
		"group_vol = 100\n"
		"\n"
		"[overlay_ducking_profiles]\n"
		";profile0 is reserved\n"
		"ducking_profile1 = sfx:65, music:65\n"
		"ducking_profile2 = sfx:80, music:50\n";

	const string ini_path = ds.getAltSoundPath() + "altsound.ini";
	std::ofstream file_out(ini_path);

	if (file_out.is_open()) {
		file_out << config_str;
		file_out.close();
	}
	else {
		//ALT_ERROR(1, "Unable to open file: %s", ini_path.c_str());

		//ALT_OUTDENT;
		//ALT_DEBUG(0, "END AltsoundIniProcessor::create_altsound_ini()");
		return false;
	}

	//ALT_OUTDENT;
	//ALT_DEBUG(0, "END AltsoundIniProcessor::create_altsound_ini()");
	return true;
}

// ---------------------------------------------------------------------------
// Helper function to check if a directory exists
// ---------------------------------------------------------------------------

bool FileParsers::dir_exists(const std::string& path_in)
{
	//ALT_DEBUG(0, "BEGIN dir_exists()");
	//ALT_INDENT;

	struct stat info;

	if (stat(path_in.c_str(), &info) != 0) {
		//ALT_INFO(0, "Directory: %s does not exist", path_in.c_str());
		//ALT_DEBUG(0, "END dir_exists()");
		return false;
	}
	//ALT_INFO(0, "Directory: %s exists", path_in.c_str());

	//ALT_OUTDENT;
	//ALT_INFO(0, "END dir_exists()");
	return (info.st_mode & S_IFDIR) != 0;
}

// ---------------------------------------------------------------------------
// Helper function to determine AltSound format
// ---------------------------------------------------------------------------

// DAR@20230623
// This is in support of altsound.ini file creation for packages that don't
// already have one (legacy).  It works in order of precedence:
//
//  1. presence of g-sound.csv
//  2. presence of altsound.csv
//	3. presence of PinSound directory structure
//
// Once the .ini file is created, it can be modified to adjust preference.
//
string FileParsers::get_altsound_format(DataStructs& ds)
{
	//ALT_DEBUG(0, "BEGIN get_altsound_format()");
	//ALT_INDENT;

	const std::vector<std::pair<string, string>> filesAndFormats{
		{ "g-sound.csv", "g-sound" },
		{ "altsound.csv", "altsound" },
	};

	for (const auto& fileAndFormat : filesAndFormats) {
		std::ifstream ini(ds.getAltSoundPath() + fileAndFormat.first);
		if (ini) {
			//ALT_INFO(0, ("Using " + fileAndFormat.second + " format").c_str());
			//ALT_OUTDENT;
			//ALT_DEBUG(0, "END get_altsound_format()");
			return fileAndFormat.second;
		}
	}

	const std::vector<string> directories{
		"jingle",
		"music",
		"sfx",
		"voice"
	};

	if (std::any_of(directories.begin(), directories.end(), [&](const auto& directory) {
		return dir_exists(ds.getAltSoundPath() + directory);
	}))
	{
		//ALT_INFO(0, "Using Legacy (PinSound) format");
		//ALT_OUTDENT;
		//ALT_DEBUG(0, "END get_altsound_format()");
		return "legacy";
	}


	//ALT_OUTDENT;
	//ALT_DEBUG(0, "END get_altsound_format()");
	return string();
}

std::string FileParsers::trim(const std::string& str)
{
	const size_t first = str.find_first_not_of(' ');
	if (std::string::npos == first)
	{
		return str;
	}
	const size_t last = str.find_last_not_of(' ');
	return str.substr(first, (last - first + 1));
}

// ----------------------------------------------------------------------------
// Helper function to trim whitespace and convert from string and convert to
// lowercase
// ---------------------------------------------------------------------------

string FileParsers::normalizeString(string str) 
{
	str = trim(str); // remove whitespace
	std::transform(str.begin(), str.end(), str.begin(), ::tolower); // convert to lowercase
	return str;
}

// ---------------------------------------------------------------------------
// Helper function to parse G-Sound behavior values
// ---------------------------------------------------------------------------

bool FileParsers::parseBehaviorValue(const IniSection& section, const string& key, SoundTypeBehaviors& st)
{
    string token;
    string parsed_value;
    inipp::get_value(section, key, parsed_value);

    std::stringstream ss(parsed_value);
    while (std::getline(ss, token, ',')) {
        token = normalizeString(token);

         if (token == "music") {
            if (key == "stops")
                st.addStops(SoundTypeBehaviors::SoundTypes::MUSIC);
            else if (key == "pauses")
                st.addPauses(SoundTypeBehaviors::SoundTypes::MUSIC);
            else if (key == "ducks")
                st.addDucks(SoundTypeBehaviors::SoundTypes::MUSIC);
        }
        else if (token == "callout") {
            if (key == "stops")
                st.addStops(SoundTypeBehaviors::SoundTypes::CALLOUT);
            else if (key == "pauses")
                st.addPauses(SoundTypeBehaviors::SoundTypes::CALLOUT);
            else if (key == "ducks")
                st.addDucks(SoundTypeBehaviors::SoundTypes::CALLOUT);
        }
        else if (token == "sfx") {
            if (key == "stops")
                st.addStops(SoundTypeBehaviors::SoundTypes::SFX);
            else if (key == "pauses")
                st.addPauses(SoundTypeBehaviors::SoundTypes::SFX);
            else if (key == "ducks")
                st.addDucks(SoundTypeBehaviors::SoundTypes::SFX);
        }
        else if (token == "solo") {
            if (key == "stops")
                st.addStops(SoundTypeBehaviors::SoundTypes::SOLO);
            else if (key == "pauses")
                st.addPauses(SoundTypeBehaviors::SoundTypes::SOLO);
            else if (key == "ducks")
                st.addDucks(SoundTypeBehaviors::SoundTypes::SOLO);
        }
        else if (token == "overlay") {
            if (key == "stops")
                st.addStops(SoundTypeBehaviors::SoundTypes::OVERLAY);
            else if (key == "pauses")
                st.addPauses(SoundTypeBehaviors::SoundTypes::OVERLAY);
            else if (key == "ducks")
                st.addDucks(SoundTypeBehaviors::SoundTypes::OVERLAY);
        } 
    }
    return true;
}

// ---------------------------------------------------------------------------
// Helper function to parse G-Sound behavior volume values
// ---------------------------------------------------------------------------

bool FileParsers::parseVolumeValue(const IniSection& section, const string& key, float& volume)
{
    string parsed_value;
    if (!inipp::get_value(section, key, parsed_value)) {
        return true;
    }

    try {
        const int val = std::stoi(parsed_value);
        volume = (float)clamp(val, 0, 100) / 100.f;
    }
    catch (const std::invalid_argument& e) {
        //ALT_ERROR(0, "Invalid number format while parsing volume value: %s\n", parsed_value.c_str());
        return false;
    }
    catch (const std::out_of_range& e) {
        //ALT_ERROR(0, "Number out of range while parsing volume value: %s\n", parsed_value.c_str());
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Helper function to parse G-Sound ducking profiles
// ---------------------------------------------------------------------------

bool FileParsers::parseDuckingProfile(const IniSection& ducking_section, SoundTypeBehaviors& st)
{
	//ALT_DEBUG(0, "BEGIN AltsoundIniProcessor::parseDuckingProfile()");
	//ALT_INDENT;

	// Iterate over the key-value pairs in the ducking_section
	for (const auto& pair : ducking_section) {
		const inipp::Ini<char>::String& key = pair.first; // profile name
		const inipp::Ini<char>::String& value = pair.second; // profile value string

		// Check if the key starts with "ducking_profile" to identify the profiles
		string subkey_test = "ducking_profile";
		string subkey = normalizeString(key.substr(0, subkey_test.size()));

		if (subkey != subkey_test) {
			//ALT_ERROR(1, "Failed to parse ini file - unexpected key: %s", key.c_str());

			//ALT_OUTDENT;
			//ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
			return false;
		}

		// Extract the profile number from the key
		// Exception handling added to catch invalid stoi conversion
		int profile_number = 0;
		try {
			profile_number = std::stoi(key.substr(subkey.size()));
		}
		catch (std::invalid_argument& e) {
			//ALT_ERROR(1, "Invalid profile number: %s", key.substr(subkey.size()).c_str());

			//ALT_OUTDENT;
			//ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
			return false;
		}

		// Parse the value to extract the individual tokens and volume values
		std::istringstream valueStream(value);
		string token;
		//DuckingProfile profile;

		while (std::getline(valueStream, token, ',')) {
			// Extract the label and volume value
			std::istringstream tokenStream(token);
			string label;
			int val;

			// Split the token at the ':' delimiter
			if (!std::getline(tokenStream, label, ':')) {
				//ALT_ERROR(1, "Failed to parse label: %s", token.c_str());

				//ALT_OUTDENT;
				//ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
				return false;
			}

			label = normalizeString(label);
			if (!(tokenStream >> val)) {
				//ALT_ERROR(1, "Failed to parse value: %s", token.c_str());

				//ALT_OUTDENT;
				//ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
				return false;
			}

			const float volume = val > 100 ? 1.0f : val <= 0 ? 0.0f : static_cast<float>(val) / 100.f;

			// Set the corresponding volume based on the label
            st.addDuckingProfile(profile_number,st.stringToSoundTypeEnum(label), volume);
        
	    }
    }

	//ALT_OUTDENT;
	//ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
	return true;
}



// ---------------------------------------------------------------------------
// Helper function to parse G-Sound ducking profiles
// ---------------------------------------------------------------------------


/*

bool AltsoundIniProcessor::parseDuckingProfile(const IniSection& ducking_section, ProfileMap& profiles)
{
ALT_DEBUG(0, "BEGIN AltsoundIniProcessor::parseDuckingProfile()");
ALT_INDENT;

// Iterate over the key-value pairs in the ducking_section
for (const auto& pair : ducking_section) {
const inipp::Ini<char>::String& key = pair.first; // profile name
const inipp::Ini<char>::String& value = pair.second; // profile value string

// Check if the key starts with "ducking_profile" to identify the profiles
string subkey_test = "ducking_profile";
string subkey = normalizeString(key.substr(0, subkey_test.size()));

if (subkey != subkey_test) {
ALT_ERROR(1, "Failed to parse ini file - unexpected key: %s", key.c_str());

ALT_OUTDENT;
ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
return false;
}

// Extract the profile number from the key
// Exception handling added to catch invalid stoi conversion
int profile_number = 0;
try {
profile_number = std::stoi(key.substr(subkey.size()));
}
catch (std::invalid_argument& e) {
ALT_ERROR(1, "Invalid profile number: %s", key.substr(subkey.size()).c_str());

ALT_OUTDENT;
ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
return false;
}

// Parse the value to extract the individual tokens and volume values
std::istringstream valueStream(value);
string token;
DuckingProfile profile;

while (std::getline(valueStream, token, ',')) {
// Extract the label and volume value
std::istringstream tokenStream(token);
string label;
int val;

// Split the token at the ':' delimiter
if (!std::getline(tokenStream, label, ':')) {
ALT_ERROR(1, "Failed to parse label: %s", token.c_str());

ALT_OUTDENT;
ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
return false;
}

label = normalizeString(label);
if (!(tokenStream >> val)) {
ALT_ERROR(1, "Failed to parse value: %s", token.c_str());

ALT_OUTDENT;
ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
return false;
}

const float volume = val > 100 ? 1.0f : val <= 0 ? 0.0f : static_cast<float>(val) / 100.f;

// Set the corresponding volume based on the label
if (label == "music") {
profile.music_duck_vol = volume;
}
else if (label == "callout") {
profile.callout_duck_vol = volume;
}
else if (label == "sfx") {
profile.sfx_duck_vol = volume;
}
else if (label == "solo") {
profile.solo_duck_vol = volume;
}
else if (label == "overlay") {
profile.overlay_duck_vol = volume;
}
else {
ALT_ERROR(1, "Unknown sample type label: %s", label.c_str());

ALT_OUTDENT;
ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
return false;
}
}

// Store the parsed profile in the ducking_profiles map
string profileKey = "profile" + std::to_string(profile_number);
profiles[profileKey] = profile;
}

ALT_OUTDENT;
ALT_DEBUG(0, "END AltsoundIniProcessor::parseDuckingProfile()");
return true;
}
*/