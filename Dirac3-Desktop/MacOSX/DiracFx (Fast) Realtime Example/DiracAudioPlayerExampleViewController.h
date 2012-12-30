//
//  DiracAudioPlayerExampleViewController.h
//  DiracAudioPlayerExample
//
//  Created by Stephan on 25.03.11.
//  Copyright 2011 The DSP Dimension. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DiracFxAudioPlayer.h"


@interface DiracAudioPlayerExampleViewController : NSViewController {

	IBOutlet NSButton *uiStartButton;
	IBOutlet NSButton *uiStopButton;
	
	IBOutlet NSSlider *uiDurationSlider;
	IBOutlet NSSlider *uiPitchSlider;
	
	IBOutlet NSTextField *uiDurationLabel;
	IBOutlet NSTextField *uiPitchLabel;
	
	IBOutlet NSButton *uiVarispeedSwitch;
	BOOL mUseVarispeed;
	
	DiracFxAudioPlayer *mDiracAudioPlayer;
	
	
}


-(IBAction)uiDurationSliderMoved:(id)sender;
-(IBAction)uiPitchSliderMoved:(id)sender;

-(IBAction)uiStartButtonTapped:(id)sender;
-(IBAction)uiStopButtonTapped:(id)sender;

-(IBAction)uiVarispeedSwitchTapped:(id)sender;


@end
