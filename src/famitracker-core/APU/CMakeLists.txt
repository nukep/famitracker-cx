set(APU_SRC
	Blip_Buffer/Blip_Buffer.cpp
	Blip_Buffer/Blip_Buffer.h

	APU.cpp
	Mixer.cpp
	DPCM.cpp
	Noise.cpp
	Square.cpp
	Triangle.cpp
	FDS.cpp
	MMC5.cpp
	N106.cpp
#	S5B.cpp
	VRC6.cpp
	VRC7.cpp
#	emu2149.c
	emu2413.c
	FDSSound.cpp

	APU.h
	Channel.h
	Mixer.h
	DPCM.h
	Noise.h
	Square.h
	Triangle.h
	External.h
	FDS.h
	MMC5.h
	N106.h
#	S5B.h
	VRC6.h
	VRC7.h
	2413tone.h
#	emu2149.h
	emu2413.h
	FDSSound.h
	vrc7tone.h
)

add_sources(${APU_SRC})
