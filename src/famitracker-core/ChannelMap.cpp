/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2010  Jonathan Liss
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "TrackerChannel.h"
#include "ChannelMap.h"
#include "APU/APU.h"
#include "Instrument.h"
#include "types.hpp"

/*
 *  This class contains the expansion chip definitions & instruments.
 *
 */

CChannelMap::CChannelMap() :
	m_iAddedChips(0)
{
	SetupSoundChips();
}

CChannelMap::~CChannelMap()
{
	for (int i = 0; i < m_iAddedChips; i++)
	{
		if (m_pChipInst[i] != NULL)
			delete m_pChipInst[i];
	}
}

void CChannelMap::SetupSoundChips()
{
	// Add available chips
#ifdef _DEBUG
	// Under development
	AddChip(SNDCHIP_NONE, new CInstrument2A03(), "Internal only (2A03/2A07)");
	AddChip(SNDCHIP_VRC6, new CInstrumentVRC6(), "Konami VRC6");
	AddChip(SNDCHIP_MMC5, new CInstrument2A03(), "Nintendo MMC5");
	AddChip(SNDCHIP_FDS,  new CInstrumentFDS(),  "Nintendo FDS sound");
	AddChip(SNDCHIP_VRC7, new CInstrumentVRC7(), "Konami VRC7");
	// TODO - dan
/*	AddChip(SNDCHIP_N106, new CInstrumentN106(), "Namco 106");
	AddChip(SNDCHIP_S5B,  new CInstrumentS5B(),  "Sunsoft 5B");*/
#else /* _DEBUG */
	// Ready for use
	AddChip(SNDCHIP_NONE, new CInstrument2A03(), "Internal only (2A03/2A07)");
	AddChip(SNDCHIP_VRC6, new CInstrumentVRC6(), "Konami VRC6");
	AddChip(SNDCHIP_MMC5, new CInstrument2A03(), "Nintendo MMC5");
	AddChip(SNDCHIP_FDS,  new CInstrumentFDS(),  "Nintendo FDS sound");
	AddChip(SNDCHIP_VRC7, new CInstrumentVRC7(), "Konami VRC7");
	// TODO - dan
/*	AddChip(SNDCHIP_N106, new CInstrumentN106(), "Namco 106/163");
	AddChip(SNDCHIP_S5B,  new CInstrumentS5B(),  "Sunsoft 5B");*/
#endif /* _DEBUG */
}

void CChannelMap::AddChip(int Ident, CInstrument *pInst, const char *pName)
{
	ftkr_Assert(m_iAddedChips < CHIP_COUNT);

	m_pChipNames[m_iAddedChips] = pName;
	m_iChipIdents[m_iAddedChips] = Ident;
	m_pChipInst[m_iAddedChips] = pInst;
	m_iAddedChips++;
}

int CChannelMap::GetChipCount() const
{
	// Return number of available chips
	return m_iAddedChips;
}

const char *CChannelMap::GetChipName(int Index) const
{
	// Get chip name from index
	return m_pChipNames[Index];
}

int CChannelMap::GetChipIdent(int Index) const
{
	// Get chip ID from index
	return m_iChipIdents[Index];
}

int	CChannelMap::GetChipIndex(int Ident) const
{
	// Get index from chip ID
	for (int i = 0; i < m_iAddedChips; i++)
	{
		if (Ident == m_iChipIdents[i])
			return i;
	}
	return 0;
}

CInstrument* CChannelMap::GetChipInstrument(int Chip) const
{
	// Get instrument from chip ID
	int Index = GetChipIndex(Chip);

	if (m_pChipInst[Index] == NULL)
		return NULL;

	return m_pChipInst[Index]->CreateNew();
}

const char * CChannelMap::GetChannelName(int channel) const
{
	switch (channel)
	{
	case CHANID_SQUARE1: return "Pulse 1";
	case CHANID_SQUARE2: return "Pulse 2";
	case CHANID_TRIANGLE: return "Triangle";
	case CHANID_NOISE: return "Noise";
	case CHANID_DPCM: return "DPCM";

	case CHANID_VRC6_PULSE1: return "Pulse 1";
	case CHANID_VRC6_PULSE2: return "Pulse 2";
	case CHANID_VRC6_SAWTOOTH: return "Sawtooth";

	case CHANID_MMC5_SQUARE1: return "Pulse 1";
	case CHANID_MMC5_SQUARE2: return "Pulse 2";
	case CHANID_MMC5_VOICE: return "Voice";

	case CHANID_N106_CHAN1: return "Namco 1";
	case CHANID_N106_CHAN2: return "Namco 2";
	case CHANID_N106_CHAN3: return "Namco 3";
	case CHANID_N106_CHAN4: return "Namco 4";
	case CHANID_N106_CHAN5: return "Namco 5";
	case CHANID_N106_CHAN6: return "Namco 6";
	case CHANID_N106_CHAN7: return "Namco 7";
	case CHANID_N106_CHAN8: return "Namco 8";

	case CHANID_FDS: return "FDS";

	case CHANID_VRC7_CH1: return "FM Channel 1";
	case CHANID_VRC7_CH2: return "FM Channel 2";
	case CHANID_VRC7_CH3: return "FM Channel 3";
	case CHANID_VRC7_CH4: return "FM Channel 4";
	case CHANID_VRC7_CH5: return "FM Channel 5";
	case CHANID_VRC7_CH6: return "FM Channel 6";

	case CHANID_S5B_CH1: return "Square 1";
	case CHANID_S5B_CH2: return "Square 2";
	case CHANID_S5B_CH3: return "Square 3";

	default: return "Unknown";
	}
}

int CChannelMap::GetChipFromChannel(int channel) const
{
	switch (channel)
	{
	case CHANID_SQUARE1:
	case CHANID_SQUARE2:
	case CHANID_TRIANGLE:
	case CHANID_NOISE:
	case CHANID_DPCM: return SNDCHIP_NONE;

	case CHANID_VRC6_PULSE1:
	case CHANID_VRC6_PULSE2:
	case CHANID_VRC6_SAWTOOTH: return SNDCHIP_VRC6;

	case CHANID_MMC5_SQUARE1:
	case CHANID_MMC5_SQUARE2:
	case CHANID_MMC5_VOICE: return SNDCHIP_MMC5;

	case CHANID_N106_CHAN1:
	case CHANID_N106_CHAN2:
	case CHANID_N106_CHAN3:
	case CHANID_N106_CHAN4:
	case CHANID_N106_CHAN5:
	case CHANID_N106_CHAN6:
	case CHANID_N106_CHAN7:
	case CHANID_N106_CHAN8: return SNDCHIP_N106;

	case CHANID_FDS: return SNDCHIP_FDS;

	case CHANID_VRC7_CH1:
	case CHANID_VRC7_CH2:
	case CHANID_VRC7_CH3:
	case CHANID_VRC7_CH4:
	case CHANID_VRC7_CH5:
	case CHANID_VRC7_CH6: return SNDCHIP_VRC7;

	case CHANID_S5B_CH1:
	case CHANID_S5B_CH2:
	case CHANID_S5B_CH3: return SNDCHIP_S5B;

	case CHANNELS:
		break;
	}
	ftkr_Assert(0);
	return SNDCHIP_NONE;
}

std::vector<int> CChannelMap::GetChannelsFromChip(int chip) const
{
	std::vector<int> vec;
	for (int i = 0; i < CHANNELS; i++)
	{
		int c = GetChipFromChannel(i);
		if (c == SNDCHIP_NONE || ( (c & chip) != 0))
		{
			vec.push_back(i);
		}
	}
	return vec;
}
