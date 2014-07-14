/*
 
 CsoundObj.m:
 
 Copyright (C) 2011 Steven Yi, Victor Lazzarini
 
 This file is part of Csound for iOS.
 
 The Csound for iOS Library is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 Csound is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with Csound; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 
 */

#import "CsoundObj.h"
#import "CsoundValueCacheable.h"
//AURE #import "CsoundMIDI.h"

OSStatus  Csound_Render(void *inRefCon,
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp,
                        UInt32 dump,
                        UInt32 inNumberFrames,
                        AudioBufferList *ioData);
void InterruptionListener(void *inClientData, UInt32 inInterruption);

@interface CsoundObj()

-(void)runCsound:(NSString*)csdFilePath;
-(void)runCsoundToDisk:(NSArray*)paths;

@end

@implementation CsoundObj

@synthesize outputURL;
@synthesize midiInEnabled = mMidiInEnabled;

- (id)init
{
    self = [super init];
    if (self) {
		mCsData.shouldMute = false;
        _valuesCache = [[NSMutableArray alloc] init];
        completionListeners = [[NSMutableArray alloc] init];
        mMidiInEnabled = NO;
        _useAudioInput = NO;
    }
    
    return self;
}

-(void)addValueCacheable:(id<CsoundValueCacheable>)valueCacheable {
    if (valueCacheable != nil) {
        [_valuesCache addObject:valueCacheable];
    }
}

-(void)removeValueCaheable:(id<CsoundValueCacheable>)valueCacheable {
	if (valueCacheable != nil && [_valuesCache containsObject:valueCacheable]) {
		[_valuesCache removeObject:valueCacheable];
	}
}

#pragma mark -

static void messageCallback(CSOUND *cs, int attr, const char *format, va_list valist)
{
	@autoreleasepool {
		CsoundObj *obj = (__bridge CsoundObj *)(csoundGetHostData(cs));
		Message info;
		info.cs = cs;
		info.attr = attr;
		info.format = format;
        va_copy(info.valist,valist);
		NSValue *infoObj = [NSValue value:&info withObjCType:@encode(Message)];
		[obj performSelector:@selector(performMessageCallback:) withObject:infoObj];
	}
}

- (void)setMessageCallback:(SEL)method withListener:(id)listener
{
	self.mMessageCallback = method;
	mMessageListener = listener;
}

- (void)performMessageCallback:(NSValue *)infoObj
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
	[mMessageListener performSelector:_mMessageCallback withObject:infoObj];
#pragma clang diagnostic pop
}

#pragma mark -

-(void)sendScore:(NSString *)score {
    if (mCsData.cs != NULL) {
        csoundReadScore(mCsData.cs, (char*)[score cStringUsingEncoding:NSASCIIStringEncoding]);
    }
}

#pragma mark -

-(void)addCompletionListener:(id<CsoundObjCompletionListener>)listener {
    [completionListeners addObject:listener];
}


-(CSOUND*)getCsound {
    if (!mCsData.running) {
        return NULL;
    }
    return mCsData.cs;
}

-(AudioUnit*)getAudioUnit {
    if (!mCsData.running) {
        return NULL;
    }
    return mCsData.aunit;
}

-(float*)getInputChannelPtr:(NSString*)channelName channelType:(controlChannelType)channelType
{
    float *value;
    csoundGetChannelPtr(mCsData.cs, &value, [channelName cStringUsingEncoding:NSASCIIStringEncoding],
                        channelType | CSOUND_INPUT_CHANNEL);
    return value;
}

-(float*)getOutputChannelPtr:(NSString *)channelName channelType:(controlChannelType)channelType
{
	float *value;
	csoundGetChannelPtr(mCsData.cs, &value, [channelName cStringUsingEncoding:NSASCIIStringEncoding],
                        channelType | CSOUND_OUTPUT_CHANNEL);
	return value;
}

