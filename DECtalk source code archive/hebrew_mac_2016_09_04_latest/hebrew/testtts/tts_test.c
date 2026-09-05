#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AUGraph.h>
#include <AudioToolbox/AudioSession.h>

#include "ttsapi.h"

AUGraph outGraph;
int  sessionInitialized = 0;
int playStatus = 0;
volatile int reset = 0;
volatile int fooindex = 10000;
int count = 0;


short fake_audio[32] = {
  0, 0,
  6000, 6000,  
  9000, 9000,
  12000, 12000,
  14000, 14000,
  12000, 12000,
  9000, 9000, 
  6000, 6000,
  0, 0,
  -6000, -6000,
  -9000, -9000,
  -12000, -12000,
  -14000, -14000,
  -12000, -12000,
  -9000, -9000,
  -6000, -6000
};
 

OSStatus	CoreAudioCallbackFunction(void 				*inRefCon, 
				AudioUnitRenderActionFlags 	*ioActionFlags, 
				const AudioTimeStamp 		*inTimeStamp, 
				UInt32 						inBusNumber, 
				UInt32 						inNumberFrames, 
					  AudioBufferList 			*ioData)
{
  int i;
  short *dest=ioData->mBuffers[0].mData;
  int length = ioData->mBuffers[0].mDataByteSize/2;

  memset(ioData->mBuffers[0].mData,0,ioData->mBuffers[0].mDataByteSize);
	
  if (reset == 1)
    return 0;

  count++;
  if ((count & 0x001F) == 0)
    {
      fooindex++;
    }

  for (i=0;i<length; i+=16)
    {
      memcpy(&dest[i],fake_audio, 32);
    }
   
  return 0;
}

OSStatus DTAudioSessionSetActive(Boolean active)
{
  OSStatus err;
#ifdef _IOSSIMULATOR_
  return 0;
#else
  err = AudioSessionSetActive(active);
  printf("err = %d\n",err);
  return err;
#endif
}

#ifndef _IOSSIMULATOR_
void interruptionListenerCallback (void    *inUserData, UInt32  interruptionState ) 
{
  static Boolean interruptedOnPlayback = false;
    
    UInt32 sessionCategory = kAudioSessionCategory_MediaPlayback;  // 1
 
    if (interruptionState == kAudioSessionBeginInterruption) {          // 4
      
            interruptedOnPlayback = true;                     // 7
 
    } else if ((interruptionState == kAudioSessionEndInterruption) &&
                                    interruptedOnPlayback) { // 8

    AudioSessionSetProperty (                                      // 2
        kAudioSessionProperty_AudioCategory,                       // 3
        sizeof (sessionCategory),                                  // 4
        &sessionCategory                                           // 5
    );

        interruptedOnPlayback = false;
    }
}
#endif

MMRESULT TextToSpeechSetConfFileInfo(char *def_lang, char *langs, char *licenses, char *dict_location)
{
  return 0;
}

MMRESULT TextToSpeechStartup( LPTTS_HANDLE_T * pphTTS,  UINT uiDeviceNumber,
			      DWORD dwDeviceOptions,
			      VOID (*DtCallbackRoutine)(LONG,LONG,DWORD,UINT),
			      LONG dwTTSInstanceParameter)
{
  AudioComponentDescription iOUnitDescription;
  OSStatus err = noErr;
  AUNode ioNode;
  AudioUnit ioUnit;
  AudioUnitElement ioUnitOutputElement = 0;
  AURenderCallbackStruct input;
  AudioStreamBasicDescription streamFormat;
    Float64 preferredSampleRate = 11025.0;
    OSStatus activationResult = 0;
  OSStatus result = 0;
UInt32 sessionCategory = kAudioSessionCategory_MediaPlayback;    // 1
UInt32 maximumFramesPerSlice = 512;
UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;

#ifndef _IOSSIMULATOR_
  if (!sessionInitialized)
    {
err = AudioSessionInitialize (
    NULL,                            // 1
    NULL,                            // 2
    interruptionListenerCallback,    // 3
    NULL                       // 4
);
 if (err !=0)
   printf("session not initialized\n");
  sessionInitialized = 1;
}

#if 0
AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute,
			 sizeof (audioRouteOverride),&audioRouteOverride);
#endif

err = AudioSessionSetProperty (
    kAudioSessionProperty_AudioCategory,                        // 2
    sizeof (sessionCategory),                                   // 3
    &sessionCategory                                            // 4
);
 if (err !=0)
   printf("category not set\n");
 //#endif
 //#if 0
    err = AudioSessionSetProperty (
                             kAudioSessionProperty_PreferredHardwareSampleRate,    // 1
                             sizeof (preferredSampleRate),                                   // 3
                             &preferredSampleRate                                            // 4
                             );
 if (err !=0)
   printf("hw sample rate not set\n");
    //#endif
 
