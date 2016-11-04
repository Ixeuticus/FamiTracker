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

//
// This will mix and resample the APU audio using blip-buffer
//
// Mixing of internal audio relies on Blargg's findings
//

#include <memory>
#include "APU/mixer.h"
#include "APU/apu.h"
#include "blip_buffer.h"

//#define LINEAR_MIXING

const double AMP_2A03 = 400.0;

CMixer::CMixer()
{
	m_pSampleBuffer = NULL;
}

CMixer::~CMixer()
{
}

inline double CMixer::CalcPin1(double Val1, double Val2)
{
	// Mix the output of APU audio pin 1: square
	//

	if ((Val1 + Val2) > 0)
		return 95.88 / ((8128.0 / (Val1 + Val2)) + 100.0);

	return 0;
}

inline double CMixer::CalcPin2(double Val1, double Val2, double Val3)
{
	// Mix the output of APU audio pin 2: triangle, noise and DPCM
	//

	if ((Val1 + Val2 + Val3) > 0)
		return 159.79 / ((1.0 / ((Val1 / 8227.0) + (Val2 / 12241.0) + (Val3 / 22638.0))) + 100.0);

	return 0;
}

bool CMixer::Init()
{
	memset(Channels, 0, sizeof(int) * CHANNELS);
	memset(ChannelLevels, 0, sizeof(int) * CHANNELS);
	memset(ChanLevelFallOff, 0, sizeof(int) * CHANNELS);
//	memset(ExternalSampleBuffer, 0, sizeof(int32) * 1000);

//	ExternalSamples = 0;

	return true;
}

void CMixer::Shutdown()
{
	if (m_pSampleBuffer)
		delete [] m_pSampleBuffer;

	m_pSampleBuffer = NULL;
}

void CMixer::ExternalSound(int Chip)
{
	ExternalChip = Chip;
}

void CMixer::UpdateSettings(int LowCut,	int HighCut, int HighDamp, int OverallVol)
{
	float fVol = float(OverallVol) / 100.0f;

	// Blip-buffer filtering
	BlipBuffer.bass_freq(LowCut);

	Synth2A03.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));
	SynthVRC6.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));
	SynthMMC5.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));
	SynthFDS.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));
	SynthN106.treble_eq(blip_eq_t(-HighDamp, HighCut, m_iSampleRate));

	Synth2A03.volume(1.0f * fVol);
	SynthVRC6.volume(3.98333f * fVol);
	SynthMMC5.volume(1.0f * fVol);
	SynthFDS.volume(1.0f);
	SynthN106.volume(1.0f);
}

void CMixer::MixSamples(blip_sample_t *pBuffer, uint32 Count)
{
	BlipBuffer.mix_samples(pBuffer, Count);
}

uint32 CMixer::GetMixSampleCount(int t)
{
	return BlipBuffer.count_samples(t);
}

bool CMixer::AllocateBuffer(unsigned int BufferLength, uint32 SampleRate, uint32 ClockRate, uint8 NrChannels)
{
	m_iSampleRate = SampleRate;

	BlipBuffer.sample_rate(SampleRate, (BufferLength * 1000) / SampleRate);
	BlipBuffer.clock_rate(ClockRate);

//	SetVolumeLevels();

	if (m_pSampleBuffer)
		delete [] m_pSampleBuffer;

	// Allocate a transfer buffer
	m_pSampleBuffer = new int32[BufferLength * NrChannels];

	return true;
}

void CMixer::SetClockRate(int Rate)
{
	// Change the clockrate
	BlipBuffer.clock_rate(Rate);
}

void CMixer::ClearBuffer()
{
	BlipBuffer.clear();
}

int CMixer::SamplesAvail()
{	
	return (int)BlipBuffer.samples_avail();
}

