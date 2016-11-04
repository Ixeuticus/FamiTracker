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

#include "stdafx.h"
#include "SWLogo.h"
#include "resource.h"

void CSWLogo::Activate()
{
}

void CSWLogo::Deactivate()
{
}

void CSWLogo::SetSampleRate(int SampleRate)
{
}

void CSWLogo::SetSampleData(int *pSamples, unsigned int iCount)
{
}

void CSWLogo::Draw(CDC *pDC, bool bMessage)
{
	if (!bMessage)
		return;

	CBitmap Bmp, *OldBmp;
	CDC		BitmapDC;

	Bmp.LoadBitmap(IDB_SAMPLEBG);
	BitmapDC.CreateCompatibleDC(pDC);
	OldBmp = BitmapDC.SelectObject(&Bmp);

	pDC->BitBlt(0, 0, WIN_WIDTH, WIN_HEIGHT, &BitmapDC, 0, 0, SRCCOPY);

	BitmapDC.SelectObject(OldBmp);
}
