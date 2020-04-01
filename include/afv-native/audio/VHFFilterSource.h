/* audio/VHFFilterSource.h
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

/*
 * WARNING:  This file must be included *first* if BiQuadFilter is not included ahead of it.
 *
 * If you don't, builds on MSVC will fail.
 */

#ifndef AFV_NATIVE_VHFFILTERSOURCE_H
#define AFV_NATIVE_VHFFILTERSOURCE_H

#include <afv-native/audio/BiQuadFilter.h>
#include <afv-native/audio/ISampleSource.h>
#include "SimpleComp.h"

namespace afv_native {
    namespace audio {
        /** VHFFilterSource implements the three filters we use to simulate the limited bandwidth of an airband VHF radio.
         *
         * If you want a more generic filter wrapper, look at FilterSource.
         *
         * @note VHFFilterSource is defined inline and staticly to encourage the compiler to inline and unroll/vectorise as
         *       much as possible.  Because we run these on every incoming sample, we actually want it to be fairly fast.
         */
        class VHFFilterSource {
        public:
            explicit VHFFilterSource():
                compressor(),
                highPass(BiQuadFilter::highPassFilter(450.0f, 1.0f)),
                lowPass(BiQuadFilter::lowPassFilter(3000.0f, 1.0f)),
                peakingEq(BiQuadFilter::peakingEqFilter(2200.0f, 0.25f, 13.0f))
            {
                compressor.setSampleRate(sampleRateHz);
                compressor.setAttack(5.0);
                compressor.setRelease(10.0);
                compressor.setThresh(16);
                compressor.setRatio(6);
                compressor.initRuntime();
                compressorPostGain = pow(10.0f, (-5.5/20.0));
            }

            virtual ~VHFFilterSource() = default;

            /** transformFrame lets use apply this filter to a normal buffer, without following the sink/source flow.
             *
             * It always performs a copy of the data from In to Out at the very least.
             */
            void transformFrame(SampleType *bufferOut, SampleType const bufferIn[]) {
                SampleType s = 0;
                double sl, sr;
                for (unsigned i = 0; i < frameSizeSamples; i++) {
                    sl = bufferIn[i];
                    sr = sl;
                    compressor.process(sl, sr);
                    s = highPass.TransformOne(sl * compressorPostGain);
                    s = peakingEq.TransformOne(s);
                    s = lowPass.TransformOne(s);
                    bufferOut[i] = s;
                }
            }

        protected:
            chunkware_simple::SimpleComp    compressor;
            float compressorPostGain;
            BiQuadFilter highPass;
            BiQuadFilter peakingEq;
            BiQuadFilter lowPass;
        };
    }
}

#endif //AFV_NATIVE_VHFFILTERSOURCE_H
