//
//  DiracAudioPlayerExampleViewController.h
//  DiracAudioPlayerExample
//
//  Created by Stephan on 22.11.2012.
//  Copyright 2012 The DSP Dimension. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DiracAudioPlayer.h"
#import <MediaPlayer/MediaPlayer.h>

@interface DiracAudioPlayerExampleViewController : UIViewController <MPMediaPickerControllerDelegate> {

	IBOutlet UIButton *uiStartButton;
	IBOutlet UIButton *uiStopButton;
	
	IBOutlet UISlider *uiDurationSlider;
	IBOutlet UISlider *uiPitchSlider;
	
	IBOutlet UILabel *uiDurationLabel;
	IBOutlet UILabel *uiPitchLabel;

	IBOutlet UISwitch *uiVarispeedSwitch;
	BOOL mUseVarispeed;

	DiracAudioPlayer *mDiracAudioPlayer;
	
	MPMediaPickerController *mPicker;
    MPMusicPlayerController *mPlayer;
    MPMediaQuery *mQuery;
    MPMediaPredicate *mPredicate;

	
}


-(IBAction)uiDurationSliderMoved:(UISlider *)sender;
-(IBAction)uiPitchSliderMoved:(UISlider *)sender;

-(IBAction)uiStartButtonTapped:(UIButton *)sender;
-(IBAction)uiStopButtonTapped:(UIButton *)sender;

-(IBAction)uiVarispeedSwitchTapped:(UISwitch *)sender;

@end

