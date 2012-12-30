//
//  DiracAudioPlayerExampleAppDelegate.m
//  DiracAudioPlayerExample
//
//  Created by Stephan on 25.03.11.
//  Copyright 2011 The DSP Dimension. All rights reserved.
//

#import "DiracAudioPlayerExampleAppDelegate.h"
#import "DiracAudioPlayerExampleViewController.h"

@implementation DiracAudioPlayerExampleAppDelegate

@synthesize window;
@synthesize viewController;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	// Insert code here to initialize your application 

	[[self.window contentView] addSubview:viewController.view];
    [self.window makeKeyAndOrderFront:self];

}


- (void)dealloc {
    [viewController release];
    [window release];
    [super dealloc];
}


@end
