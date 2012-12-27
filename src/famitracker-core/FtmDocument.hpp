#ifndef _FTM_DOCUMENT_HPP
#define _FTM_DOCUMENT_HPP

// Get access to some APU constants
#include "APU/APU.h"
#include "types.hpp"
#include "FamiTrackerTypes.h"
#include "Instrument.h"
#include "common.hpp"

#include <string>
#include <vector>

class CPatternData;
class Document;
namespace core
{
	class IO;
}
class CTrackerChannel;

namespace boost
{
	class mutex;
}

// Default song settings
const unsigned int DEFAULT_TEMPO_NTSC   = 150;
const unsigned int DEFAULT_TEMPO_PAL    = 125;
const unsigned int DEFAULT_SPEED	    = 6;
const unsigned int DEFAULT_MACHINE_TYPE = NTSC;

const unsigned int DEFAULT_SPEED_SPLIT_POINT = 32;
const unsigned int OLD_SPEED_SPLIT_POINT = 21;

const unsigned int VOLUME_EMPTY = 0x10;		// Value of cleared volume column field

const unsigned int MAX_SONGINFO_LENGTH = 31;	// not including null char

// Columns
enum
{
	C_NOTE=0,
	C_INSTRUMENT1, C_INSTRUMENT2,
	C_VOLUME,
	C_EFF_NUM, C_EFF_PARAM1, C_EFF_PARAM2,
	C_EFF_COL_COUNT=3,
	C_EFF_PARAM_COUNT=2
};

// Old sequence list, kept for compability
struct stSequence {
	unsigned int Count;
	signed char Length[MAX_SEQUENCE_ITEMS];
	signed char Value[MAX_SEQUENCE_ITEMS];
};

class FtmDocumentException : public std::exception
{
public:
	enum Type
	{
		INVALIDFILETYPE,
		TOOOLD,
		TOONEW,
		GENERALREADFAILURE,
		ASSERT,
		UNIMPLEMENTED,
		UNIMPLEMENTED_CX	// not implemented in FamiTracker CX
	};
	explicit FtmDocumentException(Type t);
	explicit FtmDocumentException(Type t, std::string msg);
	~FtmDocumentException() throw(){}

	const char * mainMessage() const throw();
	const char * specialMessage() const throw();

	const char * what() const throw();

	Type type() const{ return m_t; }
private:
	Type m_t;
	std::string m_msg;
	std::string m_concatmsg;
};

class FtmDocumentExceptionAssert : public FtmDocumentException
{
public:
	explicit FtmDocumentExceptionAssert(const char *file, int line, const char *func, const char *asrt);
	virtual ~FtmDocumentExceptionAssert() throw();
	const char * what() const throw();
private:
	const char * m_file;
	int m_line;
	const char * m_func;
	const char * m_asrt;
	char * m_msg;
};

class FAMICOREAPI FtmDocument
{
public:
	FtmDocument();
	~FtmDocument();

	// use lock/unlock while accessing the document
	void lock() const;
	void unlock() const;

	void createEmpty();

	void read(core::IO *io);
	void write(core::IO *io) const;

	bool doForceBackup() const{ return bForceBackup; }

	// TODO - dan: Deperecate from FtmDocument and move to SoundGen
/*
	void			ResetChannels();
	void			RegisterChannel(CTrackerChannel *pChannel, int ChannelType, int ChipType);
	CTrackerChannel	*GetChannel(int Index) const;

	int				GetChannelType(int Channel) const;
	int				GetChipType(int Channel) const;
	int				GetChannelCount() const{ return m_iRegisteredChannels; }
*/
	// Local (song) data
	void			SetFrameCount(unsigned int Count);
	void			SetPatternLength(unsigned int Length);
	void			SetSongSpeed(unsigned int Speed);
	void			SetSongTempo(unsigned int Tempo);

	unsigned int	GetPatternLength()		const { return m_pSelectedTune->GetPatternLength(); }
	unsigned int	GetFrameCount()			const { return m_pSelectedTune->GetFrameCount(); }
	unsigned int	GetSongSpeed()			const { return m_pSelectedTune->GetSongSpeed(); }
	unsigned int	GetSongTempo()			const { return m_pSelectedTune->GetSongTempo(); }
	unsigned int	GetAvailableChannels()	const { return m_iChannelsAvailable; }

	unsigned int	GetPatternLength(int Track) const { return m_pTunes[Track]->GetPatternLength(); }
	unsigned int	GetFrameCount(int Track)	const { return m_pTunes[Track]->GetFrameCount(); }
	unsigned int	GetSongSpeed(int Track)		const { return m_pTunes[Track]->GetSongSpeed(); }
	unsigned int	GetSongTempo(int Track)		const { return m_pTunes[Track]->GetSongTempo(); }

	unsigned int	getFramePlayLength(unsigned int frame) const;

	unsigned int	GetEffColumns(int Track, unsigned int Channel) const;
	unsigned int	GetEffColumns(unsigned int Channel) const;
	void			SetEffColumns(unsigned int Channel, unsigned int Columns);

