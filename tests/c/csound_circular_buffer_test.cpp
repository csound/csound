/*
 * File:   main.c
 * Author: stevenyi
 *
 * Created on June 7, 2012, 4:03 PM
 */

#include "csound.h"
#include "gtest/gtest.h"

class CircularBufferTests : public ::testing::Test {
public:
    CircularBufferTests ()
    {
        csoundSetGlobalEnv ("OPCODE6DIR64", "../../");
        csound = csoundCreate (0);
        csoundCreateMessageBuffer (csound, 0);
        csoundSetOption (csound, "--logfile=NULL");
    }

    virtual ~CircularBufferTests ()
    {
        csoundCleanup (csound);
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
    int i;
    int written;

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
    int read = csoundReadCircularBuffer(csound, rb, invals, 256);
    ASSERT_EQ (read, 256);

    for (i = 0 ; i <256; i++) {
        ASSERT_EQ (invals[i], i);
    }
}

TEST_F (CircularBufferTests, testReadWriteDiffSizes)
{
    int i, j;

    for (i = 0 ; i <256; i++) {
        float val = i;
        csoundWriteCircularBuffer(csound, rb, &val, 1);
    }

    float invals[512];
    float outvals[512];

    for (i = 256 ; i <512; i++) {
        invals[i] = i;
    }

    int writeindex = 0;
    int readindex = 0;

    for (i = 1 ; i < 16; i++) {
      int read = csoundReadCircularBuffer(csound, rb, outvals, 17 - i);
        ASSERT_EQ (read, 17-i);
        for (j = 0; j < read; j++) {
            ASSERT_EQ (outvals[j], readindex++);
        }
        int written = csoundWriteCircularBuffer(csound, rb, &(invals[writeindex]), i );
        ASSERT_EQ (written, i);
        writeindex += i;
    }
}

TEST_F (CircularBufferTests, testPeeking)
{
    int i, j;

    for (i = 0 ; i <256; i++) {
        float val = i;
        csoundWriteCircularBuffer(csound, rb, &val, 1);
    }

    float invals[512];
    float outvals[512];

    for (i = 256 ; i <512; i++) {
        invals[i] = i;
    }

    int writeindex = 0;
    int readindex = 0;

    for (i = 1 ; i < 16; i++) {
        int read = csoundPeekCircularBuffer(csound, rb, outvals, 17 - i);
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

        int written = csoundWriteCircularBuffer(csound, rb, &(invals[writeindex]), i );
        ASSERT_EQ (written, i);
        writeindex += i;
    }
}

TEST_F (CircularBufferTests, testWraping)
{
    int i;

    for (i = 0 ; i <16; i++) {
        float val = i;
        csoundWriteCircularBuffer(csound, rb, &val, 1);
    }

    for (i = 0 ; i < 65; i++) {
        float val;
        int read = csoundReadCircularBuffer(csound, rb, &val, 1);
        ASSERT_EQ (read, 1);
        ASSERT_EQ (val, i);
        val = i + 16;
        int written = csoundWriteCircularBuffer(csound, rb, &val, 1);
        ASSERT_EQ (written, 1);
    }
}
