/* afv/RadioSimulation.h
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

#ifndef AFV_NATIVE_RADIOSIMULATION_H
#define AFV_NATIVE_RADIOSIMULATION_H

#include <memory>
#include <unordered_map>

#include "afv-native/afv/EffectResources.h"
#include "afv-native/afv/RemoteVoiceSource.h"
#include "afv-native/afv/RollingAverage.h"
#include "afv-native/afv/VoiceCompressionSink.h"
#include "afv-native/afv/dto/voice_server/AudioRxOnTransceivers.h"
#include "afv-native/audio/ISampleSink.h"
#include "afv-native/audio/ISampleSource.h"
#include "afv-native/audio/OutputMixer.h"
#include "afv-native/audio/SineToneSource.h"
#include "afv-native/audio/SpeexPreprocessor.h"
#include "afv-native/cryptodto/UDPChannel.h"
#include "afv-native/event/EventCallbackTimer.h"

namespace afv_native {
    namespace afv {

        class RadioState {
        public:
            unsigned int Frequency;
            float Gain;
            std::shared_ptr<audio::RecordedSampleSource> Click;
            std::shared_ptr<audio::RecordedSampleSource> WhiteNoise;
            std::shared_ptr<audio::RecordedSampleSource> Crackle;
            std::shared_ptr<audio::SineToneSource> BlockTone;
            int mLastRxCount;
            bool mBypassEffects;
        };

        struct CallsignMeta {
            std::shared_ptr<RemoteVoiceSource> source;
            std::vector<dto::RxTransceiver> transceivers;
            CallsignMeta();
        };

        class RadioSimulation:
                public audio::ISampleSource,
                public audio::ISampleSink,
                public ICompressedFrameSink {
        private:
            static void mix_buffers(audio::SampleType *src_dst, const audio::SampleType *src2, float src2_gain = 1.0);
        protected:
            static const int maintenanceTimerIntervalMs = 30 * 1000; /* every 30s */

            struct event_base *mEvBase;
            std::shared_ptr<EffectResources> mResources;
            cryptodto::UDPChannel *mChannel;
            std::string mCallsign;

            std::mutex mStreamMapLock;
            std::unordered_map<std::string, struct CallsignMeta> mIncomingStreams;

            std::mutex mRadioStateLock;
            std::atomic<bool> mPtt;
            bool mLastFramePtt;
            unsigned int mTxRadio;
            std::atomic<uint32_t> mTxSequence;
            std::vector<RadioState> mRadioState;
            audio::SampleType *mMixingBuffer;
            audio::SampleType *mFetchBuffer;

            std::shared_ptr<VoiceCompressionSink> mVoiceSink;
            std::shared_ptr<audio::SpeexPreprocessor> mVoiceFilter;

            event::EventCallbackTimer mMaintenanceTimer;
            RollingAverage<double> mVuMeter;

            void resetRadioFx(unsigned int radio, bool except_click = false);

            void set_radio_effects(size_t rxIter, float crackleGain, float &whiteNoiseGain);

            bool mix_effect(std::shared_ptr<audio::ISampleSource> effect, float gain);

            void processCompressedFrame(std::vector<unsigned char> compressedData) override;

            static void dtoHandler(
                    const std::string &dtoName, const unsigned char *bufIn, size_t bufLen, void *user_data);
            void instDtoHandler(
                    const std::string &dtoName, const unsigned char *bufIn, size_t bufLen);

            void maintainIncomingStreams();
        private:
            bool _process_radio(
                    const std::map<void *, audio::SampleType[audio::frameSizeSamples]> &sampleCache,
                    size_t rxIter);

        public:
            void putAudioFrame(const audio::SampleType *bufferIn) override;
            audio::SourceStatus getAudioFrame(audio::SampleType *bufferOut) override;

            RadioSimulation(
                    struct event_base *evBase,
                    std::shared_ptr<EffectResources> resources,
                    cryptodto::UDPChannel *channel,
                    unsigned int radioCount);
            virtual ~RadioSimulation();
            void rxVoicePacket(const afv::dto::AudioRxOnTransceivers &pkt);

            void setCallsign(const std::string &newCallsign);
            void setFrequency(unsigned int radio, unsigned int frequency);
            void setGain(unsigned int radio, float gain);
            void setTxRadio(unsigned int radio);

            void setPtt(bool pressed);
            void setUDPChannel(cryptodto::UDPChannel *newChannel);

            double getVu() const;
            double getPeak() const;

            void reset();

            bool getEnableInputFilters() const;
            void setEnableInputFilters(bool enableInputFilters);

            void setEnableOutputEffects(bool enableEffects);
        };
    }
}

#endif //AFV_NATIVE_RADIOSIMULATION_H
