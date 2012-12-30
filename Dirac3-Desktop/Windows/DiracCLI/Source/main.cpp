
/*
 "main.cpp" Example Source File - Disclaimer:
 
 IMPORTANT:  This file and its contents are subject to the terms set forth in the 
 "License Agreement.txt" file that accompanies this distribution.
 
 Copyright © 2012 Stephan M. Bernsee, http://www.dspdimension.com. All Rights Reserved
 
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <memory.h>

#include "MiniAiff.h"
#include "Dirac.h"

// this defines the maximum number of files that we can use on input. This is enough to process
// 7.1 format and beyond. Increase accordingly if you need more
#define MAX_NUM_FILES		16

#ifdef WIN32
	#define strtold strtod
#endif

#pragma mark ---- Callback and structs ----


// This is the struct that holds state variables that our callback needs. In your program
// you will want to replace this by a pointer to "this" in order to access your instance methods
// and variables
typedef struct {
	unsigned long sReadPosition, sMaxFrames;
	long sTotalNumChannels, sNumFiles;
	long *sInFileNumChannels;
	char **sInFileNames;
	char **sOutFileNames;
} userDataStruct;


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
 This is the callback function that supplies data from the input stream/file(s) whenever needed.
 It should be implemented in your software by a routine that gets data from the input/buffers.
 The read requests are *always* consecutive, ie. the routine will never have to supply data out
 of order.
 */
long myReadData(float **chdata, long numFrames, void *userData)
{	
	// The userData parameter can be used to pass information about the caller (for example, "this") to
	// the callback so it can manage its audio streams.
	
	if (!chdata)	return 0;
	long res = numFrames;
	
	userDataStruct *state = (userDataStruct*)userData;
	if (!state)	return 0;
	
	long channel = 0;
	for (long v = 0; v < state->sNumFiles; v++) {
		mAiffReadData(state->sInFileNames[v], chdata+channel, state->sReadPosition, numFrames, state->sInFileNumChannels[v]);
		channel += state->sInFileNumChannels[v];
	}
	
	state->sReadPosition += numFrames;
	
	return res;	
	
}



#pragma mark ---- Main program ----

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
 Creates an output path from a given input path using the specified file name prefix
 */
