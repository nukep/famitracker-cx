#include <string.h>
#include <stdio.h>
#include <boost/thread/mutex.hpp>
#include "App.hpp"
#include "FtmDocument.hpp"
#include "Document.hpp"
#include "PatternData.h"
#include "Instrument.h"
#include "TrackerChannel.h"
#include "Sequence.h"
#include "version.hpp"

#define ftm_Assert(truth) if ((! (truth) )) throw FtmDocumentExceptionAssert(__FILE__, __LINE__, FUNCTION_NAME, #truth)

const unsigned int FILE_VER			= 0x0430;			// Current file version (4.30)
const unsigned int COMPATIBLE_VER	= 0x0100;			// Compatible file version (1.0)

#define FILE_VER_STR "4.30"

// Defaults when creating new modules
const char	DEFAULT_TRACK_NAME[] = "New song";
const int	DEFAULT_ROW_COUNT = 64;

const char	NEW_INST_NAME[] = "New instrument";

// Make 1 channel default since 8 sounds bad
const int	DEFAULT_NAMCO_CHANS = 1;

const int	DEFAULT_FIRST_HIGHLIGHT = 4;
const int	DEFAULT_SECOND_HIGHLIGHT = 16;

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

// N163
const char *FILE_BLOCK_SEQUENCES_N163 = "SEQUENCES_N163";

// Sunsoft
const char *FILE_BLOCK_SEQUENCES_S5B = "SEQUENCES_S5B";

// FTI instruments files
const char INST_HEADER[] = "FTI";
const char INST_VERSION[] = "2.3";

FtmDocumentException::FtmDocumentException(Type t)
	: m_t(t)
{
	m_concatmsg = mainMessage();
}

FtmDocumentException::FtmDocumentException(Type t, std::string msg)
	: m_t(t), m_msg(msg)
{
	m_concatmsg = mainMessage();
	m_concatmsg.append("\n");
	m_concatmsg.append(msg);
}

const char * FtmDocumentException::mainMessage() const throw()
{
	// Simple exception messages when we need them (in English)
	// Translated exception messages are handled elsewhere (eg. the gui)
	switch (m_t)
	{
	case INVALIDFILETYPE: return "Invalid file type";
	case TOOOLD: return "File version is too old (current version " FILE_VER_STR ", FT " VERSION_STRING ")";
	case TOONEW: return "File version is too new (current version " FILE_VER_STR ", FT " VERSION_STRING ")";
	case GENERALREADFAILURE: return "General read failure (corrupt file?)";
	case ASSERT: return "Corrupt file assertion";
	case UNIMPLEMENTED: return "Unimplemented feature";
	case UNIMPLEMENTED_CX: return "Unimplemented feature (in FamiTracker CX)";


	// no default - all enum values should be accounted for
	}
}

const char * FtmDocumentException::specialMessage() const throw()
{
	return m_msg.c_str();
}

const char * FtmDocumentException::what() const throw()
{
	return m_concatmsg.c_str();
}

FtmDocumentExceptionAssert::FtmDocumentExceptionAssert(const char *file, int line, const char *func, const char *asrt)
	: FtmDocumentException(FtmDocumentException::ASSERT),
	  m_file(file), m_line(line), m_func(func), m_asrt(asrt)
{
	m_msg = new char[1024];
	const char *fmt = "Corrupt file assertion: %s on line %d, %s: \"%s\"";

	snprintf(m_msg, 1024, fmt, file, line, func, asrt);
}

FtmDocumentExceptionAssert::~FtmDocumentExceptionAssert() throw()
{
	delete[] m_msg;
}

const char * FtmDocumentExceptionAssert::what() const throw()
{
	return m_msg;
}

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
	m_modifyLock = new boost::mutex;

	for (int i = 0; i < MAX_DSAMPLES; i++)
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
	for (int i = 0; i < MAX_DSAMPLES; i++)
	{
		if (m_DSamples[i].SampleData != NULL)
		{
			delete[] m_DSamples[i].SampleData;
			m_DSamples[i].SampleData = NULL;
			m_DSamples[i].SampleSize = 0;
		}
	}

	// Patterns
	for (int i = 0; i < MAX_TRACKS; i++)
	{
		if (m_pTunes[i] != NULL)
			delete m_pTunes[i];
	}

	// Instruments
	for (int i = 0; i < MAX_INSTRUMENTS; i++)
	{
		if (m_pInstruments[i] != NULL)
			delete m_pInstruments[i];
	}

	// Sequences
	for (int i = 0; i < MAX_SEQUENCES; i++)
	{
		for (int j = 0; j < SEQ_COUNT; j++)
		{
			if (m_pSequences2A03[i][j] != NULL)
				delete m_pSequences2A03[i][j];

			if (m_pSequencesVRC6[i][j] != NULL)
				delete m_pSequencesVRC6[i][j];

			if (m_pSequencesN106[i][j] != NULL)
				delete m_pSequencesN106[i][j];
		}
	}

	delete m_modifyLock;
}

void FtmDocument::lock() const
{
	m_modifyLock->lock();
}

void FtmDocument::unlock() const
{
	m_modifyLock->unlock();
}

void FtmDocument::createEmpty()
{
	m_iMachine = DEFAULT_MACHINE_TYPE;
	m_iEngineSpeed = 0;
	// Allocate first song
	SwitchToTrack(0);
	m_iTracks = 0;

	// and select 2A03 only
	SelectExpansionChip(SNDCHIP_NONE);

	// Auto-select new style vibrato for new modules
	m_iVibratoStyle = VIBRATO_NEW;
	m_bLinearPitch = false;
	m_iSpeedSplitPoint = DEFAULT_SPEED_SPLIT_POINT;

	m_highlight = DEFAULT_FIRST_HIGHLIGHT;
	m_secondHighlight = DEFAULT_SECOND_HIGHLIGHT;

	SetModifiedFlag(0);
}

void FtmDocument::read(core::IO *io)
{
	try
	{
		FtmDocument_lock_guard lock(this);

		Document doc;
		doc.setIO(io);
		bForceBackup = false;

		if (!doc.checkValidity())
		{
			throw FtmDocumentException::INVALIDFILETYPE;
		}

		unsigned int ver = doc.getFileVersion();
		m_iFileVersion = ver;

		if (ver < 0x0200)
		{
			// Older file version
			if (ver < COMPATIBLE_VER)
			{
				throw FtmDocumentException::TOOOLD;
			}

			if (!readOld(&doc))
			{
				throw FtmDocumentException::GENERALREADFAILURE;
			}

			// Create a backup of this file, since it's an old version
			// and something might go wrong when converting
			bForceBackup = true;

			// Auto-select old style vibrato for old files
			m_iVibratoStyle = VIBRATO_OLD;
			m_bLinearPitch = false;
		}
		else if (ver >= 0x0200)
		{
			// New file version

			if (ver > FILE_VER)
			{
				// File version is too new
				throw FtmDocumentException::TOONEW;
			}

			if (!readNew(&doc))
			{
				throw FtmDocumentException::GENERALREADFAILURE;
			}

			// Backup if files was of an older version
			bForceBackup = ver < FILE_VER;
		}
	}
	catch (FtmDocumentException::Type t)
	{
		throw FtmDocumentException(t);
	}
}

bool FtmDocument::readOld(Document *doc)
{
	// TODO
	throw FtmDocumentException(FtmDocumentException::TOOOLD, "File versions below 1.00 are currently unimplemented in FamiTracker CX");
	return false;
}

