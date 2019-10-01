/* test/audio/test_SinkFrameSizeAdapter.cpp
 *
 * This file is part of AFV-Native.
 *
 * Copyright (c) 2019 Christopher Collins
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include <gtest/gtest.h>
#include <memory>

#include <afv-native/audio/audio_params.h>
#include <afv-native/audio/ISampleSink.h>
#include <afv-native/audio/SinkFrameSizeAdjuster.h>

using namespace afv_native::audio;

class TestSinkAdapter: public ISampleSink {
public:
    SampleType *mBufferData;
    size_t mExpectedFrameSize;
    size_t mNextExpectedSample;
    size_t mBufferFillCount;

    explicit TestSinkAdapter(size_t len):
            mBufferData(nullptr),
            mExpectedFrameSize(len),
            mNextExpectedSample(0),
            mBufferFillCount(0)
    {
        mBufferData = new SampleType[frameSizeSamples];
    }

    void putAudioFrame(const SampleType *bufferIn) override
    {
        mBufferFillCount++;
        for (auto i = 0; i < frameSizeSamples; i++) {
            EXPECT_EQ(bufferIn[i], static_cast<SampleType>(mNextExpectedSample)) << "bad data in buffer in fill " << mBufferFillCount;
            mNextExpectedSample = (mNextExpectedSample+1)%mExpectedFrameSize;
        }
    }

    virtual ~TestSinkAdapter()
    {
        delete[] mBufferData;
    }
};

TEST(SinkFrameSizeAdjuster, LargerSinkTest)
{
    const size_t input_frame_size = frameSizeSamples + 100;
    auto ts = std::make_shared<TestSinkAdapter>(input_frame_size);
    SinkFrameSizeAdjuster testAdjuster(ts, input_frame_size);

    // initialise the test source buffer.
    auto *testSourceBuffer = new SampleType[input_frame_size];
    for (int i = 0; i < input_frame_size; i++) {
        testSourceBuffer[i] = static_cast<SampleType>(i);
    }

    // start first fill. This should trigger one test cycle.
    ASSERT_EQ(ts->mBufferFillCount, 0);
    testAdjuster.putAudioFrame(testSourceBuffer);
    EXPECT_EQ(ts->mBufferFillCount, 1);
    testAdjuster.putAudioFrame(testSourceBuffer);
    EXPECT_EQ(ts->mBufferFillCount, 2);

    delete[] testSourceBuffer;
}

TEST(SinkFrameSizeAdjuster, SmallerSinkTest)
{
    const size_t input_frame_size = frameSizeSamples - 100;
    auto ts = std::make_shared<TestSinkAdapter>(input_frame_size);
    SinkFrameSizeAdjuster testAdjuster(ts, input_frame_size);

    // initialise the test source buffer.
    auto *testSourceBuffer = new SampleType[input_frame_size];
    for (int i = 0; i < input_frame_size; i++) {
        testSourceBuffer[i] = static_cast<SampleType>(i);
    }

    // start first fill. This should trigger one test cycle.
    ASSERT_EQ(ts->mBufferFillCount, 0);
    testAdjuster.putAudioFrame(testSourceBuffer);
    EXPECT_EQ(ts->mBufferFillCount, 0);
    testAdjuster.putAudioFrame(testSourceBuffer);
    EXPECT_EQ(ts->mBufferFillCount, 1);

    delete[] testSourceBuffer;
}