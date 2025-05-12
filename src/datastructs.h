// license:GPLv3+
#include <vector>
#include <iostream>
#include <cstdint>
#include "soundtypebehaviors.h"
using std::string;

#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

class DataStructs {

    public:
        struct TestData {
            unsigned int msec;
            uint32_t snd_cmd;
        };

        typedef enum : uint64_t {
            ALTSOUND_HARDWARE_GEN_NONE = 0x0000000000000,
            ALTSOUND_HARDWARE_GEN_WPCALPHA_1 = 0x0000000000001,  // Alpha-numeric display S11 sound, Dr Dude 10/90
            ALTSOUND_HARDWARE_GEN_WPCALPHA_2 = 0x0000000000002,  // Alpha-numeric display,  - The Machine BOP 4/91
            ALTSOUND_HARDWARE_GEN_WPCDMD = 0x0000000000004,      // Dot Matrix Display, Terminator 2 7/91 - Party Zone 10/91
            ALTSOUND_HARDWARE_GEN_WPCFLIPTRON = 0x0000000000008, // Fliptronic flippers, Addams Family 2/92 - Twilight Zone 5/93
            ALTSOUND_HARDWARE_GEN_WPCDCS = 0x0000000000010,      // DCS Sound system, Indiana Jones 10/93 - Popeye 3/94
            ALTSOUND_HARDWARE_GEN_WPCSECURITY = 0x0000000000020, // Security chip, World Cup Soccer 3/94 - Jackbot 10/95
            ALTSOUND_HARDWARE_GEN_WPC95DCS = 0x0000000000040,    // Hybrid WPC95 driver + DCS sound, Who Dunnit
            ALTSOUND_HARDWARE_GEN_WPC95 = 0x0000000000080,       // Integrated boards, Congo 3/96 - Cactus Canyon 2/99
            ALTSOUND_HARDWARE_GEN_S11 = 0x0000080000000,         // No external sound board
            ALTSOUND_HARDWARE_GEN_S11X = 0x0000000000100,        // S11C sound board
            ALTSOUND_HARDWARE_GEN_S11B2 = 0x0000000000200,       // Jokerz! sound board
            ALTSOUND_HARDWARE_GEN_S11C = 0x0000000000400,        // No CPU board sound
            ALTSOUND_HARDWARE_GEN_DE = 0x0000000001000,          // DE AlphaSeg
            ALTSOUND_HARDWARE_GEN_DEDMD16 = 0x0000000002000,     // DE 128x16
            ALTSOUND_HARDWARE_GEN_DEDMD32 = 0x0000000004000,     // DE 128x32
            ALTSOUND_HARDWARE_GEN_DEDMD64 = 0x0000000008000,     // DE 192x64
            ALTSOUND_HARDWARE_GEN_GTS80 = 0x0000200000000,       // GTS80 / Gottlieb System 80
            //ALTSOUND_HARDWARE_GEN_GTS80A = ALTSOUND_HARDWARE_GEN_GTS80,
            ALTSOUND_HARDWARE_GEN_WS = 0x0004000000000,          // Whitestar
            ALTSOUND_HARDWARE_GEN_WS_1 = 0x0008000000000,        // Whitestar with extra RAM
            ALTSOUND_HARDWARE_GEN_WS_2 = 0x0010000000000,        // Whitestar with extra DMD
        } ALTSOUND_HARDWARE_GEN;

        struct InitData {
            string log_path;
            string cmd_file;
            std::vector<TestData> test_data;
            string vpm_path;
            string altsound_path;
            string game_name;
            ALTSOUND_HARDWARE_GEN hardware_gen;
        };

        struct  SDLMixerGroupChannels {
            enum Value {
                MUSIC   = 0,
                CALLOUT = 1,
                SFX     = 2,
                SOLO    = 3,
                OVERLAY = 4
            };
        };

        // Structure for holding G-Sound sample data
        typedef struct _gsound_sample_info {
            unsigned int id = 0;
            std::string type;
            float duck = 1.0f;
            float gain = 1.0f;
            std::string fname;
            bool loop = false;
            unsigned int ducking_profile = 0;
        } GSoundSampleInfo;

        InitData m_init_data;
        string m_altSoundPath;
        string m_altsound_format;

        // altsound_ini_processor
        bool m_record_sound_commands = false;
        bool m_rom_volume_control = true;
        unsigned int m_skip_count = 0;

        //store behaviors
        std::vector<SoundTypeBehaviors> m_behaviors;

        // gsound samples
        std::vector<GSoundSampleInfo> m_gsoundSamples;

        string getAltSoundPath();
    };
    #endif
