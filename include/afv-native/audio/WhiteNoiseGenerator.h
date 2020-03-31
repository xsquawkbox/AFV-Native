//
// Created by chris on 31/03/2020.
//

#ifndef AFV_NATIVE_WHITENOISEGENERATOR_H
#define AFV_NATIVE_WHITENOISEGENERATOR_H

#include "afv-native/audio/ISampleSource.h"

namespace afv_native {
    namespace audio {

        /** WhiteNoiseGenerator generates, surprise surprise, White Noise.
         *
         * This is an implementation of the fast whitenoise generator by ed.bew@hcrikdlef.dreg as posted to musicdsp.
         */
        class WhiteNoiseGenerator: public ISampleSource {
        public:
            WhiteNoiseGenerator(float level = 1.0):
                mX1(0x67452301),
                mX2(0xefcdab89),
                mLevel(level)
            {}

            void setLevel(float newLevel)
            {
                mLevel = newLevel;
            }

            inline SampleType iterateOneSample() {
                SampleType sv;
                mX1 ^= mX2;
                sv = mX2 * mLevel * sScale;
                mX2 += mX1;
                return sv;
            }

            SourceStatus getAudioFrame(SampleType *bufferOut) override
            {
                size_t ctrLeft = frameSizeSamples;
                while (ctrLeft--) {
                    *(bufferOut++) = iterateOneSample();
                }
                return SourceStatus::OK;
            }

        protected:
            int32_t mX1;
            int32_t mX2;
            float mLevel;
            static constexpr float sScale = 2.0f / 0xffffffff;
        };
    }
}

#endif //AFV_NATIVE_WHITENOISEGENERATOR_H
