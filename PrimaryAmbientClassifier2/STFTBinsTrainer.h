typedef struct PMD
{
	int kWindowLength, zeroPaddedLength;
	int kWindowLengthMinus1, zeroPaddedLengthMinus1;
	int kOverlapCount;
	int kOverlapSize;
	int kHalfWindow, zeroPaddedHalfLength;
	unsigned int mInputSamplesNeeded;
	unsigned int mInputPos;
	unsigned int *mBitRev;
	float 	*mPreWindow;
	float 	*mSineTab;
	float 	*mInput[2];
	float 	*mTempLBuffer;
	float 	*mTempRBuffer;
	int vectorAllocatedElements;
	int trainingDataCount;
	float **trainingDataOut1;
	int frame2Skip, frameSkCount;
	int zeroEnergyCount;
} STFTBinsTrainer;
int PhaseRadarTrainerInit(STFTBinsTrainer *PMA, int quality, int overlap, int frameToSkipAtStart);
void PhaseRadarTrainerFree(STFTBinsTrainer *PMA);
void PhaseRadarTrainerProcessSamples(STFTBinsTrainer *PMA, float *inLeft, float *inRight, int inSampleCount);