bool FtmDocument::readNew(Document *doc)
{
	while (!doc->isFileDone())
	{
		if (!doc->readBlock())
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
			return false;
		}
	}

#undef CMP

	if (m_iFileVersion <= 0x0201)
	{
		reorderSequences();
	}

	if (m_iFileVersion < 0x0300)
	{
		convertSequences();
	}

	return true;
}

bool FtmDocument::readNew_params(Document *doc)
{
	unsigned int block_ver = doc->getBlockVersion();
	unsigned int ver1_song_speed;
	if (block_ver == 1)
	{
		ver1_song_speed = doc->getBlockInt();
		SelectExpansionChip(SNDCHIP_NONE);
	}
	else
	{
		SelectExpansionChip(doc->getBlockChar());
	}

	m_iChannelsAvailable = doc->getBlockInt();
	m_iMachine = doc->getBlockInt();
	m_iEngineSpeed = doc->getBlockInt();

	ftm_Assert(m_iChannelsAvailable < MAX_CHANNELS);

	if (m_iMachine != NTSC && m_iMachine != PAL)
		m_iMachine = NTSC;

	if (m_iFileVersion < 0x0210)
	{
		// This has to be done for older files
		SwitchToTrack(0);
	}

	if (block_ver == 1)
	{
		m_pSelectedTune->SetSongSpeed(ver1_song_speed);
	}

	if (block_ver > 2)
		m_iVibratoStyle = doc->getBlockInt();
	else
		m_iVibratoStyle = VIBRATO_OLD;

	// TODO read m_bLinearPitch
	m_bLinearPitch = false;

	if (block_ver > 3)
	{
		m_highlight = doc->getBlockInt();
		m_secondHighlight = doc->getBlockInt();
	}
	else
	{
		m_highlight = DEFAULT_FIRST_HIGHLIGHT;
		m_secondHighlight = DEFAULT_SECOND_HIGHLIGHT;
	}

	// remark: porting bug fix over
	// This is strange. Sometimes expansion chip is set to 0xFF in files
	if (m_iChannelsAvailable == 5)
		SelectExpansionChip(SNDCHIP_NONE);

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

	if (block_ver >= 6)
	{
		m_iSpeedSplitPoint = doc->getBlockInt();
	}
	else
	{
		// Determine if new or old speed point is preferred
		m_iSpeedSplitPoint = OLD_SPEED_SPLIT_POINT;
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
		for (unsigned int i = 0; i < m_iChannelsAvailable; i++)
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

		for (unsigned int i = 0; i <= m_iTracks; i++)
		{
			AllocateSong(i);
		}

		// Track names
		if (block_ver >= 3)
		{
			for (unsigned int i = 0; i <= m_iTracks; i++)
			{
				m_sTrackNames[i] = doc->readString();
			}
		}

		for (unsigned int i = 0; i < m_iChannelsAvailable; i++)
		{
			/*unsigned char channelType = */doc->getBlockChar();	// Channel type (unused)
			for (unsigned int j = 0; j <= m_iTracks; j++)
			{
				m_pTunes[j]->SetEffectColumnCount(i, doc->getBlockChar());	// Effect columns
			}
		}
	}
	return true;
}

