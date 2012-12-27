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

#include "Instrument.h"
#include "Document.hpp"
#include "FtmDocument.hpp"
#include "Sequence.h"
//#include "Compiler.h"

/*
 * class CInstrumentVRC6
 *
 */

const int CInstrumentVRC6::SEQUENCE_TYPES[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTYCYCLE};

CInstrumentVRC6::CInstrumentVRC6()
{
	for (int i = 0; i < SEQUENCE_COUNT; i++)
	{
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}	
}

CInstrument *CInstrumentVRC6::Clone() const
{
	CInstrumentVRC6 *pNew = new CInstrumentVRC6();

	for (int i = 0; i < SEQUENCE_COUNT; i++)
	{
		pNew->SetSeqEnable(i, GetSeqEnable(i));
		pNew->SetSeqIndex(i, GetSeqIndex(i));
	}

	pNew->SetName(GetName());

	return pNew;
}

void CInstrumentVRC6::Store(Document *doc)
{
	doc->writeBlockInt(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; i++)
	{
		doc->writeBlockChar(GetSeqEnable(i));
		doc->writeBlockChar(GetSeqIndex(i));
	}
}

bool CInstrumentVRC6::Load(Document *doc)
{
	int Index;
	int SeqCnt = doc->getBlockInt();

	ftkr_Assert(SeqCnt < (SEQUENCE_COUNT + 1));

	SeqCnt = SEQUENCE_COUNT;//SEQ_COUNT;

	for (int i = 0; i < SeqCnt; i++)
	{
		SetSeqEnable(i, doc->getBlockChar());
		Index = doc->getBlockChar();
		ftkr_Assert(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	return true;
}

void CInstrumentVRC6::SaveFile(core::IO *file, FtmDocument *doc)
{
	// Sequences
	unsigned char SeqCount = SEQUENCE_COUNT;
	file->writeChar(SeqCount);

	for (int i = 0; i < SEQUENCE_COUNT; i++)
	{
		int Sequence = GetSeqIndex(i);
		if (GetSeqEnable(i))
		{
			CSequence *pSeq = doc->GetSequence(SNDCHIP_VRC6, Sequence, i);

			char Enabled = 1;
			int ItemCount = pSeq->GetItemCount();
			int LoopPoint = pSeq->GetLoopPoint();
			int ReleasePoint = pSeq->GetReleasePoint();
			int Setting	= pSeq->GetSetting();
			
			file->writeChar(Enabled);
			file->writeInt(ItemCount);
			file->writeInt(LoopPoint);
			file->writeInt(ReleasePoint);
			file->writeInt(Setting);

			for (int j = 0; j < ItemCount; j++)
			{
				signed char Value = pSeq->GetItem(j);
				file->writeChar(Value);
			}
		}
		else {
			char Enabled = 0;
			file->writeChar(Enabled);
		}
	}
}

bool CInstrumentVRC6::LoadFile(core::IO *file, int iVersion, FtmDocument *doc)
{
	// Sequences
	stSequence OldSequence;
	unsigned char SeqCount;
	unsigned char Enabled;
	int Count, Index;
	int LoopPoint, ReleasePoint, Setting;

	file->readChar(SeqCount);

	// Loop through all instrument effects
	for (int i = 0; i < SeqCount; i++)
	{
		file->readChar(Enabled);
		if (Enabled == 1)
		{
			// Read the sequence

			file->readInt(&Count);
			Index = doc->GetFreeSequenceVRC6(i);

			CSequence *pSeq = doc->GetSequence(SNDCHIP_VRC6, Index, i);

			if (iVersion < 20)
			{
				OldSequence.Count = Count;
				for (int j = 0; j < Count; j++)
				{
					file->readChar(OldSequence.Length[j]);
					file->readChar(OldSequence.Value[j]);
				}
				doc->ConvertSequence(&OldSequence, pSeq, i);	// convert
			}
			else {
				pSeq->SetItemCount(Count);
				file->readInt(&LoopPoint);
				pSeq->SetLoopPoint(LoopPoint);
				if (iVersion > 20)
				{
					file->readInt(&ReleasePoint);
					pSeq->SetReleasePoint(ReleasePoint);
				}
				if (iVersion >= 22)
				{
					file->readInt(&Setting);
					pSeq->SetSetting(Setting);
				}
				for (int j = 0; j < Count; j++)
				{
					char Val;
					file->readChar(Val);
					pSeq->SetItem(j, Val);
				}
			}
			SetSeqEnable(i, true);
			SetSeqIndex(i, Index);
		}
		else {
			SetSeqEnable(i, false);
			SetSeqIndex(i, 0);
		}
	}

	return true;
}
/*
int CInstrumentVRC6::CompileSize(CCompiler *pCompiler)
{
	int Size = 1;
	CFamiTrackerDoc *pDoc = pCompiler->GetDocument();

	for (int i = 0; i < SEQUENCE_COUNT; i++)
		if (GetSeqEnable(i) && pDoc->GetSequence(SNDCHIP_VRC6, GetSeqIndex(i), i)->GetItemCount() > 0)
			Size += 2;

	return Size;
}

int CInstrumentVRC6::Compile(CCompiler *pCompiler, int Index)
{
	int ModSwitch;
	int StoredBytes = 0;
	int iAddress;
	int i;

	CFamiTrackerDoc *pDoc = pCompiler->GetDocument();

	pCompiler->writeLog("VRC6 {");
	ModSwitch = 0;

	for (i = 0; i < SEQUENCE_COUNT; i++)
	{
		ModSwitch = (ModSwitch >> 1) | (GetSeqEnable(i) && (pDoc->GetSequence(SNDCHIP_VRC6, GetSeqIndex(i), i)->GetItemCount() > 0) ? 0x10 : 0);
	}

	pCompiler->StoreByte(ModSwitch);
	pCompiler->writeLog("%02X ", ModSwitch);
	StoredBytes++;

	for (i = 0; i < SEQUENCE_COUNT; i++)
	{
		iAddress = (GetSeqEnable(i) == 0 || (pDoc->GetSequence(SNDCHIP_VRC6, GetSeqIndex(i), i)->GetItemCount() == 0)) ? 0 : pCompiler->GetSequenceAddressVRC6(GetSeqIndex(i), i);
		if (iAddress > 0)
		{
			pCompiler->StoreShort(iAddress);
			pCompiler->writeLog("%04X ", iAddress);
			StoredBytes += 2;
		}
	}
	
	return StoredBytes;
}
*/
bool CInstrumentVRC6::CanRelease(FtmDocument *doc) const
{
	if (GetSeqEnable(0) != 0)
	{
		int index = GetSeqIndex(SEQ_VOLUME);
		return doc->GetSequence(SNDCHIP_VRC6, index, SEQ_VOLUME)->GetReleasePoint() != -1;
	}

	return false;
}

int	CInstrumentVRC6::GetSeqEnable(int Index) const
{
	return m_iSeqEnable[Index];
}

int	CInstrumentVRC6::GetSeqIndex(int Index) const
{
	return m_iSeqIndex[Index];
}

void CInstrumentVRC6::SetSeqEnable(int Index, int Value)
{
	if (m_iSeqEnable[Index] != Value)
		InstrumentChanged();

	m_iSeqEnable[Index] = Value;
}

void CInstrumentVRC6::SetSeqIndex(int Index, int Value)
{
	if (m_iSeqIndex[Index] != Value)
		InstrumentChanged();

	m_iSeqIndex[Index] = Value;
}
