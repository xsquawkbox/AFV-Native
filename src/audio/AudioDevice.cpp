/* audio/AudioDevice.cpp
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

#include "afv-native/audio/AudioDevice.h"

#include <memory>
#include <cstring>
#include <portaudio.h>

#include "afv-native/Log.h"
#include "afv-native/audio/SinkFrameSizeAdjuster.h"
#include "afv-native/audio/SourceFrameSizeAdjuster.h"

using namespace afv_native::audio;
using namespace std;

AudioDevice::AudioDevice(
        const std::string &userStreamName,
        const std::string &outputDeviceName,
        const std::string &inputDeviceName,
        AudioDevice::Api audioApi):
        mApi(audioApi),
        mUserStreamName(userStreamName),
        mOutputDeviceName(outputDeviceName),
        mInputDeviceName(inputDeviceName),
        mAudioDevice(),
        mSink(nullptr),
        mSource(nullptr)
{
    auto rv = Pa_Initialize();
    if (rv == paNoError) {
        mDidPortAudioInit = true;
    }
}

AudioDevice::~AudioDevice()
{
    if (mAudioDevice != nullptr) {
        Pa_CloseStream(mAudioDevice);
        mAudioDevice = nullptr;
    }
    if (mDidPortAudioInit) {
        Pa_Terminate();
        mDidPortAudioInit = false;
    }
}

bool AudioDevice::open()
{
    PaStreamFlags devStreamOpts = paNoFlag;

    PaStreamParameters inDevParam, outDevParam;
    //FIXME: we actually need to populate the entire PaStreamParameters struct because of the need to get default latencies.
    if (!getDeviceForName(mInputDeviceName, true, inDevParam)) {
        return false;
    }
    if (!getDeviceForName(mOutputDeviceName, false, outDevParam)) {
        return false;
    }

    LOG("AudioDevice", "Opening 1 Channel, %dHz Sampling Rate, %d samples per frame", sampleRateHz, frameSizeSamples);
    auto rv = Pa_OpenStream(
            &mAudioDevice,
            mSink ? &inDevParam : nullptr,
            mSource ? &outDevParam : nullptr,
            sampleRateHz,
            frameSizeSamples,
            devStreamOpts,
            &AudioDevice::paAudioCallback,
            this);
    if (rv != paNoError) {
        LOG("AudioDevice", "failed to open audio device: %s", Pa_GetErrorText(rv));
        return false;
    }
    rv = Pa_StartStream(mAudioDevice);
    if (rv != paNoError) {
        LOG("AudioDevice", "failed to start audio stream: %s", Pa_GetErrorText(rv));
        return false;
    }
    return true;
}

bool AudioDevice::getDeviceForName(const string &deviceName, bool forInput, PaStreamParameters &deviceParamOut)
{
    auto apiInfo = Pa_GetHostApiInfo(mApi);

    auto allDevices = forInput?getCompatibleInputDevicesForApi(mApi):getCompatibleOutputDevicesForApi(mApi);

    if (!allDevices.empty()) {
        for (const auto &devicePair: allDevices) {
            if (string(devicePair.second->name) == deviceName) {
                deviceParamOut.device = devicePair.first;
                deviceParamOut.channelCount = 1;
                deviceParamOut.sampleFormat = paFloat32;
                deviceParamOut.suggestedLatency = forInput ? devicePair.second->defaultLowInputLatency
                                                           : devicePair.second->defaultLowOutputLatency;
                deviceParamOut.hostApiSpecificStreamInfo = nullptr;
                return true;
            }
        }
        LOG("AudioDevice", "Couldn't find a compatible device \"%s\" - using default", deviceName.c_str());
    }
    // next, try the default device...
    auto devId = Pa_HostApiDeviceIndexToDeviceIndex(
            mApi,
            forInput ? apiInfo->defaultInputDevice : apiInfo->defaultOutputDevice);
    auto defaultDev = allDevices.find(devId);
    if (defaultDev != allDevices.end()) {
        deviceParamOut.device = defaultDev->first;
        deviceParamOut.channelCount = 1;
        deviceParamOut.sampleFormat = paFloat32;
        deviceParamOut.suggestedLatency = forInput ? defaultDev->second->defaultLowInputLatency
                                                   : defaultDev->second->defaultLowOutputLatency;
        deviceParamOut.hostApiSpecificStreamInfo = nullptr;
        return true;
    }
    // if the default device doesn't work, pull the first device that will.
    auto firstDev = allDevices.begin();
    if (firstDev != allDevices.end()) {
        LOG("AudioDevice", "Default can't handle our format.  Using \"%s\" instead.", firstDev->second->name);
        deviceParamOut.device = firstDev->first;
        deviceParamOut.channelCount = 1;
        deviceParamOut.sampleFormat = paFloat32;
        deviceParamOut.suggestedLatency = forInput ? firstDev->second->defaultLowInputLatency
                                                   : firstDev->second->defaultLowOutputLatency;
        deviceParamOut.hostApiSpecificStreamInfo = nullptr;
        return true;
    }
    LOG("AudioDevice", "Couldn't map a working audio device");
    return false;
}


int AudioDevice::paAudioCallback(
        const void *inputBuffer,
        void *outputBuffer,
        unsigned long nFrames,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags status,
        void *userData)
{
    auto *thisDev = reinterpret_cast<AudioDevice *>(userData);
    return thisDev->audioCallback(
            inputBuffer, outputBuffer, nFrames, timeInfo, status);
}

int AudioDevice::audioCallback(
        const void *inputBuffer,
        void *outputBuffer,
        unsigned int nFrames,
        const PaStreamCallbackTimeInfo *streamTime,
        PaStreamCallbackFlags status)
{
    if (status != 0) {
        if (status & paInputUnderflow) {
            LOG("AudioDevice", "%f: Input Underflowed", streamTime->currentTime);
        }
        if (status & paOutputUnderflowed) {
            LOG("AudioDevice", "%f: Output Underflowed", streamTime->currentTime);
        }
    }
    if (mSink && inputBuffer) {
        for (size_t i = 0; i < nFrames; i += frameSizeSamples) {
            mSink->putAudioFrame(reinterpret_cast<const float *>(inputBuffer) + i);
        }
    }
    if (outputBuffer) {
        if (mSource) {
            for (size_t i = 0; i < nFrames; i += frameSizeSamples) {
                SourceStatus rv;
                rv = mSource->getAudioFrame(reinterpret_cast<float *>(outputBuffer) + i);
                if (rv != SourceStatus::OK) {
                    ::memset(outputBuffer, 0, frameSizeBytes);
                    mSource.reset();
                    break;
                }
            }
        } else {
            // if there's no source, but there is an output buffer, zero it to avoid making horrible buzzing sounds.
            ::memset(outputBuffer, 0, sizeof(nFrames) * sizeof(SampleType));
        }
    }
    return 0;
}

void AudioDevice::setSource(std::shared_ptr<ISampleSource> newSrc)
{
    mSource = std::move(newSrc);
}

void AudioDevice::setSink(std::shared_ptr<ISampleSink> newSink)
{
    mSink = std::move(newSink);
}

void AudioDevice::close()
{
    if (mAudioDevice != nullptr) {
        if (Pa_IsStreamActive(mAudioDevice)) {
            Pa_StopStream(mAudioDevice);
        }
        Pa_CloseStream(mAudioDevice);
        mAudioDevice = nullptr;
    }
}

std::map<AudioDevice::Api, std::string> AudioDevice::getAPIs()
{
    std::map<AudioDevice::Api, std::string> apiList;
    auto rv = Pa_Initialize();
    if (rv == paNoError) {
        auto apiCount = Pa_GetHostApiCount();
        for (unsigned i = 0; i < apiCount; i++) {
            auto info = Pa_GetHostApiInfo(i);
            if (info != nullptr) {
                apiList[i] = string(info->name);
            }
        }
        Pa_Terminate();
    }
    return std::move(apiList);
}

std::map<int,const PaDeviceInfo *> AudioDevice::getCompatibleInputDevicesForApi(AudioDevice::Api api)
{
    std::map<int,const PaDeviceInfo *> deviceList;
    auto rv = Pa_Initialize();
    if (rv == paNoError) {
        auto apiInfo = Pa_GetHostApiInfo(api);

        for (unsigned i = 0; i < apiInfo->deviceCount; i++) {
            auto devId = Pa_HostApiDeviceIndexToDeviceIndex(api, i);
            if (devId < 0) {
                continue;
            }
            if (isAbleToOpen(devId, true)) {
                const auto *devInfo = Pa_GetDeviceInfo(devId);
                deviceList.emplace(devId, devInfo);
            }
        }
        Pa_Terminate();
    }
    return std::move(deviceList);
}

std::map<int,const PaDeviceInfo *> AudioDevice::getCompatibleOutputDevicesForApi(AudioDevice::Api api)
{
    std::map<int,const PaDeviceInfo *> deviceList;
    auto rv = Pa_Initialize();
    if (rv == paNoError) {
        auto apiInfo = Pa_GetHostApiInfo(api);

        for (unsigned i = 0; i < apiInfo->deviceCount; i++) {
            auto devId = Pa_HostApiDeviceIndexToDeviceIndex(api, i);
            if (devId < 0) {
                continue;
            }
            if (isAbleToOpen(devId, false)) {
                const auto *devInfo = Pa_GetDeviceInfo(devId);
                deviceList.emplace(devId, devInfo);
            }
        }
        Pa_Terminate();
    }
    return std::move(deviceList);
}



std::vector<std::string> AudioDevice::getInputDevicesForApi(AudioDevice::Api api) {
    std::vector<std::string> deviceList;

    auto apiInputDevs = getCompatibleInputDevicesForApi(api);
    for (const auto &apiPair: apiInputDevs) {
        deviceList.emplace_back(apiPair.second->name);
    }
    return std::move(deviceList);
}

std::vector<std::string> AudioDevice::getOutputDevicesForApi(AudioDevice::Api api)
{
    std::vector<std::string> deviceList;

    auto apiOutputDevs = getCompatibleOutputDevicesForApi(api);
    for (const auto &apiPair: apiOutputDevs) {
        deviceList.emplace_back(apiPair.second->name);
    }
    return std::move(deviceList);
}

bool AudioDevice::isAbleToOpen(int deviceId, int forInput) {
    auto devInfo = Pa_GetDeviceInfo(deviceId);
    if (devInfo) {
        if (forInput) {
            if (devInfo->maxInputChannels > 0) {
                PaStreamParameters inputTest = {
                        deviceId,
                        1,
                        paFloat32,
                        devInfo->defaultLowInputLatency,
                        nullptr,
                };
                if (0 == Pa_IsFormatSupported(&inputTest, nullptr, sampleRateHz)) {
                    return true;
                }
            }
        } else {
            if (devInfo->maxOutputChannels > 0) {
                PaStreamParameters outputTest = {
                        deviceId,
                        1,
                        paFloat32,
                        devInfo->defaultLowOutputLatency,
                        nullptr,
                };
                if (0 == Pa_IsFormatSupported(nullptr, &outputTest, sampleRateHz)) {
                    return true;
                }
            }
        }
    }
    return false;
}

