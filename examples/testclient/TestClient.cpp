/* testclient/TestClient.cpp
 *
 * This file is part of AFV-Native.
 *
 * Copyright (c) 2019, Christopher Collins
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

#include "TestClient.h"

#include <iostream>
#include <GL/glew.h>
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"

// Decide GL+GLSL versions
#if __APPLE__
// GL 3.2 Core + GLSL 150
const char* glsl_version = "#version 150";
#else

// GL 3.0 + GLSL 130
const char *glsl_version = "#version 130";

#endif

TestClient::TestClient(
        SDL_Window *win,
        SDL_GLContext glContext,
        struct event_base *eventBase):
        mWindow(win),
        mContext(glContext),
        mEventBase(eventBase),
        mShouldQuit(false),
        mShowDemo(false),
        mFontAtlas(),
        mClient()
{
    mClient = std::make_shared<afv_native::Client>(
            mEventBase,
            ".",
            2,
            "AFV-TestClient");

    // get the audio system information.
    mAudioProviders = afv_native::audio::AudioDevice::getAPIs();
    setAudioApi(0);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    //FIXME: this should pull from a more appropriate place
    mFontAtlas.AddFontFromFileTTF("Roboto-Regular.ttf", 16);
    mFontAtlas.AddFontDefault();

    ImGui::CreateContext(&mFontAtlas);
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(mWindow, mContext);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void
TestClient::drawFrame()
{
    if (SDL_GL_MakeCurrent(mWindow, mContext)) {
        std::cerr << "Couldn't make GL context current: " << SDL_GetError() << std::endl;
        exit(1);
    }

    int width, height;
    SDL_GetWindowSize(mWindow, &width, &height);

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize.x = width;
    io.DisplaySize.y = height;

    int nativeWidth, nativeHeight;
    SDL_GL_GetDrawableSize(mWindow, &nativeWidth, &nativeHeight);
    io.DisplayFramebufferScale.x = static_cast<double>(nativeWidth) / static_cast<double>(width);
    io.DisplayFramebufferScale.y = static_cast<double>(nativeHeight) / static_cast<double>(height);

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(mWindow);
    ImGui::NewFrame();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // main menu shenanigans.
    guiMainMenu();

    //FIXME: draw our render queue here.
    if (mShowDemo) {
        ImGui::ShowDemoWindow(&mShowDemo);
    }

    // update certain things...
    if (mClient) {
        mPeak = mClient->getInputPeak();
        mVu = mClient->getInputVu();
    }

    ImGui::Begin("AFV-Native Test Client");
    ImGui::InputText("AFV API Server URL", &mAFVAPIServer, ImGuiInputTextFlags_CharsNoBlank);
    ImGui::InputText("AFV Username", &mAFVUsername, ImGuiInputTextFlags_CharsNoBlank);
    ImGui::InputText("AFV Password", &mAFVPassword, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_Password);
    ImGui::InputText(
            "AFV Callsign",
            &mAFVCallsign,
            ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsUppercase);
    if (ImGui::CollapsingHeader("Audio Configuration")) {
        if (ImGui::BeginCombo("Sound API", nameForAudioApi(mAudioApi).c_str())) {
            if (ImGui::Selectable("Default", mAudioApi == 0)) {
                setAudioApi(0);
            }
            for (const auto &item: mAudioProviders) {
                if (ImGui::Selectable(item.second.c_str(), mAudioApi == item.first)) {
                    setAudioApi(item.first);
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::BeginCombo("Input Device", mInputDevice.c_str())) {
            if (ImGui::Selectable("Default", mInputDevice.empty())) {
                mInputDevice = "";
            }
            for (const auto &item: mInputDevices) {
                if (ImGui::Selectable(item.c_str(), mInputDevice == item)) {
                    mInputDevice = item;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::BeginCombo("Output Device", mOutputDevice.c_str())) {
            if (ImGui::Selectable("Default", mInputDevice.empty())) {
                mOutputDevice = "";
            }
            for (const auto &item: mOutputDevices) {
                if (ImGui::Selectable(item.c_str(), mOutputDevice == item)) {
                    mOutputDevice = item;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Checkbox("Enable Input Filter", &mInputFilter)) {
            if (mClient) {
                mClient->setEnableInputFilters(mInputFilter);
            }
        }
        if (ImGui::Checkbox("Enable Output Effects", &mOutputEffects)) {
            if (mClient) {
                mClient->setEnableOutputEffects(mOutputEffects);
            }
        }
    }
    if (ImGui::CollapsingHeader("Client Position")) {
        ImGui::InputDouble("Latitude", &mClientLatitude);
        ImGui::InputDouble("Longitude", &mClientLongitude);
        ImGui::InputDouble("Altitude (AMSL) (M)", &mClientAltitudeMSLM);
        ImGui::InputDouble("Height (AGL) (M)", &mClientAltitudeAGLM);
    }

    if (ImGui::CollapsingHeader("COM1", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::InputInt("COM1 Frequency (Hz)", &mCom1Freq, 5000, 1000000)) {
            if (mClient) {
                mClient->setRadioState(0, mCom1Freq);
            }
        }
        if (ImGui::SliderFloat("COM1 Gain", &mCom1Gain, 0.0f, 1.5f)) {
            mCom1Gain = std::min(1.5f, mCom1Gain);
            mCom1Gain = std::max(0.0f, mCom1Gain);
            if (mClient) {
                mClient->setRadioGain(0, mCom1Gain);
            }
        }
    }
    if (ImGui::CollapsingHeader("COM2", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::InputInt("COM2 Frequency (Hz)", &mCom2Freq, 5000, 1000000)) {
            if (mClient) {
                mClient->setRadioState(1, mCom2Freq);
            }
        }
        if (ImGui::SliderFloat("COM2 Gain", &mCom2Gain, 0.0f, 1.5f)) {
            mCom2Gain = std::min(1.5f, mCom2Gain);
            mCom2Gain = std::max(0.0f, mCom2Gain);
            if (mClient) {
                mClient->setRadioGain(1, mCom2Gain);
            }
        }
    }
    if (ImGui::BeginCombo("Transmission Radio", (mTxRadio==0)?"COM1":"COM2")) {
        if (ImGui::Selectable("COM1", mTxRadio==0)) {
            mTxRadio = 0;
            if (mClient) {
                mClient->setTxRadio(mTxRadio);
            }
        }
        if (ImGui::Selectable("COM2", mTxRadio==1)) {
            mTxRadio = 1;
            if (mClient) {
                mClient->setTxRadio(mTxRadio);
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::Checkbox("Push to Talk", &mPTT)) {
        if (mClient) {
            mClient->setPtt(mPTT);
        }
    }

    const ImVec4 red(1.0, 0.0, 0.0, 1.0), green(0.0, 1.0, 0.0, 1.0);
    ImGui::TextColored(mClient->isAPIConnected() ? green : red, "API Server Connection");
    ImGui::TextColored(mClient->isVoiceConnected() ? green : red, "Voice Server Connection");
    ImGui::SliderFloat("Input VU", &mVu,-60.0f, 0.0f);
    ImGui::SliderFloat("Input Peak", &mPeak, -60.0f, 0.0f);

    if (!mClient->isVoiceConnected()) {
        if (ImGui::Button("Connect Voice")) {
            mClient->setAudioApi(mAudioApi);
            mClient->setAudioInputDevice(mInputDevice);
            mClient->setAudioOutputDevice(mOutputDevice);
            mClient->setBaseUrl(mAFVAPIServer);
            mClient->setClientPosition(mClientLatitude, mClientLongitude, mClientAltitudeMSLM, mClientAltitudeAGLM);
            mClient->setRadioState(0, mCom1Freq);
            mClient->setRadioState(1, mCom2Freq);
            mClient->setTxRadio(mTxRadio);
            mClient->setRadioGain(0, mCom1Gain);
            mClient->setRadioGain(1, mCom2Gain);
            mClient->setCredentials(mAFVUsername, mAFVPassword);
            mClient->setCallsign(mAFVCallsign);
            mClient->setEnableInputFilters(mInputFilter);
            mClient->setEnableOutputEffects(mOutputEffects);
            mClient->connect();
        }
    } else {
        if (ImGui::Button("Disconnect")) {
            mClient->disconnect();
        }
    }
    ImGui::End();

    // Start the Dear ImGui frame
    ImGui::Render();
    glViewport(0, 0, width, height);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(mWindow);
}

void
TestClient::handleInput()
{
    SDL_Event thisEvent;
    while (SDL_PollEvent(&thisEvent) != 0) {
        ImGui_ImplSDL2_ProcessEvent(&thisEvent);
        switch (thisEvent.type) {
        case SDL_QUIT:
            mShouldQuit = true;
            break;
        case SDL_WINDOWEVENT:
            if (thisEvent.window.event == SDL_WINDOWEVENT_CLOSE &&
                thisEvent.window.windowID == SDL_GetWindowID(mWindow)) {
                mShouldQuit = true;
            }
            break;
        default:
            break;
        }
    }
}

bool
TestClient::shouldQuit() const
{
    return mShouldQuit;
}

void TestClient::guiMainMenu()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Quit")) {
                mShouldQuit = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Misc")) {
            ImGui::MenuItem("Show Demo", nullptr, &mShowDemo);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

std::string TestClient::nameForAudioApi(afv_native::audio::AudioDevice::Api apiNum)
{
    auto mIter = mAudioProviders.find(apiNum);
    if (mIter == mAudioProviders.end()) {
        if (apiNum == 0) return "UNSPECIFIED (default)";
        return "INVALID";
    }
    return mIter->second;
}

void TestClient::setAudioApi(afv_native::audio::AudioDevice::Api apiNum)
{
    mAudioApi = apiNum;
    mInputDevices = afv_native::audio::AudioDevice::getInputDevicesForApi(mAudioApi);
    mOutputDevices = afv_native::audio::AudioDevice::getOutputDevicesForApi(mAudioApi);
}
