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

#pragma once

#include "WaveEditor.h"
#include "ModSequenceEditor.h"

// CInstrumentEditorFDS dialog

class CInstrumentEditorFDS : public CInstrumentEditPanel
{
	DECLARE_DYNAMIC(CInstrumentEditorFDS)

public:
	CInstrumentEditorFDS(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditorFDS();
	virtual int GetIDD() { return IDD; };
	virtual char *GetTitle() { return "Nintendo FDS"; };

	// Public
	virtual void SelectInstrument(int Instrument);

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_FDS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CInstrumentFDS		*m_pInstrument;
	CWaveEditor			*m_pWaveEditor;
	CModSequenceEditor	*m_pModSequenceEditor;

	void WaveChanged();
	void ReadWaveString();
	void ReadModString();

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnPresetSine();
	afx_msg void OnPresetTriangle();
	afx_msg void OnPresetSquare();
	afx_msg void OnPresetSawtooth();
	afx_msg void OnModPresetFlat();
	afx_msg void OnModPresetVibrato();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnModRateChange();
	afx_msg void OnModDepthChange();
	afx_msg void OnModDelayChange();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};