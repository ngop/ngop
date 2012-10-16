/*
 *   audio_thread.c
 */

// Modfied for ALSA output 5-May-2011, Mark A. Yoder

// Based on Basic PCM audio (http://www.suse.de/~mana/alsa090_howto.html#sect02)http://www.suse.de/~mana/alsa090_howto.html#sect03

//* Standard Linux headers **
#include     <stdio.h>                          // Always include stdio.h
#include     <stdlib.h>                         // Always include stdlib.h
#include     <fcntl.h>                          // Defines open, read, write methods
#include     <unistd.h>                         // Defines close and sleep methods
//#include     <sys/ioctl.h>                      // Defines driver ioctl method
//#include     <linux/soundcard.h>                // Defines OSS driver functions
#include     <string.h>                         // Defines memcpy
#include     <alsa/asoundlib.h>			// ALSA includes
#include     <time.h>
//* Application headers **
#include     "debug.h"                          // DBG and ERR macros
#include     "audio_thread.h"                   // Audio thread definitions
#include     "audio_input_output.h"             // Audio driver input and output functions

//* ALSA and Mixer devices **
//#define     SOUND_DEVICE     "plughw:0,0"	// This uses line in
//#define     SOUND_DEVICE     "plughw:1,0"	// This uses the PS EYE mikes

// ALSA device
#define     SOUND_DEVICE     "plughw:1,0"
#define     OUT_SOUND_DEVICE "plughw:0,0"

//* The sample rate of the audio codec **
#define     SAMPLE_RATE      48000

//* The gain (0-100) of the left channel **
#define     LEFT_GAIN        100

//* The gain (0-100) of the right channel **
#define     RIGHT_GAIN       100

//*  Parameters for audio thread execution **
#define     BLOCKSIZE        48000


