#include <string>
#include "datastructs.h"
#include <algorithm>

using namespace std;

string DataStructs::getAltSoundPath()
{	
	string szPinmamePath = m_init_data.vpm_path;

	std::replace(szPinmamePath.begin(), szPinmamePath.end(), '\\', '/');

	if (szPinmamePath.back() != '/')
		szPinmamePath += '/';

    m_altSoundPath = szPinmamePath + "altsound/" + m_init_data.game_name + '/';

    return m_altSoundPath;
}