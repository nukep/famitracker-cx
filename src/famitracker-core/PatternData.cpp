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

#include <string.h>
#include "PatternData.h"
#include "types.hpp"

// This class contains pattern data
// A list of these objects exists inside the document one for each song

CPatternData::CPatternData(unsigned int PatternLength, unsigned int Speed, unsigned int Tempo)
{
	// Clear memory
	memset(m_iFrameList, 0, sizeof(short) * MAX_FRAMES * MAX_CHANNELS);
	memset(m_pPatternData, 0, sizeof(stChanNote*) * MAX_CHANNELS * MAX_PATTERN);
	memset(m_iEffectColumns, 0, sizeof(int) * MAX_CHANNELS);

	m_iPatternLength = PatternLength;
	m_iFrameCount	 = 1;
	m_iSongSpeed	 = Speed;
	m_iSongTempo	 = Tempo;

	// Pre-allocate pattern 0 for all channels
	for (int i = 0; i < MAX_CHANNELS; i++)
		AllocatePattern(i, 0);
}

CPatternData::~CPatternData()
{
	// Deallocate memory
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		for (int j = 0; j < MAX_PATTERN; j++)
		{
			if (m_pPatternData[i][j] != NULL)
				delete[] m_pPatternData[i][j];
		}
	}
}

bool CPatternData::IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row)
{
	stChanNote *Note = GetPatternData(Channel, Pattern, Row);

	bool IsFree = Note->Note == NONE &&
		Note->EffNumber[0] == 0 && Note->EffNumber[1] == 0 &&
		Note->EffNumber[2] == 0 && Note->EffNumber[3] == 0 &&
		Note->Vol == 0x10 && Note->Instrument == MAX_INSTRUMENTS;

	return IsFree;
}

bool CPatternData::IsPatternEmpty(unsigned int Channel, unsigned int Pattern)
{
	// Check if pattern is empty
	for (unsigned int i = 0; i < m_iPatternLength; i++)
	{
		if (!IsCellFree(Channel, Pattern, i))
			return false;
	}
	return true;
}

bool CPatternData::IsPatternInUse(unsigned int Channel, unsigned int Pattern)
{
	// Check if pattern is addressed in frame list
	for (unsigned i = 0; i < m_iFrameCount; i++)
	{
		if (m_iFrameList[i][Channel] == Pattern)
			return true;
	}
	return false;
}

stChanNote *CPatternData::GetPatternData(int Channel, int Pattern, int Row)
{
	if (!m_pPatternData[Channel][Pattern])		// Allocate pattern if accessed for the first time
		AllocatePattern(Channel, Pattern);

	return m_pPatternData[Channel][Pattern] + Row;
}
void CPatternData::GetPatternData(int Channel, int Pattern, int Row, stChanNote *note)
{
	stChanNote *n = GetPatternData(Channel, Pattern, Row);
	memcpy(note, n, sizeof(stChanNote));
}
void CPatternData::SetPatternData(int Channel, int Pattern, int Row, const stChanNote *note)
{
	if (!m_pPatternData[Channel][Pattern])		// Allocate pattern if accessed for the first time
		AllocatePattern(Channel, Pattern);

	stChanNote n = *note;

	// todo: use enumerator constant
	if (Channel == 3)
	{
		if (n.Note != NONE && n.Note != HALT && n.Note != RELEASE)
		{
			// normalize noise to octave 1 and 2
			int v = (n.Note - C + n.Octave*12) % 16 + 16;
			n.Octave = v / 12;
			n.Note = v % 12 + C;
		}
	}

	*(m_pPatternData[Channel][Pattern] + Row) = n;
	m_patternPlayLengths[Channel][Pattern] = -1;
}

void CPatternData::SetPatternLength(unsigned int Length)
{
	m_iPatternLength = Length;
}

unsigned int CPatternData::getPatternPlayLength(int channel, int pattern)
{
	if (m_pPatternData[channel][pattern] == NULL)
	{
		// pattern not used, and therefore no length
		return MAX_PATTERN_LENGTH;
	}

	int &l = m_patternPlayLengths[channel][pattern];
	if (l == -1)
	{
		for (unsigned int i = 0; i < MAX_PATTERN_LENGTH; i++)
		{
			const stChanNote *n = GetPatternData(channel, pattern, i);
			for (unsigned int j = 0; j <= GetEffectColumnCount(channel); j++)
			{
				char en = n->EffNumber[j];
				if (en == EF_JUMP || en == EF_SKIP || en == EF_HALT)
				{
					l = i+1;
					if (l > m_iPatternLength)
						return m_iPatternLength;
					return l;
				}
			}
		}
		l = MAX_PATTERN_LENGTH;
	}
	if (l > m_iPatternLength)
		return m_iPatternLength;
	return l;
}

unsigned int CPatternData::getFramePlayLength(int frame, int channels)
{
	unsigned int l = MAX_PATTERN_LENGTH;
	for (unsigned int i = 0; i < channels; i++)
	{
		unsigned int cl = getPatternPlayLength(i, m_iFrameList[frame][i]);
		if (cl < l)
			l = cl;
	}
	return l;
}

void CPatternData::AllocatePattern(int Channel, int Pattern)
{
	// Allocate memory
	m_pPatternData[Channel][Pattern] = new stChanNote[MAX_PATTERN_LENGTH];
	m_patternPlayLengths[Channel][Pattern] = MAX_PATTERN_LENGTH;

	// Clear memory
	for (int i = 0; i < MAX_PATTERN_LENGTH; i++)
	{
		stChanNote *pNote = m_pPatternData[Channel][Pattern] + i;
		pNote->Note		  = 0;
		pNote->Octave	  = 0;
		pNote->Instrument = MAX_INSTRUMENTS;
		pNote->Vol		  = 0x10;
		for (int n = 0; n < MAX_EFFECT_COLUMNS; n++)
		{
			pNote->EffNumber[n] = 0;
			pNote->EffParam[n] = 0;
		}
	}
}

void CPatternData::ClearEverything()
{
	// Resets everything

	// Frame list
	memset(m_iFrameList, 0, sizeof(short) * MAX_FRAMES * MAX_CHANNELS);
	
	// Patterns, deallocate everything
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		for (int j = 0; j < MAX_PATTERN; j++)
		{
			if (m_pPatternData[i][j])
			{
				delete [] m_pPatternData[i][j];
				m_pPatternData[i][j] = NULL;
			}
			m_patternPlayLengths[i][j] = -1;
		}
	}

	m_iFrameCount = 1;
}

void CPatternData::ClearPattern(int Channel, int Pattern)
{
	// Deletes a specified pattern in a channel
	if (m_pPatternData[Channel][Pattern] != NULL)
	{
		delete[] m_pPatternData[Channel][Pattern];
		m_pPatternData[Channel][Pattern] = NULL;
	}
	m_patternPlayLengths[Channel][Pattern] = -1;
}

unsigned short CPatternData::GetFramePattern(int Frame, int Channel) const
{ 
	return m_iFrameList[Frame][Channel]; 
}

void CPatternData::SetFramePattern(int Frame, int Channel, int Pattern)
{
	m_iFrameList[Frame][Channel] = Pattern;
}
