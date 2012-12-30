
Command Line Interface project for Dirac (DiracCLI)
===================================================

This is an example project that demonstrates how to develop a command line 
application that uses Dirac and miniAIFF to read and process AIFF files according 
to parameters passed in via the command line arguments. 

The following command line arguments are available in this version:

-L:	Lambda value (0-6). This sets Dirac's lambda parameter
-Q:	Quality (0-3), with higher values being slower and better

-T:	Time stretch factor
-P:	Pitch shift factor
-F:	Formant shift factor

-f:	DiracCLI interprets any following arguments as paths to input files. The
	channels in all input files will be processed in a phase locked manner.

Following are typical calls that you will make for specific applications:

./DiracCLI -L 3 -Q 3 -P 1.33 -f recording-L.aif recording-R.aif -T 1.11

Processes the two mono files recording-L.aif and recording-R.aif as a single
stereo file in a phase locked manner. kDiracLambda3 and kDiracQualityBest are
used and a pitch shifting of 1.33 and time stretching of 1.11 are applied.

./DiracCLI -L 3 -Q 3 -T 1.042709376042709 -f r-L.aif r-R.aif r-C.aif r-Ls.aif r-Rs.aif r-S.aif r-LFE.aif

Processes the surround channels r-L.aif r-R.aif r-C.aif r-Ls.aif r-Rs.aif r-S.aif 
r-LFE.aif with the 3:2 pulldown rate (25 -> 23.976 FPS conversion) in a phase
locked manner.

