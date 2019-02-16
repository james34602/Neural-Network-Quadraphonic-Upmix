#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "../HartleyTransformUtility.h"
#include "STFTBinsTrainer.h"
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef M_PI
#define M_PI 3.141592653589793f
#endif
#ifndef M_2PI
#define M_2PI 6.283185307179586f
#endif
void PhaseRadarTrainerFree(STFTBinsTrainer *PMA)
{
	int i;
	free(PMA->mBitRev);
	free(PMA->mPreWindow);
	free(PMA->mSineTab);
	free(PMA->mInput[0]);
	free(PMA->mInput[1]);
	free(PMA->mTempLBuffer);
	free(PMA->mTempRBuffer);
	for (i = 0; i < PMA->vectorAllocatedElements; i++)
		free(PMA->trainingDataOut1[i]);
	free(PMA->trainingDataOut1);
}
#define VECTORCAPACITYINCREMENT 1048576*4
#define FEATURESPERBINS 4
void trainerAdd2Vector(STFTBinsTrainer *PMA)
{
	PMA->trainingDataCount++;
	if (PMA->trainingDataCount >= PMA->vectorAllocatedElements)
	{
		PMA->vectorAllocatedElements += VECTORCAPACITYINCREMENT;
		PMA->trainingDataOut1 = (float**)realloc(PMA->trainingDataOut1, PMA->vectorAllocatedElements * sizeof(float*));
		for (int i = PMA->trainingDataCount - 1; i < PMA->vectorAllocatedElements; i++)
			PMA->trainingDataOut1[i] = (float*)malloc(FEATURESPERBINS * sizeof(float));
	}
}
void LLPAMSProcessNPR(STFTBinsTrainer *PMA)
{
	if (PMA->frameSkCount < PMA->frame2Skip)
	{
		PMA->frameSkCount++;
		PMA->mInputSamplesNeeded = PMA->kOverlapSize;
		return;
	}
	memset(PMA->mTempLBuffer, 0, PMA->zeroPaddedLength * sizeof(float));
	memset(PMA->mTempRBuffer, 0, PMA->zeroPaddedLength * sizeof(float));
	int i;
	// copy to temporary buffer and FHT
	for (i = 0; i<PMA->kWindowLength; ++i)
	{
		const unsigned int k = (i + PMA->mInputPos) & PMA->kWindowLengthMinus1;
		const float w = PMA->mPreWindow[i];
		PMA->mTempLBuffer[PMA->mBitRev[i]] = PMA->mInput[0][k] * w;
		PMA->mTempRBuffer[PMA->mBitRev[i]] = PMA->mInput[1][k] * w;
	}
	LLdiscreteHartleyFloat(PMA->mTempLBuffer, PMA->zeroPaddedLength, PMA->mSineTab);
	LLdiscreteHartleyFloat(PMA->mTempRBuffer, PMA->zeroPaddedLength, PMA->mSineTab);
	// Data collection
	int symIdx;
	// DC
	float lR = PMA->mTempLBuffer[0];
	float rR = PMA->mTempRBuffer[0];
	trainerAdd2Vector(PMA);
	PMA->trainingDataOut1[PMA->trainingDataCount - 1][0] = lR;
	PMA->trainingDataOut1[PMA->trainingDataCount - 1][1] = 0.0f;
	PMA->trainingDataOut1[PMA->trainingDataCount - 1][2] = rR;
	PMA->trainingDataOut1[PMA->trainingDataCount - 1][3] = 0.0f;
	const float fs = 44100.0f;
	for (i = 1; i < PMA->zeroPaddedHalfLength; i++)
	{
		symIdx = PMA->zeroPaddedLength - i;
		float idxFrequency = (float)i * (fs / (float)PMA->kWindowLength);
		if (idxFrequency > 15400.0f)
			break;
		float lR = PMA->mTempLBuffer[i] + PMA->mTempLBuffer[symIdx];
		float lI = PMA->mTempLBuffer[i] - PMA->mTempLBuffer[symIdx];
		float rR = PMA->mTempRBuffer[i] + PMA->mTempRBuffer[symIdx];
		float rI = PMA->mTempRBuffer[i] - PMA->mTempRBuffer[symIdx];
		trainerAdd2Vector(PMA);
		PMA->trainingDataOut1[PMA->trainingDataCount - 1][0] = lR;
		PMA->trainingDataOut1[PMA->trainingDataCount - 1][1] = lI;
		PMA->trainingDataOut1[PMA->trainingDataCount - 1][2] = rR;
		PMA->trainingDataOut1[PMA->trainingDataCount - 1][3] = rI;
	}
	PMA->mInputSamplesNeeded = PMA->kOverlapSize;
}
int PhaseRadarTrainerInit(STFTBinsTrainer *PMA, int quality, int overlap, int frameToSkipAtStart)
{
	memset(PMA, 0, sizeof(STFTBinsTrainer));
	PMA->frame2Skip = frameToSkipAtStart;
	int i;
	if (overlap < 2 || overlap > 8)
		overlap = 2;
	PMA->kOverlapCount = overlap;
	int zeroPaddingRatio = 1;
	if (!quality)
		PMA->kWindowLength = 1024;
	else if (quality == 1)
		PMA->kWindowLength = 2048;
	else
		PMA->kWindowLength = 4096;
	if (!zeroPaddingRatio)
		zeroPaddingRatio = 1;
	PMA->zeroPaddedLength = PMA->kWindowLength * zeroPaddingRatio;
	if (PMA->kWindowLength % 2)
	{
		PMA->kHalfWindow = (PMA->kWindowLength + 1) / 2;
		PMA->zeroPaddedHalfLength = (PMA->kWindowLength * zeroPaddingRatio + 1) / 2;
	}
	else
	{
		PMA->kHalfWindow = (PMA->kWindowLength / 2) + 1;
		PMA->zeroPaddedHalfLength = ((PMA->kWindowLength * zeroPaddingRatio) / 2) + 1;
	}
	PMA->kWindowLengthMinus1 = PMA->kWindowLength - 1;
	PMA->zeroPaddedLengthMinus1 = PMA->zeroPaddedLength - 1;
	PMA->kOverlapSize = PMA->kWindowLength / PMA->kOverlapCount;
	int bufferSize = PMA->zeroPaddedLength * sizeof(unsigned int);
	PMA->mBitRev = (unsigned int*)malloc(PMA->zeroPaddedLength * sizeof(unsigned int));
	bufferSize = PMA->zeroPaddedLength * sizeof(float);
	PMA->mSineTab = (float*)malloc(bufferSize);
	PMA->mInput[0] = (float*)malloc(bufferSize);
	PMA->mInput[1] = (float*)malloc(bufferSize);
	memset(PMA->mInput[0], 0, bufferSize);
	memset(PMA->mInput[1], 0, bufferSize);
	PMA->mTempLBuffer = (float*)malloc(bufferSize);
	PMA->mTempRBuffer = (float*)malloc(bufferSize);
	bufferSize = PMA->kWindowLength * sizeof(float);
	PMA->mPreWindow = (float*)malloc(bufferSize);
	LLbitReversalTbl(PMA->mBitRev, PMA->zeroPaddedLength);
	LLsinHalfTblFloat(PMA->mSineTab, PMA->zeroPaddedLength);
	PMA->mInputSamplesNeeded = PMA->kOverlapSize;
	PMA->mInputPos = 0;
	LLraisedCosTblFloat(PMA->mPreWindow, PMA->kWindowLength, PMA->zeroPaddedLength, PMA->kOverlapCount);
	for (i = 0; i<PMA->kWindowLength; ++i)
		PMA->mPreWindow[i] *= 0.5f * (2.0f / (float)PMA->kOverlapCount);
	return 1;
}
void PhaseRadarTrainerProcessSamples(STFTBinsTrainer *PMA, float *inLeft, float *inRight, int inSampleCount)
{
	while (inSampleCount > 0) {
		int copyCount = min((int)PMA->mInputSamplesNeeded, inSampleCount);
		float *sampDL = &PMA->mInput[0][PMA->mInputPos];
		float *sampDR = &PMA->mInput[1][PMA->mInputPos];
		float *max = inLeft + copyCount;
		while (inLeft < max)
		{
			*sampDL = *inLeft;
			*sampDR = *inRight;
			inLeft += 1;
			inRight += 1;
			sampDL += 1;
			sampDR += 1;
		}
		inSampleCount -= copyCount;
		PMA->mInputPos = (PMA->mInputPos + copyCount) & PMA->kWindowLengthMinus1;
		PMA->mInputSamplesNeeded -= copyCount;
		if (PMA->mInputSamplesNeeded == 0)
			LLPAMSProcessNPR(PMA);
	}
}