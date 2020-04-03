//
// Created by chris on 9/10/2019.
//

#include <iostream>

#include "afv-native/Log.h"
#include "afv-native/audio/AudioDevice.h"
#include "afv-native/audio/WhiteNoiseGenerator.h"
#include "afv-native/audio/PinkNoiseGenerator.h"

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

    auto white = std::make_shared<audio::WhiteNoiseGenerator>(0.7);
    {
        LOG("dsptest", "creating audio device");
        auto soundDevice = audio::AudioDevice::makeDevice(
            "dsptest",
            deviceId,
            "",
            defaultApi);
        soundDevice->setSource(white);
        soundDevice->open();
        LOG("dsptest", "whitenoise should be playing.  Press enter to end.");
        char input;
        std::cin.get( input );
        soundDevice->close();
        soundDevice.reset();
    }

    auto pink = std::make_shared<audio::PinkNoiseGenerator>(0.7);
    {
        LOG("dsptest", "creating audio device");
        auto soundDevice = audio::AudioDevice::makeDevice(
                "dsptest",
                deviceId,
                "",
                defaultApi);
        soundDevice->setSource(pink);
        soundDevice->open();
        LOG("dsptest", "pinknoise should be playing.  Press enter to end.");
        char input;
        std::cin.get( input );
        soundDevice->close();
        soundDevice.reset();
    }


    LOG("dsptest", "Exiting.");
    return 0;
}