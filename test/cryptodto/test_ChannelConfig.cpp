/* test/cryptodto/test_ChannelConfig.cpp
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

#include "afv-native/cryptodto/dto/ChannelConfig.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>

using namespace std;
using namespace afv_native::cryptodto::dto;
using json = nlohmann::json;

TEST(ChannelConfig, Deserialise1)
{
    std::string test_json = R"({
        "channelTag": "abc123",
        "aeadReceiveKey": "N/v4cGEr0ko04zsli340q+r5eRrctKbfQJo6tJ88UtM=",
        "aeadTransmitKey": "594N9AjfiCL8b/P1oWA0i0IgUNg2mwbVvQI7yfb0Ud8=",
        "hmacKey": "totallyFakeHmacKey"
    })";

    unsigned char rxKey[32] = {
        0x37, 0xfb, 0xf8, 0x70, 0x61, 0x2b, 0xd2, 0x4a, 0x34, 0xe3, 0x3b, 0x25, 0x8b, 0x7e, 0x34, 0xab,
        0xea, 0xf9, 0x79, 0x1a, 0xdc, 0xb4, 0xa6, 0xdf, 0x40, 0x9a, 0x3a, 0xb4, 0x9f, 0x3c, 0x52, 0xd3
    };
    unsigned char txKey[32] = {
        0xe7, 0xde, 0x0d, 0xf4, 0x08, 0xdf, 0x88, 0x22, 0xfc, 0x6f, 0xf3, 0xf5, 0xa1, 0x60, 0x34, 0x8b,
        0x42, 0x20, 0x50, 0xd8, 0x36, 0x9b, 0x06, 0xd5, 0xbd, 0x02, 0x3b, 0xc9, 0xf6, 0xf4, 0x51, 0xdf
    };

    auto j = json::parse(test_json);

    ChannelConfig cc;
    j.get_to(cc);

    EXPECT_EQ(cc.ChannelTag, "abc123") << "ChannelTag mismatch";
    EXPECT_TRUE(0 == memcmp(cc.AeadReceiveKey, rxKey, 32)) << "rxKey mismatch";
    EXPECT_TRUE(0 == memcmp(cc.AeadTransmitKey, txKey, 32)) << "txKey mismatch";
}