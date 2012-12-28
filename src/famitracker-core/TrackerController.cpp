#include <stdio.h>
#include "TrackerController.hpp"
#include "FtmDocument.hpp"
#include "TrackerChannel.h"

TrackerController::TrackerController()
	: m_frame(0), m_row(0), m_elapsedFrames(0), m_halted(true),
	  m_jumpFrame(0), m_jumpRow(0)
{
}
TrackerController::~TrackerController()
{

}

void TrackerController::tick()
{
	if (m_halted)
		return;

	if (m_lastDocTempo != m_document->GetSongTempo() || m_lastDocSpeed != m_document->GetSongSpeed())
	{
		setTempo(m_document->GetSongTempo(), m_document->GetSongSpeed());

		m_lastDocTempo = m_tempo;
		m_lastDocSpeed = m_speed;
	}

	if (m_tempoAccum <= 0)
	{
		int ticksPerSec = m_document->GetFrameRate();
		m_tempoAccum += 60 * ticksPerSec;

		playRow();
	}

	m_tempoAccum -= m_tempoDecrement;
}

void TrackerController::playRow()
{
	unsigned int track = m_document->GetSelectedTrack();
	unsigned int pattern_length = m_document->GetPatternLength();
	int channels = m_document->GetAvailableChannels();

	m_jumped = false;
	m_didJump = false;

	m_frame = m_jumpFrame;
	m_row = m_jumpRow;

	for (int i=0; i < channels; i++)
	{
		stChanNote note;
		unsigned int pattern = m_document->GetPatternAtFrame(m_frame, i);
		m_document->GetDataAtPattern(track, pattern, i, m_row, &note);
		evaluateGlobalEffects(&note, m_document->GetEffColumns(i) + 1);
		if (!muted(i))
		{
			m_trackerChannels[i]->SetNote(note);
		}
	}

	if (m_jumped)
	{
	}
	else
	{
		m_jumpRow++;
		if (m_jumpRow >= pattern_length)
		{
			m_jumpRow = 0;
			m_jumpFrame++;

			if (m_jumpFrame >= m_document->GetFrameCount())
			{
				m_jumpFrame = 0;
			}
		}
	}
}

void TrackerController::startAt(unsigned int frame, unsigned int row)
{
	unsigned int num_frames = m_document->GetFrameCount();
	unsigned int num_rows = m_document->GetPatternLength();

	m_frame = frame % num_frames;
	m_row = row % num_rows;
	m_jumpFrame = m_frame;
	m_jumpRow = m_row;

	m_elapsedFrames = 0;
	m_jumped = false;
	m_halted = false;
}

void TrackerController::setFrame(unsigned int frame)
{
	if (m_jumpFrame == frame && m_jumpRow == 0)
		return;

	unsigned int num_frames = m_document->GetFrameCount();

	m_jumpFrame = frame % num_frames;
	m_jumpRow = 0;

	m_elapsedFrames++;
	m_jumped = true;
}

void TrackerController::skip(unsigned int row)
{
	setFrame(m_frame+1);
	m_jumpRow = row;
}

void TrackerController::evaluateGlobalEffects(const stChanNote *noteData, int effColumns)
{
	// Handle global effects (effects that affects all channels)
	for (int i = 0; i < effColumns; i++)
	{
		unsigned char effNum   = noteData->EffNumber[i];
		unsigned char effParam = noteData->EffParam[i];

		switch (effNum)
		{
		// Fxx: Sets speed to xx
		case EF_SPEED:
			if (effParam == 0)
				effParam++;
			if (effParam > m_document->GetSpeedSplitPoint())
				m_tempo = effParam;
			else
				m_speed = effParam;

			m_tempoDecrement = (m_tempo * 24) / m_speed;  // 24 = 6 * 4
			break;

		// Bxx: Jump to frame xx
		case EF_JUMP:
			m_didJump = true;
			setFrame(effParam);
			break;

		// Dxx: Skip to next frame and start at row xx
		case EF_SKIP:
			if (!m_didJump)
			skip(effParam);
			break;

		// Cxx: Halt playback
		case EF_HALT:
			m_halted = true;
			break;

		default:
			break;
		}
	}
}

void TrackerController::setTempo(unsigned int tempo, unsigned int speed)
{
	m_tempo = tempo;
	m_speed = speed;

	m_tempoAccum = 0;
	m_tempoDecrement = (tempo * 24) / speed;
}

void TrackerController::initialize(FtmDocument *doc, CTrackerChannel * const * trackerChannels)
{
	m_document = doc;
	m_trackerChannels = trackerChannels;

	m_lastDocTempo = m_document->GetSongTempo();
	m_lastDocSpeed = m_document->GetSongSpeed();

	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		m_muted[i] = false;
	}
}

void TrackerController::setMuted(int channel_offset, bool mute)
{
	if (mute && !m_muted[channel_offset])
	{
		stChanNote halt;
		halt.Instrument = MAX_INSTRUMENTS;
		halt.Note = HALT;
		m_trackerChannels[channel_offset]->SetNote(halt);
	}
	m_muted[channel_offset] = mute;
}
