// license:GPLv3+
#include <vector>
#include <unordered_map>

#ifndef SOUNDTYPEBEHAVIORS_H
#define SOUNDTYPEBEHAVIORS_H
class SoundTypeBehaviors {

    public:
        struct  SoundTypes {
            enum Value {
                MUSIC   = 0,
                CALLOUT = 1,
                SFX     = 2,
                SOLO    = 3,
                OVERLAY = 4,
                UNKNOWN = 5,
            };
        };

        SoundTypes::Value m_type;
        float m_groupVolume = 100;
        bool m_stopOtherTypesOfSelf = false;
        std::vector<SoundTypes::Value> m_stops;
        std::vector<SoundTypes::Value> m_pauses;
        std::vector<SoundTypes::Value> m_ducks;
        
        void addDuckingProfile(int profileNum, SoundTypes::Value type, float volume);
        int getDuckingProfileVolByType(int profileNum, SoundTypes::Value type);
        
        // how other sounds behave when this sound type plays
        void addStops(SoundTypes::Value type);
        void addPauses(SoundTypes::Value type);
        void addDucks(SoundTypes::Value type);
        SoundTypes::Value stringToSoundTypeEnum(const std::string& type);

        private:
            std::unordered_map<int,std::vector<std::pair<SoundTypes::Value, float>>> m_duckingProfiles;
};
#endif