-(NSData*)getOutSamples {
    if (!mCsData.running) {
        return nil;
    }
    CSOUND* csound = [self getCsound];
    float* spout = csoundGetSpout(csound);
    int nchnls = csoundGetNchnls(csound);
    int ksmps = csoundGetKsmps(csound);
    NSData* data = [NSData dataWithBytes:spout length:(nchnls * ksmps * sizeof(MYFLT))];
    return data;
}

-(int)getNumChannels {
    if (!mCsData.running) {
        return -1;
    }
    return csoundGetNchnls(mCsData.cs);
}
-(int)getKsmps {
    if (!mCsData.running) {
        return -1;
    }
    return csoundGetKsmps(mCsData.cs);
}

#pragma mark Csound Code



OSStatus  Csound_Render(void *inRefCon,
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp,
                        UInt32 dump,
                        UInt32 inNumberFrames,
                        AudioBufferList *ioData)
{
    csdata *cdata = (csdata *) inRefCon;
    int ret = cdata->ret, nchnls = cdata->nchnls;
    float coef = (float) INT_MAX / csoundGet0dBFS(cdata->cs);
    CSOUND *cs = cdata->cs;
    
    int i,j,k;
    int slices = inNumberFrames/csoundGetKsmps(cs);
    int ksmps = csoundGetKsmps(cs);
    MYFLT *spin = csoundGetSpin(cs);
    MYFLT *spout = csoundGetSpout(cs);
    AudioUnitSampleType *buffer;
    
    AudioUnitRender(*cdata->aunit, ioActionFlags, inTimeStamp, 1, inNumberFrames, ioData);
    
    NSMutableArray* cache = cdata->valuesCache;
    
    for(i=0; i < slices; i++){
		
		for (int i = 0; i < cache.count; i++) {
			id<CsoundValueCacheable> cachedValue = [cache objectAtIndex:i];
			[cachedValue updateValuesToCsound];
		}
        
		/* performance */
        if(cdata->useAudioInput) {
            for (k = 0; k < nchnls; k++){
                buffer = (AudioUnitSampleType *) ioData->mBuffers[k].mData;
                for(j=0; j < ksmps; j++){
                    spin[j*nchnls+k] =(1./coef)*buffer[j+i*ksmps];
                }
            }
        }
        if(!ret) {
            ret = csoundPerformKsmps(cs);
        } else {
            cdata->running = false;
        }
		
		for (k = 0; k < nchnls; k++) {
			buffer = (AudioUnitSampleType *) ioData->mBuffers[k].mData;
			if (cdata->shouldMute == false) {
				for(j=0; j < ksmps; j++){
					buffer[j+i*ksmps] = (AudioUnitSampleType) lrintf(spout[j*nchnls+k]*coef) ;
				}
			} else {
				memset(buffer, 0, sizeof(AudioUnitSampleType) * inNumberFrames);
			}
		}
        
		
		for (int i = 0; i < cache.count; i++) {
			id<CsoundValueCacheable> cachedValue = [cache objectAtIndex:i];
			[cachedValue updateValuesFromCsound];
		}
		
    }
	
    /* AURE
	// Write to file.
	if (cdata->shouldRecord) {
		OSStatus err = ExtAudioFileWriteAsync(cdata->file, inNumberFrames, ioData);
		if (err != noErr) {
			printf("***Error writing to file: %d\n", (int)err);
		}
	}
    */
    
    cdata->ret = ret;
    return 0;
}

/* AURE
-(void)handleInterruption:(NSNotification*)notification {
   
    NSDictionary *interuptionDict = notification.userInfo;
    NSUInteger interuptionType = (NSUInteger)[interuptionDict
                                              valueForKey:AVAudioSessionInterruptionTypeKey];
    
    NSError* error;
    BOOL success;
   
    if (mCsData.running) {
        if (interuptionType == AVAudioSessionInterruptionTypeBegan) {
            AudioOutputUnitStop(*(mCsData.aunit));
        } else if (interuptionType == kAudioSessionEndInterruption) {
            // make sure we are again the active session
            success = [[AVAudioSession sharedInstance] setActive:YES error:&error];
            if(success) {
                AudioOutputUnitStart(*(mCsData.aunit));
            }
        }
    }
}
*/

