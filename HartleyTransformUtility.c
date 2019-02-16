#include "HartleyTransformUtility.h"
#include <math.h>
inline unsigned int LLIntegerLog2(unsigned int v)
{
	unsigned int i = 0;
	while (v>1)
	{
		++i;
		v >>= 1;
	}
	return i;
}
inline unsigned LLRevBits(unsigned int x, unsigned int bits)
{
	unsigned int y = 0;
	while (bits--)
	{
		y = (y + y) + (x & 1);
		x >>= 1;
	}
	return y;
}
void LLraisedCosTbl(double *dst, int n, int windowSizePadded, int overlapCount)
{
	const double twopi_over_n = 6.283185307179586476925286766559 / n;
	const double scalefac = 1.0 / windowSizePadded;
	double power = 1.0;
	if (overlapCount == 2)
		power = 0.5;
	for (int i = 0; i<n; ++i)
		dst[i] = scalefac * pow(0.5*(1.0 - cos(twopi_over_n * (i + 0.5))), power);
}
void LLsinHalfTbl(double *dst, int n)
{
	const double twopi_over_n = 6.283185307179586476925286766559 / n;
	for (int i = 0; i<n; ++i)
		dst[i] = sin(twopi_over_n * i);
}
void LLbitReversalTbl(unsigned *dst, int n)
{
	unsigned int bits = LLIntegerLog2(n);
	for (int i = 0; i<n; ++i)
		dst[i] = LLRevBits(i, bits);
}
void LLCreatePostWindow(double *dst, int windowSize, int windowSizePadded, int overlapCount)
{
	const double powerIntegrals[8] = { 1.0, 1.0 / 2.0, 3.0 / 8.0, 5.0 / 16.0, 35.0 / 128.0,
		63.0 / 256.0, 231.0 / 1024.0, 429.0 / 2048.0 };
	int power = 1;
	if (overlapCount == 2)
		power = 0;
	const double scalefac = (double)windowSizePadded * (powerIntegrals[1] / powerIntegrals[power + 1]);
	LLraisedCosTbl(dst, windowSize, windowSizePadded, overlapCount);
	for (int i = 0; i<windowSize; ++i)
		dst[i] *= scalefac;
}
void LLraisedCosTblFloat(float *dst, int n, int windowSizePadded, int overlapCount)
{
	const double twopi_over_n = 6.283185307179586476925286766559 / n;
	const double scalefac = 1.0 / windowSizePadded;
	float power = 1.0f;
	if (overlapCount == 2)
		power = 0.5f;
	for (int i = 0; i<n; ++i)
		dst[i] = (float)(scalefac * pow(0.5*(1.0 - cos(twopi_over_n * (i + 0.5))), power));
}
void LLsinHalfTblFloat(float *dst, int n)
{
	const double twopi_over_n = 6.283185307179586476925286766559 / n;
	for (int i = 0; i<n; ++i)
		dst[i] = (float)sin(twopi_over_n * i);
}
void LLCreatePostWindowFloat(float *dst, int windowSize, int windowSizePadded, int overlapCount)
{
	const float powerIntegrals[8] = { 1.0f, 1.0f / 2.0f, 3.0f / 8.0f, 5.0f / 16.0f, 35.0f / 128.0f,
		63.0f / 256.0f, 231.0f / 1024.0f, 429.0f / 2048.0f };
	int power = 1;
	if (overlapCount == 2)
		power = 0;
	const float scalefac = (float)windowSizePadded * (powerIntegrals[1] / powerIntegrals[power + 1]);
	LLraisedCosTblFloat(dst, windowSize, windowSizePadded, overlapCount);
	for (int i = 0; i<windowSize; ++i)
		dst[i] *= scalefac;
}
void LLdiscreteHartleyFloat(float *A, const int nPoints, const float *sinTab)
{
	int i, j, n, n2, theta_inc, nptDiv2;
	float alpha, beta;
	// FHT - stage 1 and 2 (2 and 4 points)
	for (i = 0; i<nPoints; i += 4)
	{
		const float	x0 = A[i];
		const float	x1 = A[i + 1];
		const float	x2 = A[i + 2];
		const float	x3 = A[i + 3];
		const float	y0 = x0 + x1;
		const float	y1 = x0 - x1;
		const float	y2 = x2 + x3;
		const float	y3 = x2 - x3;
		A[i] = y0 + y2;
		A[i + 2] = y0 - y2;
		A[i + 1] = y1 + y3;
		A[i + 3] = y1 - y3;
	}
	// FHT - stage 3 (8 points)
	for (i = 0; i<nPoints; i += 8)
	{
		alpha = A[i];
		beta = A[i + 4];
		A[i] = alpha + beta;
		A[i + 4] = alpha - beta;
		alpha = A[i + 2];
		beta = A[i + 6];
		A[i + 2] = alpha + beta;
		A[i + 6] = alpha - beta;
		alpha = A[i + 1];
		const float beta1 = 0.70710678118654752440084436210485f*(A[i + 5] + A[i + 7]);
		const float beta2 = 0.70710678118654752440084436210485f*(A[i + 5] - A[i + 7]);
		A[i + 1] = alpha + beta1;
		A[i + 5] = alpha - beta1;
		alpha = A[i + 3];
		A[i + 3] = alpha + beta2;
		A[i + 7] = alpha - beta2;
	}
	n = 16;
	n2 = 8;
	theta_inc = nPoints >> 4;
	nptDiv2 = nPoints >> 2;
	while (n <= nPoints)
	{
		for (i = 0; i<nPoints; i += n)
		{
			int theta = theta_inc;
			const int n4 = n2 >> 1;
			alpha = A[i];
			beta = A[i + n2];
			A[i] = alpha + beta;
			A[i + n2] = alpha - beta;
			alpha = A[i + n4];
			beta = A[i + n2 + n4];
			A[i + n4] = alpha + beta;
			A[i + n2 + n4] = alpha - beta;
			for (j = 1; j<n4; j++)
			{
				float	sinval = sinTab[theta];
				float	cosval = sinTab[theta + nptDiv2];
				float	alpha1 = A[i + j];
				float	alpha2 = A[i - j + n2];
				float	beta1 = A[i + j + n2] * cosval + A[i - j + n] * sinval;
				float	beta2 = A[i + j + n2] * sinval - A[i - j + n] * cosval;
				theta += theta_inc;
				A[i + j] = alpha1 + beta1;
				A[i + j + n2] = alpha1 - beta1;
				A[i - j + n2] = alpha2 + beta2;
				A[i - j + n] = alpha2 - beta2;
			}
		}
		n <<= 1;
		n2 <<= 1;
		theta_inc >>= 1;
	}
}
void LLdiscreteHartley(double *A, const int nPoints, const double *sinTab)
{
	int i, j, n, n2, theta_inc, nptDiv2;
	double alpha, beta;
	// FHT - stage 1 and 2 (2 and 4 points)
	for (i = 0; i<nPoints; i += 4)
	{
		const double	x0 = A[i];
		const double	x1 = A[i + 1];
		const double	x2 = A[i + 2];
		const double	x3 = A[i + 3];
		const double	y0 = x0 + x1;
		const double	y1 = x0 - x1;
		const double	y2 = x2 + x3;
		const double	y3 = x2 - x3;
		A[i] = y0 + y2;
		A[i + 2] = y0 - y2;
		A[i + 1] = y1 + y3;
		A[i + 3] = y1 - y3;
	}
	// FHT - stage 3 (8 points)
	for (i = 0; i<nPoints; i += 8)
	{
		alpha = A[i];
		beta = A[i + 4];
		A[i] = alpha + beta;
		A[i + 4] = alpha - beta;
		alpha = A[i + 2];
		beta = A[i + 6];
		A[i + 2] = alpha + beta;
		A[i + 6] = alpha - beta;
		alpha = A[i + 1];
		const double beta1 = 0.70710678118654752440084436210485*(A[i + 5] + A[i + 7]);
		const double beta2 = 0.70710678118654752440084436210485*(A[i + 5] - A[i + 7]);
		A[i + 1] = alpha + beta1;
		A[i + 5] = alpha - beta1;
		alpha = A[i + 3];
		A[i + 3] = alpha + beta2;
		A[i + 7] = alpha - beta2;
	}
	n = 16;
	n2 = 8;
	theta_inc = nPoints >> 4;
	nptDiv2 = nPoints >> 2;
	while (n <= nPoints)
	{
		for (i = 0; i<nPoints; i += n)
		{
			int theta = theta_inc;
			const int n4 = n2 >> 1;
			alpha = A[i];
			beta = A[i + n2];
			A[i] = alpha + beta;
			A[i + n2] = alpha - beta;
			alpha = A[i + n4];
			beta = A[i + n2 + n4];
			A[i + n4] = alpha + beta;
			A[i + n2 + n4] = alpha - beta;
			for (j = 1; j<n4; j++)
			{
				double	sinval = sinTab[theta];
				double	cosval = sinTab[theta + nptDiv2];
				double	alpha1 = A[i + j];
				double	alpha2 = A[i - j + n2];
				double	beta1 = A[i + j + n2] * cosval + A[i - j + n] * sinval;
				double	beta2 = A[i + j + n2] * sinval - A[i - j + n] * cosval;
				theta += theta_inc;
				A[i + j] = alpha1 + beta1;
				A[i + j + n2] = alpha1 - beta1;
				A[i - j + n2] = alpha2 + beta2;
				A[i - j + n] = alpha2 - beta2;
			}
		}
		n <<= 1;
		n2 <<= 1;
		theta_inc >>= 1;
	}
}