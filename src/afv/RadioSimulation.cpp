/* afv/RadioSimulation.cpp
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

#include "afv-native/audio/VHFFilterSource.h"
#include "afv-native/afv/RadioSimulation.h"

#include <cmath>
#include <atomic>
#include <xmmintrin.h>

#include "afv-native/Log.h"
#include "afv-native/afv/dto/voice_server/AudioTxOnTransceivers.h"

using namespace afv_native;
using namespace afv_native::afv;

const float fxClickGain = 1.0f;

const float fxBlockToneGain = 0.22f;

const float fxWhiteNoiseGain = 0.01f;

const float fxHfWhiteNoiseGain = 0.6f;

const float fxBlockToneFreq = 180.0f;

CallsignMeta::CallsignMeta():
        source(),
        voiceFilter(),
        transceivers()
{
    source = std::make_shared<RemoteVoiceSource>();
}

RadioSimulation::RadioSimulation(
        struct event_base *evBase,
        std::shared_ptr<EffectResources> resources,
        cryptodto::UDPChannel *channel,
        unsigned int radioCount):
        IncomingAudioStreams(0),
        AudiableAudioStreams(nullptr),
        mEvBase(evBase),
        mResources(std::move(resources)),
        mChannel(),
        mStreamMapLock(),
        mIncomingStreams(),
        mRadioStateLock(),
        mPtt(false),
        mLastFramePtt(false),
        mTxRadio(0),
        mTxSequence(0),
        mRadioState(radioCount),
        mMixingBuffer(nullptr),
        mFetchBuffer(nullptr),
        mVoiceSink(std::make_shared<VoiceCompressionSink>(*this)),
        mVoiceFilter(),
        mMaintenanceTimer(mEvBase, std::bind(&RadioSimulation::maintainIncomingStreams, this)),
        mVuMeter(300 / audio::frameLengthMs) // VU is a 300ms zero to peak response...
{
    mMixingBuffer = reinterpret_cast<audio::SampleType *>(_mm_malloc(
            sizeof(audio::SampleType) * audio::frameSizeSamples,
            16));
    mFetchBuffer = reinterpret_cast<audio::SampleType *>(_mm_malloc(
            sizeof(audio::SampleType) * audio::frameSizeSamples,
            16));
    setUDPChannel(channel);
    mMaintenanceTimer.enable(maintenanceTimerIntervalMs);
    AudiableAudioStreams = new std::atomic<uint32_t>[radioCount];
    for (int i = 0; i < radioCount; i++) {
        AudiableAudioStreams[i].store(0);
    }
}

void RadioSimulation::putAudioFrame(const audio::SampleType *bufferIn)
{
    // do the peak/Vu calcs
    {
        auto *b = bufferIn;
        int i = audio::frameSizeSamples - 1;
        audio::SampleType peak = fabs(*(b++));
        while (i-- > 0) {
            peak = std::max<audio::SampleType>(peak, fabs(*(b++)));
        }
        double peakDb = 20.0 * log10(peak);
        peakDb = std::max(-40.0, peakDb);
        peakDb = std::min(0.0, peakDb);
        mVuMeter.addDatum(peakDb);
    }
    if (!mPtt.load() && !mLastFramePtt) {
        // Tick the sequence over when we have no Ptt as the compressed endpoint wont' get called to do that.
        std::atomic_fetch_add<uint32_t>(&mTxSequence, 1);
        return;
    }
    if (mVoiceFilter) {
        mVoiceFilter->putAudioFrame(bufferIn);
    } else {
        mVoiceSink->putAudioFrame(bufferIn);
    }
}

void RadioSimulation::processCompressedFrame(std::vector<unsigned char> compressedData)
{
    if (mChannel != nullptr && mChannel->isOpen()) {
        dto::AudioTxOnTransceivers audioOutDto;
        {
            std::lock_guard<std::mutex> radioStateGuard(mRadioStateLock);

            if (!mPtt.load()) {
                audioOutDto.LastPacket = true;
                mLastFramePtt = false;
            } else {
                mLastFramePtt = true;
            }

            audioOutDto.Transceivers.emplace_back(mTxRadio);
        }
        audioOutDto.SequenceCounter = std::atomic_fetch_add<uint32_t>(&mTxSequence, 1);
        audioOutDto.Callsign = mCallsign;
        audioOutDto.Audio = std::move(compressedData);
        mChannel->sendDto(audioOutDto);
    }
}

// NOTE:  this requires the source_dst buffer is aligned.
void
RadioSimulation::mix_buffers(audio::SampleType *src_dst, const audio::SampleType *src2, float src2_gain)
{
    __m128 srcReg;
    __m128 src2Reg;
    __m128 volReg = _mm_set_ps1(src2_gain);
    int i = 0;
    for (; i < audio::frameSizeSamples - 3; i += 4) {
        srcReg = _mm_load_ps(src_dst + i);
        src2Reg = _mm_loadu_ps(src2 + i); // unaligned because the sampleCache is unaligned.  *sigh*.
        src2Reg = _mm_mul_ps(src2Reg, volReg);
        srcReg = _mm_add_ps(srcReg, src2Reg);
        _mm_store_ps(src_dst + i, srcReg);
    }
    for (; i < audio::frameSizeSamples; i++) {
        src_dst[i] += (src2_gain * src2[i]);
    }
}

inline bool
freqIsHF(unsigned int freq)
{
    return freq < 30000000;
}

bool RadioSimulation::_process_radio(
        const std::map<void *, audio::SampleType[audio::frameSizeSamples]> &sampleCache,
        std::map<void *, audio::SampleType[audio::frameSizeSamples]> &eqSampleCache,
                                     size_t rxIter)
{
    if (mPtt.load() && mTxRadio == rxIter) {
        // don't analyze and mix-in the radios transmitting, but suppress the
        // effects.
        resetRadioFx(rxIter);
        AudiableAudioStreams[rxIter].store(0);
        return true;
    }
    // now, find all streams that this applies to.
    float crackleGain = 0.0f;
    uint32_t concurrentStreams = 0;
    for (auto &srcPair: mIncomingStreams) {
        if (!srcPair.second.source || !srcPair.second.source->isActive() ||
            (sampleCache.find(srcPair.second.source.get()) == sampleCache.end())) {
            continue;
        }
        bool mUseStream = false;
        float voiceGain = 1.0f;
        for (const afv::dto::RxTransceiver &tx: srcPair.second.transceivers) {
            if (tx.Frequency == mRadioState[rxIter].Frequency) {
                mUseStream = true;

                float crackleFactor = 0.0f;
                if (!mRadioState[rxIter].mBypassEffects) {
                    crackleFactor = static_cast<float>(
                            (exp(tx.DistanceRatio) * pow(tx.DistanceRatio, -2.5) / 350.0) - 0.00776652);
                    crackleFactor = fmax(0.0f, crackleFactor);
                    crackleFactor = fmin(0.15f, crackleFactor);

                    if (freqIsHF(mRadioState[rxIter].Frequency)) {
                        voiceGain = 0.38f;
                    } else {
                        voiceGain = (1.0f - (3.7f * crackleFactor));
                    }
                    crackleGain += crackleFactor * 2.0f;
                }
                break; // matched once.  dont' bother anymore.
            }
        }
        if (mUseStream) {
            // then include this stream.
            try {
                if (mRadioState[rxIter].mBypassEffects) {
                    mix_buffers(
                            mMixingBuffer,
                            sampleCache.at(srcPair.second.source.get()),
                            voiceGain * mRadioState[rxIter].Gain);
                } else {
                    void *sPtr = srcPair.second.source.get();
                    auto eqCacheIter = eqSampleCache.find(sPtr);
                    if (eqCacheIter == eqSampleCache.end()) {
                        srcPair.second.voiceFilter.transformFrame(eqSampleCache[sPtr], sampleCache.at(sPtr));
                    }
                    mix_buffers(
                            mMixingBuffer,
                            eqSampleCache.at(sPtr),
                            voiceGain * mRadioState[rxIter].Gain);

                }
                concurrentStreams++;
            } catch (const std::out_of_range &) {
                LOG("RadioSimulation", "internal error:  Tried to mix uncached stream");
            }
        }
    }
    AudiableAudioStreams[rxIter].store(concurrentStreams);
    if (concurrentStreams > 0) {
        if (!mRadioState[rxIter].mBypassEffects) {
            float whiteNoiseGain = 0.0f;
            set_radio_effects(rxIter, crackleGain, whiteNoiseGain);
            if (!mix_effect(mRadioState[rxIter].Crackle, crackleGain * mRadioState[rxIter].Gain)) {
                mRadioState[rxIter].Crackle.reset();
            }
            if (!mix_effect(mRadioState[rxIter].WhiteNoise, whiteNoiseGain * mRadioState[rxIter].Gain)) {
                mRadioState[rxIter].WhiteNoise.reset();
            }
        } // bypass effects
        if (concurrentStreams > 1) {
            if (!mRadioState[rxIter].BlockTone) {
                mRadioState[rxIter].BlockTone = std::make_shared<audio::SineToneSource>(fxBlockToneFreq);
            }
            if (!mix_effect(mRadioState[rxIter].BlockTone, fxBlockToneGain * mRadioState[rxIter].Gain)) {
                mRadioState[rxIter].BlockTone.reset();
            }
        } else {
            if (mRadioState[rxIter].BlockTone) {
                mRadioState[rxIter].BlockTone.reset();
            }
        }
    } else {
        resetRadioFx(rxIter, true);
        if (mRadioState[rxIter].mLastRxCount > 0) {
            mRadioState[rxIter].Click = std::make_shared<audio::RecordedSampleSource>(mResources->mClick, false);
        }
    }
    mRadioState[rxIter].mLastRxCount = concurrentStreams;
    // if we have a pending click, play it.
    if (!mix_effect(mRadioState[rxIter].Click, fxClickGain * mRadioState[rxIter].Gain)) {
        mRadioState[rxIter].Click.reset();
    }
    return false;
}

audio::SourceStatus RadioSimulation::getAudioFrame(audio::SampleType *bufferOut)
{
    std::lock_guard<std::mutex> radioStateGuard(mRadioStateLock);
    std::lock_guard<std::mutex> streamGuard(mStreamMapLock);

    std::map<void *, audio::SampleType[audio::frameSizeSamples]> sampleCache;
    std::map<void *, audio::SampleType[audio::frameSizeSamples]> eqSampleCache;

    uint32_t allStreams = 0;
    // first, pull frames from all active audio sources.
    for (auto &src: mIncomingStreams) {
        if (src.second.source && src.second.source->isActive() &&
            (sampleCache.find(src.second.source.get()) == sampleCache.end())) {
            const auto rv = src.second.source->getAudioFrame(sampleCache[src.second.source.get()]);
            if (rv != audio::SourceStatus::OK) {
                sampleCache.erase(src.second.source.get());
            } else {
                allStreams++;
            }
        }
    }
    IncomingAudioStreams.store(allStreams);

    // empty the output buffer.
    ::memset(mMixingBuffer, 0, sizeof(audio::SampleType) * audio::frameSizeSamples);

    size_t rxIter = 0;
    for (rxIter = 0; rxIter < mRadioState.size(); rxIter++) {
        _process_radio(sampleCache, eqSampleCache, rxIter);
    } // rxIter
    ::memcpy(bufferOut, mMixingBuffer, sizeof(audio::SampleType) * audio::frameSizeSamples);
    return audio::SourceStatus::OK;
}

void RadioSimulation::set_radio_effects(size_t rxIter, float crackleGain, float &whiteNoiseGain)
{
    if (freqIsHF(mRadioState[rxIter].Frequency)) {
        whiteNoiseGain = fxHfWhiteNoiseGain;
        if (!mRadioState[rxIter].WhiteNoise) {
            mRadioState[rxIter].WhiteNoise = std::make_shared<
                    audio::RecordedSampleSource>(mResources->mHFWhiteNoise, true);
        }
    } else {
        whiteNoiseGain = fxWhiteNoiseGain;
        if (!mRadioState[rxIter].WhiteNoise) {
            mRadioState[rxIter].WhiteNoise = std::make_shared<
                    audio::RecordedSampleSource>(mResources->mWhiteNoise, true);
        }
    }
    if (crackleGain > 0.0f) {
        if (!mRadioState[rxIter].Crackle) {
            mRadioState[rxIter].Crackle = std::make_shared<audio::RecordedSampleSource>(
                    mResources->mCrackle,
                    true);
        }
    }
}

bool RadioSimulation::mix_effect(std::shared_ptr<ISampleSource> effect, float gain) {
    if (effect && gain > 0.0f) {
        auto rv = effect->getAudioFrame(mFetchBuffer);
        if (rv == audio::SourceStatus::OK) {
            RadioSimulation::mix_buffers(mMixingBuffer, mFetchBuffer, gain);
        } else {
            return false;
        }
    }
    return true;
}

RadioSimulation::~RadioSimulation()
{
    _mm_free(mFetchBuffer);
    _mm_free(mMixingBuffer);
    delete[] AudiableAudioStreams;
}

void RadioSimulation::rxVoicePacket(const afv::dto::AudioRxOnTransceivers &pkt)
{
    std::lock_guard<std::mutex> streamMapLock(mStreamMapLock);
    //FIXME:  Deal with the case of a single-callsign transmitting multiple different voicestreams simultaneously.
    mIncomingStreams[pkt.Callsign].source->appendAudioDTO(pkt);
    mIncomingStreams[pkt.Callsign].transceivers = pkt.Transceivers;
}

void RadioSimulation::setFrequency(unsigned int radio, unsigned int frequency)
{
    std::lock_guard<std::mutex> radioStateGuard(mRadioStateLock);
    if (radio >= mRadioState.size()) {
        return;
    }
    if (mRadioState[radio].Frequency == frequency) {
        return;
    }
    mRadioState[radio].Frequency = frequency;
    // reset all of the effects, except the click which should be audiable due to the Squelch-gate kicking in on the new frequency
    resetRadioFx(radio, true);
}

void RadioSimulation::resetRadioFx(unsigned int radio, bool except_click)
{
    if (!except_click) {
        mRadioState[radio].Click.reset();
        mRadioState[radio].mLastRxCount = 0;
    }
    mRadioState[radio].BlockTone.reset();
    mRadioState[radio].WhiteNoise.reset();
    mRadioState[radio].Crackle.reset();
}

void RadioSimulation::setPtt(bool pressed)
{
    mPtt.store(pressed);
}

void RadioSimulation::setGain(unsigned int radio, float gain)
{
    std::lock_guard<std::mutex> radioStateGuard(mRadioStateLock);
    mRadioState[radio].Gain = gain;
}

void RadioSimulation::setTxRadio(unsigned int radio)
{
    std::lock_guard<std::mutex> radioStateGuard(mRadioStateLock);
    if (radio >= mRadioState.size()) {
        return;
    }
    mTxRadio = radio;
}

void RadioSimulation::dtoHandler(const std::string &dtoName, const unsigned char *bufIn, size_t bufLen, void *user_data)
{
    auto *thisRs = reinterpret_cast<RadioSimulation *>(user_data);
    thisRs->instDtoHandler(dtoName, bufIn, bufLen);
}

void RadioSimulation::instDtoHandler(const std::string &dtoName, const unsigned char *bufIn, size_t bufLen)
{
    if (dtoName == "AR") {
        try {
            dto::AudioRxOnTransceivers audioIn;
            auto unpacker = msgpack::unpack(reinterpret_cast<const char *>(bufIn), bufLen);
            auto msgpackObj = unpacker.get();
            msgpackObj.convert(audioIn);
            rxVoicePacket(audioIn);
        } catch (msgpack::type_error &e) {
            LOG("radiosimulation", "Error unmarshalling %s packet: %s", dtoName.c_str(), e.what());
        }
    }
}

void RadioSimulation::setUDPChannel(cryptodto::UDPChannel *newChannel)
{
    if (mChannel != nullptr) {
        mChannel->unregisterDtoHandler("AR");
    }
    mChannel = newChannel;
    if (mChannel != nullptr) {
        mChannel->registerDtoHandler(
                "AR", [this](const unsigned char *data, size_t len) {
                    try {
                        dto::AudioRxOnTransceivers rxAudio;
                        auto objHdl = msgpack::unpack(reinterpret_cast<const char *>(data), len);
                        objHdl.get().convert(rxAudio);
                        this->rxVoicePacket(rxAudio);
                    } catch (const msgpack::type_error &e) {
                        LOG("radiosimulation", "unable to unpack audio data received: %s", e.what());
                        LOGDUMPHEX("radiosimulation", data, len);
                    }
                });
    }
}

void RadioSimulation::maintainIncomingStreams()
{
    std::lock_guard<std::mutex> ml(mStreamMapLock);
    std::vector<std::string> callsignsToPurge;
    util::monotime_t now = util::monotime_get();
    for (const auto &streamPair: mIncomingStreams) {
        auto idleTime = now - streamPair.second.source->getLastActivityTime();
        if ((now - streamPair.second.source->getLastActivityTime()) > audio::compressedSourceCacheTimeoutMs) {
            callsignsToPurge.emplace_back(streamPair.first);
        }
    }
    for (const auto &callsign: callsignsToPurge) {
        mIncomingStreams.erase(callsign);
    }
    mMaintenanceTimer.enable(maintenanceTimerIntervalMs);
}

void RadioSimulation::setCallsign(const std::string &newCallsign)
{
    mCallsign = newCallsign;
}

void RadioSimulation::reset()
{
    {
        std::lock_guard<std::mutex> ml(mStreamMapLock);
        mIncomingStreams.clear();
    }
    mTxSequence.store(0);
    mPtt.store(false);
    mLastFramePtt = false;
    // reset the voice compression codec state.
    mVoiceSink->reset();
}

double RadioSimulation::getVu() const
{
    return std::max(-40.0, mVuMeter.getAverage());
}

double RadioSimulation::getPeak() const
{
    return std::max(-40.0, mVuMeter.getMax());
}

bool RadioSimulation::getEnableInputFilters() const
{
    return static_cast<bool>(mVoiceFilter);
}

void RadioSimulation::setEnableInputFilters(bool enableInputFilters)
{
    if (enableInputFilters) {
        if (!mVoiceFilter) {
            mVoiceFilter = std::make_shared<audio::SpeexPreprocessor>(mVoiceSink);
        }
    } else {
        mVoiceFilter.reset();
    }
}

void RadioSimulation::setEnableOutputEffects(bool enableEffects)
{
    for (auto &thisRadio: mRadioState) {
        thisRadio.mBypassEffects = !enableEffects;
    }
}
