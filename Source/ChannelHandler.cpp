/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2009  Jonathan Liss
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

// This takes care of the 2A03 channels

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "SoundGen.h"
#include "ChannelHandler.h"

void CChannelHandler::InitChannel(CAPU *pAPU, int *pVibTable, CFamiTrackerDoc *pDoc)
{
	m_pAPU = pAPU;
	m_pcVibTable = pVibTable;
	m_pDocument = pDoc;

	m_DelayNote = NULL;
	m_bDelayEnabled = false;

	m_iEffect = 0;

	KillChannel();
}

void CChannelHandler::LimitFreq()
{
	if (m_iFrequency > 0x7FF)
		m_iFrequency = 0x7FF;

	if (m_iFrequency < 0)
		m_iFrequency = 0;
}

void CChannelHandler::Arpeggiate(unsigned int Note)
{
	m_iFrequency = TriggerNote(Note);
}

void CChannelHandler::MakeSilent()
{
	m_iVolume			= MAX_VOL;
	m_iPortaSpeed		= 0;
	m_cArpeggio			= 0;
	m_cArpVar			= 0;
	m_iVibratoSpeed		= m_iVibratoPhase = 0;
	m_iTremoloSpeed		= m_iTremoloPhase = 0;
	m_iFinePitch		= 0x80;
	m_iFrequency		= 0;
	m_iVolSlide			= 0;
//	m_iLastFrequency	= 0xFFFF;
	m_bDelayEnabled		= false;

	m_iDefaultDuty		= 0;

	KillChannel();
}

void CChannelHandler::KillChannel()
{
	m_bEnabled			= false;
	m_iLastFrequency	= 0xFFFF;
	m_iOutVol			= 0x00;
	m_iPortaTo			= 0;

	theApp.RegisterKeyState(m_iChannelID, -1);

	ClearRegisters();
}

void CChannelHandler::SetNoteTable(unsigned int *NoteLookupTable)
{
	m_pNoteLookupTable = NoteLookupTable;
}

unsigned int CChannelHandler::TriggerNote(int Note)
{
	theApp.RegisterKeyState(m_iChannelID, Note);
	return m_pNoteLookupTable[Note];
}

void CChannelHandler::RunNote(int Octave, int Note)
{
	// Run the note and handle portamento
	int NewNote, NesFreq;
	// And note
	NewNote = MIDI_NOTE(Octave, Note);
	NesFreq = TriggerNote(NewNote);

	if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO) {
		if (m_iFrequency == 0)
			m_iFrequency = NesFreq;
		m_iPortaTo = NesFreq;
	}
	else
		m_iFrequency = NesFreq;
}

void CChannelHandler::SetupSlide(int Type, int EffParam)
{
	#define GET_SLIDE_SPEED(x) (((x & 0xF0) >> 3) + 1)

	m_iPortaSpeed = GET_SLIDE_SPEED(EffParam);
	m_iEffect = Type;

	if (Type == EF_SLIDE_UP)
		m_iNote = m_iNote + (EffParam & 0xF);
	else
		m_iNote = m_iNote - (EffParam & 0xF);

	m_iPortaTo = TriggerNote(m_iNote);
}

// Handle usual effects for all channels
bool CChannelHandler::CheckCommonEffects(unsigned char EffCmd, unsigned char EffParam)
{
	switch (EffCmd) {
		case EF_PORTAMENTO:
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTAMENTO;
			if (!EffParam)
				m_iPortaTo = 0;
			break;
		case EF_VIBRATO:
			m_iVibratoDepth = (EffParam & 0x0F) + 2;
			m_iVibratoSpeed = EffParam >> 4;
			if (!EffParam)
				m_iVibratoPhase = 0;
			break;
		case EF_TREMOLO:
			m_iTremoloDepth = (EffParam & 0x0F) + 2;
			m_iTremoloSpeed = EffParam >> 4;
			if (!EffParam)
				m_iTremoloPhase = 0;
			break;
		case EF_ARPEGGIO:
			m_cArpeggio = EffParam;
			m_iEffect = EF_ARPEGGIO;
			break;
		case EF_PITCH:
			m_iFinePitch = EffParam;
			break;
		case EF_PORTA_DOWN:
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTA_DOWN;
			break;
		case EF_PORTA_UP:
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTA_UP;
			break;
		case EF_VOLUME_SLIDE:
			m_iVolSlide = EffParam;
			break;
		default:
			return false;
	}
	
	return true;
}

bool CChannelHandler::HandleDelay(stChanNote *NoteData, int EffColumns)
{
	if (m_bDelayEnabled) {
		m_bDelayEnabled = false;
		PlayNote(m_DelayNote, m_iDelayEffColumns);
		delete m_DelayNote;
	}

	// Check delay
	for (int n = 0; n < EffColumns; n++) {
		if (NoteData->EffNumber[n] == EF_DELAY) {
			m_bDelayEnabled = true;
			m_cDelayCounter = NoteData->EffParam[n];
			m_DelayNote = new stChanNote;
			m_iDelayEffColumns = EffColumns;
			memcpy(m_DelayNote, NoteData, sizeof(stChanNote));
			m_DelayNote->EffNumber[n] = EF_NONE;
			m_DelayNote->EffParam[n] = 0;
			return true;
		}
	}

	return false;
}

