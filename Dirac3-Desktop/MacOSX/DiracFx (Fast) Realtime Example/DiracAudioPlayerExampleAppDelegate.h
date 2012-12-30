//
//  DiracAudioPlayerExampleAppDelegate.h
//  DiracAudioPlayerExample
//
//  Created by Stephan on 25.03.11.
//  Copyright 2011 The DSP Dimension. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class DiracAudioPlayerExampleViewController;

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
	@interface DiracAudioPlayerExampleAppDelegate : NSObject {
#else
	@interface DiracAudioPlayerExampleAppDelegate : NSObject <NSApplicationDelegate> {
#endif
    NSWindow *window;
    DiracAudioPlayerExampleViewController *viewController;
}

@property (assign) IBOutlet NSWindow *window;
@property (nonatomic, retain) IBOutlet DiracAudioPlayerExampleViewController *viewController;

@end