bool FtmDocument::readNew_instruments(Document *doc)
{
	/*
	 * Version changes
	 *
	 *  2 - Extended DPCM octave range
	 *  3 - Added settings to the arpeggio sequence
	 *
	 */
	int count = doc->getBlockInt();
	ftm_Assert(count <= MAX_INSTRUMENTS);

	for (int i = 0; i < count; i++)
	{
		int index = doc->getBlockInt();
		ftm_Assert(index <= MAX_INSTRUMENTS);

		int type = doc->getBlockChar();

		if (!fami_isInstrumentImplemented(type))
			throw FtmDocumentException(FtmDocumentException::UNIMPLEMENTED_CX, "Instrument not supported\n"
									   "Supported: 2A03/2A07, MMC5, VRC6, VRC7, FDS");

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
		for (unsigned int i = 0; i < count; i++)
		{
			index = doc->getBlockInt();
			seqCount = doc->getBlockChar();
			ftm_Assert(index < MAX_SEQUENCES);
			ftm_Assert(seqCount < MAX_SEQUENCE_ITEMS);
			m_TmpSequences[index].Count = seqCount;
			for (unsigned int x = 0; x < seqCount; x++)
			{
				m_TmpSequences[index].Value[x] = doc->getBlockChar();
				m_TmpSequences[index].Length[x] = doc->getBlockChar();
			}
		}
	}
	else if (block_ver == 2)
	{
		for (unsigned int i = 0; i < count; i++)
		{
			index = doc->getBlockInt();
			type = doc->getBlockInt();
			seqCount = doc->getBlockChar();
			ftm_Assert(index < MAX_SEQUENCES);
			ftm_Assert(type < SEQ_COUNT);
			ftm_Assert(seqCount < MAX_SEQUENCE_ITEMS);
			m_Sequences[index][type].Count = seqCount;
			for (unsigned int x = 0; x < seqCount; x++)
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

		for (unsigned int i = 0; i < count; i++)
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

			CSequence *pSeq = GetSequence2A03(index, type);

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

			for (unsigned int x = 0; x < seqCount; x++)
			{
				value = doc->getBlockChar();
				if (x <= MAX_SEQUENCE_ITEMS)
					pSeq->SetItem(x, value);
			}
		}

		if (block_ver == 5)
		{
			// Version 5 saved the release points incorrectly, this is fixed in ver 6
			for (int i = 0; i < MAX_SEQUENCES; i++)
			{
				for (int x = 0; x < SEQ_COUNT; x++)
				{
					releasePoint = doc->getBlockInt();
					settings = doc->getBlockInt();
					if (GetSequenceItemCount(i, x) > 0)
					{
						CSequence *pSeq = GetSequence2A03(i, x);
						pSeq->SetReleasePoint(releasePoint);
						pSeq->SetSetting(settings);
					}
				}
			}
		}
		else if (block_ver >= 6)
		{
			// Read release points correctly stored
			for (unsigned int i = 0; i < count; i++)
			{
				releasePoint = doc->getBlockInt();
				settings = doc->getBlockInt();
				index = Indices[i];
				type = Types[i];
				CSequence *pSeq = GetSequence2A03(index, type);
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
		for (unsigned int i = 0; i < frameCount; i++)
		{
			for (unsigned j = 0; j < m_iChannelsAvailable; j++)
			{
				unsigned pattern = (unsigned)doc->getBlockChar();
				ftm_Assert(pattern < MAX_FRAMES);
				m_pSelectedTune->SetFramePattern(i, j, pattern);
			}
		}
	}
	else if (block_ver > 1)
	{
		for (unsigned y = 0; y <= m_iTracks; y++)
		{
			unsigned int frameCount = doc->getBlockInt();
			unsigned int speed = doc->getBlockInt();

			ftm_Assert(frameCount > 0 && frameCount <= MAX_FRAMES);
			ftm_Assert(speed > 0);

			m_pTunes[y]->SetFrameCount(frameCount);

			if (block_ver == 3)
			{
				unsigned int tempo = doc->getBlockInt();
				ftm_Assert(speed >= 0);
				ftm_Assert(tempo >= 0);
				m_pTunes[y]->SetSongTempo(tempo);
				m_pTunes[y]->SetSongSpeed(speed);
			}
			else
			{
				if (speed < 20)
				{
					unsigned int tempo = (m_iMachine == NTSC) ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL;
					ftm_Assert(tempo >= 0 && tempo <= MAX_TEMPO);
					ftm_Assert(speed >= 0);
					m_pTunes[y]->SetSongTempo(tempo);
					m_pTunes[y]->SetSongSpeed(speed);
				}
				else
				{
					ftm_Assert(speed >= 0 && speed <= MAX_TEMPO);
					m_pTunes[y]->SetSongTempo(speed);
					m_pTunes[y]->SetSongSpeed(DEFAULT_SPEED);
				}
			}

			unsigned PatternLength = (unsigned)doc->getBlockInt();
			ftm_Assert(PatternLength > 0 && PatternLength <= MAX_PATTERN_LENGTH);

			m_pTunes[y]->SetPatternLength(PatternLength);

			for (unsigned int i = 0; i < frameCount; i++)
			{
				for (unsigned j = 0; j < m_iChannelsAvailable; j++)
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

		for (unsigned int i = 0; i < items; i++)
		{
			unsigned row;
			if (m_iFileVersion == 0x0200)
				row = doc->getBlockChar();
			else
				row = doc->getBlockInt();

			ftm_Assert(row < MAX_PATTERN_LENGTH);

			stChanNote note;
			memset(&note, 0, sizeof(stChanNote));

			note.Note		 = doc->getBlockChar();
			note.Octave	 = doc->getBlockChar();
			note.Instrument = doc->getBlockChar();
			note.Vol		 = doc->getBlockChar();

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

				note.EffNumber[0]	= EffectNumber;
				note.EffParam[0]	= EffectParam;
			}
			else
			{
				for (int n = 0; n < (m_pSelectedTune->GetEffectColumnCount(channel) + 1); n++)
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

					note.EffNumber[n]	= EffectNumber;
					note.EffParam[n] 	= EffectParam;
				}
			}

			if (note.Vol > 0x10)
				note.Vol &= 0x0F;

			// Specific for version 2.0
			if (m_iFileVersion == 0x0200)
			{

				if (note.EffNumber[0] == EF_SPEED && note.EffParam[0] < 20)
					note.EffParam[0]++;

				if (note.Vol == 0)
				{
					note.Vol = 0x10;
				}
				else
				{
					note.Vol--;
					note.Vol &= 0x0F;
				}

				if (note.Note == 0)
					note.Instrument = MAX_INSTRUMENTS;
			}

			if (block_ver == 3)
			{
				// Fix for VRC7 portamento
				if (GetExpansionChip() == SNDCHIP_VRC7 && channel > 4)
				{
					for (int n = 0; n < MAX_EFFECT_COLUMNS; n++)
					{
						switch (note.EffNumber[n])
						{
							case EF_PORTA_DOWN:
								note.EffNumber[n] = EF_PORTA_UP;
								break;
							case EF_PORTA_UP:
								note.EffNumber[n] = EF_PORTA_DOWN;
								break;
						}
					}
				}
				// FDS pitch effect fix
				else if (GetExpansionChip() == SNDCHIP_FDS && channel == 5)
				{
					for (int n = 0; n < MAX_EFFECT_COLUMNS; n++)
					{
						switch (note.EffNumber[n])
						{
							case EF_PITCH:
								if (note.EffParam[n] != 0x80)
									note.EffParam[n] = (0x100 - note.EffParam[n]) & 0xFF;
								break;
						}
					}
				}
			}
#ifdef TRANSPOSE_FDS
			if (version < 5)
			{
				// FDS octave
				if (GetExpansionChip() == SNDCHIP_FDS && channel > 4 && note.Octave < 7)
				{
					note.Octave++;
				}
			}
#endif

			SetDataAtPattern(m_iTrack, pattern, channel, row, &note);
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

	for (int i = 0; i < count; i++)
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
	unsigned int count = 0, index, type;
	unsigned int loopPoint, releasePoint, settings;
	unsigned char seqCount;
	int block_ver = doc->getBlockVersion();
	char Value;

	count = doc->getBlockInt();
	ftm_Assert(count < MAX_SEQUENCES);

	if (block_ver < 4)
	{
		for (unsigned int i = 0; i < count; i++)
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
			for (unsigned int j = 0; j < seqCount; j++)
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

		for (unsigned int i = 0; i < count; i++)
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

			for (unsigned int j = 0; j < seqCount; j++)
			{
				Value = doc->getBlockChar();
				if (j <= MAX_SEQUENCE_ITEMS)
					pSeq->SetItem(j, Value);
			}
		}

		if (block_ver == 5)
		{
			// Version 5 saved the release points incorrectly, this is fixed in ver 6
			for (int i = 0; i < MAX_SEQUENCES; i++)
			{
				for (int j = 0; j < SEQ_COUNT; j++)
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
			for (unsigned int i = 0; i < count; i++)
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

void FtmDocument::write(core::IO *io) const
{
	FtmDocument_lock_guard lock(this);

	Document doc;
	doc.setIO(io);

	doc.writeBegin(FILE_VER);

	if (!writeBlocks(&doc))
	{

	}

	doc.writeEnd();
}

bool FtmDocument::writeBlocks(Document *doc) const
{
	if (!write_params(doc))
		return false;
	if (!write_songinfo(doc))
		return false;
	if (!write_header(doc))
		return false;
	if (!write_instruments(doc))
		return false;
	if (!write_sequences(doc))
		return false;
	if (!write_frames(doc))
		return false;
	if (!write_patterns(doc))
		return false;
	if (!write_dsamples(doc))
		return false;

	if (m_iExpansionChip & SNDCHIP_VRC6)
	{
		if (!write_sequencesVRC6(doc))
			return false;
	}

	return true;
}

bool FtmDocument::write_params(Document *doc) const
{
	// Module parameters
	doc->createBlock(FILE_BLOCK_PARAMS, 6);

	doc->writeBlockChar(m_iExpansionChip);		// ver 2 change
	doc->writeBlockInt(m_iChannelsAvailable);
	doc->writeBlockInt(m_iMachine);
	doc->writeBlockInt(m_iEngineSpeed);
	doc->writeBlockInt(m_iVibratoStyle);		// ver 3 change
	doc->writeBlockInt(m_highlight);			// ver 4 change
	doc->writeBlockInt(m_secondHighlight);
	doc->writeBlockInt(m_iSpeedSplitPoint);		// ver 6 change

	return doc->flushBlock();
}

bool FtmDocument::write_songinfo(Document *doc) const
{
	doc->createBlock(FILE_BLOCK_INFO, 1);

	doc->writeBlock(m_strName, 32);
	doc->writeBlock(m_strArtist, 32);
	doc->writeBlock(m_strCopyright, 32);

	return doc->flushBlock();
}

bool FtmDocument::write_header(Document *doc) const
{
	/*
	 *  Header data
	 *
	 *  Store song count and then for each channel:
	 *  channel type and number of effect columns
	 *
	 */

	// Version 3 adds song names

	// Header data
	doc->createBlock(FILE_BLOCK_HEADER, 3);

	// Write number of tracks
	doc->writeBlockChar(m_iTracks);

	// Ver 3, store track names
	for (unsigned int i = 0; i <= m_iTracks; ++i)
	{
		doc->writeString(m_sTrackNames[i]);
	}

	for (unsigned int i = 0; i < m_iChannelsAvailable; i++)
	{
		// Channel type
		int chantype = m_channelsFromChip[i];
		doc->writeBlockChar(chantype);
		for (unsigned int j = 0; j <= m_iTracks; j++)
		{
			ftm_Assert(m_pTunes[j] != NULL);
			// Effect columns
			doc->writeBlockChar(m_pTunes[j]->GetEffectColumnCount(i));
		}
	}

	return doc->flushBlock();
}

bool FtmDocument::write_instruments(Document *doc) const
{
	// A bug in v0.3.0 causes a crash if this is not 2, so change only when that ver is obsolete!
	const int BLOCK_VERSION = 5;
	// If FDS is used then version must be at least 4 or recent files won't load
	int Version = BLOCK_VERSION;

	int Count = 0;
	char Type;

	// Instruments block
	doc->createBlock(FILE_BLOCK_INSTRUMENTS, Version);

	// Count number of instruments
	for (int i = 0; i < MAX_INSTRUMENTS; i++)
	{
		if (m_pInstruments[i] != NULL)
			Count++;
	}

	doc->writeBlockInt(Count);

	for (int i = 0; i < MAX_INSTRUMENTS; i++)
	{
		// Only write instrument if it's used
		if (m_pInstruments[i] != NULL)
		{

			Type = m_pInstruments[i]->GetType();

			// Write index and type
			doc->writeBlockInt(i);
			doc->writeBlockChar(Type);

			// Store the instrument
			m_pInstruments[i]->Store(doc);

			// Store the name
			const char *name = m_pInstruments[i]->GetName();
			int namelen = strlen(name);
			doc->writeBlockInt(namelen);
			doc->writeBlock(name, namelen);
		}
	}

	return doc->flushBlock();
}

bool FtmDocument::write_sequences(Document *doc) const
{
	/*
	 * Store 2A03 sequences
	 */

	// Sequences, version 6
	doc->createBlock(FILE_BLOCK_SEQUENCES, 6);

	int Count = 0;

	// Count number of used sequences
	for (int i = 0; i < MAX_SEQUENCES; i++)
	{
		for (int j = 0; j < SEQ_COUNT; j++)
		{
			if (GetSequenceItemCount(i, j) > 0)
				Count++;
		}
	}

	doc->writeBlockInt(Count);

	for (int i = 0; i < MAX_SEQUENCES; i++)
	{
		for (int j = 0; j < SEQ_COUNT; j++)
		{
			Count = GetSequenceItemCount(i, j);
			if (Count > 0)
			{
				const CSequence *pSeq = GetSequence2A03_readonly(i, j);
				// Store index
				doc->writeBlockInt(i);
				// Store type of sequence
				doc->writeBlockInt(j);
				// Store number of items in this sequence
				doc->writeBlockChar(Count);
				// Store loop point
				doc->writeBlockInt(pSeq->GetLoopPoint());
				// Store items
				for (int k = 0; k < Count; k++)
				{
					doc->writeBlockChar(pSeq->GetItem(k));
				}
			}
		}
	}

	// v6
	for (int i = 0; i < MAX_SEQUENCES; i++)
	{
		for (int j = 0; j < SEQ_COUNT; j++)
		{
			Count = GetSequenceItemCount(i, j);
			if (Count > 0)
			{
				const CSequence *pSeq = GetSequence2A03_readonly(i, j);
				// Store release point
				doc->writeBlockInt(pSeq->GetReleasePoint());
				// Store setting
				doc->writeBlockInt(pSeq->GetSetting());
			}
		}
	}

	return doc->flushBlock();
}

bool FtmDocument::write_sequencesVRC6(Document *doc) const
{
	/*
	 * Store VRC6 sequences
	 */

	// Sequences, version 6
	doc->createBlock(FILE_BLOCK_SEQUENCES_VRC6, 6);

	int Count = 0;

	// Count number of used sequences
	for (int i = 0; i < MAX_SEQUENCES; i++)
	{
		for (int j = 0; j < SEQ_COUNT; j++)
		{
			if (GetSequenceItemCountVRC6(i, j) > 0)
				Count++;
		}
	}

	// Write it
	doc->writeBlockInt(Count);

	for (int i = 0; i < MAX_SEQUENCES; i++)
	{
		for (int j = 0; j < SEQ_COUNT; j++)
		{
			Count = GetSequenceItemCountVRC6(i, j);
			if (Count > 0) {
				const CSequence *pSeq = GetSequenceVRC6_readonly(i, j);
				// Store index
				doc->writeBlockInt(i);
				// Store type of sequence
				doc->writeBlockInt(j);
				// Store number of items in this sequence
				doc->writeBlockChar(Count);
				// Store loop point
				doc->writeBlockInt(pSeq->GetLoopPoint());
				// Store items
				for (int k = 0; k < Count; ++k) {
					doc->writeBlockChar(pSeq->GetItem(k));
				}
			}
		}
	}

	// v6
	for (int i = 0; i < MAX_SEQUENCES; i++)
	{
		for (int j = 0; j < SEQ_COUNT; j++)
		{
			Count = GetSequenceItemCountVRC6(i, j);
			if (Count > 0) {
				const CSequence *pSeq = GetSequenceVRC6_readonly(i, j);
				// Store release point
				doc->writeBlockInt(pSeq->GetReleasePoint());
				// Store setting
				doc->writeBlockInt(pSeq->GetSetting());
			}
		}
	}

	return doc->flushBlock();
}

bool FtmDocument::write_frames(Document *doc) const
{
	/* Store frame count
	 *
	 * 1. Number of channels (5 for 2A03 only)
	 * 2.
	 *
	 */

	doc->createBlock(FILE_BLOCK_FRAMES, 3);

	for (unsigned i = 0; i <= m_iTracks; i++)
	{
		CPatternData *pTune = m_pTunes[i];

		doc->writeBlockInt(pTune->GetFrameCount());
		doc->writeBlockInt(pTune->GetSongSpeed());
		doc->writeBlockInt(pTune->GetSongTempo());
		doc->writeBlockInt(pTune->GetPatternLength());

		for (unsigned int j = 0; j < pTune->GetFrameCount(); j++)
		{
			for (unsigned k = 0; k < m_iChannelsAvailable; k++)
			{
				doc->writeBlockChar((unsigned char)pTune->GetFramePattern(j, k));
			}
		}
	}

	return doc->flushBlock();
}

bool FtmDocument::write_patterns(Document *doc) const
{
	/*
	 * Version changes:
	 *
	 *  2: Support multiple tracks
	 *  3: Changed portamento effect
	 *  4: Switched portamento effects for VRC7 (1xx & 2xx), adjusted Pxx for FDS
	 *  5: Adjusted FDS octave
	 *
	 */

#ifdef TRANSPOSE_FDS
	doc->createBlock(FILE_BLOCK_PATTERNS, 5);
#else
	doc->createBlock(FILE_BLOCK_PATTERNS, 4);
#endif

	for (unsigned t = 0; t <= m_iTracks; t++)
	{
		for (unsigned i = 0; i < m_iChannelsAvailable; i++)
		{
			for (unsigned x = 0; x < MAX_PATTERN; x++)
			{
				unsigned Items = 0;

				// Save all rows
				unsigned int PatternLen = MAX_PATTERN_LENGTH;
				//unsigned int PatternLen = m_pTunes[t]->GetPatternLength();

				// Get the number of items in this pattern
				for (unsigned y = 0; y < PatternLen; y++)
				{
					if (!m_pTunes[t]->IsCellFree(i, x, y))
						Items++;
				}

				if (Items > 0)
				{
					doc->writeBlockInt(t);		// Write track
					doc->writeBlockInt(i);		// Write channel
					doc->writeBlockInt(x);		// Write pattern
					doc->writeBlockInt(Items);	// Number of items

					for (unsigned y = 0; y < PatternLen; y++)
					{
						if (!m_pTunes[t]->IsCellFree(i, x, y))
						{
							doc->writeBlockInt(y);

							stChanNote note;
							m_pTunes[t]->GetPatternData(i, x, y, &note);

							doc->writeBlockChar(note.Note);
							doc->writeBlockChar(note.Octave);
							doc->writeBlockChar(note.Instrument);
							doc->writeBlockChar(note.Vol);

							int EffColumns = (m_pTunes[t]->GetEffectColumnCount(i) + 1);

							for (int n = 0; n < EffColumns; n++)
							{
								doc->writeBlockChar(note.EffNumber[n]);
								doc->writeBlockChar(note.EffParam[n]);
							}
						}
					}
				}
			}
		}
	}

	return doc->flushBlock();
}

bool FtmDocument::write_dsamples(Document *doc) const
{
	int Count = 0;

	doc->createBlock(FILE_BLOCK_DSAMPLES, 1);

	for (int i = 0; i < MAX_DSAMPLES; i++)
	{
		if (m_DSamples[i].SampleSize > 0)
			Count++;
	}

	// Write sample count
	doc->writeBlockChar(Count);

	for (int i = 0; i < MAX_DSAMPLES; i++)
	{
		if (m_DSamples[i].SampleSize > 0)
		{
			// Write sample
			doc->writeBlockChar(i);
			doc->writeBlockInt((int)strlen(m_DSamples[i].Name));
			doc->writeBlock(m_DSamples[i].Name, (int)strlen(m_DSamples[i].Name));
			doc->writeBlockInt(m_DSamples[i].SampleSize);
			doc->writeBlock(m_DSamples[i].SampleData, m_DSamples[i].SampleSize);
		}
	}

	return doc->flushBlock();
}

void FtmDocument::SetFrameCount(unsigned int Count)
{
	ftkr_Assert(Count <= MAX_FRAMES);

	if (m_pSelectedTune->GetFrameCount() != Count)
	{
		m_pSelectedTune->SetFrameCount(Count);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::SetPatternLength(unsigned int Length)
{
	ftkr_Assert(Length <= MAX_PATTERN_LENGTH);

	if (m_pSelectedTune->GetPatternLength() != Length)
	{
		m_pSelectedTune->SetPatternLength(Length);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::SetSongSpeed(unsigned int Speed)
{
	ftkr_Assert(Speed <= MAX_SPEED);

	if (m_pSelectedTune->GetSongSpeed() != Speed)
	{
		m_pSelectedTune->SetSongSpeed(Speed);
		SetModifiedFlag();
	}
}

void FtmDocument::SetSongTempo(unsigned int Tempo)
{
	ftkr_Assert(Tempo <= MAX_TEMPO);

	if (m_pSelectedTune->GetSongTempo() != Tempo)
	{
		m_pSelectedTune->SetSongTempo(Tempo);
		SetModifiedFlag();
	}
}

unsigned int FtmDocument::getFramePlayLength(unsigned int frame) const
{
	ftkr_Assert(frame < GetFrameCount());

	return m_pSelectedTune->getFramePlayLength(frame, GetAvailableChannels());
}

unsigned int FtmDocument::GetEffColumns(int Track, unsigned int Channel) const
{
	ftkr_Assert(Channel < MAX_CHANNELS);
	return m_pTunes[Track]->GetEffectColumnCount(Channel);
}

unsigned int FtmDocument::GetEffColumns(unsigned int Channel) const
{
	ftkr_Assert(Channel < MAX_CHANNELS);
	return m_pSelectedTune->GetEffectColumnCount(Channel);
}

void FtmDocument::SetEffColumns(unsigned int Channel, unsigned int Columns)
{
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Columns < MAX_EFFECT_COLUMNS);

//	GetChannel(Channel)->SetColumnCount(Columns);
	m_pSelectedTune->SetEffectColumnCount(Channel, Columns);

	SetModifiedFlag();
	UpdateViews();
}

unsigned int FtmDocument::GetPatternAtFrame(int Track, unsigned int Frame, unsigned int Channel) const
{
	ftkr_Assert(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);
	return m_pTunes[Track]->GetFramePattern(Frame, Channel);
}

unsigned int FtmDocument::GetPatternAtFrame(unsigned int Frame, unsigned int Channel) const
{
	ftkr_Assert(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);
	return m_pSelectedTune->GetFramePattern(Frame, Channel);
}

void FtmDocument::SetPatternAtFrame(unsigned int Frame, unsigned int Channel, unsigned int Pattern)
{
	ftkr_Assert(Frame < MAX_FRAMES && Channel < MAX_CHANNELS && Pattern < MAX_PATTERN);
	m_pSelectedTune->SetFramePattern(Frame, Channel, Pattern);
//	SetModifiedFlag();
}

int FtmDocument::GetFirstFreePattern(int Channel)
{
	for (int i = 0; i < MAX_PATTERN; i++)
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
	ftkr_Assert(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);

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
	ftkr_Assert(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);

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
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	stChanNote note;

	GetNoteData(Frame, Channel, Row, &note);

	if (note.Instrument < MAX_INSTRUMENTS)
	{
		note.Instrument++;
		SetNoteData(Frame, Channel, Row, &note);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::DecreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	stChanNote note;

	GetNoteData(Frame, Channel, Row, &note);

	if (note.Instrument > 0)
	{
		note.Instrument--;
		SetNoteData(Frame, Channel, Row, &note);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::IncreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	stChanNote note;

	GetNoteData(Frame, Channel, Row, &note);

	if (note.Vol < 16)
	{
		note.Vol++;
		SetNoteData(Frame, Channel, Row, &note);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::DecreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	stChanNote note;

	GetNoteData(Frame, Channel, Row, &note);

	if (note.Vol > 1)
	{
		note.Vol--;
		SetNoteData(Frame, Channel, Row, &note);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::IncreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index)
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);
	ftkr_Assert(Index < MAX_EFFECT_COLUMNS);

	stChanNote note;

	GetNoteData(Frame, Channel, Row, &note);

	if (note.EffParam[Index] < 256)
	{
		note.EffParam[Index]++;
		SetNoteData(Frame, Channel, Row, &note);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::DecreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index)
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);
	ftkr_Assert(Index < MAX_EFFECT_COLUMNS);

	stChanNote note;

	GetNoteData(Frame, Channel, Row, &note);

	if (note.EffParam[Index] > 0)
	{
		note.EffParam[Index]--;
		SetNoteData(Frame, Channel, Row, &note);
		SetModifiedFlag();
		UpdateViews();
	}
}

void FtmDocument::increaseEffColumns(unsigned int channel)
{
	unsigned int cols = GetEffColumns(channel);
	if (cols >= MAX_EFFECT_COLUMNS-1)
		return;

	SetEffColumns(channel, cols+1);
}
void FtmDocument::decreaseEffColumns(unsigned int channel)
{
	unsigned int cols = GetEffColumns(channel);
	if (cols == 0)
		return;

	SetEffColumns(channel, cols-1);
}

void FtmDocument::SetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, const stChanNote *Data)
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	// Get notes from the pattern
	m_pSelectedTune->SetPatternData(Channel, GET_PATTERN(Frame, Channel), Row, Data);
	SetModifiedFlag();
}

void FtmDocument::GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data) const
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	// Sets the notes of the pattern
	m_pSelectedTune->GetPatternData(Channel, GET_PATTERN(Frame, Channel), Row, Data);
}

void FtmDocument::SetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, const stChanNote *Data)
{
	ftkr_Assert(Track < MAX_TRACKS);
	ftkr_Assert(Pattern < MAX_PATTERN);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	// Set a note to a direct pattern
	m_pTunes[Track]->SetPatternData(Channel, Pattern, Row, Data);
	SetModifiedFlag();
}

void FtmDocument::GetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *Data) const
{
	ftkr_Assert(Track < MAX_TRACKS);
	ftkr_Assert(Pattern < MAX_PATTERN);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	// Get note from a direct pattern
	m_pTunes[Track]->GetPatternData(Channel,Pattern, Row, Data);
}

unsigned int FtmDocument::GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);
	ftkr_Assert(Index < MAX_EFFECT_COLUMNS);

	stChanNote note;
	GetNoteData(Frame, Channel, Row, &note);
	return note.EffNumber[Index];
}

unsigned int FtmDocument::GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);
	ftkr_Assert(Index < MAX_EFFECT_COLUMNS);

	stChanNote note;
	GetNoteData(Frame, Channel, Row, &note);
	return note.EffParam[Index];
}

bool FtmDocument::InsertNote(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	stChanNote Note;

	for (int n = 0; n < MAX_EFFECT_COLUMNS; n++)
	{
		Note.EffNumber[n] = 0;
		Note.EffParam[n] = 0;
	}

	Note.Note		= 0;
	Note.Octave		= 0;
	Note.Instrument	= MAX_INSTRUMENTS;
	Note.Vol		= 0x10;

	for (unsigned int i = m_pSelectedTune->GetPatternLength() - 1; i > Row; i--)
	{
		stChanNote tmp;
		GetNoteData(Frame, Channel, i-1, &tmp);
		SetNoteData(Frame, Channel, i, &tmp);
	}

	SetNoteData(Frame, Channel, Row, &Note);

	SetModifiedFlag();

	return true;
}

bool FtmDocument::DeleteNote(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Column)
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	stChanNote note;

	GetNoteData(Frame, Channel, Row, &note);

	if (Column == C_NOTE)
	{
		note.Note = 0;
		note.Octave = 0;
		note.Instrument = MAX_INSTRUMENTS;
		note.Vol = 0x10;
	}
	else if (Column == C_INSTRUMENT1 || Column == C_INSTRUMENT2)
	{
		note.Instrument = MAX_INSTRUMENTS;
	}
	else if (Column == C_VOLUME)
	{
		note.Vol = 0x10;
	}
	else
	{
		int eff = (Column - C_EFF_NUM) / (C_EFF_COL_COUNT);
		if (eff >= MAX_EFFECT_COLUMNS)
			return false;
		note.EffNumber[eff] = 0;
		note.EffNumber[eff] = 0;
	}

	SetNoteData(Frame, Channel, Row, &note);

	SetModifiedFlag();

	return true;
}


