/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* This is a JNI example where we use native methods to play sounds
 * using OpenSL ES. See the corresponding Java source file located at:
 *
 *   src/com/example/nativeaudio/NativeAudio/NativeAudio.java
 */

#include <assert.h>
#include <jni.h>
#include <string.h>
#include <pthread.h>

// for __android_log_print(ANDROID_LOG_INFO, "YourApp", "formatted message");
#include <android/log.h>
#define APP "NativeAudio"

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

// threads
pthread_t playerThread;

// engine interfaces
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

// output mix interfaces
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

// player interfaces
static SLObjectItf playerObject = NULL;
static SLPlayItf playerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLEffectSendItf PlayerEffectSend;

// recorder interfaces
static SLObjectItf recorderObject = NULL;
static SLRecordItf recorderRecord;
static SLAndroidSimpleBufferQueueItf bqRecorderBufferQueue;

// buffer
#define FRAMES_PER_BUFFER (1024)
#define NUMBER_OF_BUFFERS (1)
#define BUFFER_SIZE (1024 * sizeof(uint16_t))
static uint16_t realtimeBuffer[FRAMES_PER_BUFFER * NUMBER_OF_BUFFERS];
static uint16_t playFrame = 0;

static unsigned recorderSize = 0;
static SLmilliHertz recorderSR;

// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static short *nextBuffer;
static unsigned nextSize;
static int nextCount;

// prototypes
void createEngine();
void createAudioRecorder();
void createAudioPlayer();

/*--------------------------------------------------------------------------------------------------------------------------------------
 * functions to be called from java code
 *------------------------------------------------------------------------------------------------------------------------------------*/

// create the engine and output mix objects
void Java_com_alex_android_ampsim_Main_startAudioLoopback(JNIEnv* env, jclass clazz)
{
	createEngine();
	createAudioRecorder();
	createAudioPlayer();
}

// enable reverb on the buffer queue player
jboolean Java_com_alex_android_ampsim_Main_enableReverb(JNIEnv* env, jclass clazz, jboolean enabled)
{
	SLresult result;

	// we might not have been able to add environmental reverb to the output mix
	if (NULL == outputMixEnvironmentalReverb)
	{
		return JNI_FALSE;
	}

	result = (*PlayerEffectSend)->EnableEffectSend(PlayerEffectSend, outputMixEnvironmentalReverb, (SLboolean) enabled, (SLmillibel) 0);
	// and even if environmental reverb was present, it might no longer be available
	if (SL_RESULT_SUCCESS != result)
	{
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

// shut down the native audio system
void Java_com_alex_android_ampsim_Main_shutdown(JNIEnv* env, jclass clazz)
{

	// destroy buffer queue audio player object, and invalidate all associated interfaces
	if (playerObject != NULL )
	{
		// kill player thread
		pthread_kill(playerThread, 0);

		(*playerObject)->Destroy(playerObject);
		playerObject = NULL;
		playerObject = NULL;
		bqPlayerBufferQueue = NULL;
		PlayerEffectSend = NULL;
//		PlayerMuteSolo = NULL;
//		PlayerVolume = NULL;
	}

	// destroy audio recorder object, and invalidate all associated interfaces
	if (recorderObject != NULL )
	{
		(*recorderObject)->Destroy(recorderObject);
		recorderObject = NULL;
		recorderRecord = NULL;
		bqRecorderBufferQueue = NULL;
	}

	// destroy output mix object, and invalidate all associated interfaces
	if (outputMixObject != NULL )
	{
		(*outputMixObject)->Destroy(outputMixObject);
		outputMixObject = NULL;
		outputMixEnvironmentalReverb = NULL;
	}

	// destroy engine object, and invalidate all associated interfaces
	if (engineObject != NULL )
	{
		(*engineObject)->Destroy(engineObject);
		engineObject = NULL;
		engineEngine = NULL;
	}
}

/*--------------------------------------------------------------------------------------------------------------------------------------
 * c functions
 *------------------------------------------------------------------------------------------------------------------------------------*/

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void* context)
{
	assert(bq == bqPlayerBufferQueue);
	assert(NULL == context);
	SLresult result;

	// PLAY!
	__android_log_print(ANDROID_LOG_INFO, APP, "emptying Buffer No.: %u", playFrame);
	result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, &realtimeBuffer[0], BUFFER_SIZE);
}

// this callback handler is called every time a buffer finishes recording
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	assert(bq == bqRecorderBufferQueue);
	assert(NULL == context);
	SLresult result;

	// RECORD!
	__android_log_print(ANDROID_LOG_INFO, APP, "filling Buffer No.: %u", playFrame);
	result = (*bqRecorderBufferQueue)->Enqueue(bqRecorderBufferQueue, &realtimeBuffer[0], BUFFER_SIZE);
}

