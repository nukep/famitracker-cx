#ifndef _APP_HPP_
#define _APP_HPP_

class SoundGen;

namespace app
{
	SoundGen * soundGenerator();
	void lockSoundGenerator();
	void unlockSoundGenerator();
}

#endif

