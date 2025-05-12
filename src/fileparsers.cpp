// license:GPLv3+
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <algorithm>
#include <sys/stat.h>
#include <unordered_set>
#include <plog/Log.h>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

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
	PLOGI << "BEGIN parseCmdFile" << std::endl;

	try {
		std::ifstream inFile(ds.m_init_data.cmd_file);
		if (!inFile.is_open())
        {
            PLOGE << "Unable to open file: " <<  ds.m_init_data.cmd_file;
			throw std::runtime_error("Unable to open file: " + ds.m_init_data.cmd_file);
        }
		string line;

		// Process paths and game name
		if (!std::getline(inFile, line))
        {
            PLOGE << "altsound_path value could not be determined";
			throw std::runtime_error("altsound_path value could not be determined");
        }

		string altsoundPath = extractValue(line);
	 	std::replace(altsoundPath.begin(), altsoundPath.end(), '\\', '/');
		if (altsoundPath.back() != '/')
			altsoundPath += '/';

		size_t altsoundPos = altsoundPath.find("/altsound/");
		if (altsoundPos == string::npos)
        {
            PLOGE << "altsound_path value could not be determined";
			throw std::runtime_error("altsound_path value could not be determined");
        }

        ds.m_init_data.altsound_path = altsoundPath;
		ds.m_init_data.vpm_path = altsoundPath.substr(0, altsoundPos + 1);

		size_t nextSlashPos = altsoundPath.find('/', altsoundPos + 10);
		if (nextSlashPos == string::npos)
        {
            PLOGE << "game name could not be determined";
			throw std::runtime_error("game name could not be determined");
        }

        ds.m_init_data.game_name = altsoundPath.substr(altsoundPos + 10, nextSlashPos - (altsoundPos + 10));

		// Process hardware_gen
		if (!std::getline(inFile, line))
        {
            PLOGE << "hardware_gen value could not be determined";
			throw std::runtime_error("hardware_gen value could not be determined");
        }

		std::string hexString = extractValue(line);
		ds.m_init_data.hardware_gen = (DataStructs::ALTSOUND_HARDWARE_GEN)std::stoull(hexString, nullptr, 16);

		PLOGI << "Altsound path: " << ds.m_init_data.altsound_path;
		PLOGI << "VPinMAME path: " << ds.m_init_data.vpm_path;
		PLOGI << "Game name: " << ds.m_init_data.game_name;
		PLOGI << "Hardware Gen: 0x" 
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
            {
                PLOGE << "Unable to parse time: " << temp;
				throw std::runtime_error("Unable to parse time: " + temp);
            }

			const string HEX_PREFIX = "0x";
			ss >> std::ws;
			if (!std::getline(ss, command, ',')) continue;
			if (command.substr(0, HEX_PREFIX.length()) == HEX_PREFIX)
				command = command.substr(HEX_PREFIX.length());
			else
            {
                PLOGE << "Command value is not in hexadecimal format: " << command;
				throw std::runtime_error("Command value is not in hexadecimal format: " + command);
            }

			data.snd_cmd = std::strtoul(command.c_str(), &end, 16);
			if (end == command.c_str())
            {
                PLOGE << "Unable to parse command: " << command;
				throw std::runtime_error("Unable to parse command: " + command);
            }

            ds.m_init_data.test_data.push_back(data);
		}

		inFile.close();
		PLOGI << "END parseCmdFile";
		return true;
	}
	catch (const std::runtime_error& e) {
		PLOGE << e.what();
		PLOGE << "END parseCmdFile";
		return false;
	}
}

