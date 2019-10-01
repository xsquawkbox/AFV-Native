/* test/cryptodto/test_SequenceTest.cpp
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

#include <afv-native/cryptodto/SequenceTest.h>

using namespace afv_native::cryptodto;

TEST(SequenceTest, Simple1)
{
    SequenceTest s(20, 8);

    ASSERT_TRUE(s.Received(20) == ReceiveOutcome::OK) << "Didn't accept first sequence";
    EXPECT_EQ(s.GetNext(), 21) << "didn't advance expected sequence";
    EXPECT_TRUE(s.Received(20) == ReceiveOutcome::Before) << "Didn't reject repeated seqeunce";
    EXPECT_TRUE(s.Received(31) == ReceiveOutcome::Overflow) << "Didn't overflow on +10 with window 8";
}

TEST(SequenceTest, OutOfOrder1)
{
    SequenceTest s(20, 8);
    ASSERT_EQ(s.GetNext(), 20);
    ASSERT_TRUE(s.Received(21) == ReceiveOutcome::OK) << "Didn't accept 1 in future";
    EXPECT_EQ(s.GetNext(), 20) << "Advanced sequence incorrectly";
    EXPECT_TRUE(s.Received(20) == ReceiveOutcome::OK) << "Didn't accept late next sequence";
    EXPECT_EQ(s.GetNext(), 22) << "Didn't advance sequence past received bits";
}

TEST(SequenceTest, OutOfOrder2)
{
    SequenceTest s(20, 8);
    ASSERT_EQ(s.GetNext(), 20);
    ASSERT_TRUE(s.Received(21) == ReceiveOutcome::OK) << "Didn't accept 1 in future";
    EXPECT_EQ(s.GetNext(), 20) << "Advanced sequence incorrectly";
    ASSERT_TRUE(s.Received(23) == ReceiveOutcome::OK) << "Didn't accept 1 in future";
    EXPECT_EQ(s.GetNext(), 20) << "Advanced sequence incorrectly";
    EXPECT_TRUE(s.Received(20) == ReceiveOutcome::OK) << "Didn't accept late next sequence";
    EXPECT_EQ(s.GetNext(), 22) << "Didn't advance sequence past received bits";
    EXPECT_TRUE(s.Received(24) == ReceiveOutcome::OK) << "Didn't accept late next sequence";
    EXPECT_EQ(s.GetNext(), 22) << "Advanced sequence incorrectly";
    EXPECT_TRUE(s.Received(22) == ReceiveOutcome::OK) << "Didn't accept late next sequence";
    EXPECT_EQ(s.GetNext(), 25) << "Didn't advance sequence past second received bits";
}

TEST(SequenceTest, OutOfOrderReplay2)
{
    SequenceTest s(20, 8);
    ASSERT_EQ(s.GetNext(), 20);
    ASSERT_TRUE(s.Received(21) == ReceiveOutcome::OK) << "Didn't accept 1 in future";
    ASSERT_TRUE(s.Received(23) == ReceiveOutcome::OK) << "Didn't accept 1 in future";
    ASSERT_TRUE(s.Received(24) == ReceiveOutcome::OK) << "Didn't accept late next sequence";
    ASSERT_EQ(s.GetNext(), 20);
    // now, replay a caught future packet.
    EXPECT_TRUE(s.Received(21) == ReceiveOutcome::Before) << "Accepted replay";
}