	unsigned int 	GetPatternAtFrame(int Track, unsigned int Frame, unsigned int Channel) const;
	unsigned int	GetPatternAtFrame(unsigned int Frame, unsigned int Channel) const;
	void			SetPatternAtFrame(unsigned int Frame, unsigned int Channel, unsigned int Pattern);

	int				GetFirstFreePattern(int Channel);

	void			ClearPatterns();

	// Pattern editing
	void			IncreasePattern(unsigned int Frame, unsigned int Channel, int Count);
	void			DecreasePattern(unsigned int Frame, unsigned int Channel, int Count);
	void			IncreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			DecreaseInstrument(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			IncreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			DecreaseVolume(unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			IncreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index);
	void			DecreaseEffect(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Index);

	void			increaseEffColumns(unsigned int channel);
	void			decreaseEffColumns(unsigned int channel);

	void			SetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, const stChanNote *Data);
	void			GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data) const;

	void			SetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, const stChanNote *Data);
	void			GetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *Data) const;

	unsigned int	GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const;
	unsigned int	GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const;

	bool			InsertNote(unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			DeleteNote(unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Column);
	bool			ClearRow(unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			RemoveNote(unsigned int Frame, unsigned int Channel, unsigned int Row);

	bool			setColumnKey(int key, unsigned int frame, unsigned int channel, unsigned int row, unsigned int column);

	// Global (module) data
	void			SetEngineSpeed(unsigned int Speed);
	void			SetMachine(unsigned int Machine);
	unsigned int	GetMachine() const		{ return m_iMachine; }
	unsigned int	GetEngineSpeed() const	{ return m_iEngineSpeed; }
	unsigned int	GetFrameRate(void) const;

	void			SelectExpansionChip(unsigned char Chip);
	unsigned char	GetExpansionChip() const { return m_iExpansionChip; }
	bool			ExpansionEnabled(int Chip) const;

	void			SetSongInfo(const char *Name, const char *Artist, const char *Copyright);
	char			*GetSongName()		 { return m_strName; }
	char			*GetSongArtist()	 { return m_strArtist; }
	char			*GetSongCopyright()	 { return m_strCopyright; }

	int				GetVibratoStyle() const;
	void			SetVibratoStyle(int Style);

	bool			GetLinearPitch() const;
	void			SetLinearPitch(bool enable);

	const std::string & GetComment() const;
	void			SetComment(const std::string &comment);

	void			SetSpeedSplitPoint(int splitPoint);
	int				GetSpeedSplitPoint() const;

	// Track management functions
	void			SelectTrack(unsigned int Track);
//	void			SelectTrackFast(unsigned int Track);	//	TODO: should be removed
	unsigned int	GetTrackCount() const{ return m_iTracks+1; }
	unsigned int	GetSelectedTrack() const{ return m_iTrack; }
	const char		*GetTrackTitle(unsigned int Track) const;
	bool			AddTrack();
	void			RemoveTrack(unsigned int Track);
	void			SetTrackTitle(unsigned int Track, std::string Title);
	void			MoveTrackUp(unsigned int Track);
	void			MoveTrackDown(unsigned int Track);

	// Instruments functions
	CInstrument		*GetInstrument(int Index);
	int				GetInstrumentCount() const;
	bool			IsInstrumentUsed(int Index) const;

	int				AddInstrument(const char *Name, int ChipType);				// Add a new instrument
	int				AddInstrument(CInstrument *pInst);
	void			RemoveInstrument(unsigned int Index);						// Remove an instrument
	void			SetInstrumentName(unsigned int Index, const char *Name);	// Set the name of an instrument
	void			GetInstrumentName(unsigned int Index, char *Name, unsigned int sz) const;	// Get the name of an instrument
	int				CloneInstrument(unsigned int Index);						// Create a copy of an instrument
	CInstrument		*CreateInstrument(int InstType);							// Creates a new instrument of InstType
	int				FindFreeInstrumentSlot();
	void			SaveInstrument(unsigned int Instrument, core::IO *io);
	int 			LoadInstrument(core::IO *io);
	int				GetInstrumentType(unsigned int Index) const;

	// Sequences functions
	CSequence		*GetSequence(int Chip, int Index, int Type);
	CSequence		*GetSequence2A03(int Index, int Type);
	CSequence		*GetSequence_readonly(int Chip, int Index, int Type) const;
	CSequence		*GetSequence2A03_readonly(int Index, int Type) const;
	int				GetSequenceItemCount(int Index, int Type) const;
	int				GetFreeSequence(unsigned char Chip, int Type) const;
	int				GetFreeSequence(int Type) const;
	int				GetSequenceCount(int Type) const;

	CSequence		*GetSequenceVRC6(int Index, int Type);
	CSequence		*GetSequenceVRC6_readonly(int Index, int Type) const;
	int				GetSequenceItemCountVRC6(int Index, int Type) const;
	int				GetFreeSequenceVRC6(int Type) const;

	// DPCM samples
	CDSample		*GetDSample(unsigned int Index);
	int				GetSampleCount() const;
	int				GetFreeDSample() const;
	void			RemoveDSample(unsigned int Index);
	void			GetSampleName(unsigned int Index, char *Name, unsigned int sz) const;
	int				GetSampleSize(unsigned int Sample);
	char			GetSampleData(unsigned int Sample, unsigned int Offset);
	int				GetTotalSampleSize() const;
	int				LoadSample(core::IO *io, const char *name);
	void			SaveSample(core::IO *io, unsigned int Index) const;

	// For file version compability
	void			ConvertSequence(stSequence *OldSequence, CSequence *NewSequence, int Type);

	void			SwitchToTrack(unsigned int Track);
	void			reorderSequences();
	void			convertSequences();

	void			AllocateSong(unsigned int Song);

	void SetModifiedFlag(bool modified=true){ m_bModified = modified; }
	void UpdateViews(){ /* TODO - dan */ }

	int				GetHighlight() const{ return m_highlight; }
	int				GetSecondHighlight() const{ return m_secondHighlight; }

	const std::vector<int> & getChannelsFromChip() const{ return m_channelsFromChip; }
private:
	bool bForceBackup;
	bool readOld(Document *doc);
	bool readNew(Document *doc);

	bool readNew_params(Document *doc);
	bool readNew_header(Document *doc);
	bool readNew_instruments(Document *doc);
	bool readNew_sequences(Document *doc);
	bool readNew_frames(Document *doc);
	bool readNew_patterns(Document *doc);
	bool readNew_dsamples(Document *doc);
	bool readNew_sequences_vrc6(Document *doc);

	bool writeBlocks(Document *doc) const;
	bool write_params(Document *doc) const;
	bool write_songinfo(Document *doc) const;
	bool write_header(Document *doc) const;
	bool write_instruments(Document *doc) const;
	bool write_sequences(Document *doc) const;
	bool write_sequencesVRC6(Document *doc) const;
	bool write_frames(Document *doc) const;
	bool write_patterns(Document *doc) const;
	bool write_dsamples(Document *doc) const;


	// TODO - dan: Deperecate from FtmDocument and move to SoundGen
/*
	CTrackerChannel	*m_pChannels[CHANNELS];
	int				m_iRegisteredChannels;
	int				m_iChannelTypes[CHANNELS];
	int				m_iChannelChip[CHANNELS];
*/
	unsigned int	m_iFileVersion;			// Loaded file version
	unsigned int	m_iTrack;				// Selected track

	CPatternData	*m_pSelectedTune;			// Points to selecte tune
	CPatternData	*m_pTunes[MAX_TRACKS];		// List of all tunes
	std::string		m_sTrackNames[MAX_TRACKS];

	unsigned int	m_iTracks;					// Track count
	unsigned int	m_iChannelsAvailable;		// Number of channels added

	// Module properties
	unsigned char	m_iExpansionChip;	// Expansion chip
	int				m_iVibratoStyle;	// 0 = old style, 1 = new style
	bool			m_bLinearPitch;

	// Instruments, samples and sequences
	CInstrument		*m_pInstruments[MAX_INSTRUMENTS];
	CDSample		m_DSamples[MAX_DSAMPLES];					// The DPCM sample list
	CSequence		*m_pSequences2A03[MAX_SEQUENCES][SEQ_COUNT];
	CSequence		*m_pSequencesVRC6[MAX_SEQUENCES][SEQ_COUNT];
	CSequence		*m_pSequencesN106[MAX_SEQUENCES][SEQ_COUNT];

	// NSF info
	char			m_strName[MAX_SONGINFO_LENGTH+1];				// Song name
	char			m_strArtist[MAX_SONGINFO_LENGTH+1];			// Song artist
	char			m_strCopyright[MAX_SONGINFO_LENGTH+1];			// Song copyright

	unsigned int	m_iMachine;					// NTSC / PAL
	unsigned int	m_iEngineSpeed;				// Refresh rate
	unsigned int	m_iSpeedSplitPoint;			// Speed/tempo split-point

	std::string		m_strComment;

	// Things below are for compability with older files
	stSequence		m_Sequences[MAX_SEQUENCES][SEQ_COUNT];		// Allocate one sequence-list for each effect
	stSequence		m_TmpSequences[MAX_SEQUENCES];

	bool m_bModified;
	int m_highlight, m_secondHighlight;

	std::vector<int> m_channelsFromChip;

	boost::mutex *	m_modifyLock;
};

class FtmDocument_lock_guard
{
public:
	FtmDocument_lock_guard(const FtmDocument *doc)
		: m_doc(doc)
	{
		m_doc->lock();
	}
	~FtmDocument_lock_guard()
	{
		m_doc->unlock();
	}

private:
	const FtmDocument *m_doc;
};

#endif

