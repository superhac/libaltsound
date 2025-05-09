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
#include <plog/Log.h>
#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Appenders/RollingFileAppender.h>


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

bool init(const string& log_path, DataStructs& ds)
{
	PLOGI << "BEGIN init()";

	FileParsers fp;
	ds.m_init_data.log_path = log_path;

	try {
		if (!fp.parseCmdFile(ds))
			throw std::runtime_error("Failed to parse command file.");

		PLOGI << "SUCCESS parseCmdFile()";;
		PLOGI << "Num commands parsed: " << ds.m_init_data.test_data.size();

		fp.parse_altsound_ini(ds);

		//AltsoundSetHardwareGen(init_data.hardware_gen);

		PLOGI << "END init()";
		return true;
	}
	catch (const std::runtime_error& e) {
		PLOGE << e.what();
		PLOGE << "END init()";
		return false;
	}
}

// ---------------------------------------------------------------------------
// Functional code
// ---------------------------------------------------------------------------

int main(int argc, const char* argv[]) {

	// init plog
	static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("plog.log", 1000000, 5);
    plog::init(plog::verbose, &fileAppender).addAppender(&consoleAppender);
	PLOGI << "libAltSound Starting";

	DataStructs ds;

	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <gamename>-cmdlog.txt path" << std::endl;
		std::cout << "Where <gamename>-cmdlog.txt path is the full path and "
				<< "filename of recording file" << std::endl;
		return 1;
	}

	//AltsoundSetLogger("./", ALTSOUND_LOG_LEVEL_DEBUG, true);

	const auto init_result = init(argv[1], ds);

	if (!init_result) {
		PLOGI << "Initialization failed.";
		return 1;
	}

	//std::cout << "Press Enter to begin playback..." << std::endl;
	//std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Wait for user input

	try {
		PLOGI << "Starting playback for \"" << argv[1] << "\"...";
		if (!playbackCommands(ds.m_init_data.test_data)) {
			std::cout << "Playback failed" << std::endl;
			return 1;
		}
		PLOGI << "Playback finished for \"" << argv[1] << "\"...";
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