// license:GPLv3+
#include <iostream>
#include "soundtypebehaviors.h"

void SoundTypeBehaviors::addDuckingProfile(int profileNum, SoundTypes::Value type, float volume)
{
    m_duckingProfiles[profileNum].emplace_back(type, volume);
}

int SoundTypeBehaviors::getDuckingProfileVolByType(int profileNum, SoundTypes::Value type)
{
    // Check if profileNum is valid
    if (profileNum < 0 || m_duckingProfiles.find(profileNum) == m_duckingProfiles.end()) {
        std::cerr << "Invalid profile index: " << profileNum << std::endl;
        return -1;
    }
    auto profile = m_duckingProfiles[profileNum];
    for (size_t pairIndex = 0; pairIndex < profile.size(); ++pairIndex) {
        const auto& pair = profile[pairIndex];
        if (type == pair.first)
        {
            std::cout << "Found: " << pair.second << std::endl;
            return pair.second;
        }
    }
    std::cout << "Did not find type in profile";
    return -1;
}

void SoundTypeBehaviors::addStops(SoundTypes::Value type)
{
    m_stops.emplace_back(type);
}

void SoundTypeBehaviors::addPauses(SoundTypes::Value type)
{
    m_pauses.emplace_back(type);
}

void SoundTypeBehaviors::addDucks(SoundTypes::Value type)
{
    m_ducks.emplace_back(type);
}

SoundTypeBehaviors::SoundTypes::Value SoundTypeBehaviors::stringToSoundTypeEnum(const std::string& type) 
{
    if (type == "music")
        return SoundTypes::MUSIC;
    else if (type == "callout")
        return SoundTypes::CALLOUT;
    else if (type == "sfx")
        return SoundTypes::SFX;
    else if (type == "solo")
        return SoundTypes::SOLO;
    else if (type == "overlay")
        return SoundTypes::OVERLAY;
    else
        return SoundTypes::UNKNOWN;
}