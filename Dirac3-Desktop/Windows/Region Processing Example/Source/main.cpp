
/*
	ABSTRACT:
	This example demonstrates how to use Dirac to process regions within a vocal recording
	by different speed/pitch settings
 
	"main.cpp" Example Source File - Disclaimer:
 
	IMPORTANT:  This file and its contents are subject to the terms set forth in the 
	"License Agreement.txt" file that accompanies this distribution.
 
	Copyright © 2012 Stephan M. Bernsee, http://www.dspdimension.com. All Rights Reserved
 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "MiniAiff.h"
#include "Dirac.h"


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma mark ---- Callback and structs ----


typedef struct {
	unsigned long sStartFrameInFile;			/* start frame number in input file */
	long sNumFrames;							/* number of frames in input file region (before time stretching) */
	long double sTimeStretchFactor;				/* time stretching factor for this region */
	long double sPitchShiftFactor;				/* pitch shift factor for this region */
} SoundFileRegion;


// This is the struct that holds state variables that our callback needs. In your program
// you will want to replace this by a pointer to "this" in order to access your instance methods
// and variables
typedef struct {
	long sReadPosition;
	long sNumChannels;
	char *sInFileName;
} userDataStruct;


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
 This is the callback function that supplies data from the input stream/file whenever needed.
 It should be implemented in your software by a routine that gets data from the input/buffers.
 The read requests are *always* consecutive, ie. the routine will never have to supply data out
 of order.
 */
long myReadData(float **chdata, long numFrames, void *userData)
{	
	// The userData parameter can be used to pass information about the caller (for example, "this") to
	// the callback so it can manage its audio streams.
	if (!chdata)	return 0;
	
	userDataStruct *state = (userDataStruct*)userData;
	if (!state)	return 0;
	
	long res = mAiffReadData(state->sInFileName, chdata, state->sReadPosition, numFrames, state->sNumChannels);
	state->sReadPosition += numFrames;
	
	return res;	
	
}

#pragma mark ---- Main program ----


inline void fadeBlock(float **audio, long numChannels, long numFrames)
{
	long margin = 1024;		/* change as you see fit */
	long offset = numFrames-margin;
	if (numFrames < 2*margin) return;
	for (long c = 0; c < numChannels; c++) {
		for (long s = 0; s < margin; s++) {
			float ml = (float)s/(float)margin;
			audio[c][s]			*= ml;
			audio[c][s+offset]	*= 1.-ml;
		}
	}
}

inline void reverseBlock(float **audio, long numChannels, long numFrames) 
{
	for (long c = 0; c < numChannels; c++) {
		for (long s = 0; s < numFrames/2; s++) {
			float tmp = audio[c][s];
			audio[c][s] = audio[c][numFrames-s-1];
			audio[c][numFrames-s-1] = tmp;
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int main()
{
	
	// Create and init output file "out.aif"
#ifdef __APPLE__
	char infileName[]="voice.aif";
#else
	char infileName[]="../../voice.aif";
#endif
	char oufileName[]="out.aif";

	long numChannels = 1;							// DIRAC LE allows mono only, PRO can do stereo (and more) as well
	float sr = mAiffGetSampleRate(infileName);		// get sample rate from our input file
	if (sr <= 0.) {
		printf("ERROR: File not found\n");
		exit(-1);
	}
	
	// We stuff all our programs' state variables that we need to access in order to read from the file in a struct
	// You will normally pass your instance pointer "this" as userData, but since this is not a class we cannot do this here
	userDataStruct state;
	state.sNumChannels = numChannels;
	state.sReadPosition = 0;
	state.sInFileName = new char[strlen(infileName)+1];
	memmove(state.sInFileName, infileName, (strlen(infileName)+1)*sizeof(char));
	

    // First we set up DIRAC to process numChannels of audio
	void *dirac = DiracCreate(kDiracLambda1, kDiracQualityBest, numChannels, sr, &myReadData, (void*)&state);
	if (!dirac) {
		printf("!! ERROR !!\n\n\tCould not create DIRAC instance\n\tCheck number of channels and sample rate!\n");
		exit(-1);
	}
	
	// Initialize our output file
	mAiffInitFile(oufileName, sr /* sample rate */, 16 /* bits */, numChannels);

	// these are arbitrary regions in the file */
#define NUM_PARTS	17
	SoundFileRegion regions[NUM_PARTS] = {	{0,			4257,	1.0, 1.0},
											{4257,		10741,	1.0, 1.0},
											{14999,		-20471,	1.0, 1.5},	/* negative length means reverse playback direction */
											{35470,		-12161,	1.0, 1.0},
											{47631,		9323,	2.0, 1.0},
											{56954,		18647,	1.0, 1.5},
											{75601,		9121,	2.0, 1.0},
											{84722,		22903,	1.0, 1.0},
											{107625,	19863,	1.0, 1.0},
											{107625,	19863,	1.0, pow(2., 1./12.)}, /* repeat and change pitch */
											{107625,	19863,	1.0, pow(2., 2./12.)}, /* repeat and change pitch */
											{127488,	39117,	1.0, .5},
											{166605,	-11553,	2.0, 1.0},
											{178158,	20269,	1.0, 1.0},
											{198427,	17430,	2.0, 1.0},
											{215857,	23309,	1.0, 1.5},
											{239166,	35266,	1.0, 1.0}	};
										
	
	for (int i=0; i<NUM_PARTS; i++)
	{
		
		printf("Processing region \t%d: {start: %d, length: %d} \twith time stretch %1.2f and pitch shift %1.2f\n", 
			   i, regions[i].sStartFrameInFile, regions[i].sNumFrames, (float)regions[i].sTimeStretchFactor, (float)regions[i].sPitchShiftFactor);
		
		/* determine the region length (output) in frames by multiplying input region length with time stretch factor */
		long numOutFrames = regions[i].sTimeStretchFactor * abs(regions[i].sNumFrames);
		
		/* set Dirac properties according to desired settings */
		DiracSetProperty(kDiracPropertyTimeFactor, regions[i].sTimeStretchFactor, dirac);
		DiracSetProperty(kDiracPropertyPitchFactor, regions[i].sPitchShiftFactor, dirac);
		DiracSetProperty(kDiracPropertyFormantFactor, 1./regions[i].sPitchShiftFactor, dirac);	/* optional */
		
		/* allocate buffer to hold output frames */
		float **audio = mAiffAllocateAudioBuffer(numChannels, numOutFrames);
		
		/* set read position to begin of region */
		state.sReadPosition = regions[i].sStartFrameInFile;
		
		/* process region */
		DiracProcess(audio, numOutFrames, dirac);
		
		/* fade region to prevent glitches */
		if (regions[i].sNumFrames < 0)
			reverseBlock(audio, numChannels, numOutFrames);
		
		fadeBlock(audio, numChannels, numOutFrames);
		
		/* write region to file */
		mAiffWriteData(oufileName, audio, numOutFrames, numChannels);
		
		/*  get rid of audio buffer */
		mAiffDeallocateAudioBuffer(audio, numChannels);
		
		/* reset Dirac instance for next region */
		DiracReset(false, dirac);
	}
	
	// destroy DIRAC instance
	DiracDestroy( dirac );
	
	// free our file name
	delete[] state.sInFileName;
	
    // Done!
    printf("\nDone!\n");
    putchar(7);
	
	/* Open audio file via system call */
#ifdef __APPLE__
	system("open out.aif");
#elif defined _WIN32
	system("start out.aif");
#elif defined __unix__
	system("xdg-open out.aif");
#endif
	
	return 0;
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
