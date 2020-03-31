//
// Created by chris on 31/03/2020.
//

#ifndef AFV_NATIVE_PINKNOISEGENERATOR_H
#define AFV_NATIVE_PINKNOISEGENERATOR_H

#include "BiQuadFilter.h"
#include "WhiteNoiseGenerator.h"

namespace afv_native {
    namespace audio {
        class PinkNoiseGenerator: public ISampleSource {
        public:
            explicit PinkNoiseGenerator(float gain = 1.0f):
                mWhiteNoise(gain),
                mPinkFilter(BiQuadFilter::pinkNoiseFilter())
            {
            }

            SourceStatus getAudioFrame(SampleType *bufferOut) override {
                size_t samplesLeft = frameSizeSamples;
                while (samplesLeft--) {
                    *(bufferOut++) = mPinkFilter.TransformOne(mWhiteNoise.iterateOneSample());
                }
                return SourceStatus::OK;
            }

        protected:
            WhiteNoiseGenerator mWhiteNoise;
            BiQuadFilter mPinkFilter;
        };
    }
}


#endif //AFV_NATIVE_PINKNOISEGENERATOR_H