void CChannelHandler::ProcessChannel()
{
	// Run channel effects for all channels
	//

	// Delay
	if (m_bDelayEnabled) {
		if (!m_cDelayCounter) {
			m_bDelayEnabled = false;
			PlayNote(m_DelayNote, m_iDelayEffColumns);
			delete m_DelayNote;
		}
		else
			m_cDelayCounter--;
	}

	if (!m_bEnabled)
		return;

	// Volume slide
	m_iVolume -= (m_iVolSlide & 0x0F);
	if (m_iVolume < 0)
		m_iVolume = 0;

	m_iVolume += (m_iVolSlide & 0xF0) >> 4;
	if (m_iVolume < 0)
		m_iVolume = MAX_VOL;

	// Vibrato and tremolo
	m_iVibratoPhase = (m_iVibratoPhase + m_iVibratoSpeed) & (VIBRATO_LENGTH - 1);
	m_iTremoloPhase = (m_iTremoloPhase + m_iTremoloSpeed) & (TREMOLO_LENGTH - 1);

	switch (m_iEffect) {
		case EF_ARPEGGIO:
			if (m_cArpeggio != 0) {
				switch (m_cArpVar) {
					case 0:
						m_iFrequency = TriggerNote(m_iNote);
						break;
					case 1:
						m_iFrequency = TriggerNote(m_iNote + (m_cArpeggio >> 4));
						if ((m_cArpeggio & 0x0F) == 0)
							m_cArpVar = 2;
						break;
					case 2:
						m_iFrequency = TriggerNote(m_iNote + (m_cArpeggio & 0x0F));
						break;
				}
				if (++m_cArpVar > 2)
					m_cArpVar = 0;
			}
			break;
		case EF_PORTAMENTO:
		case EF_SLIDE_UP:
		case EF_SLIDE_DOWN:
			// Automatic portamento
			if (m_iPortaSpeed > 0 && m_iPortaTo > 0) {
				if (m_iFrequency > m_iPortaTo) {
					m_iFrequency -= m_iPortaSpeed;
					if (m_iFrequency > 0x1000)	// it was negative
						m_iFrequency = 0x00;
					if (m_iFrequency < m_iPortaTo)
						m_iFrequency = m_iPortaTo;
				}
				else if (m_iFrequency < m_iPortaTo) {
					m_iFrequency += m_iPortaSpeed;
					if (m_iFrequency > m_iPortaTo)
						m_iFrequency = m_iPortaTo;
				}
			}
			break;
		case EF_PORTA_DOWN:
			m_iFrequency += m_iPortaSpeed;
			if (m_iFrequency > 0x7FF)
				m_iFrequency = 0x7FF;
			break;
		case EF_PORTA_UP:
			m_iFrequency -= m_iPortaSpeed;
			if (m_iFrequency > 0x8000)
				m_iFrequency = 0;
			break;
	}
}

// Used to see that everything is ok right before playing a note
bool CChannelHandler::CheckNote(stChanNote *pNoteData, int InstrumentType)
{
	CInstrument	*pInstrument;

	// No note data
	if (!pNoteData)
		return false;

	// Save instrument index
	if ((m_iInstrument = pNoteData->Instrument) == MAX_INSTRUMENTS)
		m_iInstrument = m_iLastInstrument;

	// Halt and release
	if (pNoteData->Note == HALT || pNoteData->Note == RELEASE) {
//		m_iVolume = 0x10;
//		KillChannel();
		// Allow incorrect instruments for note off
		return true;
	}

	pInstrument = m_pDocument->GetInstrument(m_iInstrument);
	
	// No instrument
	if (!pInstrument)
		return false;

	// Wrong type of instrument
	if (pInstrument->GetType() != InstrumentType)
		return false;

	return true;
}

int CChannelHandler::GetVibrato()
{
	// Vibrato offset
	int VibFreq;

//	VibFreq = sinf(float(m_iVibratoPhase) / 10.0f) * float(m_iVibratoDepth);

	
	VibFreq	= m_pcVibTable[m_iVibratoPhase] >> (0x8 - (m_iVibratoDepth >> 1));
	if ((m_iVibratoDepth & 1) == 0)
		VibFreq -= (VibFreq >> 1);

		
	return VibFreq;
}

int CChannelHandler::GetTremolo()
{
	// Tremolo offset
	int TremVol;

	TremVol	= (m_pcVibTable[m_iTremoloPhase] >> 4) >> (4 - (m_iTremoloDepth >> 1));

	if ((m_iTremoloDepth & 1) == 0)
		TremVol -= (TremVol >> 1);

	return TremVol;
}
