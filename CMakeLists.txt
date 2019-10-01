cmake_minimum_required(VERSION 3.10)
project(afv_native)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(CMAKE_CXX_STANDARD 14)
enable_testing()

set(AFV_NATIVE_HEADERS
		include/afv-native/Client.h
		include/afv-native/event.h
		include/afv-native/Log.h
		include/afv-native/afv/APISession.h
		include/afv-native/afv/EffectResources.h
		include/afv-native/afv/params.h
		include/afv-native/afv/RadioSimulation.h
		include/afv-native/afv/RemoteVoiceSource.h
		include/afv-native/afv/RollingAverage.h
		include/afv-native/afv/VoiceCompressionSink.h
		include/afv-native/afv/VoiceSession.h
		include/afv-native/afv/dto/AuthRequest.h
		include/afv-native/afv/dto/PostCallsignResponse.h
		include/afv-native/afv/dto/Station.h
		include/afv-native/afv/dto/Transceiver.h
		include/afv-native/afv/dto/VoiceServerConnectionData.h
		include/afv-native/afv/dto/interfaces/IAudio.h
		include/afv-native/afv/dto/interfaces/IAudioDirect.h
		include/afv-native/afv/dto/interfaces/IAudioVoiceRoom.h
		include/afv-native/afv/dto/domain/RxTransceiver.h
		include/afv-native/afv/dto/domain/TxTransceiver.h
		include/afv-native/afv/dto/voice_server/AudioOnDirect.h
		include/afv-native/afv/dto/voice_server/AudioRxOnTransceivers.h
		include/afv-native/afv/dto/voice_server/AudioTxOnTransceivers.h
		include/afv-native/afv/dto/voice_server/Heartbeat.h
		include/afv-native/audio/audio_params.h
		include/afv-native/audio/AudioDevice.h
		include/afv-native/audio/ISampleSink.h
		include/afv-native/audio/ISampleSource.h
		include/afv-native/audio/ISampleStorage.h
		include/afv-native/audio/OutputMixer.h
		include/afv-native/audio/RecordedSampleSource.h
		include/afv-native/audio/SineToneSource.h
		include/afv-native/audio/SinkFrameSizeAdjuster.h
		include/afv-native/audio/SourceFrameSizeAdjuster.h
		include/afv-native/audio/SpeexPreprocessor.h
		include/afv-native/audio/WavFile.h
		include/afv-native/audio/WavSampleStorage.h
		include/afv-native/cryptodto/Channel.h
		include/afv-native/cryptodto/dto/ICryptoDTO.h
		include/afv-native/cryptodto/params.h
		include/afv-native/cryptodto/SequenceTest.h
		include/afv-native/cryptodto/UDPChannel.h
		include/afv-native/cryptodto/dto/ChannelConfig.h
		include/afv-native/cryptodto/dto/Header.h
		include/afv-native/event/EventTimer.h
		include/afv-native/event/EventCallbackTimer.h
		include/afv-native/http/EventTransferManager.h
		include/afv-native/http/http.h
		include/afv-native/http/Request.h
		include/afv-native/http/RESTRequest.h
		include/afv-native/http/TransferManager.h
		include/afv-native/util/base64.h
		include/afv-native/util/ChainedCallback.h
		include/afv-native/util/monotime.h
		)
