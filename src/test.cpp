// ---------------------------------------------------------------------------
// test.cpp
// 07/23/23 - Dave Roscoe
//
// Standalone executive for AltSound for use by devs and authors.  This 
// executable links in all AltSound format processing, ingests sound
// commands from a file, and plays them through the same libraries used for
// VPinMAME.  The command file can be generated from live gameplay or 
// created by hand, to test scripted sound playback scenarios.  Authors can
// use this to test mix levels of one or more sounds in any combination to
// finalize the AltSound mix for a table.  This is particularly useful when
// testing modes.  Authors can script the specific sequences by hand, or
// capture the data from live gameplay.  Then the file can be edited to
// include only what is needed.  From there, the author can iterate on the
// specific sounds-under-test, without having to create it repeatedly on the
// table.
//
// Devs can use this tool to isolate problems and run it through a
// debugger as many times as need to find and fix a problem.  If a user finds
// a problem, all they need to do is:
// 1. enable sound command recording
// 2. set logging level to DEBUG
// 3. recreate the problem
// 4. send the problem description, along with the altsound.log and cmdlog.txt
// ---------------------------------------------------------------------------
// license:<TODO>
// ---------------------------------------------------------------------------

#include "altsound.h"
#include "fileparsers.h"
#include "datastructs.h"

#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

using std::string;

bool playbackCommands(const std::vector<DataStructs::TestData>& test_data)
{
	for (size_t i = 0; i < test_data.size(); ++i) {
		const DataStructs::TestData& td = test_data[i];
		if (!AltsoundProcessCommand(td.snd_cmd, 0)) {
			//throw std::runtime_error("Command playback failed");
		}

		// Sleep for the duration specified in msec for each command, except for the last command.
		if (i < test_data.size() - 1)
			std::this_thread::sleep_for(std::chrono::milliseconds(td.msec));
		else {
			// Sleep for 5 seconds before exiting
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		}
	}
	return true;
}

std::pair<bool, DataStructs::InitData> init(const string& log_path)
{
	std::cout << "BEGIN init()" << std::endl;

	FileParsers fp;
	DataStructs ds;
	//DataStructs::InitData init_data;
	ds.m_init_data.log_path = log_path;

	try {
		if (!fp.parseCmdFile(ds))
			throw std::runtime_error("Failed to parse command file.");

		std::cout << "SUCCESS parseCmdFile()" << std::endl;
		std::cout << "Num commands parsed: " << ds.m_init_data.test_data.size() << std::endl;

		fp.altsoundInit(ds);
		//AltsoundSetHardwareGen(init_data.hardware_gen);

		std::cout << "END init()" << std::endl;
		return std::make_pair(true, ds.m_init_data);
	}
	catch (const std::runtime_error& e) {
		std::cout << e.what() << std::endl;
		std::cout << "END init()" << std::endl;
		return std::make_pair(false, DataStructs::InitData{});
	}
}

// ---------------------------------------------------------------------------
// Functional code
// ---------------------------------------------------------------------------

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <gamename>-cmdlog.txt path" << std::endl;
		std::cout << "Where <gamename>-cmdlog.txt path is the full path and "
				<< "filename of recording file" << std::endl;
		return 1;
	}

	AltsoundSetLogger("./", ALTSOUND_LOG_LEVEL_DEBUG, true);

	const auto init_result = init(argv[1]);

	if (!init_result.first) {
		std::cout << "Initialization failed." << std::endl;
		return 1;
	}

	//std::cout << "Press Enter to begin playback..." << std::endl;
	//std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Wait for user input

	try {
		std::cout << "Starting playback for \"" << init_result.second.altsound_path << "\"..." << std::endl;
		if (!playbackCommands(init_result.second.test_data)) {
			std::cout << "Playback failed" << std::endl;
			return 1;
		}
		std::cout << "Playback finished for \"" << init_result.second.altsound_path << "\"..." << std::endl;
	}
	catch (const std::exception& e) {
		std::cout << "Unexpected error during playback:" << e.what()  << std::endl;
		return 1;
	}

	std::cout << "Playback completed! Press Enter to exit..." << std::endl;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Wait for user input

	AltsoundShutdown();
	return 0;
}