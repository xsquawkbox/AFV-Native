/* test/util/test_base64.cpp
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

#include "afv-native/util/base64.h"
#include <gtest/gtest.h>
#include <cstring>

using namespace afv_native::util;

TEST(Base64Encode, Simple1)
{
    const char *bufIn = "abcd1234";
    auto strOut = Base64Encode(reinterpret_cast<const unsigned char *>(bufIn), strlen(bufIn));

    ASSERT_EQ(strOut, "YWJjZDEyMzQ=");
}

TEST(Base64Decode, Simple1)
{
    const std::string encodedIn = "dGVzdCBkYXRhIDEyMzQ=";
    const std::string expectedOut = "test data 1234";

    const size_t outBufLen = Base64DecodeLen(encodedIn.length());
    ASSERT_GE(outBufLen, expectedOut.length()) << "underallocated output buffer";

    // allocate 1 extra to ensure we have a NUL at the end for when we then string convert the result.
    auto *bufOut = new unsigned char[outBufLen+1];
    memset(bufOut, 0, outBufLen+1);

    Base64Decode(encodedIn, bufOut, outBufLen);

    ASSERT_EQ(std::string(reinterpret_cast<char *>(bufOut)), expectedOut) << "didn't get input";

    delete[] bufOut;
}