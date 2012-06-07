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
#include "../APU/APU.h"
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
	// TODO - dan
/*	AddChip(SNDCHIP_VRC7, new CInstrumentVRC7(), "Konami VRC7");
	AddChip(SNDCHIP_FDS,  new CInstrumentFDS(),  "Nintendo FDS sound");
	AddChip(SNDCHIP_N106, new CInstrumentN106(), "Namco 106");
	AddChip(SNDCHIP_S5B,  new CInstrumentS5B(),  "Sunsoft 5B");*/
#else /* _DEBUG */
	// Ready for use
	AddChip(SNDCHIP_NONE, new CInstrument2A03(), "Internal only (2A03/2A07)");
	AddChip(SNDCHIP_VRC6, new CInstrumentVRC6(), "Konami VRC6");
	AddChip(SNDCHIP_MMC5, new CInstrument2A03(), "Nintendo MMC5");
	// TODO - dan
/*	AddChip(SNDCHIP_VRC7, new CInstrumentVRC7(), "Konami VRC7");
	AddChip(SNDCHIP_FDS,  new CInstrumentFDS(),  "Nintendo FDS sound");
//	AddChip(SNDCHIP_N106, new CInstrumentN106(), "Namco 106/163");
//	AddChip(SNDCHIP_S5B,  new CInstrumentS5B(),  "Sunsoft 5B");*/
#endif /* _DEBUG */
}

void CChannelMap::AddChip(int Ident, CInstrument *pInst, const char *pName)
{
	ftm_Assert(m_iAddedChips < CHIP_COUNT);

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
