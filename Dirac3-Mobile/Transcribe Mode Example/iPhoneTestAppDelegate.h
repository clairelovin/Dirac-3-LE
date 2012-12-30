//
//  iPhoneTestAppDelegate.h
//  iPhoneTest
//
//  Created by Stephan on 05.10.09.
//  Copyright The DSP Dimension 2009. All rights reserved.
//

#import <UIKit/UIKit.h>


#import "EAFRead.h"
#import "EAFWrite.h"

@interface iPhoneTestAppDelegate : NSObject <UIApplicationDelegate, AVAudioPlayerDelegate> {
    UIWindow *window;
	AVAudioPlayer *player;
	IBOutlet UILabel *text;
	IBOutlet UILabel *version;
	IBOutlet UIProgressView *progressView;
	
	float percent;
	
	NSURL *inUrl;
	NSURL *outUrl;
	EAFRead *reader;
	EAFWrite *writer;
	
}


@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (readonly) EAFRead *reader;

@end

