/*===============================================================================================
 Dirac Time Stretching Example
 Copyright (c), Firelight Technologies Pty, Ltd 2004-2011.
 Copyright (c), The DSP Dimension 2012

 This example shows how to use Dirac to change pitch and speed of music independently from
 each other during playback by using FMOD's sound streaming capabilities.
 
 This demo inserts a custom DSP callback into the FMOD signal chain and pulls audio data from 
 the input sound through that callback for processing.

 Several other implementations are possible, but this appears to offer the best trade off 
 between code complexity and realtime performance.

 Note that using the DiracFx API would provide even better performance but it would mean
 sacrificing some audio quality in the process.

 Note also that unlike the FMOD stream engine we do not insert a resampler into the signal
 chain. If the file's sample rate does not match the system's sample rate your file will
 play at the wrong speed and sound out of tune. You can use Dirac to correct for this, but
 we left that part out of the code to keep it more readable.

===============================================================================================*/
#ifdef __APPLE__

	#include "../../api/inc/fmod.hpp"
	#include "../../api/inc/fmod_errors.h"
	#include "../common/wincompat.h"
	#define	SOUNDFILE_PATH		"../../local_media/test-loop.wav"

#else

	#include "../../api/inc/fmod.hpp"
	#include "../../api/inc/fmod_errors.h"
	#include <windows.h>
	#include <stdio.h>
	#include <math.h>
	#include <conio.h>
	typedef signed int        int32_t;
	#define	SOUNDFILE_PATH		"./local_media/test-loop.wav"

#endif

#include "Dirac.h"

/* ****************************************************************************
	Set up some handy constants
 **************************************************************************** */
const float div8 = 0.0078125f;			//1./pow(2., 8.-1.);
const double div16 = 0.00003051757812;	//1./pow(2., 16.-1.);
const double div32 = 0.00000000046566;	//1./pow(2., 32.-1.);

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* ****************************************************************************
	This is the struct that holds state variables that our callback needs. In
	your program you will want to replace this by a pointer to "this" in order
	to access your instance methods and member variables
 **************************************************************************** */
