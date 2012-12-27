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
#include "Instrument.h"
#include "Document.hpp"
#include "FtmDocument.hpp"
#include "Sequence.h"
//#include "Compiler.h"

// 2A03 instruments

const int CInstrument2A03::SEQUENCE_TYPES[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTYCYCLE};

CInstrument2A03::CInstrument2A03() : 
	m_iPitchOption(0)
{
	for (int i = 0; i < SEQUENCE_COUNT; i++)
	{
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}

	for (int i = 0; i < OCTAVE_RANGE; i++)
	{
		for (int j = 0; j < 12; j++)
		{
			m_cSamples[i][j] = 0;
			m_cSamplePitch[i][j] = 0;
			m_cSampleLoopOffset[i][j] = 0;
		}
	}
}

CInstrument *CInstrument2A03::Clone() const
{
	CInstrument2A03 *pNew = new CInstrument2A03();

	for (int i = 0; i < SEQUENCE_COUNT; i++)
	{
		pNew->SetSeqEnable(i, GetSeqEnable(i));
		pNew->SetSeqIndex(i, GetSeqIndex(i));
	}

	for (int i = 0; i < OCTAVE_RANGE; i++)
	{
		for (int j = 0; j < 12; j++)
		{
			pNew->SetSample(i, j, GetSample(i, j));
			pNew->SetSamplePitch(i, j, GetSamplePitch(i, j));
		}
	}

	pNew->SetName(GetName());

	return pNew;
}

void CInstrument2A03::Store(Document *doc)
{
	doc->writeBlockInt(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; i++)
	{
		doc->writeBlockChar(GetSeqEnable(i));
		doc->writeBlockChar(GetSeqIndex(i));
	}

	for (int i = 0; i < OCTAVE_RANGE; i++)
	{
		for (int j = 0; j < 12; j++)
		{
			doc->writeBlockChar(GetSample(i, j));
			doc->writeBlockChar(GetSamplePitch(i, j));
		}
	}
}

