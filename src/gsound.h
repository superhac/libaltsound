// license:GPLv3+
#include <random>
#include "datastructs.h"

class GSound {
    public:
        DataStructs m_pds;
        GSound(DataStructs& ds);

        bool m_sdlAudioinitialized = false;
    private: 
        std::mt19937 m_generator; // mersenne twister
        float m_masterVol = 1.0f;
        float m_globalVol = 1.0f;
        bool m_rom_volume_control = true;
        bool m_record_sound_commands = false;
        unsigned int m_skip_count = 0; // can this use ds.m_skip_count???

        void init();
        bool loadSamples();

    };