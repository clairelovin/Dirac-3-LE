
/*

	ABSTRACT:
	This example demonstrates how to use the DiracAudioFile player to process and play back
	an audio file in real time (high quality setting)
 
 */

//
//  DiracAudioPlayerExampleViewController.m
//  DiracAudioPlayerExample
//
//  Created by Stephan on 25.03.11.
//  Copyright 2011 The DSP Dimension. All rights reserved.
//

#import "DiracAudioPlayerExampleViewController.h"


@implementation DiracAudioPlayerExampleViewController

- (void)diracPlayerDidFinishPlaying:(DiracAudioPlayerBase *)player successfully:(BOOL)flag
{
	NSLog(@"Dirac player instance (0x%lx) is done playing", (long)player);
}


// ---------------------------------------------------------------------------------------------------------------------------------------------

- (void)loadView 
{
	[super loadView];

	NSString *inputSound  = [[NSBundle mainBundle] pathForResource:  @"SMB2MasterDspS" ofType: @"caf"];
	NSURL *inUrl = [NSURL fileURLWithPath:inputSound];
	
	NSError *error = nil;
	mDiracAudioPlayer = [[DiracAudioPlayer alloc] initWithContentsOfURL:inUrl channels:1 error:&error];		// LE only supports 1 channel!
	[mDiracAudioPlayer setDelegate:self];
	
	
	mUseVarispeed = NO;
}


// ---------------------------------------------------------------------------------------------------------------------------------------------

-(IBAction)uiDurationSliderMoved:(id)sender;
{
	[mDiracAudioPlayer changeDuration:[sender floatValue]];
	[uiDurationLabel setStringValue:[NSString stringWithFormat:@"%3.2f", [sender floatValue]]];
	
	if (mUseVarispeed) {
		float val = 1.f/[sender floatValue];
		[uiPitchSlider setFloatValue:(int)12.f*log2f(val)];
		[uiPitchLabel setStringValue:[NSString stringWithFormat:@"%d", (int)[uiPitchSlider floatValue]]];
		[mDiracAudioPlayer changePitch:val];
	}
}
// ---------------------------------------------------------------------------------------------------------------------------------------------

-(IBAction)uiPitchSliderMoved:(id)sender;
{
	[mDiracAudioPlayer changePitch:powf(2.f, (int)[sender floatValue] / 12.f)];
	[uiPitchLabel setStringValue:[NSString stringWithFormat:@"%d", (int)[sender floatValue]]];
}
// ---------------------------------------------------------------------------------------------------------------------------------------------

-(IBAction)uiStartButtonTapped:(id)sender;
{
	[mDiracAudioPlayer play];
}
// ---------------------------------------------------------------------------------------------------------------------------------------------

-(IBAction)uiStopButtonTapped:(id)sender;
{
	[mDiracAudioPlayer stop];
}
// ---------------------------------------------------------------------------------------------------------------------------------------------

-(IBAction)uiVarispeedSwitchTapped:(id)sender;
{
	if ([sender state] == NSOnState) {
		mUseVarispeed = YES;
		
		[uiPitchSlider setEnabled:NO];
		
		float val = 1.f/[uiDurationSlider floatValue];
		[uiPitchSlider setFloatValue:(int)12.f*log2f(val)];
		[uiPitchLabel setStringValue:[NSString stringWithFormat:@"%d", (int)[uiPitchSlider floatValue]]];
		[mDiracAudioPlayer changePitch:val];		
		
	} else {
		mUseVarispeed = NO;
		[uiPitchSlider setEnabled:YES];
	}

}
// ---------------------------------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
	[mDiracAudioPlayer release];
    [super dealloc];
}
// ---------------------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------------------------------


@end