char *createOutputFilePath(char *inPath, char *prefix)
{
	char *pathToDirectory = NULL;
	char *fileName = NULL;
	long lastSeparator = strlen(inPath)-1;
	while (lastSeparator >= 0 && (inPath[lastSeparator] != '/' && inPath[lastSeparator] != '\\')) {
		lastSeparator--;
	}
	lastSeparator++;
	if (lastSeparator) {
		pathToDirectory = new char[lastSeparator+1];
		memset(pathToDirectory, 0, (lastSeparator+1)*sizeof(char));
		memmove(pathToDirectory, inPath, lastSeparator);
		pathToDirectory[lastSeparator] = '\0';
	}
	long fileNameLength = strlen(inPath) - lastSeparator;
	if (fileNameLength) {
		fileName = new char[fileNameLength+1];
		memset(fileName, 0, (fileNameLength+1)*sizeof(char));
		memmove(fileName, inPath+lastSeparator, fileNameLength);
		fileName[fileNameLength] = '\0';
	}
	long outPathLength = 0;
	if (pathToDirectory)
		outPathLength += strlen(pathToDirectory);
	if (fileName)
		outPathLength += strlen(fileName);
	
	char *outFilePath = new char[outPathLength+strlen(prefix)+1];
	memset(outFilePath, 0, (outPathLength+strlen(prefix)+1)*sizeof(char));
	long dest = 0;
	if (pathToDirectory) {
		memmove(outFilePath, pathToDirectory, strlen(pathToDirectory));
		dest += strlen(pathToDirectory);
	}
	memmove(outFilePath+dest, prefix, strlen(prefix));
	dest += strlen(prefix);
	if (fileName) {
		memmove(outFilePath+dest, fileName, strlen(fileName));
		dest += strlen(fileName);
	}
	if (pathToDirectory)
		delete[] pathToDirectory;
	if (fileName)
		delete[] fileName;
	return outFilePath;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
 Prints usage and CLI parameters to stdout.
 */

void usage(char *s)
{
	printf("%s -{options} -f <infile> {<infile2> <infile3> ...}\n",s);
	printf(" <infile> must be in AIFF format\n\n");
	printf(" options\n");
	printf("   -L     <int>          : Lambda value (0-6). This sets Dirac's lambda parameter\n");
	printf("                           default=0 (preview)\n");
	printf("   -Q     <int>          : Quality (0-3), with higher values being slower and better\n");
	printf("                           default=0 (preview)\n");
	printf("   -T     <long double>  : Time stretch factor\n");
	printf("                           default=1.0 (no change)\n");
	printf("   -P     <long double>  : Pitch shift factor\n");
	printf("                           default=1.0 (no change)\n");
	printf("   -F     <long double>  : Formant shift factor\n");
	printf("                           default=1.0 (no change)\n");
	printf("   -f     <string>       : Path to input file(s),\n");
	printf("\n");
	printf("   -h                    : print this message.\n\n");
	exit(1);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int main (int argc, char **argv)
{
	
	int numFiles = 0;
	int numChannels = 0, v;
	char *inFileNames[MAX_NUM_FILES];
	char *outFileNames[MAX_NUM_FILES];
	long fileChannelCounts[MAX_NUM_FILES];
	unsigned long maxFrames = 0;
	
	/* options */
	if(argc<2) {
	    usage(argv[0]);
	}
	
	long double time = 1., pitch = 1., formant = 1.;
	int lambda = 0;
	int quality = 0;
	
	long i=1;
	while(i<argc && argv[i][0]=='-'){
		switch(argv[i][1]){
			case 'L':
				++i; 
				lambda=atoi(argv[i]);
				printf("lambda = %d\n", lambda);
				break;
			case 'Q':
				++i;
				quality=atoi(argv[i]);  
				printf("quality = %d\n", quality);
				break;
			case 'T':
				++i; 
				time=strtold(argv[i], NULL);
				printf("time = %Lf\n", time);
				break;
			case 'P':
				++i;
				pitch=strtold(argv[i], NULL);  
				printf("pitch = %Lf\n", pitch);
				break;
			case 'F':
				++i;
				formant=strtold(argv[i], NULL);  
				printf("formant = %Lf\n", formant);
				break;
			case 'f':
				++i;
				while(i<argc && argv[i][0]!='-'){
					inFileNames[numFiles] = argv[i];
					numFiles++;
					if (numFiles >= MAX_NUM_FILES)
						break;
					
					printf("file = %s\n", argv[i]);
					++i;
				}
				--i;
				break;
			case 'h':
				usage(argv[0]); 
				break;
		}
		++i;
	}
	
	if (!numFiles) {
		printf("!!! No input files specified - exiting\n");
		exit(0);
	}
	
	printf("\n------------------------------------------------------\n");
	printf("total number of files to process = %d\n\n", numFiles);
	for ( v = 0; v < numFiles; v++) {
		printf("file #%d = %s\t\t", v, inFileNames[v]);
		bool fileValid = (mAiffGetSampleRate(inFileNames[v]) > 0.);
		if (fileValid) {
			numChannels += (fileChannelCounts[v] = mAiffGetNumberOfChannels(inFileNames[v]));
			unsigned long numFrames = mAiffGetNumberOfFrames(inFileNames[v]);
			if (numFrames > maxFrames)
				maxFrames = numFrames;
			outFileNames[v] = createOutputFilePath(inFileNames[v], "processed-");
			printf("<OK>\t%d channels --> %s\n", fileChannelCounts[v], outFileNames[v]);
		} else {
			printf("!!! invalid !!!\n");
			printf("!!! Aborting due to invalid input file format\n");
			exit(0);
		}
	}
	printf("\ntotal number of channels to process = %d\n", numChannels);
	printf("------------------------------------------------------\n\n");
	
	
	// first file determines sample rate
	float sr = mAiffGetSampleRate(inFileNames[0]);
	
	// We stuff all our programs' state variables that we need to access in order to read from the file in a struct
	// You will normally pass your instance pointer "this" as userData, but since this is not a class we cannot do this here
	userDataStruct state;
	state.sTotalNumChannels		= numChannels;
	state.sReadPosition			= 0;
	state.sMaxFrames			= maxFrames;
	state.sInFileNames			= inFileNames;
	state.sInFileNumChannels	= fileChannelCounts;
	state.sOutFileNames			= outFileNames;
	state.sNumFiles				= numFiles;
	
	
	void *dirac = DiracCreate(kDiracLambdaPreview+lambda, kDiracQualityPreview+quality, numChannels, sr, &myReadData, (void*)&state);		// (1) fastest
	if (!dirac) {
		printf("!! ERROR !!\n\n\tCould not create DIRAC instance\n\tCheck number of channels and sample rate!\n");
		exit(-1);
	}
	
	
	
	// Initialize our output files
	for ( v = 0; v < numFiles; v++) {
		mAiffInitFile(outFileNames[v], 
					  mAiffGetSampleRate(inFileNames[v]), 
					  mAiffGetWordlength(inFileNames[v]), 
					  mAiffGetNumberOfChannels(inFileNames[v]));
	}
	
    // Pass the values to our DIRAC instance 	
    DiracSetProperty(kDiracPropertyTimeFactor, time, dirac);
    DiracSetProperty(kDiracPropertyPitchFactor, pitch, dirac);
    DiracSetProperty(kDiracPropertyFormantFactor, formant, dirac);
	
	// Print our settings to the console
	DiracPrintSettings(dirac);
	
    printf("Running DIRAC version %s\nStarting processing\n", DiracVersion());
	
    // This is an arbitrary number of frames. Change as you see fit
    long numFramesPerCall = 4096;
	
    // Allocate buffer for output
	float **audio = mAiffAllocateAudioBuffer(numChannels, numFramesPerCall);
	long lastPercent = -1;
	for(;;) {
		
        // Call the DIRAC process function with current time and pitch settings
        // Returns: the number of frames in audio
        long ret = DiracProcess(audio, numFramesPerCall, dirac);
		
		// print performance measurements
		long percent = (long)(100.f*(double)state.sReadPosition / (double)state.sMaxFrames);
        if (lastPercent != percent) {
            printf("\t%d%% done\n", (int)percent);
            lastPercent = percent;
			fflush(stdout);
		}
		
		long channel = 0;
		for ( v = 0; v < numFiles; v++) {
			mAiffWriteData(outFileNames[v], audio+channel, numFramesPerCall, fileChannelCounts[v]);
			channel += fileChannelCounts[v];
		}
		
		if (state.sReadPosition > state.sMaxFrames + numFramesPerCall)
			break;
   	}
	
	// Free buffers
	mAiffDeallocateAudioBuffer(audio, numChannels);
	
	// destroy DIRAC instance
	DiracDestroy( dirac );
	
	// free our file names
	for ( v = 0; v < numFiles; v++)
		delete[] outFileNames[v];
	
	return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