-(void)startCsound:(NSString*)csdFilePath {
	mCsData.shouldRecord = false;
    [self performSelectorInBackground:@selector(runCsound:) withObject:csdFilePath];
}

-(void)startCsound:(NSString *)csdFilePath recordToURL:(NSURL *)outputURL_{
	mCsData.shouldRecord = true;
	self.outputURL = outputURL_;
	[self performSelectorInBackground:@selector(runCsound:) withObject:csdFilePath];
}

-(void)startCsoundToDisk:(NSString*)csdFilePath outputFile:(NSString*)outputFile {
	mCsData.shouldRecord = false;
    
    [self performSelectorInBackground:@selector(runCsoundToDisk:)
                           withObject:[NSMutableArray arrayWithObjects:csdFilePath, outputFile, nil]];
}

/* AURE
-(void)recordToURL:(NSURL *)outputURL_
{
    // Define format for the audio file.
    AudioStreamBasicDescription destFormat, clientFormat;
    memset(&destFormat, 0, sizeof(AudioStreamBasicDescription));
    memset(&clientFormat, 0, sizeof(AudioStreamBasicDescription));
    destFormat.mFormatID = kAudioFormatLinearPCM;
    destFormat.mFormatFlags = kLinearPCMFormatFlagIsPacked | kLinearPCMFormatFlagIsSignedInteger;
    destFormat.mSampleRate = csoundGetSr(mCsData.cs);
    destFormat.mChannelsPerFrame = mCsData.nchnls;
    destFormat.mBytesPerPacket = mCsData.nchnls * 2;
    destFormat.mBytesPerFrame = mCsData.nchnls * 2;
    destFormat.mBitsPerChannel = 16;
    destFormat.mFramesPerPacket = 1;
    
    // Create the audio file.
    OSStatus err = noErr;
    CFURLRef fileURL = (__bridge CFURLRef)outputURL_;
    err = ExtAudioFileCreateWithURL(fileURL, kAudioFileWAVEType, &destFormat, NULL, kAudioFileFlags_EraseFile, &(mCsData.file));
    if (err == noErr) {
        // Get the stream format from the AU...
        UInt32 propSize = sizeof(AudioStreamBasicDescription);
        AudioUnitGetProperty(*(mCsData.aunit), kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &clientFormat, &propSize);
        // ...and set it as the client format for the audio file. The file will use this
        // format to perform any necessary conversions when asked to read or write.
        ExtAudioFileSetProperty(mCsData.file, kExtAudioFileProperty_ClientDataFormat, sizeof(clientFormat), &clientFormat);
        // Warm the file up.
        ExtAudioFileWriteAsync(mCsData.file, 0, NULL);
    } else {
        printf("***Not recording. Error: %d\n", (int)err);
        err = noErr;
    }
    
    mCsData.shouldRecord = true;
}
 */

/* AURE
-(void)stopRecording
{
    mCsData.shouldRecord = false;
    ExtAudioFileDispose(mCsData.file);
}
 */

-(void)stopCsound {
    mCsData.running = false;
}

-(void)muteCsound{
	mCsData.shouldMute = true;
}

-(void)unmuteCsound{
	mCsData.shouldMute = false;
}


