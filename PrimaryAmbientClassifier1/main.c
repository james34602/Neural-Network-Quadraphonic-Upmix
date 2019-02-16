#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
//  Windows
#ifdef _WIN32
#include <Windows.h>
////////////////////////////////////////////////////////////////////
// Performance timer
double get_wall_time()
{
	LARGE_INTEGER time, freq;
	if (!QueryPerformanceFrequency(&freq))
		return 0;
	if (!QueryPerformanceCounter(&time))
		return 0;
	return (double)time.QuadPart / freq.QuadPart;
}
double get_cpu_time()
{
	FILETIME a, b, c, d;
	if (GetProcessTimes(GetCurrentProcess(), &a, &b, &c, &d) != 0)
		return (double)(d.dwLowDateTime | ((unsigned long long)d.dwHighDateTime << 32)) * 0.0000001;
	else
		return 0;
}
#else
#include <time.h>
#include <sys/time.h>
double get_wall_time()
{
	struct timeval time;
	if (gettimeofday(&time, NULL))
		return 0;
	return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
double get_cpu_time()
{
	return (double)clock() / CLOCKS_PER_SEC;
}
#endif
////////////////////////////////////////////////////////////////////
char *inputString(FILE* fp, size_t size) {
	//The size is extended by the input with the value of the provisional
	char *str;
	int ch;
	size_t len = 0;
	str = realloc(NULL, sizeof(char)*size);//size is start size
	if (!str)return str;
	while (EOF != (ch = fgetc(fp)) && ch != '\n') {
		str[len++] = ch;
		if (len == size) {
			str = realloc(str, sizeof(char)*(size += 16));
			if (!str)return str;
		}
	}
	str[len++] = '\0';
	return realloc(str, sizeof(char)*len);
}
char *basename(char const *path)
{
#ifdef _MSC_VER
	char *s = strrchr(path, '\\');
#else
	char *s = strrchr(path, '/');
#endif
	if (!s)
		return strdup(path);
	else
		return strdup(s + 1);
}
#include "sndfile.h"
void channel_joinFloat(float **chan_buffers, unsigned int num_channels, float *buffer, unsigned int num_frames)
{
	unsigned int i, samples = num_frames * num_channels;
	for (i = 0; i < samples; i++)
		buffer[i] = chan_buffers[i % num_channels][i / num_channels];
}
void channel_join(double **chan_buffers, unsigned int num_channels, double *buffer, unsigned int num_frames)
{
	unsigned int i, samples = num_frames * num_channels;
	for (i = 0; i < samples; i++)
		buffer[i] = chan_buffers[i % num_channels][i / num_channels];
}
void channel_split(double *buffer, unsigned int num_frames, double **chan_buffers, unsigned int num_channels)
{
	unsigned int i, samples = num_frames * num_channels;
	for (i = 0; i < samples; i++)
		chan_buffers[i % num_channels][i / num_channels] = buffer[i];
}
void channel_splitFloat(float *buffer, unsigned int num_frames, float **chan_buffers, unsigned int num_channels)
{
	unsigned int i, samples = num_frames * num_channels;
	for (i = 0; i < samples; i++)
		chan_buffers[i % num_channels][i / num_channels] = buffer[i];
}
void normalise(float *buffer, int num_samps)
{
	int i;
	float max = 0.0f;
	for (i = 0; i < num_samps; i++)
		max = fabsf(buffer[i]) < max ? max : fabsf(buffer[i]);
	max = 1.0f / max;
	for (i = 0; i < num_samps; i++)
		buffer[i] *= max;
}
#include "STFTBinsTrainer.h"
#include "kann.h"
#define BLOCK_SIZE 2048
#define FILENAME "Mixture4Ch.wav"
int main()
{
	int i;
	SF_INFO sfAudioInfo;
	memset(&sfAudioInfo, 0, sizeof(SF_INFO));
	char *path = 0;
	SNDFILE *sfFile = sf_open(FILENAME, SFM_READ, &sfAudioInfo);
	if (!sfFile)
	{
		printf("Invalid file / File not found");
		free(path);
		// Open failed or invalid audio file
		return -1;
	}
	// Sanity check
	if (sfAudioInfo.channels < 1)
	{
		printf("Invalid audio channels count");
		free(path);
		sf_close(sfFile);
		return -1;
	}
	if ((sfAudioInfo.samplerate <= 0) || (sfAudioInfo.frames <= 0))
	{
		// Negative sampling rate or empty data ?
		printf("Invalid audio sample rate / frame count");
		free(path);
		sf_close(sfFile);
		return -1;
	}
	char *filename = basename(FILENAME);
	STFTBinsTrainer stft1, stft2;
	PhaseRadarTrainerInit(&stft1, 1, 2, 5);
	PhaseRadarTrainerInit(&stft2, 1, 2, 5);
	float buf[BLOCK_SIZE];
	sf_count_t frames;
	int k, m, readcount;
	frames = BLOCK_SIZE / sfAudioInfo.channels;
	float *channelBuffer[4];
	for (i = 0; i < 4; i++)
		channelBuffer[i] = (float*)malloc(frames * sizeof(float));
	printf("[Info] Processing...\n");
	while ((readcount = sf_readf_float(sfFile, buf, frames)) > 0)
	{
		for (k = 0; k < readcount; k++)
			for (m = 0; m < sfAudioInfo.channels; m++)
				channelBuffer[m][k] = buf[k * sfAudioInfo.channels + m];
		PhaseRadarTrainerProcessSamples(&stft1, channelBuffer[0], channelBuffer[1], readcount);
		PhaseRadarTrainerProcessSamples(&stft2, channelBuffer[2], channelBuffer[3], readcount);
	}
	sf_close(sfFile);
	for (i = 0; i < 4; i++)
		free(channelBuffer[i]);
	// Data preparation, concat and label data
	int featuresPerVectorIn = 4;
	int featuresPerVectorOut = 1;
	int NfeatureVectors = stft1.trainingDataCount + stft2.trainingDataCount; // Feature vectors count
	float **dataX = (float**)malloc(NfeatureVectors * sizeof(float*));
	float **labels = (float**)malloc(NfeatureVectors * sizeof(float*));
	for (i = 0; i < NfeatureVectors; i++)
	{
		dataX[i] = (float*)malloc(featuresPerVectorIn * sizeof(float));
		labels[i] = (float*)malloc(featuresPerVectorOut * sizeof(float));
	}
	for (i = 0; i < stft1.trainingDataCount; i++)
	{
		for (int j = 0; j < featuresPerVectorIn; j++)
			dataX[i][j] = stft1.trainingDataOut1[i][j];
		labels[i][0] = 1.0f;
	}
	PhaseRadarTrainerFree(&stft1);
	for (i ; i < NfeatureVectors; i++)
	{
		for (int j = 0; j < featuresPerVectorIn; j++)
			dataX[i][j] = stft2.trainingDataOut1[i - stft1.trainingDataCount][j];
		labels[i][0] = 0.0f;
	}
	PhaseRadarTrainerFree(&stft2);
	// Neural network section
	kann_srand(1);
	kad_node_t *t;
	t = kann_layer_input(featuresPerVectorIn); // Input nodes
	t = kad_relu(kann_layer_dense(t, 16));
	t = kad_relu(kann_layer_dense(t, 8));
	t = kad_relu(kann_layer_dense(t, 5));
	t = kad_relu(kann_layer_dense(t, 3));
	kann_t *ann = kann_new(kann_layer_cost(t, featuresPerVectorOut, KANN_C_MSE), 0);
	int mini_size = 256; // 64 mini size
	int max_epoch = 100;
	float learningRate = 0.00005f;
	int max_drop_streak = 200;
	kann_mt(ann, 8, mini_size);
	kann_train_fnn1(ann, learningRate, mini_size, max_epoch, max_drop_streak, 0.1f, NfeatureVectors, dataX, labels);
	kann_delete(ann);
	for (i = 0; i < NfeatureVectors; i++)
	{
		free(dataX[i]);
		free(labels[i]);
	}
	free(dataX);
	free(labels);
	free(filename);
	return 0;
}