bool FtmDocument::ClearRow(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	stChanNote note;

	GetNoteData(Frame, Channel, Row, &note);

	note.Note = 0;
	note.Octave = 0;
	note.Instrument = MAX_INSTRUMENTS;
	note.Vol = 0x10;

	SetNoteData(Frame, Channel, Row, &note);

	SetModifiedFlag();

	return true;
}

bool FtmDocument::RemoveNote(unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ftkr_Assert(Frame < MAX_FRAMES);
	ftkr_Assert(Channel < MAX_CHANNELS);
	ftkr_Assert(Row < MAX_PATTERN_LENGTH);

	if (Row == 0)
		return false;

	stChanNote Note;

	for (int n = 0; n < MAX_EFFECT_COLUMNS; n++)
	{
		Note.EffNumber[n] = 0;
		Note.EffParam[n] = 0;
	}

	Note.Note = 0;
	Note.Octave = 0;
	Note.Instrument = MAX_INSTRUMENTS;
	Note.Vol = 0x10;

	for (unsigned int i = Row - 1; i < (m_pSelectedTune->GetPatternLength() - 1); i++)
	{
		stChanNote tmp;
		GetNoteData(Frame, Channel, i+1, &tmp);
		SetNoteData(Frame, Channel, i, &tmp);
	}

	SetNoteData(Frame, Channel, m_pSelectedTune->GetPatternLength() - 1, &Note);

	SetModifiedFlag();

	return true;
}