//*******************************************************************************
//*  audio_thread_fxn                                                          **
//*******************************************************************************
//*  Input Parameters:                                                         **
//*      void *envByRef    --  a pointer to an audio_thread_env structure      **
//*                            as defined in audio_thread.h                    **
//*                                                                            **
//*          envByRef.quit -- when quit != 0, thread will cleanup and exit     **
//*                                                                            **
//*  Return Value:                                                             **
//*      void *            --  AUDIO_THREAD_SUCCESS or AUDIO_THREAD_FAILURE as **
//*                            defined in audio_thread.h                       **
//*******************************************************************************
void *audio_thread_fxn( void *envByRef )
{

// Variables and definitions
// *************************

    // Thread parameters and return value
    audio_thread_env * envPtr = envByRef;                  // < see above >
    void             * status = AUDIO_THREAD_SUCCESS;      // < see above >
    // The levels of initialization for initMask

    #define     INPUT_ALSA_INITIALIZED      0x1
    #define     INPUT_BUFFER_ALLOCATED      0x2
    #define     OUTPUT_ALSA_INITIALIZED     0x4
    #define     OUTPUT_BUFFER_ALLOCATED     0x8

    unsigned  int   initMask =  0x0;	// Used to only cleanup items that were init'd

    // Input and output driver variables
    //FILE	* inputFile = NULL;		// Input file pointer (i.e. handle)
    snd_pcm_t	*pcm_output_handle;
    snd_pcm_t	*pcm_capture_handle;		// Handle for the PCM device
    snd_pcm_uframes_t exact_bufsize;		// bufsize is in frames.  Each frame is 4 bytes

    int   blksize = BLOCKSIZE;			// Raw input or output frame size in bytes
    char *outputBuffer = NULL;
    char *outputBuffer2 = NULL;			// Output buffer for driver to read from
	char *playingBuffer = NULL;
// Thread Create Phase -- secure and initialize resources
// ******************************************************

    // Setup audio input device
    // ************************
    // Open an ALSA device channel for audio input
    exact_bufsize = blksize/BYTESPERFRAME;

    if( audio_io_setup( &pcm_capture_handle, SOUND_DEVICE, SAMPLE_RATE, 
			SND_PCM_STREAM_CAPTURE, &exact_bufsize ) == AUDIO_FAILURE )
    {
        ERR( "Audio_input_setup failed in audio_thread_fxn\n\n" );
        status = AUDIO_THREAD_FAILURE;
        goto cleanup;
    }
    DBG( "exact_bufsize = %d\n", (int) exact_bufsize);

    // Record that input OSS device was opened in initialization bitmask
    initMask |= INPUT_ALSA_INITIALIZED;

    blksize = exact_bufsize*BYTESPERFRAME;

    // Initialize audio output device
    // ******************************
    // Initialize the output ALSA device
    DBG( "pcm_output_handle before audio_output_setup = %d\n", (int) pcm_output_handle);
    exact_bufsize = blksize/BYTESPERFRAME;
    DBG( "Requesting bufsize = %d\n", (int) exact_bufsize);
    if( audio_io_setup( &pcm_output_handle, OUT_SOUND_DEVICE, SAMPLE_RATE, 
			SND_PCM_STREAM_PLAYBACK, &exact_bufsize) == AUDIO_FAILURE )
    {
        ERR( "audio_output_setup failed in audio_thread_fxn\n" );
        status = AUDIO_THREAD_FAILURE;
        goto  cleanup ;
    }
	blksize = exact_bufsize;
	DBG( "pcm_output_handle after audio_output_setup = %d\n", (int) pcm_output_handle);
	DBG( "blksize = %d, exact_bufsize = %d\n", blksize, (int) exact_bufsize);

    // Record that input ALSA device was opened in initialization bitmask
    initMask |= OUTPUT_ALSA_INITIALIZED;

    // Create output buffer to write from into ALSA output device
    if( ( outputBuffer = malloc( blksize ) ) == NULL )
    {
        ERR( "Failed to allocate memory for output block (%d)\n", blksize );
        status = AUDIO_THREAD_FAILURE;
        goto  cleanup ;
    }
	// Create 5 seconds of output buffer to circularly update
    if( ( outputBuffer2 = malloc( 500*blksize ) ) == NULL )
    {
        ERR( "Failed to allocate memory for output block (%d)\n", blksize );
        status = AUDIO_THREAD_FAILURE;
        goto  cleanup ;
    }
	// This adds the two audio files together for play when enter is pressed
    if( ( playingBuffer = malloc( 500*blksize ) ) == NULL )
    {
        ERR( "Failed to allocate memory for output block (%d)\n", blksize );
        status = AUDIO_THREAD_FAILURE;
        goto  cleanup ;
    }

    DBG( "Allocated output audio buffer of size %d to address %p\n", blksize, outputBuffer );

    // Record that the output buffer was allocated in initialization bitmask
    initMask |= OUTPUT_BUFFER_ALLOCATED;


// Thread Execute Phase -- perform I/O and processing
// **************************************************
    // Get things started by sending some silent buffers out.
    int i;

    memset(outputBuffer, 0, blksize);		// Clear the buffer
    memset(outputBuffer2, 0, blksize);		// Clear the buffer
    memset(playingBuffer, 0, blksize);		// Clear the buffer
    for(i=0; i<4; i++) {
	snd_pcm_readi(pcm_capture_handle, outputBuffer, blksize/BYTESPERFRAME);
	if ((snd_pcm_writei(pcm_output_handle, outputBuffer,
		exact_bufsize)) < 0) {
	    snd_pcm_prepare(pcm_output_handle);
	    ERR( "<<<Pre Buffer Underrun >>> \n");
	      }
	}


//
// Processing loop
//
    DBG( "Entering audio_thread_fxn processing loop\n" );

    int count = 0;			//count for the specific audio frame being played
	int count2 = 0;			//count to 500 to keep track of circular loop
	int flag = 0;			//goes high to indicate that 500 frames are captured and ready to be played back
	int playaudiocheck = 1; //goes high to indicate that 500 frames have been played and are ready for capture and play again
    while( !envPtr->quit )
    {

        // Read capture buffer from ALSA input device

        if( snd_pcm_readi(pcm_capture_handle, outputBuffer, blksize/BYTESPERFRAME) < 0 )
        {
	    snd_pcm_prepare(pcm_capture_handle);
        }

        // Write output buffer into ALSA output device
		if(!audiocaller || !flag){
			//If not playing last five seconds of audio or the buffer isn't filled to the 5 second capacity, then we want to continue to play audio regularly
        	if (snd_pcm_writei(pcm_output_handle, outputBuffer, blksize/BYTESPERFRAME) < 0) {
            	snd_pcm_prepare(pcm_output_handle);
	    		// Send out an extra blank buffer if there is an underrun and try again.
	    memset(outputBuffer, 0, blksize);		// Clear the buffer
	    snd_pcm_writei(pcm_output_handle, outputBuffer, exact_bufsize);
      		}
		}

		//Now we want to check to see if the buffer is full and the audio needs to be played back as well as if the audio has finished playing back
		if(audiocaller && (count2 == 499) && playaudiocheck){
		// This will determine that we are playing audio from this point as well we will say not to come back into this loop until the audio has been played and set count2 back to 0 for playing from the beginning
			count2 = 0;
			flag = 1;
			playaudiocheck = 0;
		}

		//Now we will play audio only if the flag says that we have a full buffer
		if(audiocaller && flag){
		//We have to add the signals from what sound is currently happening to the sounds that happened over the last 5 seconds together and then have them playback byter for byte
		for(i=0; i < blksize; i++){
			playingBuffer[i] = outputBuffer2[(count2*blksize)+i] + outputBuffer[i];
		}
		
		//When the audio has been formed it them plays back through the speakers with a snd_pcm_writei call for playingBuffer
		if (snd_pcm_writei(pcm_output_handle, playingBuffer, blksize/BYTESPERFRAME) < 0){ 
		   	snd_pcm_prepare(pcm_output_handle);
		    // Send out an extra blank buffer if there is an underrun and try again.
	        memset(playingBuffer, 0, blksize);		// Clear the buffer
	        snd_pcm_writei(pcm_output_handle, playingBuffer, blksize/BYTESPERFRAME);
        }
		//We now need to detect whether or not the buffer has come to an end and if it has we need to let the program know that we don't want to continue playing both audios together as well as let the buffer know it can be recalled to play again
		if(count2 > 498){
			audiocaller = 0;
			playaudiocheck = 1;
			flag = 0;
		}

}
	//Copies the recorded audio into the new buffer at the specified index
	memcpy(outputBuffer2 + (count2*blksize), outputBuffer, blksize);
	count++;
	count2++;
	count2 = count2 % 500;

	//DBG("%d, ", count++);
    }
    DBG("\n");

    DBG( "Exited audio_thread_fxn processing loop\n" );


// Thread Delete Phase -- free up resources allocated by this file
// ***************************************************************

cleanup:

    DBG( "Starting audio thread cleanup to return resources to system\n" );


    // Close output ALSA device
    if( initMask & OUTPUT_ALSA_INITIALIZED )
        if( audio_io_cleanup( pcm_output_handle ) != AUDIO_SUCCESS )
        {
            ERR( "audio_output_cleanup() failed for file descriptor %d\n", (int)pcm_output_handle );
            status = AUDIO_THREAD_FAILURE;
        }

    // Free allocated buffers
    // **********************

    // Free output buffer
    if( initMask & OUTPUT_BUFFER_ALLOCATED )
    {
        free( outputBuffer );
		free( outputBuffer2 );
		free( playingBuffer );
        DBG( "Freed audio output buffer at location %p\n", outputBuffer );
    }

    // Return from audio_thread_fxn function
    // *************************************
	
    // Return the status at exit of the thread's execution
    DBG( "Audio thread cleanup complete. Exiting audio_thread_fxn\n" );
    return status;
}