add_library(afv_native
		${AFV_NATIVE_HEADERS}
		src/afv/APISession.cpp
		src/afv/EffectResources.cpp
		src/afv/RadioSimulation.cpp
		src/afv/RemoteVoiceSource.cpp
		src/afv/VoiceCompressionSink.cpp
		src/afv/VoiceSession.cpp
		src/afv/dto/AuthRequest.cpp
		src/afv/dto/PostCallsignResponse.cpp
		src/afv/dto/Station.cpp
		src/afv/dto/Transceiver.cpp
		src/afv/dto/VoiceServerConnectionData.cpp
		src/audio/AudioDevice.cpp
		src/audio/OutputMixer.cpp
		src/audio/RecordedSampleSource.cpp
		src/audio/SineToneSource.cpp
		src/audio/SinkFrameSizeAdjuster.cpp
		src/audio/SourceFrameSizeAdjuster.cpp
		src/audio/SpeexPreprocessor.cpp
		src/audio/WavFile.cpp
		src/audio/WavSampleStorage.cpp
		src/core/Client.cpp
		src/core/Log.cpp
        src/cryptodto/Channel.cpp
        src/cryptodto/SequenceTest.cpp
		src/cryptodto/UDPChannel.cpp
        src/cryptodto/dto/ChannelConfig.cpp
		src/cryptodto/dto/Header.cpp
		src/event/EventCallbackTimer.cpp
		src/event/EventTimer.cpp
		src/http/EventTransferManager.cpp
		src/http/TransferManager.cpp
		src/http/Request.cpp
		src/http/RESTRequest.cpp
        src/util/base64.cpp
		src/util/monotime.cpp
		)
target_link_libraries(afv_native
        PUBLIC
        CONAN_PKG::OpenSSL
        CONAN_PKG::msgpack
        CONAN_PKG::jsonformoderncpp
        CONAN_PKG::libcurl
        CONAN_PKG::libevent
        CONAN_PKG::portaudio
        CONAN_PKG::libopus
		CONAN_PKG::speexdsp)
target_include_directories(afv_native
		PRIVATE ${CMAKE_SOURCE_DIR}/extern/cpp-jwt/include
        PUBLIC ${CMAKE_SOURCE_DIR}/include)

include(GoogleTest)
add_executable(
        afv_native_test
        test/main.cpp
		test/audio/test_SinkFrameSizeAdapter.cpp
		test/audio/test_SourceFrameSizeAdapter.cpp
		test/cryptodto/test_ChannelConfig.cpp
        test/cryptodto/test_SequenceTest.cpp
		test/http/test_http_async.cpp
		test/http/test_http_sync.cpp
		test/util/test_base64.cpp
)
target_link_libraries(afv_native_test
        CONAN_PKG::gtest
        afv_native)
gtest_discover_tests(afv_native_test
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(audiotest
        examples/audiotest/main.cpp)
target_link_libraries(audiotest
        afv_native)

add_executable(
		testclient
        examples/testclient/main.cpp
        examples/testclient/TestClient.cpp
        examples/testclient/TestClient.h
		extern/imgui/imgui.cpp
		extern/imgui/imgui_draw.cpp
		extern/imgui/imgui_widgets.cpp
		extern/imgui/imgui_internal.h
		extern/imgui/imgui_demo.cpp
		extern/imgui/imconfig.h
		extern/imgui/imgui.h
		extern/imgui/misc/cpp/imgui_stdlib.cpp
		extern/imgui/misc/cpp/imgui_stdlib.h
		extern/imgui/examples/imgui_impl_opengl3.cpp
		extern/imgui/examples/imgui_impl_opengl3.h
		extern/imgui/examples/imgui_impl_sdl.cpp
		extern/imgui/examples/imgui_impl_sdl.h
		)
target_include_directories(
		testclient
		PRIVATE
		${CMAKE_SOURCE_DIR}/extern/imgui
		${CMAKE_SOURCE_DIR}/extern/imgui/examples
		${CMAKE_SOURCE_DIR}/extern/imgui/misc/cpp
		)
target_compile_definitions(testclient
		PRIVATE
		IMGUI_IMPL_OPENGL_LOADER_GLEW=1)
target_link_libraries(testclient
		afv_native
		CONAN_PKG::libevent
		CONAN_PKG::sdl2
		CONAN_PKG::glew)

install(TARGETS afv_native
		RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
		)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include" # source directory
		DESTINATION "." # target directory
		FILES_MATCHING # install only matched files
		PATTERN "*.h" # select header files
		)