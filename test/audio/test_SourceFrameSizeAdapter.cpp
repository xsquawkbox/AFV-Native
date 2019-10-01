/* test/audio/test_SourceFrameSizeAdapter.cpp
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
#include <afv-native/audio/ISampleSource.h>
#include <afv-native/audio/SourceFrameSizeAdjuster.h>

using namespace afv_native::audio;

class TestSourceAdapter: public ISampleSource {
public:
    TestSourceAdapter() = default;

    SourceStatus getAudioFrame(SampleType *bufferOut) override
    {
        for (int i = 0; i < frameSizeSamples; i++) {
            bufferOut[i] = static_cast<SampleType>(i);
        }
        return SourceStatus::OK;
    }
};

TEST(SourceFrameSizeAdjuster, LargerSourceTest)
{
    auto ts = std::make_shared<TestSourceAdapter>();
    const int output_frame_size = frameSizeSamples + 100;
    SourceFrameSizeAdjuster testAdjuster(ts, output_frame_size);

    auto *testSinkBuffer = new SampleType[output_frame_size + 10];
    ::memset(testSinkBuffer, 0, sizeof(SampleType) * (output_frame_size + 10));
    testSinkBuffer[output_frame_size] = -100.0;

    int expectedValue = 0;
    testAdjuster.getAudioFrame(testSinkBuffer);
    int idx = 0;
    for (idx = 0; idx < output_frame_size; idx++) {
        EXPECT_EQ(static_cast<SampleType>(expectedValue), testSinkBuffer[idx]);
        expectedValue++;
        expectedValue %= frameSizeSamples;
    }
    EXPECT_EQ(testSinkBuffer[idx], -100.0) << "overwrote past the end of the buffer";
    testAdjuster.getAudioFrame(testSinkBuffer);
    for (idx = 0; idx < output_frame_size; idx++) {
        EXPECT_EQ(static_cast<SampleType>(expectedValue), testSinkBuffer[idx]);
        expectedValue++;
        expectedValue %= frameSizeSamples;
    }

    delete[] testSinkBuffer;
}

TEST(SourceFrameSizeAdjuster, SmallerSourceTest)
{
    auto ts = std::make_shared<TestSourceAdapter>();
    const int output_frame_size = frameSizeSamples - 100;
    SourceFrameSizeAdjuster testAdjuster(ts, output_frame_size);

    auto *testSinkBuffer = new SampleType[output_frame_size + 10];
    ::memset(testSinkBuffer, 0, sizeof(SampleType) * (output_frame_size + 10));
    testSinkBuffer[output_frame_size] = -100.0;

    int expectedValue = 0;
    testAdjuster.getAudioFrame(testSinkBuffer);
    int idx = 0;
    for (idx = 0; idx < output_frame_size; idx++) {
        EXPECT_EQ(static_cast<SampleType>(expectedValue), testSinkBuffer[idx]);
        expectedValue++;
        expectedValue %= frameSizeSamples;
    }
    EXPECT_EQ(testSinkBuffer[idx], -100.0) << "overwrote past the end of the buffer";
    testAdjuster.getAudioFrame(testSinkBuffer);
    for (idx = 0; idx < output_frame_size; idx++) {
        EXPECT_EQ(static_cast<SampleType>(expectedValue), testSinkBuffer[idx]);
        expectedValue++;
        expectedValue %= frameSizeSamples;
    }
	EXPECT_EQ(testSinkBuffer[idx], -100.0) << "overwrote past the end of the buffer";
	
    delete[] testSinkBuffer;
}