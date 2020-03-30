/* audio/BiQuadFilter.h
 *
 * This file is part of AFV-Native.
 *
 * Copyright (c) 2020 Christopher Collins
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

#ifndef AFV_NATIVE_BIQUADFILTER_H
#define AFV_NATIVE_BIQUADFILTER_H

#include <cmath>
#include <memory>

#include "afv-native/audio/IFilter.h"

namespace afv_native {
    namespace audio {
        /**
         * @note The BiQuadFilter is designed off of the work by Robert Bristow-Johnson as published in his
         * "Cookbook formulae for audio EQ biquad filter coefficients" as found on https://www.musicdsp.org/
         */
        class BiQuadFilter: public IFilter {
        public:
            constexpr BiQuadFilter(float a0, float a1, float a2, float b0, float b1, float b2):
                mA0(a0),
                mA1(a1),
                mA2(a2),
                mB0(b0),
                mB1(b1),
                mB2(b2),
                mHistoryIn{0, 0, 0},
                mHistoryOut{0, 0, 0},
                hpos(0)
            {}

            SampleType TransformOne(SampleType sampleIn) override {
                hpos %= 3;
                mHistoryIn[hpos] = sampleIn;
                mHistoryOut[hpos] = (mB0 / mA0) * mHistoryIn[hpos]
                        + (mB1 / mA0) * mHistoryIn[(hpos+2)%3]
                        + (mB2 / mA0) * mHistoryIn[(hpos+1)%3]
                        - (mA1 / mA0) * mHistoryIn[(hpos+2)%3]
                        - (mA2 / mA0) * mHistoryIn[(hpos+1)%3];
                return mHistoryOut[hpos++];
            }

            static BiQuadFilter lowPassFilter(float f0, float q) {
                float w0 = 2.0f * M_PI * f0 / static_cast<float>(sampleRateHz);
                float cosw0 = cos(w0);
                float alpha = sin(w0) / (2 * q);
                return BiQuadFilter(
                        1.0f + alpha,
                        -2.0f * cosw0,
                        1.0f - alpha,
                        (1.0f - cosw0) / 2.0f,
                        1.0f - cosw0,
                        (1.0f - cosw0) / 2.0f);
            }

            static BiQuadFilter highPassFilter(float f0, float q) {
                float w0 = 2.0f * M_PI * f0 / static_cast<float>(sampleRateHz);
                float cosw0 = cos(w0);
                float alpha = sin(w0) / (2 * q);

                return BiQuadFilter(
                        1.0f + alpha,
                        -2.0f * cosw0,
                        1.0f - alpha,
                        (1.0f + cosw0) / 2.0f,
                        -(1.0f + cosw0),
                        (1.0f + cosw0) / 2.0f);
            }

            static BiQuadFilter bandPassFilter(float f0, float q) {
                float w0 = 2.0f * M_PI * f0 / static_cast<float>(sampleRateHz);
                float cosw0 = cos(w0);
                float alpha = sin(w0) / (2 * q);

                return BiQuadFilter(
                        1.0f + alpha,
                        -2.0f * cosw0,
                        1.0f - alpha,
                        q * alpha,
                        0.0f,
                        -q * alpha);
            }

            static BiQuadFilter notchFilter(float f0, float q) {
                float w0 = 2.0f * M_PI * f0 / static_cast<float>(sampleRateHz);
                float cosw0 = cos(w0);
                float alpha = sin(w0) / (2 * q);

                return BiQuadFilter(
                        1.0f + alpha,
                        -2.0f * cosw0,
                        1.0f - alpha,
                        1.0f,
                        -2.0f * cosw0,
                        1.0f);
            }

            static BiQuadFilter peakingEqFilter(float f0, float q, float dbGain) {
                float a =  pow(10.0f, dbGain / 40.0f);
                float w0 = 2.0f * M_PI * f0 / static_cast<float>(sampleRateHz);
                float cosw0 = cos(w0);
                float alpha = sin(w0) / (2 * q);

                return BiQuadFilter(
                        1.0f + (alpha / a),
                        -2.0f * cosw0,
                        1.0f - (alpha / a),
                        1.0f + (alpha * a),
                        -2.0f * cosw0,
                        1.0f - (alpha * a));
            }

            static BiQuadFilter lowShelfFilter(float f0, float q, float dbGain) {
                float a =  pow(10.0f, dbGain / 40.0f);
                float w0 = 2.0f * M_PI * f0 / static_cast<float>(sampleRateHz);
                float cosw0 = cos(w0);
                float alpha = sin(w0) / (2 * q);

                return BiQuadFilter(
                        (a + 1.0f) + (a - 1.0f) * cosw0 + 2.0f * sqrt(a) * alpha,
                        -2.0f * ( (a - 1.0f) + (a + 1.0f) * cosw0),
                        (a + 1.0f) + (a - 1.0f) * cosw0 - 2.0f * sqrt(a) * alpha,
                        a * ((a + 1.0f) - (a - 1.0f) * cosw0 + 2.0f * sqrt(a) * alpha),
                        2.0f * a * ((a - 1.0f) - (a + 1.0f) * cosw0),
                        a * ((a + 1.0f) - (a - 1.0f) * cosw0 - 2.0f * sqrt(a) * alpha));
            }

            static BiQuadFilter highShelfFilter(float f0, float q, float dbGain) {
                float a =  pow(10.0f, dbGain / 40.0f);
                float w0 = 2.0f * M_PI * f0 / static_cast<float>(sampleRateHz);
                float cosw0 = cos(w0);
                float alpha = sin(w0) / (2 * q);

                return BiQuadFilter(
                        (a + 1.0f) - (a - 1.0f) * cosw0 + 2.0f * sqrt(a) * alpha,
                        2.0f * ((a - 1.0f) - (a + 1.0f) * cosw0                         ),
                        (a + 1.0f) - (a - 1.0f) * cosw0 - 2.0f * sqrt(a) * alpha,
                        a * ((a + 1.0f) + (a - 1.0f) * cosw0 + 2.0f * sqrt(a) * alpha),
                        -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cosw0                         ),
                        a * ((a + 1.0f) + (a - 1.0f) * cosw0 - 2.0f * sqrt(a) * alpha));
            }

        protected:
            float mA0, mA1, mA2;
            float mB0, mB1, mB2;
            SampleType mHistoryIn[3];
            SampleType mHistoryOut[3];
            unsigned int hpos;
        };
    }
}


#endif //AFV_NATIVE_BIQUADFILTER_H
