/* test/http/test_http_sync.cpp
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

#include <gtest/gtest.h>
#include <afv-native/http/Request.h>
#include <afv-native/http/TransferManager.h>

#include <string>

using namespace std;
using namespace afv_native::http;

TEST(Request,SyncGet_Simple1) {
    Request r("http://status.vatsim.net/", Method::GET);

    ASSERT_TRUE(r.doSync());

    EXPECT_TRUE(r.getProgress() == Progress::Finished);
    EXPECT_EQ(r.getStatusCode(), 200);
    EXPECT_EQ(r.getContentType(), "text/plain");
}

TEST(Request,SyncGetHttps_Simple1) {
    auto r = make_shared<Request>("https://status.vatsim.net/", Method::GET);

    ASSERT_TRUE(r->doSync());

    EXPECT_TRUE(r->getProgress() == Progress::Finished);
    EXPECT_EQ(r->getStatusCode(), 200);
    EXPECT_EQ(r->getContentType(), "text/plain");
}


// NB:  this should be faster than 2x SyncGetHttps_Simple1 as the SSL session
//      data and connection should be shared between the two sync requests.
TEST(Request,SyncGetHttps_Shared1) {
    TransferManager tm;
    auto r = make_shared<Request>("https://status.vatsim.net/", Method::GET);

    r->shareState(tm);
    ASSERT_TRUE(r->doSync());
    EXPECT_TRUE(r->getProgress() == Progress::Finished);
    EXPECT_EQ(r->getStatusCode(), 200);
    EXPECT_EQ(r->getContentType(), "text/plain");

    // repeat the last sequence.
    auto r2 = make_shared<Request>("https://status.vatsim.net/", Method::GET);

    r2->shareState(tm);
    EXPECT_TRUE(r2->doSync());
    EXPECT_TRUE(r2->getProgress() == Progress::Finished);
    EXPECT_EQ(r2->getStatusCode(), 200);
    EXPECT_EQ(r2->getContentType(), "text/plain");
}