static inline int hexFromChar(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;

	return -1;
}
static inline int charToEffNum(char c)
{
	for (int i = 0; i < EF_COUNT-1; i++)
	{
		if (EFF_CHAR[i] == c)
		{
			return i+1;
		}
	}
	return -1;
}

bool FtmDocument::setColumnKey(int key, unsigned int frame, unsigned int channel, unsigned int row, unsigned int column)
{
	stChanNote note;
	GetNoteData(frame, channel, row, &note);
	switch(column)
	{
	case C_NOTE: return false;
	case C_INSTRUMENT1:
	{
		int i = hexFromChar(key);
		if (i < 0)
			return false;
		if (note.Instrument == MAX_INSTRUMENTS)
			note.Instrument = 0;
		note.Instrument = (i<<4) | (note.Instrument & 0x0F);
		if (note.Instrument >= MAX_INSTRUMENTS)
			note.Instrument = MAX_INSTRUMENTS-1;
	}
		break;
	case C_INSTRUMENT2:
	{
		int i = hexFromChar(key);
		if (i < 0)
			return false;
		if (note.Instrument == MAX_INSTRUMENTS)
			note.Instrument = 0;
		note.Instrument = (note.Instrument & 0xF0) | i;
		if (note.Instrument >= MAX_INSTRUMENTS)
			note.Instrument = MAX_INSTRUMENTS-1;
	}
		break;
	case C_VOLUME:
	{
		int i = hexFromChar(key);
		if (i < 0)
			return false;
		note.Vol = i;
	}
		break;
	default:
	{
		int c = column - C_EFF_NUM;
		int ec = c % C_EFF_COL_COUNT;
		int en = c / C_EFF_COL_COUNT;

		if (en >= MAX_EFFECT_COLUMNS)
			return false;

		if (ec == 0)
		{
			int n = charToEffNum(key);
			if (n < 0)
				return false;
			note.EffNumber[en] = n;
			note.EffParam[en] = 0;
		}
		else
		{
			if (note.EffNumber[en] == EF_NONE)
				return false;
			int i = hexFromChar(key);
			if (i < 0)
				return false;
			if (ec == 1)
				note.EffParam[en] = (i<<4) | (note.EffParam[en] & 0x0F);
			else if (ec == 2)
				note.EffParam[en] = (note.EffParam[en] & 0xF0) | i;
		}
	}
		break;
	}
	SetNoteData(frame, channel, row, &note);
	return true;
}

