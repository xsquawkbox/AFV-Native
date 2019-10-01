/* test/http/test_http_async.cpp
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

#include <ctime>

#include <afv-native/http/TransferManager.h>
#include <afv-native/http/Request.h>
#include <afv-native/http/EventTransferManager.h>

using namespace afv_native::http;
using namespace std;

TEST(TransferManager,AsyncSimple1) {
    time_t start = time(nullptr);

    TransferManager tm;
    auto r = make_shared<Request>("http://status.vatsim.net/", Method::GET);

    r->shareState(tm);
    r->doAsync(tm);

    while ((time(nullptr) < start+20) && r->getProgress() != Progress::Finished) {
        tm.process();
        RecordProperty("UploadProgress", r->getUploadProgress());
        RecordProperty("UploadTotal", r->getUploadTotal());
        RecordProperty("DownloadProgress", r->getDownloadProgress());
        RecordProperty("DownloadTotal", r->getDownloadTotal());
    }
    EXPECT_TRUE(r->getProgress() == Progress::Finished);
    EXPECT_EQ(r->getStatusCode(), 200);
    EXPECT_EQ(r->getContentType(), "text/plain");
}

TEST(EventTransferManager,Simple1) {
    time_t start = time(nullptr);

    auto *evb = event_base_new();

    EventTransferManager tm(evb);
    auto r = make_shared<Request>("http://status.vatsim.net/", Method::GET);

    r->shareState(tm);
    r->doAsync(tm);

    while ((time(nullptr) < start+20) && r->getProgress() != Progress::Finished) {
        event_base_loop(evb, EVLOOP_ONCE);
    }
    EXPECT_TRUE(r->getProgress() == Progress::Finished);
    EXPECT_EQ(r->getStatusCode(), 200);
    EXPECT_EQ(r->getContentType(), "text/plain");
}