//
// Created by chris on 9/10/2019.
//

#include <iostream>

#include "afv-native/Log.h"
#include "afv-native/audio/AudioDevice.h"
#include "afv-native/audio/RecordedSampleSource.h"
#include "afv-native/audio/SineToneSource.h"
#include "afv-native/audio/WavFile.h"
#include "afv-native/audio/WavSampleStorage.h"

using namespace afv_native;

void
logger(const char *subsystem, const char *file, int line, const char *output)
{
    printf("%s: %s(%d): %s\n", subsystem, file, line, output);
    fflush(stdout);
}

int
main(int argc, char **argv)
{
    setLogger(logger);

    auto allApis = audio::AudioDevice::getAPIs();
    for (const auto &apiPair: allApis) {
        std::cout << "API " << static_cast<int>(apiPair.first) << ": " << apiPair.second << std::endl;
    }
    const auto defaultApi = allApis.begin()->first;

    auto allOutputDevs = audio::AudioDevice::getCompatibleOutputDevicesForApi(defaultApi);
    for (const auto &devPair: allOutputDevs) {
        std::cout << "Output Device " << devPair.first << ": " << devPair.second.name << std::endl;
    }
    auto deviceId = allOutputDevs.begin()->second.id;

    auto *wavData = audio::LoadWav("testsnd.wav");
    if (nullptr == wavData) {
        LOG("audiotest", "failed to load wavfile");
    } else {
        auto thisSound = std::make_shared<audio::WavSampleStorage>(*wavData);
        auto soundPlayer = std::make_shared<audio::RecordedSampleSource>(thisSound, false);

        {
            LOG("audiotest", "creating audio device");
            auto soundDevice = audio::AudioDevice::makeDevice(
                    "audiotest",
                    deviceId,
                    "",
                    defaultApi);
            soundDevice->setSource(soundPlayer);
            soundDevice->open();
            LOG("audiotest", "sample should be playing");
            while (soundPlayer->isPlaying()) {
            }
            LOG("audiotest", "waiting for bufferlocks to give up");
            while (soundPlayer.use_count() > 1) {}

            soundDevice->close();
            soundDevice.reset();
        }
    }

    auto sine = std::make_shared<audio::SineToneSource>(160);
    {
        LOG("audiotest", "creating audio device");
        auto soundDevice = audio::AudioDevice::makeDevice(
            "audiotest",
            deviceId,
            "",
            defaultApi);
        soundDevice->setSource(sine);
        soundDevice->open();
        LOG("audiotest", "sinetone should be playing.  Press enter to end.");
        char input;
        std::cin.get( input );
        soundDevice->close();
        soundDevice.reset();
    }

    LOG("audiotest", "Exiting.");
    return 0;
}