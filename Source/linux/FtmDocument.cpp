#include <string.h>
#include <stdio.h>
#include "App.hpp"
#include "FtmDocument.hpp"
#include "Document.hpp"
#include "PatternData.h"
#include "Instrument.h"
#include "TrackerChannel.h"
#include "Sequence.h"

const unsigned int FILE_VER			= 0x0420;			// Current file version (4.20)
const unsigned int COMPATIBLE_VER	= 0x0100;			// Compatible file version (1.0)

// Defaults when creating new modules
const char	DEFAULT_TRACK_NAME[] = "New song";
const int	DEFAULT_ROW_COUNT = 64;

const char	NEW_INST_NAME[] = "New instrument";

const char FILE_HEADER_ID[] = "FamiTracker Module";

const char FILE_BLOCK_PARAMS[]		= "PARAMS";
const char FILE_BLOCK_INFO[]		= "INFO";
const char FILE_BLOCK_INSTRUMENTS[]	= "INSTRUMENTS";
const char FILE_BLOCK_SEQUENCES[]	= "SEQUENCES";
const char FILE_BLOCK_FRAMES[]		= "FRAMES";
const char FILE_BLOCK_PATTERNS[]	= "PATTERNS";
const char FILE_BLOCK_DSAMPLES[]	= "DPCM SAMPLES";
const char FILE_BLOCK_HEADER[]		= "HEADER";

// VRC6
const char *FILE_BLOCK_SEQUENCES_VRC6 = "SEQUENCES_VRC6";

// FTI instruments files
const char INST_HEADER[] = "FTI";
const char INST_VERSION[] = "2.3";

CDSample::CDSample()
	: SampleSize(0), SampleData(NULL)
{
	memset(Name, 0, 256);
}

CDSample::CDSample(int Size, char *pData)
	: SampleSize(Size), SampleData(pData)
{
	if (SampleData == NULL)
		SampleData = new char[Size];

	memset(Name, 0, 256);
}

CDSample::CDSample(CDSample &sample)
	: SampleSize(sample.SampleSize), SampleData(new char[sample.SampleSize])
{
	memcpy(SampleData, sample.SampleData, SampleSize);
	safe_strcpy(Name, sample.Name, sizeof(Name));
}

CDSample::~CDSample()
{
	if (SampleData != NULL)
	{
		delete[] SampleData;
	}
}

void CDSample::Copy(const CDSample *pDSample)
{
	if (SampleData != NULL)
		delete[] SampleData;

	SampleSize = pDSample->SampleSize;
	SampleData = new char[SampleSize];

	memcpy(SampleData, pDSample->SampleData, SampleSize);
	strcpy(Name, pDSample->Name);
}

void CDSample::Allocate(int iSize, char *pData)
{
	if (SampleData != NULL)
		delete[] SampleData;

	SampleData = new char[iSize];
	SampleSize = iSize;

	if (pData != NULL)
		memcpy(SampleData, pData, iSize);
}


FtmDocument::FtmDocument()
{
	for (int i=0;i<MAX_DSAMPLES;i++)
	{
		m_DSamples[i].SampleSize = 0;
		m_DSamples[i].SampleData = NULL;
	}

	// Clear pointer arrays
	memset(m_pTunes, 0, sizeof(CPatternData*) * MAX_TRACKS);
	memset(m_pInstruments, 0, sizeof(CInstrument*) * MAX_INSTRUMENTS);
	memset(m_pSequences2A03, 0, sizeof(CSequence*) * MAX_SEQUENCES * SEQ_COUNT);
	memset(m_pSequencesVRC6, 0, sizeof(CSequence*) * MAX_SEQUENCES * SEQ_COUNT);
	memset(m_pSequencesN106, 0, sizeof(CSequence*) * MAX_SEQUENCES * SEQ_COUNT);

	memset(m_strName, 0, sizeof(m_strName));
	memset(m_strArtist, 0, sizeof(m_strArtist));
	memset(m_strCopyright, 0, sizeof(m_strCopyright));
}

FtmDocument::~FtmDocument()
{
	// Clean up

	// DPCM samples
	for (int i=0;i<MAX_DSAMPLES;i++) {
		if (m_DSamples[i].SampleData != NULL)
		{
			delete[] m_DSamples[i].SampleData;
			m_DSamples[i].SampleData = NULL;
			m_DSamples[i].SampleSize = 0;
		}
	}

	// Patterns
	for (int i=0;i<MAX_TRACKS;i++)
	{
		if (m_pTunes[i] != NULL)
			delete m_pTunes[i];
	}

	// Instruments
	for (int i=0;i<MAX_INSTRUMENTS;i++)
	{
		if (m_pInstruments[i] != NULL)
			delete m_pInstruments[i];
	}

	// Sequences
	for (int i=0;i<MAX_SEQUENCES;i++)
	{
		for (int j=0;j<SEQ_COUNT;j++)
		{
			if (m_pSequences2A03[i][j] != NULL)
				delete m_pSequences2A03[i][j];

			if (m_pSequencesVRC6[i][j] != NULL)
				delete m_pSequencesVRC6[i][j];

			if (m_pSequencesN106[i][j] != NULL)
				delete m_pSequencesN106[i][j];
		}
	}
}

void FtmDocument::createEmpty()
{
	// Allocate first song
	SwitchToTrack(0);

	// and select 2A03 only
	SelectExpansionChip(SNDCHIP_NONE);

	// Auto-select new style vibrato for new modules
	m_iVibratoStyle = VIBRATO_NEW;

	SetModifiedFlag(0);
}

void FtmDocument::read(IO *io)
{
	Document doc;
	bForceBackup = false;

	if (!doc.checkValidity(io))
	{
		return;
	}

	unsigned int ver = doc.getFileVersion();
	m_iFileVersion = ver;

	if (ver < 0x0200)
	{
		// Older file version
		if (ver < COMPATIBLE_VER)
		{
			return;
		}

		if (!readOld(&doc, io))
		{
			return;
		}

		// Create a backup of this file, since it's an old version
		// and something might go wrong when converting
		bForceBackup = true;

		// Auto-select old style vibrato for old files
		m_iVibratoStyle = VIBRATO_OLD;
	}
	else if (ver >= 0x0200)
	{
		// New file version

		if (ver > FILE_VER)
		{
			// File version is too new
			return;
		}

		if (!readNew(&doc, io))
		{
			return;
		}

		// Backup if files was of an older version
		bForceBackup = ver < FILE_VER;
	}
}

bool FtmDocument::readOld(Document *doc, IO *io)
{
	// TODO
	return false;
}