void FtmDocument::SetEngineSpeed(unsigned int Speed)
{
	ftkr_Assert(Speed <= 800); // hardcoded at the moment, TODO: fix this
	ftkr_Assert(Speed >= 10 || Speed == 0);

	m_iEngineSpeed = Speed;
	SetModifiedFlag();
}

void FtmDocument::SetMachine(unsigned int Machine)
{
	ftkr_Assert(Machine == PAL || Machine == NTSC);
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

	m_channelsFromChip = app::channelMap()->GetChannelsFromChip(Chip);
	m_iChannelsAvailable = m_channelsFromChip.size();

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
	ftkr_Assert(Name != NULL && Artist != NULL && Copyright != NULL);

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

bool FtmDocument::GetLinearPitch() const
{
	return m_bLinearPitch;
}

void FtmDocument::SetLinearPitch(bool enable)
{
	m_bLinearPitch = enable;
}

const std::string & FtmDocument::GetComment() const
{
	return m_strComment;
}

void FtmDocument::SetComment(const std::string &comment)
{
	m_strComment = comment;
}

int FtmDocument::GetSpeedSplitPoint() const
{
	return m_iSpeedSplitPoint;
}

void FtmDocument::SetSpeedSplitPoint(int splitPoint)
{
	m_iSpeedSplitPoint = splitPoint;
}

// Track functions

void FtmDocument::SelectTrack(unsigned int Track)
{
	ftkr_Assert(Track < MAX_TRACKS);
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
	ftkr_Assert(m_iTracks > 0);
	ftkr_Assert(m_pTunes[Track] != NULL);

	delete m_pTunes[Track];

	// Move down all other tracks
	for (unsigned int i = Track; i < m_iTracks; i++)
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
	if (Track == 0)
		return;

	const std::string Temp = m_sTrackNames[Track];
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
	if (Track >= m_iTracks)
		return;

	const std::string Temp = m_sTrackNames[Track];
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
	ftkr_Assert(Index >= 0 && Index < MAX_INSTRUMENTS);
	return m_pInstruments[Index];
}

int FtmDocument::GetInstrumentCount() const
{
	int count = 0;
	for (int i = 0; i < MAX_INSTRUMENTS; i++)
	{
		if( IsInstrumentUsed( i ) ) count++;
	}
	return count;
}

bool FtmDocument::IsInstrumentUsed(int Index) const
{
	ftkr_Assert(Index >= 0 && Index < MAX_INSTRUMENTS);
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

	ftkr_Assert(m_pInstruments[Slot] != NULL);

	// TODO: move this to instrument classes

	switch (ChipType)
	{
		case SNDCHIP_NONE:
		case SNDCHIP_MMC5:
			for (int i = 0; i < SEQ_COUNT; i++)
			{
				((CInstrument2A03*)m_pInstruments[Slot])->SetSeqEnable(i, 0);
				((CInstrument2A03*)m_pInstruments[Slot])->SetSeqIndex(i, GetFreeSequence(i));
			}
			break;
		case SNDCHIP_VRC6:
			for (int i = 0; i < SEQ_COUNT; i++)
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

	ftkr_Assert(Index < MAX_INSTRUMENTS);

	if (m_pInstruments[Index] == NULL)
		return;

	delete m_pInstruments[Index];
	m_pInstruments[Index] = NULL;

	SetModifiedFlag();
	UpdateViews();
}

void FtmDocument::GetInstrumentName(unsigned int Index, char *Name, unsigned int sz) const
{
	ftkr_Assert(Index < MAX_INSTRUMENTS);
	ftkr_Assert(m_pInstruments[Index] != NULL);

	m_pInstruments[Index]->GetName(Name, sz);
}

void FtmDocument::SetInstrumentName(unsigned int Index, const char *Name)
{
	ftkr_Assert(Index < MAX_INSTRUMENTS);
	ftkr_Assert(m_pInstruments[Index] != NULL);

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
	ftkr_Assert(Index < MAX_INSTRUMENTS);

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
		case INST_FDS: return new CInstrumentFDS;
		case INST_VRC7: return new CInstrumentVRC7;
		// TODO - dan
/*		case INST_N106:	return new CInstrumentN106;
		case INST_S5B: return new CInstrumentS5B;*/
	default:
		ftkr_Assert(0);
	}
}

int FtmDocument::FindFreeInstrumentSlot()
{
	// Returns a free instrument slot, or -1 if no free slots exists
	for (int i = 0; i < MAX_INSTRUMENTS; i++)
	{
		if (m_pInstruments[i] == NULL)
			return i;
	}
	return -1;
}

void FtmDocument::SaveInstrument(unsigned int Instrument, core::IO *io)
{
	CInstrument *pInstrument = GetInstrument(Instrument);

	ftkr_Assert(pInstrument != NULL);

	// Write
	io->write_e(INST_HEADER, sizeof(INST_HEADER)-1);
	io->write_e(INST_VERSION, sizeof(INST_VERSION)-1);

	// Write type
	char InstType = pInstrument->GetType();
	io->writeChar(InstType);

	// Write name
	const char *name = pInstrument->GetName();
	int nameSize = strlen(name);
	io->writeInt(nameSize);
	io->write_e(name, nameSize);

	// Write instrument data
	pInstrument->SaveFile(io, this);
}

int FtmDocument::LoadInstrument(core::IO *io)
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

	ftkr_Assert(NameLen < 256);

	io->read(Text, NameLen);
	Text[NameLen] = 0;

	pInstrument->SetName(Text);

	ftkr_Assert(pInstrument->LoadFile(io, iInstVer, this));

	return Slot;
}

