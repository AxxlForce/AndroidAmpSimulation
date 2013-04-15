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

// threads
pthread_t playerThread;
pthread_t recorderThread;

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

static SLDataFormat_PCM FORMAT_PCM = { SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16, SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN };

// buffer
#define BUFFER_SIZE 512
#define BUFFER_SIZE_IN_SAMPLES (BUFFER_SIZE / 2)

static uint16_t inputBuffer[2][BUFFER_SIZE];
static uint16_t outputBuffer[2][BUFFER_SIZE];

static uint16_t recordingShift = 0;
static uint16_t playingShift = 0;

// prototypes
void createEngine();
void createAudioRecorder();
void createAudioPlayer();
void recordSingleBuffer(const void *pBuffer, SLuint32 size);

/*--------------------------------------------------------------------------------------------------------------------------------------
 * functions to be called from java code
 *------------------------------------------------------------------------------------------------------------------------------------*/

// create the engine and output mix objects
void Java_com_alex_android_ampsim_jni_JNIWrapper_createEngine(JNIEnv* env, jclass clazz)
{
	createEngine();
	createAudioRecorder();
	createAudioPlayer();
}

void Java_com_alex_android_ampsim_jni_JNIWrapper_runEngine(JNIEnv* env, jclass clazz)
{
	// kickoff loopback!
	recordSingleBuffer(&realtimeBuffer[0], BUFFER_SIZE);
}

jfloat Java_com_alex_android_ampsim_jni_JNIWrapper_getLatency(JNIEnv* env, jclass clazz)
{
	return SAMPLES_PER_BUFFER / 4.41;
}

// enable reverb on the buffer queue player
jboolean Java_com_alex_android_ampsim_jni_JNIWrapper_enableReverb(JNIEnv* env, jclass clazz, jboolean enabled)
{
	SLresult result;

	// we might not have been able to add environmental reverb to the output mix
	if ((NULL == outputMixEnvironmentalReverb) || (PlayerEffectSend == NULL ))
	{
		return JNI_FALSE;
		__android_log_print(ANDROID_LOG_INFO, APP, "reverb failed!");
	}

	result = (*PlayerEffectSend)->EnableEffectSend(PlayerEffectSend, outputMixEnvironmentalReverb, (SLboolean) enabled, (SLmillibel) 0);
	// and even if environmental reverb was present, it might no longer be available
	if (SL_RESULT_SUCCESS != result)
	{
		__android_log_print(ANDROID_LOG_INFO, APP, "reverb failed!");
		return JNI_FALSE;
	}
	return JNI_TRUE;
}

// shut down the native audio system
void Java_com_alex_android_ampsim_jni_JNIWrapper_shutdown(JNIEnv* env, jclass clazz)
{
	// destroy buffer queue audio player object, and invalidate all associated interfaces
	if (playerObject != NULL )
	{
		// kill player thread
		pthread_kill(playerThread, 0);
		pthread_kill(recorderThread, 0);

		(*playerObject)->Destroy(playerObject);
		playerObject = NULL;
		playerObject = NULL;
		bqPlayerBufferQueue = NULL;
		PlayerEffectSend = NULL;
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
void recordSingleBuffer(const void *pBuffer, SLuint32 size)
{
	// RECORD!
	//__android_log_print(ANDROID_LOG_INFO, APP, "recording to Buffer at address: %X", pBuffer);
	(*bqRecorderBufferQueue)->Enqueue(bqRecorderBufferQueue, pBuffer, size);
}

void playSingleBuffer(const void *pBuffer, SLuint32 size)
{
	// Play!
	//__android_log_print(ANDROID_LOG_INFO, APP, "playing Buffer at address: %X", pBuffer);
	(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, pBuffer, size);
}

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void* context)
{
	//if (isRunning)
	{
		++playingShift;
		playSingleBuffer(&realtimeBuffer[playingShift], BUFFER_SIZE);
	}
}

// this callback handler is called every time a buffer finishes recording
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	//if (isRunning)
	{
		if (playingShift == 0)
			playSingleBuffer(&realtimeBuffer[0], BUFFER_SIZE);

		++recordingShift;
		recordSingleBuffer(&realtimeBuffer[recordingShift], BUFFER_SIZE);
	}
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
	result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb);
	if (SL_RESULT_SUCCESS == result)
	{
		result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(outputMixEnvironmentalReverb, &reverbSettings);
		__android_log_print(ANDROID_LOG_INFO, APP, "audio engine successfully created");
	}
	else
	{
		__android_log_print(ANDROID_LOG_INFO, APP, "audio engine could not be created!");
	}
}

void* RecorderThread(void* ptr)
{
	SLresult result;

	// configure audio source
	SLDataLocator_IODevice loc_dev = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL };
	SLDataSource audioSrc = { &loc_dev, NULL };

	// configure audio sink
	SLDataLocator_AndroidSimpleBufferQueue loc_bq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
	SLDataFormat_PCM format_pcm = FORMAT_PCM;
	SLDataSink audioSnk = { &loc_bq, &format_pcm };

	// create audio recorder
	// (requires the RECORD_AUDIO permission)
	const SLInterfaceID id[1] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
	const SLboolean req[1] = { SL_BOOLEAN_TRUE };
	result = (*engineEngine)->CreateAudioRecorder(engineEngine, &recorderObject, &audioSrc, &audioSnk, 1, id, req);

	// realize the audio recorder
	result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE );

	// get the record interface
	result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
	assert(SL_RESULT_SUCCESS == result);

	// get the buffer queue interface
	result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bqRecorderBufferQueue);
	assert(SL_RESULT_SUCCESS == result);

	// register callback on the buffer queue
	result = (*bqRecorderBufferQueue)->RegisterCallback(bqRecorderBufferQueue, bqRecorderCallback, NULL );
	assert(SL_RESULT_SUCCESS == result);

	// start recording
	result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING );
	assert(SL_RESULT_SUCCESS == result);

	// thread stuff...
	while (1)
	{
	}
	return NULL;
}

// create audio recorder
void createAudioRecorder()
{
	pthread_create(&recorderThread, NULL, RecorderThread, NULL );
}

void* PlayerThread(void* ptr)
{
	SLresult result;

	// configure audio source
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
	SLDataFormat_PCM format_pcm = FORMAT_PCM;
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

	// get the player interface
	result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay);
	assert(SL_RESULT_SUCCESS == result);

	// get the buffer queue interface
	result = (*playerObject)->GetInterface(playerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bqPlayerBufferQueue);
	assert(SL_RESULT_SUCCESS == result);

	// register callback on the buffer queue
	result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL );
	assert(SL_RESULT_SUCCESS == result);

	// get the effect send interface
	result = (*playerObject)->GetInterface(playerObject, SL_IID_EFFECTSEND, &PlayerEffectSend);
	assert(SL_RESULT_SUCCESS == result);

	// let the player start
	result = (*playerPlay)->SetPlayState(playerPlay, SL_PLAYSTATE_PLAYING );
	assert(SL_RESULT_SUCCESS == result);

	// thread stuff...
	while (1)
	{
	}
	return NULL ;
}

// create audio recorder
void createAudioPlayer()
{
	pthread_create(&playerThread, NULL, PlayerThread, NULL );
}