bool FtmDocument::readNew(Document *doc, IO *io)
{
	if (m_iFileVersion < 0x0210)
	{
		// This has to be done for older files
		SwitchToTrack(0);
	}
	while (!doc->isFileDone())
	{
		if (!doc->readBlock(io))
			return false;

		if (doc->isFileDone())
			break;

		const char *id = doc->blockID();

#define CMP(token) (strcmp(id, token) == 0)

		if (CMP(FILE_BLOCK_INFO))
		{
			doc->getBlock(m_strName, 32);
			doc->getBlock(m_strArtist, 32);
			doc->getBlock(m_strCopyright, 32);
		}
		else if (CMP(FILE_BLOCK_PARAMS))
		{
			if (!readNew_params(doc)) return false;
		}
		else if (CMP(FILE_BLOCK_HEADER))
		{
			if (!readNew_header(doc)) return false;
		}
		else if (CMP(FILE_BLOCK_INSTRUMENTS))
		{
			if (!readNew_instruments(doc)) return false;
		}
		else if (CMP(FILE_BLOCK_SEQUENCES))
		{
			if (!readNew_sequences(doc)) return false;
		}
		else if (CMP(FILE_BLOCK_FRAMES))
		{
			if (!readNew_frames(doc)) return false;
		}
		else if (CMP(FILE_BLOCK_PATTERNS))
		{
			if (!readNew_patterns(doc)) return false;
		}
		else if (CMP(FILE_BLOCK_DSAMPLES))
		{
			if (!readNew_dsamples(doc)) return false;
		}
		else if (CMP(FILE_BLOCK_SEQUENCES_VRC6))
		{
			if (!readNew_sequences_vrc6(doc)) return false;
		}
		else
		{
			ftm_Assert(0);
		}
	}

#undef CMP

	return true;
}

