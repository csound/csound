/*
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#include "csound.h"
#include "csound_circular_buffer.h"
#include "gtest/gtest.h"

class CircularBufferTests : public ::testing::Test {
public:
    CircularBufferTests ()
    {
        csoundSetGlobalEnv ("OPCODE6DIR64", "../../");
        csound = csoundCreate (NULL,NULL);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "--logfile=NULL");
    }

    virtual ~CircularBufferTests ()
    {
        csoundDestroyMessageBuffer (csound);
        csoundDestroy (csound);
        csound = nullptr;
    }

    virtual void SetUp ()
    {
        rb = csoundCreateCircularBuffer (csound, 512, sizeof (float));
        ASSERT_TRUE (rb != 0);
    }

    virtual void TearDown ()
    {
        csoundDestroyCircularBuffer (csound, rb);
    }

    CSOUND* csound = nullptr;
    void* rb = nullptr;
};

TEST_F (CircularBufferTests, testReadWrite)
{
    int32_t i;
    int32_t written;

    // Write test
    for (i = 0 ; i <256; i++) {
        float val = i;
        written = csoundWriteCircularBuffer(csound, rb, &val, 1);
        if (written != 1) {
            break;
        }
    }

    ASSERT_TRUE (written == 1);

    // Read test
    float invals[256];
    int32_t read = csoundReadCircularBuffer(csound, rb, invals, 256);
    ASSERT_EQ (read, 256);

    for (i = 0 ; i <256; i++) {
        ASSERT_EQ (invals[i], i);
    }
}

TEST_F (CircularBufferTests, testRequestedCapacity)
{
    float outvals[512];
    float invals[512];
    float val = 512.0f;

    for (int32_t i = 0; i < 512; i++) {
        outvals[i] = (float) i;
    }

    int32_t written = csoundWriteCircularBuffer(csound, rb, outvals, 512);
    ASSERT_EQ (written, 512);

    written = csoundWriteCircularBuffer(csound, rb, &val, 1);
    ASSERT_EQ (written, 0);

    int32_t read = csoundReadCircularBuffer(csound, rb, invals, 512);
    ASSERT_EQ (read, 512);

    for (int32_t i = 0; i < 512; i++) {
        ASSERT_EQ (invals[i], i);
    }

    read = csoundReadCircularBuffer(csound, rb, &val, 1);
    ASSERT_EQ (read, 0);
}

TEST_F (CircularBufferTests, testReadWriteDiffSizes)
{
    int32_t i, j;

    for (i = 0 ; i <256; i++) {
        float val = i;
        csoundWriteCircularBuffer(csound, rb, &val, 1);
    }

    float invals[512];
    float outvals[512];

    for (i = 256 ; i <512; i++) {
        invals[i] = i;
    }

    int32_t writeindex = 0;
    int32_t readindex = 0;

    for (i = 1 ; i < 16; i++) {
      int32_t read = csoundReadCircularBuffer(csound, rb, outvals, 17 - i);
        ASSERT_EQ (read, 17-i);
        for (j = 0; j < read; j++) {
            ASSERT_EQ (outvals[j], readindex++);
        }
        int32_t written = csoundWriteCircularBuffer(csound, rb, &(invals[writeindex]), i );
        ASSERT_EQ (written, i);
        writeindex += i;
    }
}

TEST_F (CircularBufferTests, testPeeking)
{
    int32_t i, j;

    for (i = 0 ; i <256; i++) {
        float val = i;
        csoundWriteCircularBuffer(csound, rb, &val, 1);
    }

    float invals[512];
    float outvals[512];

    for (i = 256 ; i <512; i++) {
        invals[i] = i;
    }

    int32_t writeindex = 0;
    int32_t readindex = 0;

    for (i = 1 ; i < 16; i++) {
        int32_t read = csoundPeekCircularBuffer(csound, rb, outvals, 17 - i);
        ASSERT_EQ (read, 17-i);

        for (j = 0; j < read; j++) {
            ASSERT_EQ (outvals[j], readindex++);
        }

        readindex -= read;
        read = csoundReadCircularBuffer(csound, rb, outvals, 17 - i);
        ASSERT_EQ (read, 17-i);

        for (j = 0; j < read; j++) {
            ASSERT_EQ (outvals[j], readindex++);
        }

        int32_t written = csoundWriteCircularBuffer(csound, rb, &(invals[writeindex]), i );
        ASSERT_EQ (written, i);
        writeindex += i;
    }
}

#define SIZ 128
TEST_F (CircularBufferTests, testCheckSpace)
{
    int32_t i;
    float buf[SIZ];
    i = csoundWriteCircularBuffer(csound, rb, buf, SIZ);
    ASSERT_EQ(i, SIZ);
    i = csoundCheckCircularBuffer(csound, rb, 1);
    ASSERT_EQ(i, 512 - SIZ);
    i = csoundCheckCircularBuffer(csound, rb, 0);
    ASSERT_EQ(i, SIZ);    
}


TEST_F (CircularBufferTests, testWraping)
{
    int32_t i;

    for (i = 0 ; i <16; i++) {
        float val = i;
        csoundWriteCircularBuffer(csound, rb, &val, 1);
    }

    for (i = 0 ; i < 65; i++) {
        float val;
        int32_t read = csoundReadCircularBuffer(csound, rb, &val, 1);
        ASSERT_EQ (read, 1);
        ASSERT_EQ (val, i);
        val = i + 16;
        int32_t written = csoundWriteCircularBuffer(csound, rb, &val, 1);
        ASSERT_EQ (written, 1);
    }
}



extern "C" {
void csoundFree(CSOUND *, void *);
void *csoundCallocAligned(CSOUND *, size_t, size_t);
}

TEST_F (CircularBufferTests, testAlignedMemory) {
  MYFLT *p = (MYFLT *) csoundCallocAligned(csound,sizeof(MYFLT)*10, 32);
  for(int32_t i = 0; i < 10; i++) {
    p[i] = (MYFLT) i;
  }

  for(int32_t i = 0; i < 10; i++) {
    ASSERT_EQ(i, (int32_t) p[i]);
    }
  csoundFree(csound, p);
}
