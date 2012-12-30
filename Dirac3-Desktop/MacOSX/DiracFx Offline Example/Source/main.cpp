
/*
 
 ABSTRACT:
 Demonstrates how to use the high speed/low resolution DiracFx algorithm for offline processing
 
 
 DiracFx "main.cpp" Example Source File - Disclaimer:
 Copyright © 2005-2012 Stephan M. Bernsee, http://www.dspdimension.com. All Rights Reserved
 
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "MiniAiff.h"
#include "Dirac.h"

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
double gExecTimeTotal = 0.;
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


int main()
{
	
	/* ***************** SET UP ******************* */
	/* First we set our time an pitch manipulation values */
    float time      = 1.15;				// 115% length
	float pitch     = pow(2., 3./12.);	// pitch shift (3 semitones)
	
	/* set up our input/output files and retrieve */
	/* sample rate, length and number of channels */
	/* Note that DiracLE supports only one channel! */
	// Create and init output file "out.aif"
#ifdef __APPLE__
	char infileName[]="test.aif";
#else
	char infileName[]="../../test.aif";
#endif
	char oufileName[]="out.aif";
	
	/* get file info */
	long numChannels	= 1;
	float sampleRate	= mAiffGetSampleRate(infileName);
	unsigned long inputNumFrames = mAiffGetNumberOfFrames(infileName);
	if (sampleRate <= 0.f) {printf("Error opening input file\n"); exit(-1);}
	
	/* Instantiate our DiracFx object */
	void *diracFx = DiracFxCreate(kDiracQualityGood, sampleRate, numChannels);
	if (!diracFx) {
		printf("!! ERROR !!\n\n\tCould not create DiracFx instance\n");
		exit(-1);
	}
	
	/* Initialize our output file */
	mAiffInitFile(oufileName, sampleRate /* sample rate */, 16 /* bits */, numChannels);
	
	/* Print version info to stdout */
    printf("Running DIRAC version %s\nStarting processing\n", DiracVersion());
	
	/* Keep track of how many frames we've already processed */
    unsigned long inputFramesProcessed = 0;
	
	/* This is the amount of frames per read operation */
	/* It is an arbitrary number of frames. Change as you see fit */
    long numFrames = 8192;
	
	/* Allocate buffer for output */
	float **audioIn = mAiffAllocateAudioBuffer(numChannels, numFrames);
	float **audioOut = mAiffAllocateAudioBuffer(numChannels, 
												DiracFxMaxOutputBufferFramesRequired(time, pitch, numFrames));
	
	/* ***************** HANDLE LATENCY ******************* */
	/* Get latency estimate */
	long latencyFrames = DiracFxLatencyFrames(sampleRate);
	
	/* Establish a separate buffer to account for latency. */
	/* We could do this with our processing buffers but we're lazy. */
	float **latencyBufferIn = mAiffAllocateAudioBuffer(numChannels, latencyFrames);
	float **latencyBufferOut = mAiffAllocateAudioBuffer(numChannels, 
														DiracFxMaxOutputBufferFramesRequired(time, pitch, latencyFrames));
	
	/* Read the first chunk from the file */
	mAiffReadData(infileName, latencyBufferIn, 0, latencyFrames, numChannels);
	
	/* The first block is processed manually to account for the latency */
	DiracFxProcessFloat(time, pitch, latencyBufferIn,
						latencyBufferOut, latencyFrames, diracFx);
	
	/* The first block is processed manually to account for the latency */
	/* but increase our read position */
	inputFramesProcessed += latencyFrames;
	
	/* ***************** MAIN PROCESSING LOOP STARTS HERE ******************* */
	/* for speed measurement */
	double bavg = 0;
	for(;;) {
		
		/* read chunk at position inputFramesProcessed */
		mAiffReadData(infileName, audioIn, inputFramesProcessed, numFrames, 
					  numChannels);
		
		DiracStartClock();								// ............................. start timer ..........................................
		
		/* Call the process function with current time and pitch settings */
		/* Returns: the number of frames in audioOut */
        long ret = DiracFxProcessFloat(time, pitch, audioIn, audioOut, 
									   numFrames, diracFx);
		
		bavg += (numFrames/sampleRate);
		gExecTimeTotal += DiracClockTimeSeconds();		// ............................. stop timer ..........................................
		
		/* Write data to the output file */
        mAiffWriteData(oufileName, audioOut, ret, numChannels);
		
		/* Increase our input position */
        inputFramesProcessed += numFrames;
		
		/* As soon as we've read enough frames we exit the main loop */
		if (inputFramesProcessed >= inputNumFrames + latencyFrames) 
			break;
   	}
	/* ***************** END MAIN PROCESSING LOOP ******************* */
	
	printf("Avg. algorithm speed vs. realtime = %3.2fx : 1 (DSP only) = %3.1f%% CPU load\n", bavg/gExecTimeTotal, 100.*(gExecTimeTotal/bavg));
	
	
	/* ***************** CLEAN UP ******************* */
	/* Free processing buffers */
	mAiffDeallocateAudioBuffer(audioIn, numChannels);
	mAiffDeallocateAudioBuffer(audioOut, numChannels);
	mAiffDeallocateAudioBuffer(latencyBufferIn, numChannels);
	mAiffDeallocateAudioBuffer(latencyBufferOut, numChannels);
	
	/* Destroy DiracFx instance */
	DiracFxDestroy( diracFx );
	
	/* We're done! */
    printf("\nDone!\n");
	
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


