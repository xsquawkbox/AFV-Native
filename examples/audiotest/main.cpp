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

    auto *wavData = audio::LoadWav("testsnd.wav");
    if (nullptr == wavData) {
        LOG("audiotest", "failed to load wavfile");
    } else {
        auto thisSound = std::make_shared<audio::WavSampleStorage>(*wavData);
        auto soundPlayer = std::make_shared<audio::RecordedSampleSource>(thisSound, false);

        {
            LOG("audiotest", "creating audio device");
            auto soundDevice = audio::AudioDevice(
                    "audiotest",
                    "",
                    "",0);
            soundDevice.setSource(soundPlayer);
            soundDevice.open();
            LOG("audiotest", "sample should be playing");
            while (soundPlayer->isPlaying()) {
            }
            LOG("audiotest", "waiting for bufferlocks to give up");
            while (soundPlayer.use_count() > 1) {}
        }
    }

    auto sine = std::make_shared<audio::SineToneSource>(160);
    {
        LOG("audiotest", "creating audio device");
        auto soundDevice = audio::AudioDevice("audiotest", "", "", 0);
        soundDevice.setSource(sine);
        soundDevice.open();
        LOG("audiotest", "sinetone should be playing.  Press enter to end.");
        char input;
        std::cin.get( input );
    }

    LOG("audiotest", "Exiting.");
    return 0;
}