-(void)runCsoundToDisk:(NSArray*)paths {
	
    @autoreleasepool {
        
        
        CSOUND *cs;
        
        cs = csoundCreate(NULL);
        
        char *argv[4] = { "csound",
            (char*)[[paths objectAtIndex:0] cStringUsingEncoding:NSASCIIStringEncoding], "-o", (char*)[[paths objectAtIndex:1] cStringUsingEncoding:NSASCIIStringEncoding]};
        int ret = csoundCompile(cs, 4, argv);
        
        /* SETUP VALUE CACHEABLE */
        
        for (int i = 0; i < _valuesCache.count; i++) {
            id<CsoundValueCacheable> cachedValue = [_valuesCache objectAtIndex:i];
            [cachedValue setup:self];
        }
        
        /* NOTIFY COMPLETION LISTENERS*/
        
        for (id<CsoundObjCompletionListener> listener in completionListeners) {
            [listener csoundObjDidStart:self];
        }
        
        /* SET VALUES FROM CACHE */
        for (int i = 0; i < _valuesCache.count; i++) {
			id<CsoundValueCacheable> cachedValue = [_valuesCache objectAtIndex:i];
			[cachedValue updateValuesToCsound];
		}
        
        if(!ret) {
            
            csoundPerform(cs);
            csoundCleanup(cs);
            csoundDestroy(cs);
        }
        
        /* CLEANUP VALUE CACHEABLE */
        
        for (int i = 0; i < _valuesCache.count; i++) {
            id<CsoundValueCacheable> cachedValue = [_valuesCache objectAtIndex:i];
            [cachedValue cleanup];
        }
        
        /* NOTIFY COMPLETION LISTENERS*/
        
        for (id<CsoundObjCompletionListener> listener in completionListeners) {
            [listener csoundObjComplete:self];
        }
    }
}

uintptr_t csThread(void *data)
{
    csdata *cdata = (csdata *) data;
    NSMutableArray *cache = cdata->valuesCache;
    if(!cdata->ret)
    {
        int result = 0;
        while(result == 0) {
            for (int i = 0; i < cache.count; i++) {
                id<CsoundValueCacheable> cachedValue = [cache objectAtIndex:i];
                [cachedValue updateValuesToCsound];
            }
            result = csoundPerformKsmps(cdata->cs);
        }
        for (int i = 0; i < cache.count; i++) {
            id<CsoundValueCacheable> cachedValue = [cache objectAtIndex:i];
            [cachedValue updateValuesFromCsound];
        }
        
        
    }
    cdata->ret = 0;
    return 1;
}


-(void)runCsound:(NSString*)csdFilePath {
	
    CSOUND *cs;
    
	cs = csoundCreate(NULL);
    ///    csoundPreCompile(cs);
    csoundSetHostImplementedAudioIO(cs, 0, 0);
    csoundSetMessageCallback(cs, messageCallback);
    
    
    NSLog(@"!!!! Running Csound");
    char *argv[2] = { "csound", (char*)[csdFilePath cStringUsingEncoding:NSASCIIStringEncoding]};
	int ret = csoundCompile(cs, 2, argv);
	mCsData.running = true;
    
    if(!ret) {
        
		mCsData.cs = cs;
		mCsData.ret = ret;
		mCsData.nchnls = csoundGetNchnls(cs);
		mCsData.bufframes = (int)(csoundGetOutputBufferSize(cs))/mCsData.nchnls;
		mCsData.running = true;
        mCsData.valuesCache = _valuesCache;
        
		
        // SETUP VALUE CACHEABLE
        
        for (int i = 0; i < _valuesCache.count; i++) {
            id<CsoundValueCacheable> cachedValue = [_valuesCache objectAtIndex:i];
            [cachedValue setup:self];
        }
        
        csoundCreateThread(csThread, &mCsData);
        
        
        for (id<CsoundObjCompletionListener> listener in completionListeners) {
            [listener csoundObjDidStart:self];
        }
        
        while (!mCsData.ret && mCsData.running);
    }
    NSLog(@"!!!! Destroying Csound");
    csoundDestroy(cs);
    NSLog(@"!!!! Destroyed Csound");
    mCsData.running = false;
        NSLog(@"!!!! CachedValue Cleanup");
    for (int i = 0; i < _valuesCache.count; i++) {
        id<CsoundValueCacheable> cachedValue = [_valuesCache objectAtIndex:i];
        [cachedValue cleanup];
    }
    
    // NOTIFY COMPLETION LISTENERS
    NSLog(@"!!!! Notifying Completion Listeners");
    for (id<CsoundObjCompletionListener> listener in completionListeners) {
        [listener csoundObjComplete:self];
    }}


@end
