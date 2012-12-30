
/*
	"main.cpp" Example Source File - Disclaimer:
 
	IMPORTANT:  This file and its contents are subject to the terms set forth in the 
	"License Agreement.txt" file that accompanies this distribution.
 
	Copyright © 2005-2011 Stephan M. Bernsee, http://www.dspdimension.com. All Rights Reserved

 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "MiniAiff.h"
#include "Dirac.h"


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
double gExecTimeTotal = 0.;
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma mark ---- Callback and structs ----


// This is the struct that holds state variables that our callback needs. In your program
// you will want to replace this by a pointer to "this" in order to access your instance methods
// and variables
typedef struct {
	unsigned long sReadPosition;
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

	// we want to exclude the time it takes to read in the data from disk or memory, so we stop the clock until 
	// we've read in the requested amount of data
	gExecTimeTotal += DiracClockTimeSeconds(); 		// ............................. stop timer ..........................................
	
	long res = mAiffReadData(state->sInFileName, chdata, state->sReadPosition, numFrames, state->sNumChannels);
	state->sReadPosition += numFrames;
	
	DiracStartClock();								// ............................. start timer ..........................................
	
	return res;	
	
}

#pragma mark ---- Main program ----


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int main()
{
	
	// Create and init output file "out.aif"
#ifdef __APPLE__
	char infileName[]="../../../../test.aif";
#else
#ifdef TARGET_LINUX
	char infileName[]="test.aif";
#else
	char infileName[]="../../test.aif";

#endif
#endif
	char oufileName[]="out.aif";

	long numChannels = 2;							// DIRAC LE allows mono only, our PRO Demo can do stereo (and more) as well
	float sr = mAiffGetSampleRate(infileName);		// get sample rate from our input file
	if (sr <= 0.) {
		printf("ERROR: File not found (%s)\n", infileName);
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
	// N.b.: The fastest option is kDiracLambdaPreview / kDiracQualityPreview, best is kDiracLambda3 / kDiracQualityBest
	// For extreme stretch ratios we recommend kDiracLambdaTranscribe / kDiracQualityGood as this will produce less artifacts
	// The probably best default option for general purpose signals is kDiracLambda3 / kDiracQualityGood
	void *dirac = DiracCreate(kDiracLambdaPreview, kDiracQualityPreview, numChannels, sr, &myReadData, (void*)&state);		// (1) fastest
	//void *dirac = DiracCreate(kDiracLambda1, kDiracQualityBest, numChannels, sr, &myReadData, (void*)&state);				// (2) best for voice
	//void *dirac = DiracCreate(kDiracLambda3, kDiracQualityBest, numChannels, sr, &myReadData, (void*)&state);				// (3) best general purpose option
	//void *dirac = DiracCreate(kDiracLambdaTranscribe, kDiracQualityBest, numChannels, sr, &myReadData, (void*)&state);	// (4) for extreme ratios
	if (!dirac) {
		printf("!! ERROR !!\n\n\tCould not create DIRAC instance\n\tCheck number of channels and sample rate!\n");
		exit(-1);
	}
	
	// Initialize our output file
	mAiffInitFile(oufileName, sr /* sample rate */, 16 /* bits */, numChannels);
	
    // Here we set our time an pitch manipulation values
    float time      = 1.13;                 // 113% length
	float pitch     = pow(2., 0./12.);     // pitch shift (0 semitones)
	float formant   = pow(2., 0./12.);    // formant shift (0 semitones). N.b. formants are reciprocal to pitch in natural transposing. Setting this != 1.0 uses a lot more CPU!
	
    // Pass the values to our DIRAC instance 	
    DiracSetProperty(kDiracPropertyTimeFactor, time, dirac);
    DiracSetProperty(kDiracPropertyPitchFactor, pitch, dirac);
    DiracSetProperty(kDiracPropertyFormantFactor, formant, dirac);
	
	// Print our settings to the console
	DiracPrintSettings(dirac);
	
    printf("Running DIRAC version %s\nStarting processing\n", DiracVersion());
	
	// Get the number of frames from the file to display our simplistic progress bar
	float numf = mAiffGetNumberOfFrames(infileName);
    unsigned long outframes = 0;
    unsigned long newOutframe = numf*time;
    long lastPercent = -1;
	
    // This is an arbitrary number of frames. Change as you see fit
    long numFrames = 8192;
	
    // Allocate buffer for output
	float **audio = mAiffAllocateAudioBuffer(numChannels, numFrames);
	
	// for time measurement
	double bavg = 0;
	
	// MAIN PROCESSING LOOP STARTS HERE
	for(;;) {
		
		
		DiracStartClock();								// ............................. start timer ..........................................
		
        // Call the DIRAC process function with current time and pitch settings
        // Returns: the number of frames in audio
        long ret = DiracProcess(audio, numFrames, dirac);
		bavg += (numFrames/sr);
		gExecTimeTotal += DiracClockTimeSeconds();		// ............................. stop timer ..........................................

		// print performance measurements
		long percent = 100.f*(double)outframes / (double)newOutframe;
        if (lastPercent != percent) {
            printf("\t%d%% done, avg. algorithm speed vs. realtime = %3.2f : 1 (DSP only), CPU load (peak, DSP+disk): %3.2f%%\n", (int)percent, bavg/gExecTimeTotal, DiracPeakCpuUsagePercent(dirac));
            lastPercent = percent;
			fflush(stdout);
		}
		
        // Write the data to the output file
        mAiffWriteData(oufileName, audio, numFrames, numChannels);
		
        // Increase our counter for the percentage
        outframes += numFrames;
		
        // As soon as we've written enough frames we exit the main loop
		if (ret <= 0) break;
   	}
	
	
    // Free buffers
	mAiffDeallocateAudioBuffer(audio, numChannels);
	
	// destroy DIRAC instance
	DiracDestroy( dirac );
	
	// free our file name
	delete[] state.sInFileName;
	
    // Done!
    printf("\nDone!\n");
    putchar(7);
	
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