bool FtmDocument::readNew_params(Document *doc)
{
	unsigned int block_ver = doc->getBlockVersion();
	if (block_ver == 1)
	{
		m_pSelectedTune->SetSongSpeed(doc->getBlockInt());
	}
	else
	{
		m_iExpansionChip = doc->getBlockChar();
	}

	m_iChannelsAvailable = doc->getBlockInt();
	m_iMachine = doc->getBlockInt();
	m_iEngineSpeed = doc->getBlockInt();

	if (m_iMachine != NTSC && m_iMachine != PAL)
		m_iMachine = NTSC;

	if (block_ver > 2)
		m_iVibratoStyle = doc->getBlockInt();
	else
		m_iVibratoStyle = VIBRATO_OLD;

	if (block_ver > 3)
	{
		Highlight = doc->getBlockInt();
		SecondHighlight = doc->getBlockInt();
	}
	else
	{
		Highlight = 4;
		SecondHighlight = 16;
	}

	// remark: porting bug fix over
	// This is strange. Sometimes expansion chip is set to 0xFF in files
	if (m_iChannelsAvailable == 5)
		m_iExpansionChip = 0;

	if (doc->getFileVersion() == 0x0200)
	{
		int speed = m_pSelectedTune->GetSongSpeed();
		if (speed < 20)
			m_pSelectedTune->SetSongSpeed(speed+1);
	}

	if (block_ver == 1)
	{
		if (m_pSelectedTune->GetSongSpeed() > 19)
		{
			m_pSelectedTune->SetSongTempo(m_pSelectedTune->GetSongSpeed());
			m_pSelectedTune->SetSongSpeed(6);
		}
		else
		{
			m_pSelectedTune->SetSongTempo(m_iMachine == NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
		}
	}
	return true;
}

bool FtmDocument::readNew_header(Document *doc)
{
	unsigned int block_ver = doc->getBlockVersion();

	if (block_ver == 1)
	{
		// Single track
		m_iTracks = 0;
		SwitchToTrack(0);
		for (unsigned int i=0;i<m_iChannelsAvailable;i++)
		{
			// Channel type (unused)
			doc->getBlockChar();
			// Effect columns
			m_pSelectedTune->SetEffectColumnCount(i, doc->getBlockChar());
		}
	}
	else if (block_ver >= 2)
	{
		// Multiple tracks
		m_iTracks = doc->getBlockChar();
		ftm_Assert(m_iTracks <= MAX_TRACKS);

		for (unsigned int i=0;i<=m_iTracks;i++)
		{
			AllocateSong(i);
		}

		// Track names
		if (block_ver >= 3)
		{
			for (unsigned int i=0;i<=m_iTracks;i++)
			{
				m_sTrackNames[i] = doc->readString();
			}
		}

		for (unsigned int i=0;i<m_iChannelsAvailable;i++)
		{
			/*unsigned char channelType = */doc->getBlockChar();	// Channel type (unused)
			for (unsigned int j=0;j<=m_iTracks;j++)
			{
				m_pTunes[j]->SetEffectColumnCount(i, doc->getBlockChar());	// Effect columns
			}
		}
	}
	return true;
}

bool FtmDocument::readNew_instruments(Document *doc)
{
	int count = doc->getBlockInt();
	ftm_Assert(count <= MAX_INSTRUMENTS);

	for (int i=0;i<count;i++)
	{
		int index = doc->getBlockInt();
		ftm_Assert(index <= MAX_INSTRUMENTS);

		int type = doc->getBlockChar();

		CInstrument *inst = CreateInstrument(type);

		ftm_Assert(inst->Load(doc));

		unsigned int size = doc->getBlockInt();
		ftm_Assert(size < 256);
		char name[257];
		doc->getBlock(name, size);
		name[size] = 0;
		inst->SetName(name);

		m_pInstruments[index] = inst;
	}
	return true;
}

bool FtmDocument::readNew_sequences(Document *doc)
{
	unsigned int block_ver = doc->getBlockVersion();
	unsigned int count = doc->getBlockInt();
	char value, length;

	unsigned int index, type;
	unsigned char seqCount;

	ftm_Assert(count < (MAX_SEQUENCES * SEQ_COUNT));

	if (block_ver == 1)
	{
		for (unsigned int i=0;i<count;i++)
		{
			index = doc->getBlockInt();
			seqCount = doc->getBlockChar();
			ftm_Assert(index < MAX_SEQUENCES);
			ftm_Assert(seqCount < MAX_SEQUENCE_ITEMS);
			m_TmpSequences[index].Count = seqCount;
			for (unsigned int x=0;x<seqCount;x++)
			{
				m_TmpSequences[index].Value[x] = doc->getBlockChar();
				m_TmpSequences[index].Length[x] = doc->getBlockChar();
			}
		}
	}
	else if (block_ver == 2)
	{
		for (unsigned int i=0;i<count;i++)
		{
			index = doc->getBlockInt();
			type = doc->getBlockInt();
			seqCount = doc->getBlockChar();
			ftm_Assert(index < MAX_SEQUENCES);
			ftm_Assert(type < SEQ_COUNT);
			ftm_Assert(seqCount < MAX_SEQUENCE_ITEMS);
			m_Sequences[index][type].Count = seqCount;
			for (unsigned int x=0;x<seqCount;x++)
			{
				value = doc->getBlockChar();
				length = doc->getBlockChar();
				m_Sequences[index][type].Value[x] = value;
				m_Sequences[index][type].Length[x] = length;
			}
		}
	}
	else if (block_ver >= 3)
	{
		int Indices[MAX_SEQUENCES * SEQ_COUNT];
		int Types[MAX_SEQUENCES * SEQ_COUNT];

		unsigned int loopPoint, releasePoint=-1, settings=0;

		for (unsigned int i=0;i<count;i++)
		{
			index	  = doc->getBlockInt();
			type	  = doc->getBlockInt();
			seqCount  = doc->getBlockChar();
			loopPoint = doc->getBlockInt();

			// Work-around for some older files
			if (loopPoint == seqCount)
				loopPoint = -1;

			Indices[i] = index;
			Types[i] = type;

			ftm_Assert(index < MAX_SEQUENCES);
			ftm_Assert(type < SEQ_COUNT);

			CSequence *pSeq = GetSequence(index, type);

			pSeq->Clear();
			pSeq->SetItemCount(seqCount < MAX_SEQUENCE_ITEMS ? seqCount : MAX_SEQUENCE_ITEMS);
			pSeq->SetLoopPoint(loopPoint);

			if (block_ver == 4)
			{
				releasePoint = doc->getBlockInt();
				settings = doc->getBlockInt();
				pSeq->SetReleasePoint(releasePoint);
				pSeq->SetSetting(settings);
			}

			for (unsigned int x=0;x<seqCount;x++)
			{
				value = doc->getBlockChar();
				if (x <= MAX_SEQUENCE_ITEMS)
					pSeq->SetItem(x, value);
			}
		}

		if (block_ver == 5)
		{
			// Version 5 saved the release points incorrectly, this is fixed in ver 6
			for (int i=0;i<MAX_SEQUENCES;i++)
			{
				for (int x=0;x<SEQ_COUNT;x++)
				{
					releasePoint = doc->getBlockInt();
					settings = doc->getBlockInt();
					if (GetSequenceItemCount(i, x) > 0)
					{
						CSequence *pSeq = GetSequence(i, x);
						pSeq->SetReleasePoint(releasePoint);
						pSeq->SetSetting(settings);
					}
				}
			}
		}
		else if (block_ver >= 6)
		{
			// Read release points correctly stored
			for (unsigned int i=0;i<count;i++)
			{
				releasePoint = doc->getBlockInt();
				settings = doc->getBlockInt();
				index = Indices[i];
				type = Types[i];
				CSequence *pSeq = GetSequence(index, type);
				pSeq->SetReleasePoint(releasePoint);
				pSeq->SetSetting(settings);
			}
		}
	}
	return true;
}

bool FtmDocument::readNew_frames(Document *doc)
{
	unsigned int block_ver = doc->getBlockVersion();

	if (block_ver == 1)
	{
		unsigned int frameCount = doc->getBlockInt();
		m_pSelectedTune->SetFrameCount(frameCount);
		m_iChannelsAvailable = doc->getBlockInt();
		ftm_Assert(frameCount <= MAX_FRAMES);
		ftm_Assert(m_iChannelsAvailable <= MAX_CHANNELS);
		for (unsigned int i=0;i<frameCount;i++)
		{
			for (unsigned j=0;j<m_iChannelsAvailable;j++)
			{
				unsigned pattern = (unsigned)doc->getBlockChar();
				ftm_Assert(pattern < MAX_FRAMES);
				m_pSelectedTune->SetFramePattern(i, j, pattern);
			}
		}
	}
	else if (block_ver > 1)
	{
		for (unsigned y=0;y<=m_iTracks;y++)
		{
			unsigned int frameCount = doc->getBlockInt();
			unsigned int speed = doc->getBlockInt();

			ftm_Assert(frameCount > 0 && frameCount <= MAX_FRAMES);
			ftm_Assert(speed > 0);

			m_pTunes[y]->SetFrameCount(frameCount);

			if (block_ver == 3)
			{
				unsigned int Tempo = doc->getBlockInt();
				m_pTunes[y]->SetSongTempo(Tempo);
				m_pTunes[y]->SetSongSpeed(speed);
			}
			else
			{
				if (speed < 20)
				{
					unsigned int Tempo = (m_iMachine == NTSC) ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL;
					m_pTunes[y]->SetSongTempo(Tempo);
					m_pTunes[y]->SetSongSpeed(speed);
				}
				else {
					m_pTunes[y]->SetSongTempo(speed);
					m_pTunes[y]->SetSongSpeed(DEFAULT_SPEED);
				}
			}

			unsigned PatternLength = (unsigned)doc->getBlockInt();
			ftm_Assert(PatternLength > 0 && PatternLength <= MAX_PATTERN_LENGTH);

			m_pTunes[y]->SetPatternLength(PatternLength);

			for (unsigned int i=0;i<frameCount;i++)
			{
				for (unsigned j=0;j<m_iChannelsAvailable;j++)
				{
					// Read pattern index
					unsigned int pattern = (unsigned char)doc->getBlockChar();
					ftm_Assert(pattern < MAX_PATTERN);
					m_pTunes[y]->SetFramePattern(i, j, pattern);
				}
			}
		}
	}

	return true;
}

bool FtmDocument::readNew_patterns(Document *doc)
{
	unsigned int block_ver = doc->getBlockVersion();

	if (block_ver == 1)
	{
		int patternLen = doc->getBlockInt();
		ftm_Assert(patternLen <= MAX_PATTERN_LENGTH);
		m_pSelectedTune->SetPatternLength(patternLen);
	}

	while (!doc->blockDone())
	{
		unsigned int track;
		if (block_ver > 1)
			track = doc->getBlockInt();
		else if (block_ver == 1)
			track = 0;

		unsigned int channel = doc->getBlockInt();
		unsigned int pattern = doc->getBlockInt();
		unsigned int items	= doc->getBlockInt();

		if (channel > MAX_CHANNELS)
			return true;

		ftm_Assert(track < MAX_TRACKS);
		ftm_Assert(channel < MAX_CHANNELS);
		ftm_Assert(pattern < MAX_PATTERN);
		ftm_Assert((items - 1) < MAX_PATTERN_LENGTH);

		SwitchToTrack(track);

		for (unsigned int i=0;i<items;i++)
		{
			unsigned row;
			if (m_iFileVersion == 0x0200)
				row = doc->getBlockChar();
			else
				row = doc->getBlockInt();

			ftm_Assert(row < MAX_PATTERN_LENGTH);

			stChanNote *note = m_pSelectedTune->GetPatternData(channel, pattern, row);
			memset(note, 0, sizeof(stChanNote));

			note->Note		 = doc->getBlockChar();
			note->Octave	 = doc->getBlockChar();
			note->Instrument = doc->getBlockChar();
			note->Vol		 = doc->getBlockChar();

			if (m_iFileVersion == 0x0200)
			{
				unsigned char EffectNumber, EffectParam;
				EffectNumber = doc->getBlockChar();
				EffectParam = doc->getBlockChar();
				if (block_ver < 3)
				{
					if (EffectNumber == EF_PORTAOFF)
					{
						EffectNumber = EF_PORTAMENTO;
						EffectParam = 0;
					}
					else if (EffectNumber == EF_PORTAMENTO)
					{
						if (EffectParam < 0xFF)
							EffectParam++;
					}
				}

				stChanNote *note = m_pSelectedTune->GetPatternData(channel, pattern, row);

				note->EffNumber[0]	= EffectNumber;
				note->EffParam[0]	= EffectParam;
			}
			else
			{
				for (int n=0;n<(m_pSelectedTune->GetEffectColumnCount(channel) + 1);n++)
				{
					unsigned char EffectNumber, EffectParam;
					EffectNumber = doc->getBlockChar();
					EffectParam = doc->getBlockChar();

					if (block_ver < 3)
					{
						if (EffectNumber == EF_PORTAOFF)
						{
							EffectNumber = EF_PORTAMENTO;
							EffectParam = 0;
						}
						else if (EffectNumber == EF_PORTAMENTO)
						{
							if (EffectParam < 0xFF)
								EffectParam++;
						}
					}

					note->EffNumber[n]	= EffectNumber;
					note->EffParam[n] 	= EffectParam;
				}
			}

			if (note->Vol > 0x10)
				note->Vol &= 0x0F;

			// Specific for version 2.0
			if (m_iFileVersion == 0x0200)
			{

				if (note->EffNumber[0] == EF_SPEED && note->EffParam[0] < 20)
					note->EffParam[0]++;

				if (note->Vol == 0)
				{
					note->Vol = 0x10;
				}
				else
				{
					note->Vol--;
					note->Vol &= 0x0F;
				}

				if (note->Note == 0)
					note->Instrument = MAX_INSTRUMENTS;
			}

			if (block_ver == 3)
			{
				// Fix for VRC7 portamento
				if (GetExpansionChip() == SNDCHIP_VRC7 && channel > 4)
				{
					for (int n=0;n<MAX_EFFECT_COLUMNS;n++)
					{
						switch (note->EffNumber[n])
						{
							case EF_PORTA_DOWN:
								note->EffNumber[n] = EF_PORTA_UP;
								break;
							case EF_PORTA_UP:
								note->EffNumber[n] = EF_PORTA_DOWN;
								break;
						}
					}
				}
				// FDS pitch effect fix
				else if (GetExpansionChip() == SNDCHIP_FDS && channel == 5)
				{
					for (int n=0;n<MAX_EFFECT_COLUMNS;n++)
					{
						switch (note->EffNumber[n])
						{
							case EF_PITCH:
								if (note->EffParam[n] != 0x80)
									note->EffParam[n] = (0x100 - note->EffParam[n]) & 0xFF;
								break;
						}
					}
				}
			}
#ifdef TRANSPOSE_FDS
			if (version < 5)
			{
				// FDS octave
				if (GetExpansionChip() == SNDCHIP_FDS && channel > 4 && note->Octave < 7) {
					note->Octave++;
				}
			}
#endif
		}
	}

	return true;
}

bool FtmDocument::readNew_dsamples(Document *doc)
{
	/*int Version = */doc->getBlockVersion();

	int count = doc->getBlockChar();
	ftm_Assert(count <= MAX_DSAMPLES);

	memset(m_DSamples, 0, sizeof(CDSample) * MAX_DSAMPLES);

	for (int i=0;i<count;i++)
	{
		int item = doc->getBlockChar();
		int len = doc->getBlockInt();
		ftm_Assert(item < MAX_DSAMPLES);
		ftm_Assert(len < 256);
		doc->getBlock(m_DSamples[item].Name, len);
		m_DSamples[item].Name[len] = 0;
		int size = doc->getBlockInt();
		ftm_Assert(size < 0x8000);
		m_DSamples[item].SampleData = new char[size];
		m_DSamples[item].SampleSize = size;
		doc->getBlock(m_DSamples[item].SampleData, size);
	}

	return true;
}

bool FtmDocument::readNew_sequences_vrc6(Document *doc)
{
	unsigned int i, j, count = 0, index, type;
	unsigned int loopPoint, releasePoint, settings;
	unsigned char seqCount;
	int block_ver = doc->getBlockVersion();
	char Value;

	count = doc->getBlockInt();
	ftm_Assert(count < MAX_SEQUENCES);

	if (block_ver < 4)
	{
		for (i=0;i<count;i++)
		{
			index	  = doc->getBlockInt();
			type	  = doc->getBlockInt();
			seqCount  = doc->getBlockChar();
			loopPoint = doc->getBlockInt();
//			if (SeqCount > MAX_SEQUENCE_ITEMS)
//				SeqCount = MAX_SEQUENCE_ITEMS;
			ftm_Assert(index < MAX_SEQUENCES);
			ftm_Assert(type < SEQ_COUNT);
//			ftm_Assert(SeqCount <= MAX_SEQUENCE_ITEMS);
			CSequence *pSeq = GetSequenceVRC6(index, type);
			pSeq->Clear();
			pSeq->SetItemCount(seqCount < MAX_SEQUENCE_ITEMS ? seqCount : MAX_SEQUENCE_ITEMS);
			pSeq->SetLoopPoint(loopPoint);
			for (j=0;j<seqCount;j++)
			{
				Value = doc->getBlockChar();
				if (j <= MAX_SEQUENCE_ITEMS)
					pSeq->SetItem(j, Value);
			}
		}
	}
	else
	{
		int Indices[MAX_SEQUENCES];
		int Types[MAX_SEQUENCES];

		for (i=0;i<count;i++)
		{
			index	  = doc->getBlockInt();
			type	  = doc->getBlockInt();
			seqCount  = doc->getBlockChar();
			loopPoint = doc->getBlockInt();

			Indices[i] = index;
			Types[i] = type;
/*
			if (SeqCount >= MAX_SEQUENCE_ITEMS)
				SeqCount = MAX_SEQUENCE_ITEMS - 1;
*/
			ftm_Assert(index < MAX_SEQUENCES);
			ftm_Assert(type < SEQ_COUNT);
//			ftm_Assert(SeqCount <= MAX_SEQUENCE_ITEMS);

			CSequence *pSeq = GetSequenceVRC6(index, type);

			pSeq->Clear();
			pSeq->SetItemCount(seqCount);
			pSeq->SetLoopPoint(loopPoint);

			if (block_ver == 4)
			{
				releasePoint = doc->getBlockInt();
				settings = doc->getBlockInt();
				pSeq->SetReleasePoint(releasePoint);
				pSeq->SetSetting(settings);
			}

			for (j=0;j<seqCount;j++)
			{
				Value = doc->getBlockChar();
				if (j <= MAX_SEQUENCE_ITEMS)
					pSeq->SetItem(j, Value);
			}
		}

		if (block_ver == 5)
		{
			// Version 5 saved the release points incorrectly, this is fixed in ver 6
			for (int i=0;i<MAX_SEQUENCES;i++)
			{
				for (int j=0;j<SEQ_COUNT;j++)
				{
					releasePoint = doc->getBlockInt();
					settings = doc->getBlockInt();
					if (GetSequenceItemCountVRC6(i, j) > 0)
					{
						CSequence *pSeq = GetSequenceVRC6(i, j);
						pSeq->SetReleasePoint(releasePoint);
						pSeq->SetSetting(settings);
					}
				}
			}
		}
		else if (block_ver >= 6)
		{
			for (i=0;i<count;i++)
			{
				releasePoint = doc->getBlockInt();
				settings = doc->getBlockInt();
				index = Indices[i];
				type = Types[i];
				CSequence *pSeq = GetSequenceVRC6(index, type);
				pSeq->SetReleasePoint(releasePoint);
				pSeq->SetSetting(settings);
			}
		}
	}

	return true;
}

void FtmDocument::write(IO *io)
{
}
/*
void FtmDocument::ResetChannels()
{
	// Clears all channels from the document
	m_iRegisteredChannels = 0;
}

void FtmDocument::RegisterChannel(CTrackerChannel *pChannel, int ChannelType, int ChipType)
{
	// Adds a channel to the document
	m_pChannels[m_iRegisteredChannels] = pChannel;
	m_iChannelTypes[m_iRegisteredChannels] = ChannelType;
	m_iChannelChip[m_iRegisteredChannels] = ChipType;
	m_iRegisteredChannels++;
}

CTrackerChannel *FtmDocument::GetChannel(int Index) const
{
	ftm_Assert(m_iRegisteredChannels != 0 && Index < m_iRegisteredChannels);
	ftm_Assert(m_pChannels[Index] != NULL);
	return m_pChannels[Index];
}

int FtmDocument::GetChannelType(int Channel) const
{
	ftm_Assert(m_iRegisteredChannels != 0);
	ftm_Assert(Channel < m_iRegisteredChannels);
	return m_iChannelTypes[Channel];
}
*/
void FtmDocument::SetFrameCount(unsigned int Count)
{
	ftm_Assert(Count <= MAX_FRAMES);

	if (m_pSelectedTune->GetFrameCount() != Count)
	{
		m_pSelectedTune->SetFrameCount(Count);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::SetPatternLength(unsigned int Length)
{
	ftm_Assert(Length <= MAX_PATTERN_LENGTH);

	if (m_pSelectedTune->GetPatternLength() != Length)
	{
		m_pSelectedTune->SetPatternLength(Length);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::SetSongSpeed(unsigned int Speed)
{
	ftm_Assert(Speed <= MAX_SPEED);

	if (m_pSelectedTune->GetSongSpeed() != Speed)
	{
		m_pSelectedTune->SetSongSpeed(Speed);
		SetModifiedFlag();
	}
}

void FtmDocument::SetSongTempo(unsigned int Tempo)
{
	ftm_Assert(Tempo <= MAX_TEMPO);

	if (m_pSelectedTune->GetSongTempo() != Tempo)
	{
		m_pSelectedTune->SetSongTempo(Tempo);
		SetModifiedFlag();
	}
}

unsigned int FtmDocument::GetEffColumns(int Track, unsigned int Channel) const
{
	ftm_Assert(Channel < MAX_CHANNELS);
	return m_pTunes[Track]->GetEffectColumnCount(Channel);
}

unsigned int FtmDocument::GetEffColumns(unsigned int Channel) const
{
	ftm_Assert(Channel < MAX_CHANNELS);
	return m_pSelectedTune->GetEffectColumnCount(Channel);
}

void FtmDocument::SetEffColumns(unsigned int Channel, unsigned int Columns)
{
	ftm_Assert(Channel < MAX_CHANNELS);
	ftm_Assert(Columns < MAX_EFFECT_COLUMNS);

//	GetChannel(Channel)->SetColumnCount(Columns);
	m_pSelectedTune->SetEffectColumnCount(Channel, Columns);

	SetModifiedFlag();
	UpdateViews();
}

unsigned int FtmDocument::GetPatternAtFrame(int Track, unsigned int Frame, unsigned int Channel) const
{
	ftm_Assert(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);
	return m_pTunes[Track]->GetFramePattern(Frame, Channel);
}

unsigned int FtmDocument::GetPatternAtFrame(unsigned int Frame, unsigned int Channel) const
{
	ftm_Assert(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);
	return m_pSelectedTune->GetFramePattern(Frame, Channel);
}

void FtmDocument::SetPatternAtFrame(unsigned int Frame, unsigned int Channel, unsigned int Pattern)
{
	ftm_Assert(Frame < MAX_FRAMES && Channel < MAX_CHANNELS && Pattern < MAX_PATTERN);
	m_pSelectedTune->SetFramePattern(Frame, Channel, Pattern);
//	SetModifiedFlag();
}

int FtmDocument::GetFirstFreePattern(int Channel)
{
	for (int i=0;i<MAX_PATTERN;i++)
	{
		if (!m_pSelectedTune->IsPatternInUse(Channel, i) && m_pSelectedTune->IsPatternEmpty(Channel, i))
			return i;

		// Changed v0.3.7, patterns can't be used earlier
		//if (m_pSelectedTune->IsPatternFree(Channel, i))
		//	return i;
	}

	return 0;
}

void FtmDocument::ClearPatterns()
{
	m_pSelectedTune->ClearEverything();
}

#define GET_PATTERN(Frame, Channel) m_pSelectedTune->GetFramePattern(Frame, Channel)

void FtmDocument::IncreasePattern(unsigned int Frame, unsigned int Channel, int Count)
{
	ftm_Assert(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);

	int Current = m_pSelectedTune->GetFramePattern(Frame, Channel);

	// Selects the next channel pattern
	if ((Current + Count) < (MAX_PATTERN - 1))
	{
		m_pSelectedTune->SetFramePattern(Frame, Channel, Current + Count);
		SetModifiedFlag();
		UpdateViews();
	}
	else {
		m_pSelectedTune->SetFramePattern(Frame, Channel, MAX_PATTERN - 1);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::DecreasePattern(unsigned int Frame, unsigned int Channel, int Count)
{
	ftm_Assert(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);

	int Current = m_pSelectedTune->GetFramePattern(Frame, Channel);

	// Selects the previous channel pattern
	if (Current > Count)
	{
		m_pSelectedTune->SetFramePattern(Frame, Channel, Current - Count);
		SetModifiedFlag();
		UpdateViews();
	}
	else {
		m_pSelectedTune->SetFramePattern(Frame, Channel, 0);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::IncreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ftm_Assert(Frame < MAX_FRAMES);
	ftm_Assert(Channel < MAX_CHANNELS);
	ftm_Assert(Row < MAX_PATTERN_LENGTH);

	unsigned int Inst = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Instrument;

	if (Inst < MAX_INSTRUMENTS)
	{
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Instrument = Inst + 1;
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::DecreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ftm_Assert(Frame < MAX_FRAMES);
	ftm_Assert(Channel < MAX_CHANNELS);
	ftm_Assert(Row < MAX_PATTERN_LENGTH);

	unsigned int Inst = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Instrument;

	if (Inst > 0)
	{
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Instrument = Inst - 1;
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::IncreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ftm_Assert(Frame < MAX_FRAMES);
	ftm_Assert(Channel < MAX_CHANNELS);
	ftm_Assert(Row < MAX_PATTERN_LENGTH);

	unsigned int Vol = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Vol;

	if (Vol < 16)
	{
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Vol = Vol + 1;
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::DecreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ftm_Assert(Frame < MAX_FRAMES);
	ftm_Assert(Channel < MAX_CHANNELS);
	ftm_Assert(Row < MAX_PATTERN_LENGTH);

	unsigned int Vol = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Vol;

	if (Vol > 1)
	{
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->Vol = Vol - 1;
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::IncreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index)
{
	ftm_Assert(Frame < MAX_FRAMES);
	ftm_Assert(Channel < MAX_CHANNELS);
	ftm_Assert(Row < MAX_PATTERN_LENGTH);
	ftm_Assert(Index < MAX_EFFECT_COLUMNS);

	unsigned int Effect = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffParam[Index];

	if (Effect < 256)
	{
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffParam[Index] = Effect + 1;
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::DecreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index)
{
	ftm_Assert(Frame < MAX_FRAMES);
	ftm_Assert(Channel < MAX_CHANNELS);
	ftm_Assert(Row < MAX_PATTERN_LENGTH);
	ftm_Assert(Index < MAX_EFFECT_COLUMNS);

	unsigned int Effect = m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffParam[Index];

	if (Effect > 0)
	{
		m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row)->EffParam[Index] = Effect - 1;
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::SetEngineSpeed(unsigned int Speed)
{
	ftm_Assert(Speed <= 800); // hardcoded at the moment, TODO: fix this
	ftm_Assert(Speed >= 10 || Speed == 0);

	m_iEngineSpeed = Speed;
	SetModifiedFlag();
}

void FtmDocument::SetMachine(unsigned int Machine)
{
	ftm_Assert(Machine == PAL || Machine == NTSC);
	m_iMachine = Machine;
	SetModifiedFlag();
}

unsigned int FtmDocument::GetFrameRate(void) const
{
	if (m_iEngineSpeed == 0)
		return (m_iMachine == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;

	return m_iEngineSpeed;
}

void FtmDocument::SelectExpansionChip(unsigned char Chip)
{
	// Store the chip
	m_iExpansionChip = Chip;

	// Register the channels
	// TODO - dan
//	theApp.GetSoundGenerator()->RegisterChannels(Chip);

//	m_iChannelsAvailable = GetChannelCount();

	SetModifiedFlag();
	UpdateViews();
}

bool FtmDocument::ExpansionEnabled(int Chip) const
{
	// Returns true if a specified chip is enabled
	return (GetExpansionChip() & Chip) == Chip;
}

void FtmDocument::SetSongInfo(const char *Name, const char *Artist, const char *Copyright)
{
	ftm_Assert(Name != NULL && Artist != NULL && Copyright != NULL);

	// Check if strings actually changed
	if (strcmp(Name, m_strName) || strcmp(Artist, m_strArtist) || strcmp(Copyright, m_strCopyright))
		SetModifiedFlag();

	safe_strcpy(m_strName, Name, sizeof(m_strName));
	safe_strcpy(m_strArtist, Artist, sizeof(m_strArtist));
	safe_strcpy(m_strCopyright, Copyright, sizeof(m_strCopyright));
}

// Vibrato functions

int FtmDocument::GetVibratoStyle() const
{
	return m_iVibratoStyle;
}

void FtmDocument::SetVibratoStyle(int Style)
{
	m_iVibratoStyle = Style;
	// TODO - dan
//	theApp.GetSoundGenerator()->GenerateVibratoTable(Style);
}

// Track functions

void FtmDocument::SelectTrack(unsigned int Track)
{
	ftm_Assert(Track < MAX_TRACKS);
	SwitchToTrack(Track);
	UpdateViews();
}

void FtmDocument::SwitchToTrack(unsigned int Track)
{
	// Select a new track, initialize if it wasn't
	// TODO: should this fail if track didn't exist?
	m_iTrack = Track;
	AllocateSong(Track);
	m_pSelectedTune = m_pTunes[Track];
}

const char *FtmDocument::GetTrackTitle(unsigned int Track) const
{
	return m_sTrackNames[Track].c_str();
}

bool FtmDocument::AddTrack()
{
	if (m_iTracks >= (MAX_TRACKS - 1))
		return false;

	AllocateSong(++m_iTracks);

	SetModifiedFlag();
	UpdateViews();

	return true;
}

void FtmDocument::RemoveTrack(unsigned int Track)
{
	ftm_Assert(m_iTracks > 0);
	ftm_Assert(m_pTunes[Track] != NULL);

	delete m_pTunes[Track];

	// Move down all other tracks
	for (unsigned int i = Track; i < m_iTracks;i++)
	{
		m_sTrackNames[i] = m_sTrackNames[i + 1];
		m_pTunes[i] = m_pTunes[i + 1];
	}

	m_pTunes[m_iTracks] = NULL;

	--m_iTracks;

	if (m_iTrack > m_iTracks)
		m_iTrack = m_iTracks;	// Last track was removed

	SwitchToTrack(m_iTrack);

	SetModifiedFlag();
	UpdateViews();
}

void FtmDocument::SetTrackTitle(unsigned int Track, std::string Title)
{
	if (m_sTrackNames[Track] == Title)
		return;

	m_sTrackNames[Track] = Title;

	SetModifiedFlag();
	UpdateViews();
}

void FtmDocument::MoveTrackUp(unsigned int Track)
{
	const std::string &Temp = m_sTrackNames[Track];
	m_sTrackNames[Track] = m_sTrackNames[Track - 1];
	m_sTrackNames[Track - 1] = Temp;

	CPatternData *pTemp = m_pTunes[Track];
	m_pTunes[Track] = m_pTunes[Track - 1];
	m_pTunes[Track - 1] = pTemp;

	SetModifiedFlag();
	UpdateViews();
}

void FtmDocument::MoveTrackDown(unsigned int Track)
{
	const std::string &Temp = m_sTrackNames[Track];
	m_sTrackNames[Track] = m_sTrackNames[Track + 1];
	m_sTrackNames[Track + 1] = Temp;

	CPatternData *pTemp = m_pTunes[Track];
	m_pTunes[Track] = m_pTunes[Track + 1];
	m_pTunes[Track + 1] = pTemp;

	SetModifiedFlag();
	UpdateViews();
}

CInstrument *FtmDocument::GetInstrument(int Index)
{
	// This may return a NULL pointer
	ftm_Assert(Index >= 0 && Index < MAX_INSTRUMENTS);
	return m_pInstruments[Index];
}

int FtmDocument::GetInstrumentCount() const
{
	int count = 0;
	for(int i=0;i<MAX_INSTRUMENTS;i++)
	{
		if( IsInstrumentUsed( i ) ) ++count;
	}
	return count;
}

bool FtmDocument::IsInstrumentUsed(int Index) const
{
	ftm_Assert(Index >= 0 && Index < MAX_INSTRUMENTS);
	return !(m_pInstruments[Index] == NULL);
}

int FtmDocument::AddInstrument(CInstrument *pInst)
{
	// Add an existing instrument to instrument list

	int Slot = FindFreeInstrumentSlot();

	if (Slot == -1)
		return -1;

	m_pInstruments[Slot] = pInst;

	SetModifiedFlag();
	UpdateViews();

	return Slot;
}

int FtmDocument::AddInstrument(const char *Name, int ChipType)
{
	// Adds a new instrument to the module
	int Slot = FindFreeInstrumentSlot();

	if (Slot == -1)
		return -1;

	m_pInstruments[Slot] = app::channelMap()->GetChipInstrument(ChipType);

	ftm_Assert(m_pInstruments[Slot] != NULL);

	// TODO: move this to instrument classes

	switch (ChipType)
	{
		case SNDCHIP_NONE:
		case SNDCHIP_MMC5:
			for (int i=0;i<SEQ_COUNT;i++)
			{
				((CInstrument2A03*)m_pInstruments[Slot])->SetSeqEnable(i, 0);
				((CInstrument2A03*)m_pInstruments[Slot])->SetSeqIndex(i, GetFreeSequence(i));
			}
			break;
		case SNDCHIP_VRC6:
			for (int i=0;i<SEQ_COUNT;i++)
			{
				((CInstrumentVRC6*)m_pInstruments[Slot])->SetSeqEnable(i, 0);
				((CInstrumentVRC6*)m_pInstruments[Slot])->SetSeqIndex(i, GetFreeSequenceVRC6(i));
			}
			break;
	}

	m_pInstruments[Slot]->SetName(Name);

	SetModifiedFlag();
	UpdateViews();

	return Slot;
}

void FtmDocument::RemoveInstrument(unsigned int Index)
{
	// Removes an instrument from the module

	ftm_Assert(Index < MAX_INSTRUMENTS);

	if (m_pInstruments[Index] == NULL)
		return;

	delete m_pInstruments[Index];
	m_pInstruments[Index] = NULL;

	SetModifiedFlag();
	UpdateViews();
}

void FtmDocument::GetInstrumentName(unsigned int Index, char *Name, unsigned int sz) const
{
	ftm_Assert(Index < MAX_INSTRUMENTS);
	ftm_Assert(m_pInstruments[Index] != NULL);

	m_pInstruments[Index]->GetName(Name, sz);
}

void FtmDocument::SetInstrumentName(unsigned int Index, const char *Name)
{
	ftm_Assert(Index < MAX_INSTRUMENTS);
	ftm_Assert(m_pInstruments[Index] != NULL);

	if (m_pInstruments[Index] != NULL)
	{
		if (strcmp(m_pInstruments[Index]->GetName(), Name) != 0)
		{
			m_pInstruments[Index]->SetName(Name);
			SetModifiedFlag();
		}
	}
}

int FtmDocument::CloneInstrument(unsigned int Index)
{
	ftm_Assert(Index < MAX_INSTRUMENTS);

	if (!IsInstrumentUsed(Index))
		return -1;

	int Slot = FindFreeInstrumentSlot();

	if (Slot != -1)
	{
		m_pInstruments[Slot] = GetInstrument(Index)->Clone();
		SetModifiedFlag();
		//UpdateAllViews(NULL, UPDATE_INSTRUMENTS);
	}

	return Slot;
}

CInstrument * FtmDocument::CreateInstrument(int type)
{
	// Creates a new instrument of selected type
	switch (type)
	{
		case INST_2A03: return new CInstrument2A03;
		case INST_VRC6: return new CInstrumentVRC6;
	// TODO - dan
/*		case INST_VRC7: return new CInstrumentVRC7();
		case INST_N106:	return new CInstrumentN106();
		case INST_FDS: return new CInstrumentFDS();
		case INST_S5B: return new CInstrumentS5B();*/
	}

	ftm_Assert(0);
	return NULL;
}

int FtmDocument::FindFreeInstrumentSlot()
{
	// Returns a free instrument slot, or -1 if no free slots exists
	for (int i=0;i<MAX_INSTRUMENTS;i++)
	{
		if (m_pInstruments[i] == NULL)
			return i;
	}
	return -1;
}

void FtmDocument::SaveInstrument(unsigned int Instrument, IO *io)
{
	CInstrument *pInstrument = GetInstrument(Instrument);

	ftm_Assert(pInstrument != NULL);

	// Write
	io->write_e(INST_HEADER, sizeof(INST_HEADER)-1);
	io->write_e(INST_VERSION, sizeof(INST_VERSION)-1);

	// Write type
	char InstType = pInstrument->GetType();
	io->writeChar(InstType);

	// Write name
	char Name[256];
	pInstrument->GetName(Name, sizeof(Name));
	int NameSize = (int)strlen(Name);
	io->writeInt(NameSize);
	io->write_e(Name, NameSize);

	// Write instrument data
	pInstrument->SaveFile(io, this);
}

int FtmDocument::LoadInstrument(IO *io)
{
	int Slot = FindFreeInstrumentSlot();

	if (Slot == -1)
	{
		// inst_limit
		return -1;
	}

	// Signature
	char Text[256];
	io->read_e(Text, sizeof(INST_HEADER)-1);
	Text[sizeof(INST_HEADER)-1] = 0;

	if (strcmp(Text, INST_HEADER) != 0)
	{
		// failed
		return -1;
	}

	// Version
	io->read_e(Text, sizeof(INST_VERSION)-1);
	Text[sizeof(INST_VERSION)-1] = 0;

	int iInstMaj, iInstMin;

	sscanf(Text, "%i.%i", &iInstMaj, &iInstMin);
	int iInstVer = iInstMaj * 10 + iInstMin;

	sscanf(INST_VERSION, "%i.%i", &iInstMaj, &iInstMin);
	int iCurrentVer = iInstMaj * 10 + iInstMin;

	if (iInstVer > iCurrentVer)
	{
		// version not supported
		return -1;
	}

	// Type
	char InstType;
	io->readChar(InstType);

	if (!InstType)
		InstType = INST_2A03;

	CInstrument *pInstrument = CreateInstrument(InstType);
	m_pInstruments[Slot] = pInstrument;

	// Name
	unsigned int NameLen;
	io->readInt(&NameLen);

	ftm_Assert(NameLen < 256);

	io->read(Text, NameLen);
	Text[NameLen] = 0;

	pInstrument->SetName(Text);

	ftm_Assert(pInstrument->LoadFile(io, iInstVer, this));

	return Slot;
}

int FtmDocument::GetInstrumentType(unsigned int Index) const
{
	ftm_Assert(Index < MAX_INSTRUMENTS);

	if (!IsInstrumentUsed(Index))
		return INST_NONE;

	return m_pInstruments[Index]->GetType();
}

// Sequences

CSequence *FtmDocument::GetSequence(int Chip, int Index, int Type)
{
	ftm_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	switch (Chip)
	{
		case SNDCHIP_NONE:
			return GetSequence(Index, Type);
		case SNDCHIP_VRC6:
			return GetSequenceVRC6(Index, Type);
		// TODO - dan
/*		case SNDCHIP_N106:
			return GetSequenceN106(Index, Type);*/
	}

	return NULL;
}

CSequence *FtmDocument::GetSequence(int Index, int Type)
{
	ftm_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequences2A03[Index][Type] == NULL)
		m_pSequences2A03[Index][Type] = new CSequence();

	return m_pSequences2A03[Index][Type];
}

int FtmDocument::GetSequenceItemCount(int Index, int Type) const
{
	ftm_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequences2A03[Index][Type] == NULL)
		return 0;

	return m_pSequences2A03[Index][Type]->GetItemCount();
}

int FtmDocument::GetFreeSequence(int Type) const
{
	// Return a free sequence slot
	for (int i=0;i<MAX_SEQUENCES;i++)
	{
		if (GetSequenceItemCount(i, Type) == 0)
			return i;
	}
	return 0;
}

int FtmDocument::GetSequenceCount(int Type) const
{
	// Return number of allocated sequences of Type
	int Count = 0;
	for (int i=0;i<MAX_SEQUENCES;i++)
	{
		if (GetSequenceItemCount(i, Type) > 0)
			++Count;
	}
	return Count;
}

// VRC6

CSequence *FtmDocument::GetSequenceVRC6(int Index, int Type)
{
	ftm_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequencesVRC6[Index][Type] == NULL)
		m_pSequencesVRC6[Index][Type] = new CSequence();

	return m_pSequencesVRC6[Index][Type];
}

CSequence *FtmDocument::GetSequenceVRC6(int Index, int Type) const
{
	ftm_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);
	return m_pSequencesVRC6[Index][Type];
}

int FtmDocument::GetSequenceItemCountVRC6(int Index, int Type) const
{
	ftm_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequencesVRC6[Index][Type] == NULL)
		return 0;

	return m_pSequencesVRC6[Index][Type]->GetItemCount();
}

int FtmDocument::GetFreeSequenceVRC6(int Type) const
{
	for (int i=0;i<MAX_SEQUENCES;i++)
	{
		if (GetSequenceItemCountVRC6(i, Type) == 0)
			return i;
	}
	return 0;
}

// DMC Stuff, DPCM samples

CDSample * FtmDocument::GetDSample(unsigned int Index)
{
	ftm_Assert(Index < MAX_DSAMPLES);
	return &m_DSamples[Index];
}

int FtmDocument::GetSampleCount() const
{
	int Count = 0;
	for (int i=0;i<MAX_DSAMPLES;i++)
	{
		if (m_DSamples[i].SampleSize > 0)
			++Count;
	}
	return Count;
}

int FtmDocument::GetFreeDSample() const
{
	for (int i=0;i<MAX_DSAMPLES;i++)
	{
		if (m_DSamples[i].SampleSize == 0)
		{
			return i;
		}
	}
	// Out of free samples
	return -1;
}

void FtmDocument::RemoveDSample(unsigned int Index)
{
	ftm_Assert(Index < MAX_DSAMPLES);

	if (m_DSamples[Index].SampleSize != 0)
	{
		if (m_DSamples[Index].SampleData != NULL)
		{
			delete[] m_DSamples[Index].SampleData;
			m_DSamples[Index].SampleData = NULL;
		}

		m_DSamples[Index].SampleSize = 0;
		SetModifiedFlag();
	}
}

void FtmDocument::GetSampleName(unsigned int Index, char *Name, unsigned int sz) const
{
	ftm_Assert(Index < MAX_DSAMPLES);
	ftm_Assert(m_DSamples[Index].SampleSize > 0);
	safe_strcpy(Name, m_DSamples[Index].Name, sz);
}

int FtmDocument::GetSampleSize(unsigned int Sample)
{
	ftm_Assert(Sample < MAX_DSAMPLES);
	return m_DSamples[Sample].SampleSize;
}

char FtmDocument::GetSampleData(unsigned int Sample, unsigned int Offset)
{
	ftm_Assert(Sample < MAX_DSAMPLES);

	if (Offset >= m_DSamples[Sample].SampleSize)
		return 0;

	return m_DSamples[Sample].SampleData[Offset];
}

int FtmDocument::GetTotalSampleSize() const
{
	// Return total size of all loaded samples
	int Size = 0;
	for (int i=0;i<MAX_DSAMPLES;i++)
	{
		Size += m_DSamples[i].SampleSize;
	}
	return Size;
}

void FtmDocument::ConvertSequence(stSequence *OldSequence, CSequence *NewSequence, int Type)
{
	// This function is used to convert old version sequences (used by older file versions)
	// to the current version

	if (OldSequence->Count > 0 && OldSequence->Count < MAX_SEQUENCE_ITEMS)
	{

		// Save a pointer to this
		int iLoopPoint = -1;
		int iLength = 0;
		int ValPtr = 0;

		// Store the sequence
		int Count = OldSequence->Count;

		for (int k=0;k<Count;k++)
		{
			int Value	= OldSequence->Value[k];
			int Length	= OldSequence->Length[k];

			if (Length < 0)
			{
				iLoopPoint = 0;
				for (int l=signed(OldSequence->Count)+Length-1; l<signed(OldSequence->Count)-1; l++)
				{
					iLoopPoint += (OldSequence->Length[l] + 1);
				}
			}
			else {
				for (int l=0;l<Length+1;l++)
				{
					if ((Type == SEQ_PITCH || Type == SEQ_HIPITCH) && l > 0)
						NewSequence->SetItem(ValPtr++, 0);
					else
						NewSequence->SetItem(ValPtr++, (unsigned char)Value);
					iLength++;
				}
			}
		}

		if (iLoopPoint != -1)
		{
			if (iLoopPoint > iLength)
				iLoopPoint = iLength;
			iLoopPoint = iLength - iLoopPoint;
		}

		NewSequence->SetItemCount(ValPtr);
		NewSequence->SetLoopPoint(iLoopPoint);
	}
}

void FtmDocument::AllocateSong(unsigned int Song)
{
	// Allocate a new song if not already done
	if (m_pTunes[Song] == NULL)
	{
		int Tempo = (m_iMachine == NTSC) ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL;
		m_pTunes[Song] = new CPatternData(DEFAULT_ROW_COUNT, DEFAULT_SPEED, Tempo);
		m_sTrackNames[Song] = DEFAULT_TRACK_NAME;
	}

//	SetModifiedFlag();
}
