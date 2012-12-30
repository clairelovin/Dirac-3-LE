//
//  DiracAudioPlayerExampleAppDelegate.h
//  DiracAudioPlayerExample
//
//  Created by Stephan on 20.03.11.
//  Copyright 2011 The DSP Dimension. All rights reserved.
//

#import <UIKit/UIKit.h>

@class DiracAudioPlayerExampleViewController;

@interface DiracAudioPlayerExampleAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    DiracAudioPlayerExampleViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet DiracAudioPlayerExampleViewController *viewController;

@end