typedef struct {
	unsigned int sReadPosition, sFileNumFrames;
	int sNumChannels, sNumBits;
	FMOD::Sound *sSound;
} userDataStruct;

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ERRCHECK(FMOD_RESULT result)
{
    if (result != FMOD_OK)
    {
        printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
        exit(-1);
    }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------

/* ****************************************************************************
	This converts the raw format from the decoded file to the float format that
	Dirac expects. 
 **************************************************************************** */
static void intToFloat(float *dest, void *src, long size, int wordlength)
{
	long wordlengthInBytes = wordlength / 8;
	long numElementsInBuffer = size / wordlengthInBytes;
	long i;
	switch (wordlength) {
		case 8:
		{
			signed char *v = (signed char *)src;
			for ( i = 0; i < numElementsInBuffer; i++)	
				dest[i]=(float)v[i] * div8;
		}
			break;
		case 16:
		{
			signed short *v = (signed short *)src;	
			for ( i = 0; i < numElementsInBuffer; i++) {
				dest[i]=(float)v[i] * div16;
			}
		}
			break;
		case 24:
		{
			unsigned char *v = (unsigned char *)src;
			long c = 0;
			for ( i = 0; i < numElementsInBuffer; i++)	{
				int32_t value = 0;
				unsigned char *valuePtr = (unsigned char *)&value;

				valuePtr[0] = 0;
				valuePtr[1] = v[c]; c++;
				valuePtr[2] = v[c]; c++;
				valuePtr[3] = v[c]; c++;
				
				dest[i]=(double)value * div32;
			}
		}
			break;
		case 32:
		{
			int32_t *v = (int32_t *)src;	
			for ( i = 0; i < numElementsInBuffer; i++) {
				dest[i]=(double)v[i] * div32;
			}
		}
			break;
		default:
			break;
	}
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/* ****************************************************************************
	Reads a chunk of raw audio from the FMOD sound. If the read access was
	successful we convert the data to float
 **************************************************************************** */
int readFromSound(float *targetBuffer, unsigned int startFrame, unsigned int numFrames, FMOD::Sound *sound, int numBits, int numChannels)
{
	void  *data1 = NULL;
	void  *data2 = NULL;
	unsigned int length1;
	unsigned int length2;
	
	int ret = -1;
	
	int framesToBytes = numChannels * numBits / 8;

	// lock the buffer
	sound->lock(startFrame * framesToBytes, numFrames * framesToBytes, &data1, &data2, &length1, &length2);
	
	if (data1)
		intToFloat(targetBuffer, data1, length1, numBits);

	// unlock the buffer once you're done
	sound->unlock(data1, data2, length1, length2);	
	return ret;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* ****************************************************************************
	This is the callback function that supplies data from the input stream/file
	when needed by Dirac. The read requests are *always* consecutive, ie. the 
	routine will never have to supply data out of order.
	Should return the number of frames read, 0 when EOF, and -1 on error.
 **************************************************************************** */
long diracDataProviderCallback(float *chdata, long numFrames, void *userData)
{	
	// The userData parameter can be used to pass information about the caller (for example, "this") to
	// the callback so it can manage its audio streams.
	
	if (!chdata)	return 0;

	userDataStruct *state = (userDataStruct*)userData;
	if (!state)	return 0;

	FMOD::Sound *mySound = state->sSound;
	
	
	// demonstrates how to loop our sound seamlessly when the file is done playing:
	// 
	// we have this many frames left before we hit EOF
	unsigned int framesToReadBeforeEndOfFile = 0;
	
	// if our current read position plus the required amount of frames takes us past the end of the file
	if (state->sReadPosition + numFrames > state->sFileNumFrames) {
		
		// we have this many frames left until EOF
		framesToReadBeforeEndOfFile = state->sFileNumFrames-state->sReadPosition;
		
		// read the remaining frames until EOF
		readFromSound(chdata, state->sReadPosition, framesToReadBeforeEndOfFile, mySound, state->sNumBits, state->sNumChannels);
		
		// rewind the file
		state->sReadPosition = 0;
		
		// remove the amount we just read from the amount that we actually want
		numFrames -= framesToReadBeforeEndOfFile;
	}
	
	// here we read the second part of the buffer (in case we hit EOF along the way), or just a chunk of audio from the file (in case we have not encountered EOF yet)
	readFromSound(chdata+framesToReadBeforeEndOfFile*state->sNumChannels, state->sReadPosition, numFrames, mySound, state->sNumBits, state->sNumChannels);
	state->sReadPosition += numFrames;
	
	return numFrames;	
	
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/* ****************************************************************************
	This is our custom dsp callback as implemented by the FMOD example
	dsp_custom. 
 **************************************************************************** */
FMOD_RESULT F_CALLBACK myDSPCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int outchannels) 
{ 
    unsigned int userdata;
    char name[256]; 
    FMOD::DSP *thisdsp = (FMOD::DSP *)dsp_state->instance;

	int ret = kDiracErrorNoErr;
	
    /* 
        This redundant call just shows using the instance parameter of FMOD_DSP_STATE and using it to 
        call a DSP information function. 
    */
    thisdsp->getInfo(name, 0, 0, 0, 0);

	// userData points to our Dirac instance
    thisdsp->getUserData((void **)&userdata);
	
	if (!userdata)
		return FMOD_ERR_NOTREADY;
	
	ret = DiracProcessInterleaved(outbuffer, length, (void*)userdata);
	
	switch (ret) {
		case kDiracErrorDemoTimeoutReached:
			printf("!!! Dirac Evaluation has reached its demo timeout\n\tSwitching to BYPASS\n");
			thisdsp->setBypass(true);
			break;
		default:
			break;
	}
	
    return FMOD_OK; 
} 

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    FMOD::System       *system;
    FMOD::Sound        *sound;
    FMOD::Channel      *channel;
    FMOD::DSP          *mydsp;
    FMOD_RESULT         result;
    int                 key;
    unsigned int        version;
	int                 systemSampleRate;
	float               fileSampleRate;

	
    /*
        Create a System object and initialize.
    */
    result = FMOD::System_Create(&system);
    ERRCHECK(result);

    result = system->getVersion(&version);
    ERRCHECK(result);

    if (version < FMOD_VERSION)
    {
        printf("Error!  You are using an old version of FMOD %08x.  This program requires %08x\n", version, FMOD_VERSION);
        return 0;
    }

    result = system->init(32, FMOD_INIT_NORMAL, 0);
    ERRCHECK(result);
	
	// get the output (=processing) sample rate from the system
	// actually, we should use the file's sample rate, but I'm not sure if there is a call for that in FMOD
	result = system->getSoftwareFormat(&systemSampleRate, NULL, NULL, NULL, NULL, NULL);
    ERRCHECK(result);
	
    result = system->createSound(SOUNDFILE_PATH, FMOD_SOFTWARE | FMOD_LOOP_NORMAL | FMOD_ACCURATETIME, 0, &sound);
	ERRCHECK(result);

    printf("===============================================================================\n");
    printf("Dirac DSP example. Copyright (c) Firelight Technologies 2004-2011 and \n");
    printf("(c) 2012 The DSP Dimension, Stephan M. Bernsee \n");
    printf("===============================================================================\n");
    printf("Press 'f' to activate, deactivate Dirac\n");
    printf("Press 'Esc' to quit\n");
    printf("\n");

	
	
	
	/* ****************************************************************************
		DIRAC SETUP
		-----------
		We put all of Dirac's state variables that we need to access later in a
		struct. In a C++ program you will normally pass your instance pointer
		"this" as Dirac userData, but since this is not a class we cannot do 
		this here
	 **************************************************************************** */
	userDataStruct state;
	state.sReadPosition = 0;
	state.sSound = sound;
	
	// obtain the length of the sound in sample frames (needed to loop the sound properly)
	result = sound->getLength(&state.sFileNumFrames, FMOD_TIMEUNIT_PCM);
	ERRCHECK(result);

	// get the number of channels and number of bits. Needed to obtain the correct seek position in the file when using Sound::lock
	result = sound->getFormat(NULL, NULL, &state.sNumChannels, &state.sNumBits);
	ERRCHECK(result);

	// get the number of channels and number of bits. Needed to obtain the correct seek position in the file when using Sound::lock
	result = sound->getDefaults(&fileSampleRate, NULL, NULL, NULL);
	ERRCHECK(result);
	
	// create our Dirac instance. We use the fastest possible setting for the Dirac core API here
	void *dirac = DiracCreateInterleaved(kDiracLambdaPreview, kDiracQualityPreview, state.sNumChannels, fileSampleRate, &diracDataProviderCallback, (void*)&state);
	if (!dirac) {
		printf("!! ERROR !!\n\n\tCould not create DIRAC instance\n\tCheck number of channels and sample rate!\n");
		exit(-1);
	}
	
	// Here we set our time stretch an pitch shift values
	float time      = 1.25f;					// 125% length
	float pitch     = pow(2.f, 0.f/12.f);		// pitch shift (0 semitones)
	
	// Pass the values to our DIRAC instance 	
	DiracSetProperty(kDiracPropertyTimeFactor, time, dirac);
	DiracSetProperty(kDiracPropertyPitchFactor, pitch, dirac);
	
	// Print our settings to the console
	DiracPrintSettings(dirac);
	
	printf("Running DIRAC version %s\nStarting processing\n", DiracVersion());

	
	
	
	
	// start playback
    result = system->playSound(FMOD_CHANNEL_FREE, sound, false, &channel);
    ERRCHECK(result);

    /*
        Create the DSP effects.
    */  
    { 
        FMOD_DSP_DESCRIPTION  dspdesc; 

        memset(&dspdesc, 0, sizeof(FMOD_DSP_DESCRIPTION)); 

        strcpy(dspdesc.name, "My Dirac Time Stretch Unit"); 
        dspdesc.channels     = state.sNumChannels;                   // 0 = whatever comes in, else specify. 
        dspdesc.read         = myDSPCallback; 
        dspdesc.userdata     = (void *)dirac; 

        result = system->createDSP(&dspdesc, &mydsp); 
        ERRCHECK(result); 
    } 

    /*
        Inactive by default.
    */
	static bool active = false;
    mydsp->setBypass(active);

    /*
        Main loop.
    */
    result = system->addDSP(mydsp, 0); 


    /*
        Main loop.
    */
    do
    {
        if (kbhit())
        {
            key = getch();

            switch (key)
            {
                case 'f' : 
                case 'F' : 
                {
                    active = !active;
                    mydsp->setBypass(active);
					if (!active)	printf("\nProcessing with Dirac\n");
					else			printf("\nBYPASS ON\n");
                    break;
                }
            }
        }

        system->update();

        Sleep(10);

    } while (key != 27);

    printf("\n");

    /*
        Shut down
    */
    result = sound->release();
    ERRCHECK(result);

    result = mydsp->release();
    ERRCHECK(result);

    result = system->close();
    ERRCHECK(result);
    result = system->release();
    ERRCHECK(result);

	DiracDestroy(dirac);
	
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

