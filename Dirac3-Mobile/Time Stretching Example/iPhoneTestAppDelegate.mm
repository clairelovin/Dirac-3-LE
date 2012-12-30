//
//	ABSTRACT:
//	This project demonstrates how to use Dirac for offline time stretching
//
//
//  iPhoneTestAppDelegate.m
//  iPhoneTest
//
//  Created by Stephan on 05.10.09.
//  Copyright The DSP Dimension 2009-2011. All rights reserved.
//

#include "Dirac.h"
#include <stdio.h>
#include <sys/time.h>

#import <AVFoundation/AVAudioPlayer.h>
#import <AVFoundation/AVFoundation.h>

#import "iPhoneTestAppDelegate.h"
#import "EAFRead.h"
#import "EAFWrite.h"
#include "Utilities.h"

double gExecTimeTotal = 0.;


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*
 This is the callback function that supplies data from the input stream/file whenever needed.
 It should be implemented in your software by a routine that gets data from the input/buffers.
 The read requests are *always* consecutive, ie. the routine will never have to supply data out
 of order.
 */
long myReadData(float **chdata, long numFrames, void *userData)
{	
	// The userData parameter can be used to pass information about the caller (for example, "self") to
	// the callback so it can manage its audio streams.
	if (!chdata)	return 0;
	
	iPhoneTestAppDelegate *Self = (iPhoneTestAppDelegate*)userData;
	if (!Self)	return 0;
	
	// we want to exclude the time it takes to read in the data from disk or memory, so we stop the clock until 
	// we've read in the requested amount of data
	gExecTimeTotal += DiracClockTimeSeconds(); 		// ............................. stop timer ..........................................
		
	OSStatus err = [Self.reader readFloatsConsecutive:numFrames intoArray:chdata];
	
	DiracStartClock();								// ............................. start timer ..........................................

	return err;
	
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


@implementation iPhoneTestAppDelegate 

@synthesize window;
@synthesize reader;

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


-(void)playOnMainThread:(id)param
{
	NSError *error = nil;
	[text setText:@"Now Playing..."];
	player = [[AVAudioPlayer alloc] initWithContentsOfURL:outUrl error:&error];
	if (error)
		NSLog(@"AVAudioPlayer error %@, %@", error, [error userInfo]);
	
	player.delegate = self;
	[player play];
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

-(void)updateBarOnMainThread:(id)param
{
	[progressView setProgress:(percent/100.f)];
}


//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

-(void)processThread:(id)param
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	[text setText:@"Processing..."];
	[version setText:[NSString stringWithFormat:@"TimeStretching Example\nDIRAC Version: %s", DiracVersion()]];
    [window makeKeyAndVisible];

	long numChannels = 1;		// DIRAC LE allows mono only
	float sampleRate = 44100.;

	// open input file
	[reader openFileForRead:inUrl sr:sampleRate channels:numChannels];
	
	// create output file (overwrite if exists)
	[writer openFileForWrite:outUrl sr:sampleRate channels:numChannels wordLength:16 type:kAudioFileAIFFType];	
	
	// DIRAC parameters
	// Here we set our time an pitch manipulation values
	float time      = 1.15;                 // 115% length
	float pitch     = pow(2., 0./12.);     // pitch shift (0 semitones)
	float formant   = pow(2., 0./12.);    // formant shift (0 semitones). Note formants are reciprocal to pitch in natural transposing
	
	// First we set up DIRAC to process numChannels of audio at 44.1kHz
	// N.b.: The fastest option is kDiracLambdaPreview / kDiracQualityPreview, best is kDiracLambda3, kDiracQualityBest
	// The probably best *default* option for general purpose signals is kDiracLambda3 / kDiracQualityGood
	void *dirac = DiracCreate(kDiracLambdaPreview, kDiracQualityPreview, numChannels, sampleRate, &myReadData, (void*)self);
	//	void *dirac = DiracCreate(kDiracLambda3, kDiracQualityBest, numChannels, sampleRate, &myReadData);
	if (!dirac) {
		printf("!! ERROR !!\n\n\tCould not create DIRAC instance\n\tCheck number of channels and sample rate!\n");
		printf("\n\tNote that the free DIRAC LE library supports only\n\tone channel per instance\n\n\n");
		exit(-1);
	}
	
	// Pass the values to our DIRAC instance 	
	DiracSetProperty(kDiracPropertyTimeFactor, time, dirac);
	DiracSetProperty(kDiracPropertyPitchFactor, pitch, dirac);
	DiracSetProperty(kDiracPropertyFormantFactor, formant, dirac);
	
	// upshifting pitch will be slower, so in this case we'll enable constant CPU pitch shifting
	if (pitch > 1.0)
		DiracSetProperty(kDiracPropertyUseConstantCpuPitchShift, 1, dirac);

	// Print our settings to the console
	DiracPrintSettings(dirac);
	
	NSLog(@"Running DIRAC version %s\nStarting processing", DiracVersion());
	
	// Get the number of frames from the file to display our simplistic progress bar
	SInt64 numf = [reader fileNumFrames];
	SInt64 outframes = 0;
	SInt64 newOutframe = numf*time;
	long lastPercent = -1;
	percent = 0;
	
	// This is an arbitrary number of frames per call. Change as you see fit
	long numFrames = 8192;
	
	// Allocate buffer for output
	float **audio = AllocateAudioBuffer(numChannels, numFrames);

	double bavg = 0;
	
	// MAIN PROCESSING LOOP STARTS HERE
	for(;;) {
		
		// Display ASCII style "progress bar"
		percent = 100.f*(double)outframes / (double)newOutframe;
		long ipercent = percent;
		if (lastPercent != percent) {
			[self performSelectorOnMainThread:@selector(updateBarOnMainThread:) withObject:self waitUntilDone:NO];
			printf("\rProgress: %3i%% [%-40s] ", ipercent, &"||||||||||||||||||||||||||||||||||||||||"[40 - ((ipercent>100)?40:(2*ipercent/5))] );
			lastPercent = ipercent;
			fflush(stdout);
		}
		
		DiracStartClock();								// ............................. start timer ..........................................
		
		// Call the DIRAC process function with current time and pitch settings
		// Returns: the number of frames in audio
		long ret = DiracProcess(audio, numFrames, dirac);
		bavg += (numFrames/sampleRate);
		gExecTimeTotal += DiracClockTimeSeconds();		// ............................. stop timer ..........................................
		
		printf("x realtime = %3.3f : 1 (DSP only), CPU load (peak, DSP+disk): %3.2f%%\n", bavg/gExecTimeTotal, DiracPeakCpuUsagePercent(dirac));
		
		// Process only as many frames as needed
		long framesToWrite = numFrames;
		unsigned long nextWrite = outframes + numFrames;
		if (nextWrite > newOutframe) framesToWrite = numFrames - nextWrite + newOutframe;
		if (framesToWrite < 0) framesToWrite = 0;
		
		// Write the data to the output file
		[writer writeFloats:framesToWrite fromArray:audio];
		
		// Increase our counter for the progress bar
		outframes += numFrames;
		
		// As soon as we've written enough frames we exit the main loop
		if (ret <= 0) break;
	}
	
	percent = 100;
	[self performSelectorOnMainThread:@selector(updateBarOnMainThread:) withObject:self waitUntilDone:NO];

	
	// Free buffer for output
	DeallocateAudioBuffer(audio, numChannels);
	
	// destroy DIRAC instance
	DiracDestroy( dirac );
	
	// Done!
	NSLog(@"\nDone!");
	
	[reader release];
	[writer release]; // important - flushes data to file
	
	// start playback on main thread
	[self performSelectorOnMainThread:@selector(playOnMainThread:) withObject:self waitUntilDone:NO];
	
	[pool release];
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// done playing? exit
- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag
{
	exit(0);
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

- (void)applicationDidFinishLaunching:(UIApplication *)application 
{    
	NSString *inputSound  = [[[NSBundle mainBundle] pathForResource:  @"test" ofType: @"aif"] retain];
	NSString *outputSound = [[[NSHomeDirectory() stringByAppendingString:@"/Documents/"] stringByAppendingString:@"out.aif"] retain];
	inUrl = [[NSURL fileURLWithPath:inputSound] retain];
	outUrl = [[NSURL fileURLWithPath:outputSound] retain];
	reader = [[EAFRead alloc] init];
	writer = [[EAFWrite alloc] init];

	// this thread does the processing
	[NSThread detachNewThreadSelector:@selector(processThread:) toTarget:self withObject:nil];
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

- (void)dealloc 
{
	[player release];
    [window release];
	[inUrl release];
	[outUrl release];

    [super dealloc];
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

@end
