/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2012  Jonathan Liss
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
#include "Instrument.h"
#include "Document.hpp"
#include "FtmDocument.hpp"
#include "Sequence.h"

/*
 * class CInstrumentVRC7
 *
 */

CInstrumentVRC7::CInstrumentVRC7() :
	m_iPatch(0)
{
	m_iRegs[0] = 0;
	m_iRegs[1] = 0;
	m_iRegs[2] = 0;
	m_iRegs[3] = 0;
	m_iRegs[4] = 0;
	m_iRegs[5] = 0;
	m_iRegs[6] = 0;
	m_iRegs[7] = 0;
}

CInstrument *CInstrumentVRC7::Clone() const
{
	CInstrumentVRC7 *pNew = new CInstrumentVRC7();

	pNew->SetPatch(GetPatch());

	for (int i = 0; i < 8; ++i)
		pNew->SetCustomReg(i, GetCustomReg(i));

	pNew->SetName(GetName());

	return pNew;
}

void CInstrumentVRC7::Store(Document *doc)
{
	doc->writeBlockInt(m_iPatch);

	for (int i = 0; i < 8; i++)
	{
		doc->writeBlockChar(GetCustomReg(i));
	}
}

bool CInstrumentVRC7::Load(Document *doc)
{
	m_iPatch = doc->getBlockInt();

	for (int i = 0; i < 8; i++)
	{
		SetCustomReg(i, doc->getBlockChar());
	}

	return true;
}

void CInstrumentVRC7::SaveFile(core::IO *file, FtmDocument *doc)
{
	file->writeInt(m_iPatch);

	for (int i = 0; i < 8; i++)
	{
		unsigned char var = GetCustomReg(i);
		file->writeChar(var);
	}
}

bool CInstrumentVRC7::LoadFile(core::IO *file, int iVersion, FtmDocument *doc)
{
	file->readInt(&m_iPatch);

	for (int i = 0; i < 8; i++)
	{
		unsigned char var;
		file->readChar(var);
		SetCustomReg(i, var);
	}

	return true;
}

bool CInstrumentVRC7::CanRelease(FtmDocument *doc) const
{
	return false;	// This can use release but disable it when previewing notes
}

void CInstrumentVRC7::SetPatch(unsigned int Patch)
{
	m_iPatch = Patch;
	InstrumentChanged();
}

unsigned int CInstrumentVRC7::GetPatch() const
{
	return m_iPatch;
}

void CInstrumentVRC7::SetCustomReg(int Reg, unsigned int Value)
{
	m_iRegs[Reg] = Value;
	InstrumentChanged();
}

unsigned int CInstrumentVRC7::GetCustomReg(int Reg) const
{
	return m_iRegs[Reg];
}