int FtmDocument::GetInstrumentType(unsigned int Index) const
{
	ftkr_Assert(Index < MAX_INSTRUMENTS);

	if (!IsInstrumentUsed(Index))
		return INST_NONE;

	return m_pInstruments[Index]->GetType();
}

// Sequences

CSequence *FtmDocument::GetSequence(int Chip, int Index, int Type)
{
	ftkr_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	switch (Chip)
	{
		case SNDCHIP_NONE:
			return GetSequence2A03(Index, Type);
		case SNDCHIP_VRC6:
			return GetSequenceVRC6(Index, Type);
		// TODO - dan
/*		case SNDCHIP_N106:
			return GetSequenceN106(Index, Type);*/
	}

	return NULL;
}

CSequence * FtmDocument::GetSequence_readonly(int Chip, int Index, int Type) const
{
	ftkr_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	switch (Chip)
	{
		case SNDCHIP_NONE:
			return GetSequence2A03_readonly(Index, Type);
		case SNDCHIP_VRC6:
			return GetSequenceVRC6_readonly(Index, Type);
		// TODO - dan
/*		case SNDCHIP_N106:
			return GetSequenceN106(Index, Type);*/
	}

	return NULL;
}

CSequence * FtmDocument::GetSequence2A03(int Index, int Type)
{
	ftkr_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequences2A03[Index][Type] == NULL)
		m_pSequences2A03[Index][Type] = new CSequence();

	return m_pSequences2A03[Index][Type];
}

CSequence * FtmDocument::GetSequence2A03_readonly(int Index, int Type) const
{
	ftkr_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	return m_pSequences2A03[Index][Type];
}

int FtmDocument::GetSequenceItemCount(int Index, int Type) const
{
	ftkr_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequences2A03[Index][Type] == NULL)
		return 0;

	return m_pSequences2A03[Index][Type]->GetItemCount();
}

int FtmDocument::GetFreeSequence(unsigned char Chip, int Type) const
{
	if (Chip == SNDCHIP_NONE)
		return GetFreeSequence(Type);
	else if (Chip == SNDCHIP_VRC6)
		return GetFreeSequenceVRC6(Type);
	return 0;
}

int FtmDocument::GetFreeSequence(int Type) const
{
	// Return a free sequence slot
	for (int i = 0; i < MAX_SEQUENCES; i++)
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
	for (int i = 0; i < MAX_SEQUENCES; i++)
	{
		if (GetSequenceItemCount(i, Type) > 0)
			Count++;
	}
	return Count;
}

