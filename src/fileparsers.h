// license:GPLv3+

#include <vector>
#include <iostream>
#include <cstdint>
#include "datastructs.h"
#include "inipp.h"
#include "soundtypebehaviors.h"

using IniSection = std::map<inipp::Ini<char>::String, inipp::Ini<char>::String>;
using std::string;

class FileParsers {
    public:
        bool parseCmdFile(DataStructs& ds);
        bool parse_altsound_ini(DataStructs& ds);
        bool parseGSoundCVS(DataStructs& ds);
    private: 
        string extractValue(const string& line); 
        bool create_altsound_ini(DataStructs& ds);
        string get_altsound_format(DataStructs& ds);
        string normalizeString(string str);
        bool parseBehaviorValue(const IniSection& section, const string& key, SoundTypeBehaviors& st);
        std::string trim(const std::string& str);
        bool dir_exists(const std::string& path_in);
        bool parseVolumeValue(const IniSection& section, const string& key, float& volume);
        bool parseDuckingProfile(const IniSection& ducking_section, SoundTypeBehaviors& st);

    };
    
