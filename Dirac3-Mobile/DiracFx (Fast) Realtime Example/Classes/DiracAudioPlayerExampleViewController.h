//
//  DiracAudioPlayerExampleViewController.h
//  DiracAudioPlayerExample
//
//  Created by Stephan on 20.03.11.
//  Copyright 2011 The DSP Dimension. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DiracFxAudioPlayer.h"

@interface DiracAudioPlayerExampleViewController : UIViewController {

	IBOutlet UIButton *uiStartButton;
	IBOutlet UIButton *uiStopButton;
	
	IBOutlet UISlider *uiDurationSlider;
	IBOutlet UISlider *uiPitchSlider;
	
	IBOutlet UILabel *uiDurationLabel;
	IBOutlet UILabel *uiPitchLabel;

	IBOutlet UISwitch *uiVarispeedSwitch;
	BOOL mUseVarispeed;

	DiracFxAudioPlayer *mDiracAudioPlayer;
	
}


-(IBAction)uiDurationSliderMoved:(UISlider *)sender;
-(IBAction)uiPitchSliderMoved:(UISlider *)sender;

-(IBAction)uiStartButtonTapped:(UIButton *)sender;
-(IBAction)uiStopButtonTapped:(UIButton *)sender;

-(IBAction)uiVarispeedSwitchTapped:(UISwitch *)sender;

@end

