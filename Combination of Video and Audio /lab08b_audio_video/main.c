/*
 *   main.c
 *
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2005
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */

// Standard Linux headers
#include     <stdio.h>	// Always include this header
#include     <stdlib.h>	// Always include this header
#include     <signal.h>	// Defines signal-handling functions (i.e. trap Ctrl-C)
#include     <unistd.h>
#include     <pthread.h>

// Application headers
#include     "debug.h"
#include     "video_thread.h"
#include     "audio_thread.h"

/* Global thread environments */
video_thread_env video_env = {0};
audio_thread_env audio_env = {0};

/* Store previous signal handler and call it */
void (*pSigPrev)(int sig);

/* Callback called when SIGINT is sent to the process (Ctrl-C) */
void signal_handler(int sig) {
    DBG( "Ctrl-C pressed, cleaning up and exiting..\n" );

    video_env.quit = 1;
#ifdef _DEBUG_
	sleep(1);
#endif
    audio_env.quit = 1;

    if( pSigPrev != NULL )
        (*pSigPrev)( sig );
}

//*****************************************************************************
//*  main
//*****************************************************************************
int main(int argc, char *argv[])
{
/* The levels of initialization for initMask */
#define VIDEOTHREADATTRSCREATED 0x1
#define VIDEOTHREADCREATED      0x2
#define AUDIOTHREADATTRSCREATED 0x4
#define AUDIOTHREADCREATED      0x8

    unsigned int    initMask  = 0;
    int             status    = EXIT_SUCCESS;
    pthread_t audioThread, videoThread;
    void *audioThreadReturn, *videoThreadReturn;
    pthread_attr_t audioThreadAttrs;
    struct sched_param audioThreadParams;

    /* Set the signal callback for Ctrl-C */
    pSigPrev = signal( SIGINT, signal_handler );

    /* Make video frame buffer visible */
    system("cd ..; ./vid1Show");

    /* Create a thread for video */
    DBG( "Creating video thread\n" );
    printf( "\tPress Ctrl-C to exit\n" );

    if(pthread_create(&videoThread, SCHED_OTHER, video_thread_fxn, (void *) &video_env)){
    	ERR("pthread create failed for video thread\n");
	status = EXIT_FAILURE;
	goto cleanup;
}

    initMask |= VIDEOTHREADCREATED;

#ifdef _DEBUG_
	sleep(1);
#endif

pthread_attr_init(&audioThreadAttrs);
pthread_attr_setinheritsched(&audioThreadAttrs, PTHREAD_EXPLICIT_SCHED);
pthread_attr_setschedpolicy(&audioThreadAttrs, SCHED_RR);
audioThreadParams.sched_priority=99;
pthread_attr_setschedparam(&audioThreadAttrs, &audioThreadParams);

   initMask |= AUDIOTHREADATTRSCREATED;

if(pthread_create(&audioThread, &audioThreadAttrs, audio_thread_fxn, (void *) &audio_env)){
    	ERR("pthread create failed for audio thread\n");
	status = EXIT_FAILURE;
	goto cleanup;
}

    initMask |= AUDIOTHREADCREATED;

cleanup:
    /* Make video frame buffer invisible */
    
    pthread_join(videoThread, (void **) &videoThreadReturn);
    pthread_join(audioThread, (void **) &audioThreadReturn);

    system("cd ..; ./resetVideo");

    exit( status );
}

