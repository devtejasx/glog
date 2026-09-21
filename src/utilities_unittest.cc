// Copyright (c) 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Shinichiro Hamaji
#include "utilities.h"

#include "glog/logging.h"
#include "googletest.h"

#ifdef GLOG_USE_GFLAGS
#  include <gflags/gflags.h>
using namespace GFLAGS_NAMESPACE;
#endif

using namespace google;

TEST(utilities, InitGoogleLoggingDeathTest) {
  ASSERT_DEATH(InitGoogleLogging("foobar"), "");
}

namespace {
class CountingSink : public google::LogSink {
 public:
  void send(LogSeverity /*severity*/, const char* /*full_filename*/,
            const char* /*base_filename*/, int /*line*/,
            const LogMessageTime& /*time*/, const char* /*message*/,
            size_t /*message_len*/) override {
    ++count;
  }

  int count = 0;
};

template <typename Log>
int CountEmitted(int iterations, Log log) {
  CountingSink sink;
  google::AddLogSink(&sink);
  for (int i = 0; i < iterations; ++i) {
    log(i);
  }
  google::RemoveLogSink(&sink);
  return sink.count;
}
}  // namespace

// LOG_IF_EVERY_N used to compute "% n", so n == 0 divided by zero. It now
// counts the same way as LOG_EVERY_N. Each macro keeps its counter in a
// static at its call site, so every case gets its own call sites.
TEST(LogEveryN, IfEveryNMatchesEveryN) {
  EXPECT_EQ(CountEmitted(7, [](int) { LOG_IF_EVERY_N(INFO, true, 1) << "x"; }),
            CountEmitted(7, [](int) { LOG_EVERY_N(INFO, 1) << "x"; }));
  EXPECT_EQ(CountEmitted(7, [](int) { LOG_IF_EVERY_N(INFO, true, 2) << "x"; }),
            CountEmitted(7, [](int) { LOG_EVERY_N(INFO, 2) << "x"; }));
  EXPECT_EQ(CountEmitted(7, [](int) { LOG_IF_EVERY_N(INFO, true, 3) << "x"; }),
            3);
}

TEST(LogEveryN, IfEveryNWithZeroPeriodDoesNotCrash) {
  EXPECT_EQ(CountEmitted(5, [](int) { LOG_IF_EVERY_N(INFO, true, 0) << "x"; }),
            CountEmitted(5, [](int) { LOG_EVERY_N(INFO, 0) << "x"; }));
}

TEST(LogEveryN, IfEveryNCountsOnlyWhenConditionHolds) {
  // Iterations 0, 2, 4, 6 pass the condition; every second of those logs.
  EXPECT_EQ(CountEmitted(
                8, [](int i) { LOG_IF_EVERY_N(INFO, i % 2 == 0, 2) << "x"; }),
            2);
}

int main(int argc, char** argv) {
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);

  CHECK_EQ(RUN_ALL_TESTS(), 0);
}
