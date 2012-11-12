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

#include <vector>
#include "common.hpp"

class CInstrument;

class FAMICOREAPI CChannelMap
{
public:
	CChannelMap();
	~CChannelMap();
	void SetupSoundChips();

	int			GetChipCount() const;
	const char *GetChipName(int Index) const;
	int			GetChipIdent(int Index) const;
	int			GetChipIndex(int Ident) const;
	CInstrument	*GetChipInstrument(int Chip) const;

	const char * GetChannelName(int channel) const;
	int			GetChipFromChannel(int channel) const;
	std::vector<int> GetChannelsFromChip(int chip) const;
public:
	static const int CHIP_COUNT = 8;	// Number of allowed expansion chips

protected:
	void AddChip(int Ident, CInstrument *pInst, const char *pName);

protected:
	// Chips
	int				m_iAddedChips;
	int				m_iChipIdents[CHIP_COUNT];
	const char *	m_pChipNames[CHIP_COUNT];
	CInstrument		*m_pChipInst[CHIP_COUNT];
};