bool CInstrument2A03::Load(Document *doc)
{
	int SeqCnt;
	int Octaves = OCTAVE_RANGE;
	int Index;
	int Version = doc->getBlockVersion();

	SeqCnt = doc->getBlockInt();
	ftkr_Assert(SeqCnt < (SEQUENCE_COUNT + 1));

	for (int i = 0; i < SeqCnt; i++)
	{
		SetSeqEnable(i, doc->getBlockChar());
		Index = doc->getBlockChar();
		ftkr_Assert(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	if (Version == 1)
		Octaves = 6;

	for (int i = 0; i < Octaves; i++)
	{
		for (int j = 0; j < 12; j++)
		{
			Index = doc->getBlockChar();
			if (Index >= MAX_DSAMPLES)
				Index = 0;
			SetSample(i, j, Index);
			SetSamplePitch(i, j, doc->getBlockChar());
		}
	}

	return true;
}

void CInstrument2A03::SaveFile(core::IO *file, FtmDocument *pDoc)
{
	// Saves an 2A03 instrument
	// Current version 2.2

	// Sequences
	unsigned char SeqCount = SEQUENCE_COUNT;
	file->writeChar(SeqCount);

	for (int i = 0; i < SEQUENCE_COUNT; i++)
	{
		int Sequence = GetSeqIndex(i);

		if (GetSeqEnable(i))
		{
			CSequence *pSeq = pDoc->GetSequence2A03(Sequence, i);
			char Enabled = 1;
			int ItemCount = pSeq->GetItemCount();
			int LoopPoint = pSeq->GetLoopPoint();
			int ReleasePoint = pSeq->GetReleasePoint();
			int Setting = pSeq->GetSetting();
			file->writeChar(Enabled);
			file->writeInt(ItemCount);
			file->writeInt(LoopPoint);
			file->writeInt(ReleasePoint);
			file->writeInt(Setting);
			for (unsigned int j = 0; j < pSeq->GetItemCount(); j++)
			{
				int Value = pSeq->GetItem(j);
				file->writeChar(Value);
			}
		}
		else {
			char Enabled = 0;
			file->writeChar(Enabled);
		}
	}

	unsigned int Count = 0;

	// Count assigned keys
	for (int i = 0; i < 8; i++)	// octaves
	{
		for (int j = 0; j < 12; j++)	// notes
		{
			if (GetSample(i, j) > 0)
				Count++;
		}
	}

	file->writeInt(Count);

	bool UsedSamples[MAX_DSAMPLES];
	memset(UsedSamples, 0, sizeof(bool) * MAX_DSAMPLES);

	// DPCM
	for (int i = 0; i < 8; i++)	// octaves
	{
		for (int j = 0; j < 12; j++)	// notes
		{
			if (GetSample(i, j) > 0)
			{
				unsigned char Index = i * 12 + j;
				unsigned char Sample = GetSample(i, j);
				unsigned char Pitch = GetSamplePitch(i, j);
				file->writeChar(Index);
				file->writeChar(Sample);
				file->writeChar(Pitch);
				UsedSamples[Sample - 1] = true;
			}
		}
	}

	int SampleCount = 0;

	// Count samples
	for (int i = 0; i < MAX_DSAMPLES; i++)
	{
		if (pDoc->GetSampleSize(i) > 0 && UsedSamples[i])
			SampleCount++;
	}

	// Write the number
	file->writeInt(SampleCount);

	// List of sample names
	for (int i = 0; i < MAX_DSAMPLES; i++)
	{
		if (pDoc->GetSampleSize(i) > 0 && UsedSamples[i])
		{
			CDSample *pSample = pDoc->GetDSample(i);
			int Len = (int)strlen(pSample->Name);
			file->writeInt(i);
			file->writeInt(Len);
			file->write(pSample->Name, Len);
			file->writeInt(pSample->SampleSize);
			file->write(pSample->SampleData, pSample->SampleSize);
		}
	}
}

bool CInstrument2A03::LoadFile(core::IO *file, int iVersion, FtmDocument *pDoc)
{
	// Reads an FTI file
	//

	char SampleNames[MAX_DSAMPLES][256];
	stSequence OldSequence;

	// Sequences
	unsigned char SeqCount;
	file->readChar(SeqCount);

	// Loop through all instrument effects
	for (unsigned int i = 0; i < SeqCount; i++)
	{

		unsigned char Enabled;
		file->readChar(Enabled);

		if (Enabled == 1)
		{
			// Read the sequence
			int Count;
			file->readInt(&Count);

			if (Count < 0 || Count > MAX_SEQUENCE_ITEMS)
				return false;

			// Find a free sequence
			int Index = pDoc->GetFreeSequence(i);
			CSequence *pSeq = pDoc->GetSequence2A03(Index, i);

			if (iVersion < 20)
			{
				OldSequence.Count = Count;
				for (int j = 0; j < Count; j++)
				{
					file->readChar(OldSequence.Length[j]);
					file->readChar(OldSequence.Value[j]);
				}
				pDoc->ConvertSequence(&OldSequence, pSeq, i);	// convert
			}
			else
			{
				pSeq->SetItemCount(Count);
				int LoopPoint;
				int Setting;
				file->readInt(&LoopPoint);
				pSeq->SetLoopPoint(LoopPoint);
				if (iVersion > 20)
				{
					int ReleasePoint;
					file->readInt(&ReleasePoint);
					pSeq->SetReleasePoint(ReleasePoint);
				}
				if (iVersion >= 23)
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
		else
		{
			SetSeqEnable(i, false);
			SetSeqIndex(i, 0);
		}
	}

	bool SamplesFound[MAX_DSAMPLES];
	memset(SamplesFound, 0, sizeof(bool) * MAX_DSAMPLES);

	unsigned int Count;
	file->readInt(&Count);

	// DPCM instruments
	for (unsigned int i = 0; i < Count; i++)
	{
		unsigned char InstNote;
		file->readChar(InstNote);
		int Octave = InstNote / 12;
		int Note = InstNote % 12;
		unsigned char Sample, Pitch;
		file->readChar(Sample);
		file->readChar(Pitch);
		SetSamplePitch(Octave, Note, Pitch);
		SetSample(Octave, Note, Sample);
	}

	// DPCM samples list
	bool bAssigned[OCTAVE_RANGE][NOTE_RANGE];
	memset(bAssigned, 0, sizeof(bool) * OCTAVE_RANGE * NOTE_RANGE);

	unsigned int SampleCount;
	file->readInt(&SampleCount);

	for (unsigned int i = 0; i < SampleCount; i++)
	{
		int Index, Len;
		file->readInt(&Index);
		file->readInt(&Len);
		if (Index >= MAX_DSAMPLES || Len >= 256)
			return false;
		file->read(SampleNames[Index], Len);
		SampleNames[Index][Len] = 0;
		int Size;
		file->readInt(&Size);
		char *SampleData = new char[Size];
		file->read(SampleData, Size);
		bool Found = false;
		for (int j = 0; j < MAX_DSAMPLES; j++)
		{
			CDSample *pSample = pDoc->GetDSample(j);
			// Compare size and name to see if identical sample exists
			if (pSample->SampleSize == Size && !strcmp(pSample->Name, SampleNames[Index]))
			{
				Found = true;
				// Assign sample
				for (int o = 0; o < OCTAVE_RANGE; o++)
				{
					for (int n = 0; n < NOTE_RANGE; n++)
					{
						if (GetSample(o, n) == (Index + 1) && !bAssigned[o][n])
						{
							SetSample(o, n, j + 1);
							bAssigned[o][n] = true;
						}
					}
				}
			}
		}

		if (!Found)
		{
			// Load sample			
			int FreeSample = pDoc->GetFreeDSample();
			if (FreeSample != -1)
			{
				if ((pDoc->GetTotalSampleSize() + Size) <= MAX_SAMPLE_SPACE)
				{
					CDSample *Sample = pDoc->GetDSample(FreeSample);
					safe_strcpy(Sample->Name, SampleNames[Index], sizeof(Sample->Name));
					Sample->SampleSize = Size;
					Sample->SampleData = SampleData;
					// Assign it
					for (int o = 0; o < OCTAVE_RANGE; o++)
					{
						for (int n = 0; n < NOTE_RANGE; n++)
						{
							if (GetSample(o, n) == (Index + 1) && !bAssigned[o][n])
							{
								SetSample(o, n, FreeSample + 1);
								bAssigned[o][n] = true;
							}
						}
					}
				}
				else
				{
					ftkr_Assert(0);
				//	AfxMessageBox(IDS_OUT_OF_SAMPLEMEM, MB_ICONERROR);
				//	SAFE_RELEASE_ARRAY(SampleData);
					return false;
				}
			}
			else
			{
				ftkr_Assert(0);
			//	AfxMessageBox(IDS_OUT_OF_SLOTS, MB_ICONERROR);
			//	SAFE_RELEASE_ARRAY(SampleData);
				return false;
			}
		}
		else
		{
			ftkr_Assert(0);
		//	SAFE_RELEASE_ARRAY(SampleData);
		}
	}

	return true;
}
// TODO - dan
/*
int CInstrument2A03::CompileSize(CCompiler *pCompiler)
{
	int Size = 1;
	CFamiTrackerDoc *pDoc = pCompiler->GetDocument();

	for (int i = 0; i < SEQUENCE_COUNT; i++)
	{
		if (GetSeqEnable(i))
		{
			CSequence *pSeq = pDoc->GetSequence(GetSeqIndex(i), i);
			if (pSeq->GetItemCount() > 0)
				Size += 2;
		}
	}

	return Size;
}

int CInstrument2A03::Compile(CCompiler *pCompiler, int Index)
{
	int ModSwitch;
	int StoredBytes = 0;
	int iAddress;
	int i;

	CFamiTrackerDoc *pDoc = pCompiler->GetDocument();

	pCompiler->WriteLog("2A03 {");
	ModSwitch = 0;
	
	for (i = 0; i < SEQUENCE_COUNT; i++)
	{
		ModSwitch = (ModSwitch >> 1) | ((GetSeqEnable(i) && (pDoc->GetSequence(GetSeqIndex(i), i)->GetItemCount() > 0)) ? 0x10 : 0);
	}
	
	pCompiler->StoreByte(ModSwitch);
	pCompiler->WriteLog("%02X ", ModSwitch);
	StoredBytes++;

	for (i = 0; i < SEQUENCE_COUNT; i++)
	{
		iAddress = (GetSeqEnable(i) == 0 || (pDoc->GetSequence(GetSeqIndex(i), i)->GetItemCount() == 0)) ? 0 : pCompiler->GetSequenceAddress2A03(GetSeqIndex(i), i);
		if (iAddress > 0)
		{
			pCompiler->StoreShort(iAddress);
			pCompiler->WriteLog("%04X ", iAddress);
			StoredBytes += 2;
		}
	}

	return StoredBytes;
}
*/
bool CInstrument2A03::CanRelease(FtmDocument *doc) const
{
	if (GetSeqEnable(0) != 0)
	{
		int index = GetSeqIndex(SEQ_VOLUME);
		return doc->GetSequence(SNDCHIP_NONE, index, SEQ_VOLUME)->GetReleasePoint() != -1;
	}

	return false;
}

int	CInstrument2A03::GetSeqEnable(int Index) const
{
	return m_iSeqEnable[Index];
}

int	CInstrument2A03::GetSeqIndex(int Index) const
{
	return m_iSeqIndex[Index];
}

void CInstrument2A03::SetSeqEnable(int Index, int Value)
{
	if (m_iSeqEnable[Index] != Value)
		InstrumentChanged();

	m_iSeqEnable[Index] = Value;
}

void CInstrument2A03::SetSeqIndex(int Index, int Value)
{
	if (m_iSeqIndex[Index] != Value)
		InstrumentChanged();

	m_iSeqIndex[Index] = Value;
}

char CInstrument2A03::GetSample(int Octave, int Note) const
{
	return m_cSamples[Octave][Note];
}

char CInstrument2A03::GetSamplePitch(int Octave, int Note) const
{
	return m_cSamplePitch[Octave][Note];
}

bool CInstrument2A03::GetSampleLoop(int Octave, int Note) const
{
	return (m_cSamplePitch[Octave][Note] & 0x80) == 0x80;
}

char CInstrument2A03::GetSampleLoopOffset(int Octave, int Note) const
{
	return m_cSampleLoopOffset[Octave][Note];
}

void CInstrument2A03::SetSample(int Octave, int Note, char Sample)
{
	m_cSamples[Octave][Note] = Sample;
	InstrumentChanged();
}

void CInstrument2A03::SetSamplePitch(int Octave, int Note, char Pitch)
{
	m_cSamplePitch[Octave][Note] = Pitch;
	InstrumentChanged();
}

void CInstrument2A03::SetSampleLoop(int Octave, int Note, bool Loop)
{
	m_cSamplePitch[Octave][Note] = (m_cSamplePitch[Octave][Note] & 0x7F) | (Loop ? 0x80 : 0);
	InstrumentChanged();
}

void CInstrument2A03::SetSampleLoopOffset(int Octave, int Note, char Offset)
{
	m_cSampleLoopOffset[Octave][Note] = Offset;
}

bool CInstrument2A03::AssignedSamples() const
{
	// Returns true if there are assigned samples in this instrument	

	for (int i = 0; i < OCTAVE_RANGE; i++)
	{
		for (int j = 0; j < NOTE_RANGE; j++)
		{
			if (GetSample(i, j) != 0)
				return true;
		}
	}

	return false;
}

void CInstrument2A03::SetPitchOption(int Option)
{
	m_iPitchOption = Option;
	InstrumentChanged();
}

int CInstrument2A03::GetPitchOption() const
{
	return m_iPitchOption;
}
