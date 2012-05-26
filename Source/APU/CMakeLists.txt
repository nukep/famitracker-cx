project(apu)
cmake_minimum_required(VERSION 2.6)

set(SRC
	../Blip_Buffer/Blip_Buffer.cpp
	../Blip_Buffer/Blip_Buffer.h

	APU.CPP
	Mixer.cpp
	DPCM.CPP
	Noise.cpp
	Square.cpp
	Triangle.cpp
	FDS.CPP
	MMC5.CPP
	N106.CPP
	S5B.CPP
	VRC6.CPP
	VRC7.CPP
	emu2149.c
	emu2413.c
	FDSSound.cpp

	APU.H
	Channel.h
	Mixer.h
	DPCM.H
	Noise.h
	Square.h
	Triangle.h
	External.h
	FDS.H
	MMC5.H
	N106.H
	S5B.H
	VRC6.H
	VRC7.H
	2413tone.h
	emu2149.h
	emu2413.h
	FDSSound.h
	vrc7tone.h
)

add_library(apu ${SRC})