void createEngine()
{
	SLresult result;

	// create engine
	result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL );
	assert(SL_RESULT_SUCCESS == result);

	// realize the engine
	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE );
	assert(SL_RESULT_SUCCESS == result);

	// get the engine interface, which is needed in order to create other objects
	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
	assert(SL_RESULT_SUCCESS == result);

	// create output mix, with environmental reverb specified as a non-required interface
	const SLInterfaceID ids[1] = { SL_IID_ENVIRONMENTALREVERB };
	const SLboolean req[1] = { SL_BOOLEAN_FALSE };

	result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
	assert(SL_RESULT_SUCCESS == result);

	// realize the output mix
	result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE );
	assert(SL_RESULT_SUCCESS == result);

	// get the environmental reverb interface
	// this could fail if the environmental reverb effect is not available,
	// either because the feature is not present, excessive CPU load, or
	// the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
	//result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb); TODO
	if (SL_RESULT_SUCCESS == result)
	{
		//result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &reverbSettings); TODO
		__android_log_print(ANDROID_LOG_INFO, APP, "audio engine successfully created");
	}
	else
	{
		__android_log_print(ANDROID_LOG_INFO, APP, "audio engine could not be created!");
	}
}

// create audio recorder
void createAudioRecorder()
{
	SLresult result;

	// configure audio source
	SLDataLocator_IODevice loc_dev = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
	SLDataSource audioSrc = { &loc_dev, NULL };

	// configure audio sink
	SLDataLocator_AndroidSimpleBufferQueue loc_bq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
	SLDataFormat_PCM format_pcm = { SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN };
	SLDataSink audioSnk = { &loc_bq, &format_pcm };

	// create audio recorder
	// (requires the RECORD_AUDIO permission)
	const SLInterfaceID id[1] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
	const SLboolean req[1] = { SL_BOOLEAN_TRUE };
	result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audioSrc, &audioSnk, 1, id, req);
	if (SL_RESULT_SUCCESS != result)
	{
		__android_log_print(ANDROID_LOG_INFO, APP, "creating recorder failed");
		return;
	}

	// realize the audio recorder
	result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE );
	if (SL_RESULT_SUCCESS != result)
	{
		__android_log_print(ANDROID_LOG_INFO, APP, "realizing recorder failed");
		return;
	}

	// get the record interface
	result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
	assert(SL_RESULT_SUCCESS == result);

	// get the buffer queue interface
	result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bqRecorderBufferQueue);
	assert(SL_RESULT_SUCCESS == result);

	// register callback on the buffer queue
	result = (*bqRecorderBufferQueue)->RegisterCallback(bqRecorderBufferQueue, bqRecorderCallback, NULL );
	assert(SL_RESULT_SUCCESS == result);

	// enqueue first empty buffer to be filled by the recorder
	result = (*bqRecorderBufferQueue)->Enqueue(bqRecorderBufferQueue, &realtimeBuffer[0], BUFFER_SIZE);
	assert(SL_RESULT_SUCCESS == result);

	// start recording
	result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING );
	assert(SL_RESULT_SUCCESS == result);

	__android_log_print(ANDROID_LOG_INFO, APP, "successfully created recorder! now recording...");
}

void* PlayerThread(void* ptr)
{
	__android_log_print(ANDROID_LOG_INFO, APP, "entering player thread...");

	SLresult result;

	// configure audio source
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
	SLDataFormat_PCM format_pcm = { SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN };
	SLDataSource audioSrc = { &loc_bufq, &format_pcm };

	// configure audio sink
	SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, outputMixObject };
	SLDataSink audioSnk = { &loc_outmix, NULL };

	// create audio player
	const SLInterfaceID ids[1] = { SL_IID_BUFFERQUEUE };
	const SLboolean req[1] = { SL_BOOLEAN_TRUE };

	result = (*engineEngine)->CreateAudioPlayer(engineEngine, &playerObject, &audioSrc, &audioSnk, 1, ids, req);
	assert(SL_RESULT_SUCCESS == result);

	// realize the audio recorder
	result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE );
	assert(SL_RESULT_SUCCESS == result);

	// get the record interface
	result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay);
	assert(SL_RESULT_SUCCESS == result);

	// get the buffer queue interface
	result = (*playerObject)->GetInterface(playerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bqPlayerBufferQueue);
	assert(SL_RESULT_SUCCESS == result);

	// register callback on the buffer queue
	result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL );
	assert(SL_RESULT_SUCCESS == result);

	// let the player start
	result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PLAYING );
	assert(SL_RESULT_SUCCESS == result);

	static int lazyInit = SL_BOOLEAN_FALSE;
	if (lazyInit == SL_BOOLEAN_FALSE )
	{
		lazyInit = SL_BOOLEAN_TRUE;

		// PLAY!
		__android_log_print(ANDROID_LOG_INFO, APP, "initial play");
		result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, &realtimeBuffer[0], BUFFER_SIZE);
	}

	// thread stuff...
	while (1)
	{
	}
	retun NULL;
}

// create audio recorder
void createAudioPlayer()
{
	pthread_create(&playerThread, NULL, PlayerThread, NULL );
}