int CMixer::FinishBuffer(int t)
{
	BlipBuffer.end_frame(t);

	for (int i = 0; i < CHANNELS; i++) {
		if (ChanLevelFallOff[i] > 0)
			ChanLevelFallOff[i]--;
		else {
			ChanLevelFallOff[i] = 1;
			if (ChannelLevels[i] > 0) {
				if (i == 4) {
					ChannelLevels[i] -= 8;
					if (ChannelLevels[i] < 0)
						ChannelLevels[i] = 0;
				}
				else
					ChannelLevels[i] -= 1;
				if (ChannelLevels[i] < 0)
					ChannelLevels[i] = 0;
			}
		}
	}

	return BlipBuffer.samples_avail();
}

//
// Mixing
//

void CMixer::MixInternal(int Value, int Time)
{
	static double LastSum;
	double Sum, Delta;

#ifdef LINEAR_MIXING
	SumL = ((Channels[CHANID_SQUARE1].Left + Channels[CHANID_SQUARE2].Left) * 0.00752 + (0.00851 * Channels[CHANID_TRIANGLE].Left + 0.00494 * Channels[CHANID_NOISE].Left + 0.00335 * Channels[CHANID_DPCM].Left)) * InternalVol;
	SumR = ((Channels[CHANID_SQUARE1].Right + Channels[CHANID_SQUARE2].Right) *  0.00752 + (0.00851 * Channels[CHANID_TRIANGLE].Right + 0.00494 * Channels[CHANID_NOISE].Right + 0.00335 * Channels[CHANID_DPCM].Right)) * InternalVol;
#else
	Sum = CalcPin1(Channels[CHANID_SQUARE1], Channels[CHANID_SQUARE2]) + 
		  CalcPin2(Channels[CHANID_TRIANGLE], Channels[CHANID_NOISE], Channels[CHANID_DPCM]);
#endif

	Delta = (Sum - LastSum) * AMP_2A03;
	LastSum = Sum;

	if (Delta)
		Synth2A03.offset(Time, (int)Delta, &BlipBuffer);
}

void CMixer::MixN106(int Value, int Time)
{
	SynthN106.offset(Time, Value, &BlipBuffer);
}

void CMixer::MixFDS(int Value, int Time)
{
	SynthFDS.offset(Time, Value, &BlipBuffer);
}

void CMixer::MixVRC6(int Value, int Time)
{
	SynthVRC6.offset(Time, Value, &BlipBuffer);
}

void CMixer::MixMMC5(int Value, int Time)
{
	if (Value)
		SynthMMC5.offset(Time, Value, &BlipBuffer);
}

void CMixer::AddValue(int ChanID, int Chip, int Value, int FrameCycles)
{
	// Add a new channel value to mix
	//

//	Channels[ChanID].Left  = int32(float(Value) * Channels[ChanID].VolLeft);
//	Channels[ChanID].Right = int32(float(Value) * Channels[ChanID].VolRight);

	int AbsVol;

	AbsVol = abs(Value);

	if (ChanID == CHANID_VRC6_SAWTOOTH)
		AbsVol = (AbsVol * 3) / 4;
//		AbsVol *= 0.7;
	if (AbsVol >= ChannelLevels[ChanID]) {
		ChannelLevels[ChanID] = AbsVol;
		ChanLevelFallOff[ChanID] = 3;
	}

	Channels[ChanID] = Value;

	switch (Chip) {
		case SNDCHIP_NONE:
			MixInternal(Value, FrameCycles);
			break;
		case SNDCHIP_N106:
			MixN106(Value, FrameCycles);
			break;
		case SNDCHIP_FDS:
			MixFDS(Value, FrameCycles);
			break;
		case SNDCHIP_MMC5:
			MixMMC5(Value, FrameCycles);
			break;
		case SNDCHIP_VRC6:
			MixVRC6(Value, FrameCycles);
			break;
		case SNDCHIP_5B:
			break;
	}
}

int CMixer::ReadBuffer(int Size, void *Buffer, bool Stereo)
{
	return BlipBuffer.read_samples((blip_sample_t*)Buffer, Size);
}

int32 CMixer::GetChanOutput(uint8 Chan)
{
	int32 Ret = ChannelLevels[Chan];
//	m_bVolRead = true;
	return Ret;
}