bool FileParsers::parse_altsound_ini(DataStructs& ds)
{
    PLOGI << "parse_altsound_ini()";
	string szPinmamePath = ds.m_init_data.vpm_path;
	std::replace(szPinmamePath.begin(), szPinmamePath.end(), '\\', '/');

	if (szPinmamePath.back() != '/')
		szPinmamePath += '/';

    ds.m_altSoundPath = szPinmamePath + "altsound/" + ds.m_init_data.game_name + '/';

	PLOGI << "BEGIN FileParsers::parse_altsound_ini()";
	
	// if altsound.ini does not exist, create it
	string ini_path = ds.getAltSoundPath() + "altsound.ini";
    
	std::ifstream file_in(ini_path);
	if (!file_in.good()) {
        PLOGI << "\"altsound.ini\" not found. Creating it." << ")";
		if (!create_altsound_ini(ds)) {
            PLOGE << "FAILED FileParsers::create_ini_file()";
			PLOGI << "END FileParsers::parse_altsound_ini()";
			return false;
		}
	    PLOGI << "SUCCESS FileParsers::create_ini_file()";

		// .ini file is created, open it
		file_in.open(ini_path);
	}

	if (!file_in.good()) {
        PLOGE << "Failed to open \"altsound.ini\"";
		PLOGI << "END parse_altsound_ini()";
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
    PLOGI << "Parsed \"record_sound_cmds\":" <<  std::string((ds.m_record_sound_commands ? "true" : "false"));

	// get ROM volume control flag
	string rom_control;
	inipp::get_value(ini.sections["system"], "rom_volume_ctrl", rom_control);
	ds.m_rom_volume_control = (rom_control == "1");
	PLOGI << "Parsed \"rom_volume_ctrl\": " << (ds.m_rom_volume_control ? "true" : "false");

	// get skip count
	string skip_count_str;
	inipp::get_value(ini.sections["system"], "cmd_skip_count", skip_count_str);
	try {
		if (!skip_count_str.empty()) {
			const int val = std::stoi(skip_count_str);
			ds.m_skip_count = val;
			PLOGI << "Parsed \"cmd_skip_count\": " << ds.m_skip_count;
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
	PLOGI <<  "Parsed \"format\": " << ds.m_altsound_format;

	// ------------------------------------------------------------------------
	// Logging parsing
	// ------------------------------------------------------------------------
	string logging;
	inipp::get_value(ini.sections["logging"], "logging_level", logging);

    // SUPERHAC - need to figure this out

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

	bool success = true;

	// ------------------------------------------------------------------------
	// MUSIC behavior parsing
	// ------------------------------------------------------------------------
    SoundTypeBehaviors music_type;
    music_type.m_type = SoundTypeBehaviors::SoundTypes::MUSIC;
	const auto& music_section = ini.sections["music"];
    music_type.m_stopOtherTypesOfSelf = true; // MUSIC stops other MUSIC streams
	success &= parseBehaviorValue(music_section, "stops", music_type);
    success &= parseVolumeValue(music_section, "group_vol", music_type.m_groupVolume);
    ds.m_behaviors.push_back(music_type);

	// ------------------------------------------------------------------------
	// CALLOUT behavior parsing
	// ------------------------------------------------------------------------

    SoundTypeBehaviors callout_type;
    callout_type.m_type = SoundTypeBehaviors::SoundTypes::CALLOUT;
	auto& callout_section = ini.sections["callout"];
    callout_type.m_stopOtherTypesOfSelf = true; // CALLOUT stops other CALLOUT streams
	success &= parseBehaviorValue(callout_section, "stops", callout_type);
	success &= parseBehaviorValue(callout_section, "ducks", callout_type);
	auto& callout_ducking_section = ini.sections["callout_ducking_profiles"];
	if (!callout_ducking_section.empty()) {
		success &= parseDuckingProfile(callout_ducking_section, callout_type);
	}
	else {
		// CALLOUT behavior specifies stream ducking but no profile defined.
		PLOGE << "No ducking profiles defined for CALLOUT behavior";
		success &= false;
	}
	success &= parseBehaviorValue(callout_section, "pauses", callout_type);
	success &= parseVolumeValue(callout_section, "group_vol", callout_type.m_groupVolume);
    ds.m_behaviors.push_back(callout_type);

	// ------------------------------------------------------------------------
	// SFX behavior parsing
	// ------------------------------------------------------------------------
    // Parse SFX "STOPS" behavior
	// SFX streams do not stop other streams
    // SFX streams do not pause other streams

    SoundTypeBehaviors sfx_type;
    sfx_type.m_type = SoundTypeBehaviors::SoundTypes::SFX;
	const auto& sfx_section = ini.sections["sfx"];
	success &= parseBehaviorValue(sfx_section, "ducks", sfx_type);
	const auto& sfx_ducking_section = ini.sections["sfx_ducking_profiles"];
	if (!sfx_ducking_section.empty()) {
		success &= parseDuckingProfile(sfx_ducking_section, sfx_type);
	}
	else {
		// SFX behavior specifies stream ducking but no profile defined.
		PLOGE <<  "No ducking profiles defined for SFX behavior";
		success &= false;
	}
	success &= parseVolumeValue(sfx_section, "group_vol", sfx_type.m_groupVolume);
    ds.m_behaviors.push_back(sfx_type);

	// ------------------------------------------------------------------------
	// SOLO behavior parsing
	// ------------------------------------------------------------------------

    SoundTypeBehaviors solo_type;
    solo_type.m_type = SoundTypeBehaviors::SoundTypes::SOLO;
	const auto& solo_section = ini.sections["solo"];
    solo_type.m_stopOtherTypesOfSelf = true; // SOLO stops other SOLO streams
	success &= parseBehaviorValue(solo_section, "stops", solo_type);
	success &= parseVolumeValue(solo_section, "group_vol", solo_type.m_groupVolume);
    ds.m_behaviors.push_back(solo_type);

	// ------------------------------------------------------------------------
	// OVERLAY behavior parsing
	// ------------------------------------------------------------------------
    // Parse OVERLAY "STOPS" behavior
	// OVERLAY streams only stop other OVERLAY streams
    // OVERLAY streams do not pause other streams

    SoundTypeBehaviors overlay_type;
    overlay_type.m_type = SoundTypeBehaviors::SoundTypes::OVERLAY;
	const auto& overlay_section = ini.sections["overlay"];
    overlay_type.m_stopOtherTypesOfSelf = true; // OVERLAY stops other OVERLAY streams
	success &= parseBehaviorValue(overlay_section, "ducks", overlay_type);
	const auto& overlay_ducking_section = ini.sections["overlay_ducking_profiles"];
	if (!overlay_ducking_section.empty()) {
		success &= parseDuckingProfile(overlay_ducking_section, overlay_type);
	}
	else {
			// OVERLAY behavior specifies stream ducking but no profile defined.
			PLOGE << "No ducking profiles defined for OVERLAY behavior";
			success &= false;
	}
	success &= parseVolumeValue(overlay_section, "group_vol", overlay_type.m_groupVolume);
    ds.m_behaviors.push_back(overlay_type);

	PLOGI << "END parse_altsound_ini()"; 
	return success;
}

bool FileParsers::create_altsound_ini(DataStructs& ds)
{
	PLOGE << "BEGIN FileParsers::create_altsound_ini()";
	const string format = get_altsound_format(ds);

	if (format.empty()) {
		PLOGE << "FAILED AltsoundIniProcessor::get_altsound_format()";
		PLOGI << "END AltsoundIniProcessor::create_altsound_ini()";
		return false;
	}
	PLOGI << "SUCCESS AltsoundIniProcessor::get_altsound_format(): " << format;

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

	//const string ini_path = ds.getAltSoundPath() + "altsound.ini";
	std::ofstream file_out(ds.getAltSoundPath() + "altsound.ini");

	if (file_out.is_open()) {
		file_out << config_str;
		file_out.close();
	}
	else {
		PLOGE << "Unable to open file: %s",ds.getAltSoundPath() + "altsound.ini";
		PLOGI << "END AltsoundIniProcessor::create_altsound_ini()";
		return false;
	}

	PLOGI << "END AltsoundIniProcessor::create_altsound_ini()";
	return true;
}

// ---------------------------------------------------------------------------
// Helper function to check if a directory exists
// ---------------------------------------------------------------------------

bool FileParsers::dir_exists(const std::string& path_in)
{
	PLOGI << "BEGIN dir_exists()";
	struct stat info;

	if (stat(path_in.c_str(), &info) != 0) {
		PLOGE << "Directory: " << path_in << "does not exist";
		PLOGI <<  "END dir_exists()";
		return false;
	}
	PLOGI << "Directory: " << path_in <<  "exists";
	PLOGI << "END dir_exists()";
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
	PLOGI << "BEGIN get_altsound_format()";

	const std::vector<std::pair<string, string>> filesAndFormats{
		{ "g-sound.csv", "g-sound" },
		{ "altsound.csv", "altsound" },
	};

	for (const auto& fileAndFormat : filesAndFormats) {
		std::ifstream ini(ds.getAltSoundPath() + fileAndFormat.first);
		if (ini) {
			PLOGI << "Using " << fileAndFormat.second << " format";
			PLOGI << "END get_altsound_format()";
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
		PLOGI << "Using Legacy (PinSound) format";
		PLOGI <<  "END get_altsound_format()";
		return "legacy";
	}

	PLOGI << "END get_altsound_format()";
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
        PLOGE << "Invalid number format while parsing volume value: " << parsed_value;
        return false;
    }
    catch (const std::out_of_range& e) {
        PLOGE << "Number out of range while parsing volume value: " << parsed_value;
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Helper function to parse G-Sound ducking profiles
// ---------------------------------------------------------------------------

bool FileParsers::parseDuckingProfile(const IniSection& ducking_section, SoundTypeBehaviors& st)
{
	PLOGI << "BEGIN FileParsers::parseDuckingProfile()";

	// Iterate over the key-value pairs in the ducking_section
	for (const auto& pair : ducking_section) {
		const inipp::Ini<char>::String& key = pair.first; // profile name
		const inipp::Ini<char>::String& value = pair.second; // profile value string

		// Check if the key starts with "ducking_profile" to identify the profiles
		string subkey_test = "ducking_profile";
		string subkey = normalizeString(key.substr(0, subkey_test.size()));

		if (subkey != subkey_test) {
			PLOGE << "Failed to parse ini file - unexpected key: " << key;
			PLOGI << "FileParsers::parseDuckingProfile()";
			return false;
		}

		// Extract the profile number from the key
		// Exception handling added to catch invalid stoi conversion
		int profile_number = 0;
		try {
			profile_number = std::stoi(key.substr(subkey.size()));
		}
		catch (std::invalid_argument& e) {
			PLOGE <<  "Invalid profile number: " << key.substr(subkey.size()).c_str();
			PLOGI << "END FileParsers::parseDuckingProfile()";
			return false;
		}

		// Parse the value to extract the individual tokens and volume values
		std::istringstream valueStream(value);
		string token;
		
		while (std::getline(valueStream, token, ',')) {
			// Extract the label and volume value
			std::istringstream tokenStream(token);
			string label;
			int val;

			// Split the token at the ':' delimiter
			if (!std::getline(tokenStream, label, ':')) {
				PLOGE << "Failed to parse label: " << token.c_str();
				PLOGI << "END FileParsers::parseDuckingProfile()";
				return false;
			}

			label = normalizeString(label);
			if (!(tokenStream >> val)) {
				PLOGE << "Failed to parse value: " << token.c_str();
				PLOGI << "END FileParsers::parseDuckingProfile()";
				return false;
			}

			const float volume = val > 100 ? 1.0f : val <= 0 ? 0.0f : static_cast<float>(val) / 100.f;

			// Set the corresponding volume based on the label
            st.addDuckingProfile(profile_number,st.stringToSoundTypeEnum(label), volume);
	    }
    }

	PLOGI << "END FileParsers::parseDuckingProfile()";
	return true;
}

bool FileParsers::parseGSoundCVS(DataStructs& ds)
{
	string filename = ds.m_altSoundPath + "g-sound.csv";
	PLOGI << "BEGIN parsing gsound csv: " << filename;

	std::ifstream file(filename);
	if (!file.is_open()) {
		PLOGE <<  "Unable to open file: %s", filename.c_str();
		PLOGI <<  "END parsing gsound csv";
		return false;
	}

	string line;

	// skip header row
	std::getline(file, line);

	std::unordered_set<string> allowed_types = {
		"music",
		"callout",
		"solo",
		"sfx",
		"overlay" 
	};
	
	bool success = true;

	try {
		while (std::getline(file, line)) {
			if (!line.empty() && line.back() == '\r')
				line.pop_back();

			if (line.empty())
				continue;

			// Some Altsounds use quotes around fields.  These need to be removed.
			line.erase(std::remove(line.begin(), line.end(), '\"'), line.end());

			std::stringstream ss(line);
			string field;
			DataStructs::GSoundSampleInfo entry;

			// Read ID field (unsigned hexadecimal)
			if (std::getline(ss, field, ',')) {
				field = trim(field);
				entry.id = std::stoul(field, nullptr, 16);
			}
			else {
				PLOGE <<  "Failed to parse ID field";
				success = false;
				break;
			}

			// Read TYPE field
			if (std::getline(ss, field, ',')) {
				field = trim(field);
				std::transform(field.begin(), field.end(), field.begin(), ::tolower);
				entry.type = field;

				if (allowed_types.find(field) == allowed_types.end()) {
					PLOGE << "%s is not a known sample type";
					success = false;
					break;
				}

				if (field == "music") {
					entry.loop = true;
				}
			}
			else {
				PLOGE <<  "Failed to parse TYPE field";
				entry.type.clear();
				success = false;
				break;
			}

			// Read GAIN field (float)
			if (std::getline(ss, field, ',')) {
				field = trim(field);
				float val = std::stof(field);
				entry.gain = val < 0.0f ? 0.0f : val > 100.0f ? 1.0f : val / 100.0f;
			}
			else {
				PLOGE << "Failed to parse GAIN field";
				entry.gain = 1.0f;
				success = false;
				break;
			}

			// Read DUCKING_PROFILE field (uint)
			if (std::getline(ss, field, ','))
			{
				if (field.empty()) {
					field = "0"; // default value
				}

				field = trim(field);
				unsigned int val = std::stoul(field);
				entry.ducking_profile = val;
			}
			else {
				PLOGE << "Failed to parse DUCKING_PROFILE field";
				entry.ducking_profile = 0;
				success = false;
				break;
			}

			// Read FNAME field
			if (std::getline(ss, field, ','))
			{
				field = trim(field);
				if (field.empty()) {
					PLOGE << "Sample filename is blank";
					success = false;
					entry.fname.clear();  // assign some default value
					break;
				}

				string sample_path = ds.m_altSoundPath + field;

				// Normalize to forward slashes
				std::replace(sample_path.begin(), sample_path.end(), '\\', '/');
				entry.fname = sample_path;
			}
			else {
				PLOGE << "Failed to parse FNAME";
				success = false;
				entry.fname.clear();  // assign some default value
				break;
			}
			
			ds.m_gsoundSamples.push_back(entry);

			std::ostringstream debug_stream;
			debug_stream << "ID = 0x" << std::setfill('0') << std::setw(4) << std::hex << entry.id << std::dec
				<< ", TYPE = " << entry.type
				<< ", GAIN = " << std::fixed << std::setprecision(2) << entry.gain
				<< ", DUCK_PRF = " << entry.ducking_profile
				<< ", FNAME = " << entry.fname;

			PLOGI << debug_stream.str().c_str();
		}
	}
	catch (const std::exception& e) {
	PLOGE << "GSoundCsvParser::parse(): " << e.what();
	}

	file.close();

	PLOGI << "END BEGIN parsing gsound csv";
	return success;
}