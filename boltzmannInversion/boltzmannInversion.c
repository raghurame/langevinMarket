#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define N_BINS_DIST 25

typedef struct marketValues
{
	float open, high, low, close, adjClose;
	float logOpen, logHigh, logLow, logClose, logAdjClose;
	int volume;
} MARKET_VALUES;

typedef struct boundary
{
	float lo, hi;
} BOUNDS;

typedef struct popDistribution
{
	float lo, hi;
	int count;
	float negLogCount;
} POP_DIST;

int countLines (FILE *input)
{
	int nLines = 0;
	char lineString[3000];

	while (fgets (lineString, 3000, input) != NULL)
	{
		nLines++;
	}

	nLines -= 1;
	rewind (input);

	return nLines;
}

MARKET_VALUES *readValues (FILE *input, int nDataPoints, MARKET_VALUES *ticker)
{
	char lineString[3000];
	int currentLine = 0;

	rewind (input);

	fgets (lineString, 3000, input);
	while (fgets (lineString, 3000, input) != NULL)
	{
		sscanf (lineString, "%f %f %f %f %f %d\n", &ticker[currentLine].open, &ticker[currentLine].high, &ticker[currentLine].low, &ticker[currentLine].close, &ticker[currentLine].adjClose, &ticker[currentLine].volume);
		currentLine++;
	}

	for (int i = 0; i < currentLine; ++i)
	{
		ticker[i].logOpen = -logf (ticker[i].open);
		ticker[i].logHigh = -logf (ticker[i].high);
		ticker[i].logLow = -logf (ticker[i].low);
		ticker[i].logClose = -logf (ticker[i].close);
		ticker[i].logAdjClose = -logf (ticker[i].adjClose);
	}

	return ticker;
}

BOUNDS findBounds (MARKET_VALUES *ticker, int nDataPoints, BOUNDS tickerBounds)
{
	tickerBounds.lo = ticker[0].close;
	tickerBounds.hi = ticker[0].close;

	for (int i = 1; i < nDataPoints; ++i)
	{
		if (ticker[i].close < tickerBounds.lo)
		{
			tickerBounds.lo = ticker[i].close;
		}
		else if (ticker[i].close > tickerBounds.hi)
		{
			tickerBounds.hi = ticker[i].close;
		}
	}

	printf("lo: %f; hi: %f\n", tickerBounds.lo, tickerBounds.hi);

	return tickerBounds;
}

BOUNDS findBoundsLog (MARKET_VALUES *ticker, int nDataPoints, BOUNDS tickerBounds)
{
	tickerBounds.lo = ticker[0].logClose;
	tickerBounds.hi = ticker[0].logClose;

	for (int i = 1; i < nDataPoints; ++i)
	{
		if (ticker[i].logClose < tickerBounds.lo)
		{
			tickerBounds.lo = ticker[i].logClose;
		}
		else if (ticker[i].logClose > tickerBounds.hi)
		{
			tickerBounds.hi = ticker[i].logClose;
		}
	}

	printf("lo: %f; hi: %f\n", tickerBounds.lo, tickerBounds.hi);

	return tickerBounds;
}

MARKET_VALUES *rescale (MARKET_VALUES *ticker, int nDataPoints, BOUNDS tickerBounds)
{
	tickerBounds = findBounds (ticker, nDataPoints, tickerBounds);

	for (int i = 0; i < nDataPoints; ++i)
	{
		ticker[i].logClose -= tickerBounds.lo;
	}

	tickerBounds = findBounds (ticker, nDataPoints, tickerBounds);

	for (int i = 0; i < nDataPoints; ++i)
	{
		ticker[i].logClose /= tickerBounds.hi;
	}

	return ticker;
}

POP_DIST *computeDistribution (MARKET_VALUES *ticker, int nDataPoints, POP_DIST *tickerDist, BOUNDS tickerBounds)
{
	tickerBounds = findBounds (ticker, nDataPoints, tickerBounds);
	float binWidth = (tickerBounds.hi - tickerBounds.lo) / N_BINS_DIST;

	tickerDist[0].lo = tickerBounds.lo;
	tickerDist[0].hi = tickerBounds.lo + binWidth;
	tickerDist[0].count = 0;

	for (int i = 1; i < N_BINS_DIST; ++i)
	{
		tickerDist[i].lo = tickerDist[i - 1].hi;
		tickerDist[i].hi = tickerDist[i].lo + binWidth;
		tickerDist[i].count = 0;
	}

	for (int i = 0; i < nDataPoints; ++i)
	{
		for (int j = 0; j < N_BINS_DIST; ++j)
		{
			if (ticker[i].close < tickerDist[j].hi && ticker[i].close > tickerDist[j].lo)
			{
				tickerDist[j].count++;
			}
		}
	}

	for (int i = 0; i < N_BINS_DIST; ++i)
	{
		tickerDist[i].negLogCount = -logf ((float)tickerDist[i].count);
	}

	return tickerDist;
}

void printPotEnergy (POP_DIST *EWWDist)
{
	FILE *outputEnergy;
	outputEnergy = fopen ("energy.output", "w");

	for (int i = 0; i < N_BINS_DIST; ++i)
	{
		fprintf(outputEnergy, "%f %f %d %f\n", EWWDist[i].lo, EWWDist[i].hi, EWWDist[i].count, EWWDist[i].negLogCount);
	}

	fclose (outputEnergy);
}

int main(int argc, char const *argv[])
{
	FILE *input;
	input = fopen ("EWW.csv", "r");

	MARKET_VALUES *EWW;
	int nEWW = countLines (input);
	EWW = (MARKET_VALUES *) malloc (nEWW * sizeof (MARKET_VALUES));

	EWW = readValues (input, nEWW, EWW);

	POP_DIST *EWWDist;
	EWWDist = (POP_DIST *) malloc (N_BINS_DIST * sizeof (POP_DIST));

	BOUNDS EWWBounds;
	EWWDist = computeDistribution (EWW, nEWW, EWWDist, EWWBounds);

	// EWWBounds = findBounds (EWW, nEWW, EWWBounds);
	// EWW = rescale (EWW, nEWW, EWWBounds);

	printPotEnergy (EWWDist);

	fclose (input);
	return 0;
}