#endif    


  NewAUGraph(&outGraph);
 
    
    // I/O unit
  iOUnitDescription.componentType          = kAudioUnitType_Output;
  iOUnitDescription.componentSubType       = kAudioUnitSubType_RemoteIO;
  iOUnitDescription.componentManufacturer  = kAudioUnitManufacturer_Apple;
  iOUnitDescription.componentFlags         = 0;
  iOUnitDescription.componentFlagsMask     = 0;
  
    AUGraphAddNode(outGraph, &iOUnitDescription, &ioNode);

    // now do the wiring. The graph needs to be open before you call AUGraphNodeInfo
    AUGraphOpen(outGraph);
    
    AUGraphNodeInfo(outGraph, ioNode, 0, &ioUnit);
    

  input.inputProc = CoreAudioCallbackFunction;
  input.inputProcRefCon = &reset;
  
  err = AudioUnitSetProperty (ioUnit, 
			      kAudioUnitProperty_SetRenderCallback, 
			      kAudioUnitScope_Input,
			      0, 
			      &input, 
			      sizeof(input));
  

    err = AudioUnitSetProperty(ioUnit,
							   kAudioUnitProperty_SampleRate,
							   kAudioUnitScope_Input,
							   0,
							   &preferredSampleRate,
							   sizeof(preferredSampleRate));
    
    
  streamFormat.mSampleRate = 11025.0;		//	the sample rate of the audio stream
  streamFormat.mFormatID = kAudioFormatLinearPCM;	//	the specific encoding type of audio stream
  streamFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger 
 //   | kAudioFormatFlagsNativeEndian
    | kLinearPCMFormatFlagIsPacked;
 //   | kAudioFormatFlagIsNonInterleaved;
  streamFormat.mBytesPerPacket = 568;
  streamFormat.mFramesPerPacket = 142;
  streamFormat.mBytesPerFrame = 4;
  streamFormat.mChannelsPerFrame = 2;	
  streamFormat.mBitsPerChannel = 16;	
  
  err = AudioUnitSetProperty (ioUnit,
			      kAudioUnitProperty_StreamFormat,
			      kAudioUnitScope_Input,
			      0,
			      &streamFormat,
			      sizeof(AudioStreamBasicDescription));

/*    err = AudioUnitSetProperty (ioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Output,
                                1,
                                &streamFormat,
                                sizeof(AudioStreamBasicDescription));
*/
    
 
AudioUnitSetProperty (
    ioUnit,
    kAudioUnitProperty_MaximumFramesPerSlice,
    kAudioUnitScope_Global,
    0,                        // global scope always uses element 0
    &maximumFramesPerSlice,
    sizeof (maximumFramesPerSlice)
);
    
return 0;
  
}

MMRESULT TextToSpeechSpeakA( LPTTS_HANDLE_T phTTS, LPSTR text, DWORD force)
{
   OSStatus err = noErr;
  Boolean outIsInitialized;
  Boolean isRunning;
  reset = 0;
   if (outGraph) 
    {
      AUGraphIsInitialized(outGraph, &outIsInitialized);
      
      if(!outIsInitialized)
        AUGraphInitialize(outGraph);

        AUGraphIsRunning(outGraph, &isRunning);

        if(!isRunning)
	  {
	    DTAudioSessionSetActive (true);

            AUGraphStart(outGraph);
	  }
    }
   playStatus = 1;
    return 0;
}

MMRESULT TextToSpeechReset( LPTTS_HANDLE_T phTTS, BOOL foo )
{
   OSStatus err = noErr;
  Boolean outIsInitialized;
  Boolean isRunning;
  reset = 1;
       AUGraphIsRunning(outGraph, &isRunning);

        if(isRunning)
	  {
          err = AUGraphStop(outGraph);
	  }

      AUGraphIsInitialized(outGraph, &outIsInitialized);
      
      if(outIsInitialized)
          err = AUGraphUninitialize(outGraph);
      DTAudioSessionSetActive(false);
      playStatus = 0;

      return 0;
}

MMRESULT TextToSpeechShutdown( LPTTS_HANDLE_T phTTS)
{
   OSStatus err = noErr;
  Boolean outIsInitialized;
  Boolean isRunning;

  reset = 1;
        AUGraphIsRunning(outGraph, &isRunning);

        if(isRunning)
	  {
          err = AUGraphStop(outGraph);
	  }

      AUGraphIsInitialized(outGraph, &outIsInitialized);
      
      if(outIsInitialized)
          err = AUGraphUninitialize(outGraph);

      DTAudioSessionSetActive(false);
      
      DisposeAUGraph(outGraph);

      playStatus = 0;

      return 0;
}

MMRESULT TextToSpeechGetStatusEx(LPTTS_HANDLE_T phTTS, int *iPlayStatus, int *iCurIndex, unsigned int *uiInstanceParam)
{
  *iPlayStatus = playStatus;
  *iCurIndex = fooindex;
  *uiInstanceParam = 0;

  return 0;

}