// VRC6

CSequence *FtmDocument::GetSequenceVRC6(int Index, int Type)
{
	ftkr_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequencesVRC6[Index][Type] == NULL)
		m_pSequencesVRC6[Index][Type] = new CSequence();

	return m_pSequencesVRC6[Index][Type];
}

CSequence *FtmDocument::GetSequenceVRC6_readonly(int Index, int Type) const
{
	ftkr_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);
	return m_pSequencesVRC6[Index][Type];
}

int FtmDocument::GetSequenceItemCountVRC6(int Index, int Type) const
{
	ftkr_Assert(Index >= 0 && Index < MAX_SEQUENCES && Type >= 0 && Type < SEQ_COUNT);

	if (m_pSequencesVRC6[Index][Type] == NULL)
		return 0;

	return m_pSequencesVRC6[Index][Type]->GetItemCount();
}

int FtmDocument::GetFreeSequenceVRC6(int Type) const
{
	for (int i = 0; i < MAX_SEQUENCES; i++)
	{
		if (GetSequenceItemCountVRC6(i, Type) == 0)
			return i;
	}
	return 0;
}

// DMC Stuff, DPCM samples

CDSample * FtmDocument::GetDSample(unsigned int Index)
{
	ftkr_Assert(Index < MAX_DSAMPLES);
	return &m_DSamples[Index];
}

int FtmDocument::GetSampleCount() const
{
	int Count = 0;
	for (int i = 0; i < MAX_DSAMPLES; i++)
	{
		if (m_DSamples[i].SampleSize > 0)
			Count++;
	}
	return Count;
}

int FtmDocument::GetFreeDSample() const
{
	for (int i = 0; i < MAX_DSAMPLES; i++)
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
	ftkr_Assert(Index < MAX_DSAMPLES);

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
	ftkr_Assert(Index < MAX_DSAMPLES);
	ftkr_Assert(m_DSamples[Index].SampleSize > 0);
	safe_strcpy(Name, m_DSamples[Index].Name, sz);
}

int FtmDocument::GetSampleSize(unsigned int Sample)
{
	ftkr_Assert(Sample < MAX_DSAMPLES);
	return m_DSamples[Sample].SampleSize;
}

char FtmDocument::GetSampleData(unsigned int Sample, unsigned int Offset)
{
	ftkr_Assert(Sample < MAX_DSAMPLES);

	if (Offset >= m_DSamples[Sample].SampleSize)
		return 0;

	return m_DSamples[Sample].SampleData[Offset];
}

int FtmDocument::GetTotalSampleSize() const
{
	// Return total size of all loaded samples
	int Size = 0;
	for (int i = 0; i < MAX_DSAMPLES; i++)
	{
		Size += m_DSamples[i].SampleSize;
	}
	return Size;
}

int FtmDocument::LoadSample(core::IO *io, const char *name)
{
	int idx = GetFreeDSample();
	if (idx == -1)
		return -1;

	CDSample *samp = GetDSample(idx);
	Quantity sz = io->size();
	safe_strcpy(samp->Name, name, sizeof(samp->Name));
	samp->Allocate(sz);
	io->read(samp->SampleData, sz);

	return idx;
}

void FtmDocument::SaveSample(core::IO *io, unsigned int Index) const
{
	const CDSample *samp = &m_DSamples[Index];
	io->write(samp->SampleData, samp->SampleSize);
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

		for (int k = 0; k < Count; k++)
		{
			int Value	= OldSequence->Value[k];
			int Length	= OldSequence->Length[k];

			if (Length < 0)
			{
				iLoopPoint = 0;
				for (int l = signed(OldSequence->Count)+Length-1; l < signed(OldSequence->Count)-1; l++)
				{
					iLoopPoint += (OldSequence->Length[l] + 1);
				}
			}
			else {
				for (int l = 0; l < Length+1; l++)
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

#define LIMIT(v, max, min) v = ((v > max) ? max : ((v < min) ? min : v));//  if (v > max) v = max; else if (v < min) v = min;

void FtmDocument::reorderSequences()
{
	int Keepers[SEQ_COUNT] = {0, 0, 0, 0, 0};
	int Indices[MAX_SEQUENCES][SEQ_COUNT];
	int Index;

	memset(Indices, 0xFF, MAX_SEQUENCES * SEQ_COUNT * sizeof(int));

	CInstrument2A03	*pInst;

	// Organize sequences
	for (int i = 0; i < MAX_INSTRUMENTS; i++)
	{
		if (m_pInstruments[i] != NULL) {
			pInst = (CInstrument2A03*)m_pInstruments[i];
			for (int x = 0; x < SEQ_COUNT; x++)
			{
				if (pInst->GetSeqEnable(x))
				{
					Index = pInst->GetSeqIndex(x);
					if (Indices[Index][x] >= 0 && Indices[Index][x] != -1)
					{
						pInst->SetSeqIndex(x, Indices[Index][x]);
					}
					else {
						memcpy(&m_Sequences[Keepers[x]][x], &m_TmpSequences[Index], sizeof(stSequence));
						for (unsigned int j = 0; j < m_Sequences[Keepers[x]][x].Count; j++)
						{
							switch (x)
							{
								case SEQ_VOLUME: LIMIT(m_Sequences[Keepers[x]][x].Value[j], 15, 0); break;
								case SEQ_DUTYCYCLE: LIMIT(m_Sequences[Keepers[x]][x].Value[j], 3, 0); break;
							}
						}
						Indices[Index][x] = Keepers[x];
						pInst->SetSeqIndex(x, Keepers[x]);
						Keepers[x]++;
					}
				}
				else
				{
					pInst->SetSeqIndex(x, 0);
				}
			}
		}
	}
}

void FtmDocument::convertSequences()
{
	int i, j, k;
	int iLength, ValPtr, Count, Value, Length;
	stSequence	*pSeq;
	CSequence	*pNewSeq;

	// This function is used to convert the old type sequences to new type

	for (i = 0; i < MAX_SEQUENCES; i++)
	{
		for (j = 0; j < /*MAX_SEQUENCE_ITEMS*/ SEQ_COUNT; j++)
		{
			pSeq = &m_Sequences[i][j];
			if (pSeq->Count > 0 && pSeq->Count < MAX_SEQUENCE_ITEMS)
			{
				pNewSeq = GetSequence2A03(i, j);

				// Save a pointer to this
				int iLoopPoint = -1;
				iLength = 0;
				ValPtr = 0;

				// Store the sequence
				Count = pSeq->Count;
				for (k = 0; k < Count; k++)
				{
					Value	= pSeq->Value[k];
					Length	= pSeq->Length[k];

					if (Length < 0)
					{
						iLoopPoint = 0;
						for (int l = signed(pSeq->Count) + Length - 1; l < signed(pSeq->Count) - 1; l++)
						{
							iLoopPoint += (pSeq->Length[l] + 1);
						}
					}
					else {
						for (int l = 0; l < Length + 1 && ValPtr < MAX_SEQUENCE_ITEMS; l++)
						{
							if ((j == SEQ_PITCH || j == SEQ_HIPITCH) && l > 0)
								pNewSeq->SetItem(ValPtr++, 0);
							else
								pNewSeq->SetItem(ValPtr++, (unsigned char)Value);
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

				pNewSeq->SetItemCount(ValPtr);
				pNewSeq->SetLoopPoint(iLoopPoint);
			}
		}
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
