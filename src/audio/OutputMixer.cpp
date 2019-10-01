/* audio/OutputMixer.cpp
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

#include "afv-native/audio/OutputMixer.h"

#include "afv-native/Log.h"
#include "afv-native/audio/SourceStatus.h"

#if defined(_MSC_VER)

#include <intrin.h>

#endif

#include <xmmintrin.h>
#include <cstring>

using namespace afv_native::audio;

OutputMixer::OutputMixer()
{
}

OutputMixer::~OutputMixer()
{
}

SourceStatus
OutputMixer::getAudioFrame(SampleType *bufferOut)
{
    SourceStatus src_rv;
    bool didMix = false;
    int i = 0;
    __m128 srcReg;
    __m128 dstReg;
    __m128 volReg;
    auto *intermediate_buffer = reinterpret_cast<SampleType *>(_mm_malloc(sizeof(SampleType)*frameSizeSamples, 16 ));

    ::memset(bufferOut, 0, sizeof(SampleType) * frameSizeSamples);

    for (auto &src_iter: mSources) {
        src_rv = src_iter.src->getAudioFrame(intermediate_buffer);
        volReg = _mm_set_ps1(src_iter.gain);
        if (src_rv == SourceStatus::OK) {
            didMix = true;
            i = 0;
            for (; i < frameSizeSamples - 3; i += 4) {
                srcReg = _mm_loadu_ps(bufferOut + i);
                dstReg = _mm_load_ps(intermediate_buffer + i);
                dstReg = _mm_mul_ps(dstReg, volReg);
                srcReg = _mm_add_ps(srcReg, dstReg);
                _mm_storeu_ps(bufferOut + i, srcReg);
            }
            for (; i < frameSizeSamples; i++) {
                bufferOut[i] += (src_iter.gain * intermediate_buffer[i]);
            }
        } else {
            if (src_rv == SourceStatus::Error) {
                LOG("outputmixer", "Error reading from stream.  Removing from mixer.");
            }
            // otherwise the stream closed, and we can close it silently!
            src_iter.src.reset();
        }
    }
    mSources.remove_if([](MixerSource ms) -> bool { return !ms.src; });
    _mm_free(intermediate_buffer);
    // apply final volume adjustment.
    if (didMix) {
        i = 0;
        volReg = _mm_set_ps1(mGain);
        for (; i < frameSizeSamples - 3; i += 4) {
            srcReg = _mm_loadu_ps(bufferOut + i);
            srcReg = _mm_mul_ps(srcReg, volReg);
            _mm_storeu_ps(bufferOut + i, srcReg);
        }
        for (; i < frameSizeSamples; i++) {
            bufferOut[i] *= mGain;
        }
    }
    return SourceStatus::OK;
}

void OutputMixer::setSource(const std::shared_ptr<ISampleSource> &src, float gain)
{
    bool duplicate = false;
    for (auto &src_iter: mSources) {
        if (src_iter.src == src) {
            duplicate = true;
            src_iter.gain = gain;
            break;
        }
    }
    if (duplicate) {
        return;
    }
    mSources.emplace_front(MixerSource{src, gain});
}

void OutputMixer::removeSource(const std::shared_ptr<ISampleSource> &src)
{
    mSources.remove_if([src](const MixerSource &thisSrc) -> bool { return thisSrc.src == src; });
}

void OutputMixer::setGain(float newGain)
{
    mGain = newGain;
}

