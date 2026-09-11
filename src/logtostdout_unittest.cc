// Copyright (c) 2026, Google Inc.
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

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "base/commandlineflags.h"
#include "glog/logging.h"
#include "googletest.h"

#ifdef GLOG_USE_GFLAGS
#  include <gflags/gflags.h>
using namespace GFLAGS_NAMESPACE;
#endif

using namespace google;

namespace {

constexpr char kInfoMessage[] = "buffered info message";
constexpr char kWarningMessage[] = "unbuffered warning message";

// Reads what has actually reached the capture file so far. Unlike
// GetCapturedTestStdout(), this does not stop the capture and therefore does
// not fflush(3) the stream itself, which is what makes it possible to observe
// whether glog flushed stdout on its own.
string PeekCapturedStdout(const CapturedStream& capture) {
  std::ifstream file{capture.filename(), std::ios::binary};
  std::ostringstream contents;
  contents << file.rdbuf();
  return contents.str();
}

}  // namespace

// Regression test for https://github.com/google/glog/issues/943: with stdout
// fully buffered, a message whose severity exceeds --logbuflevel must be
// flushed by glog itself instead of lingering in the stdio buffer.
TEST(LogToStdout, HonorsLogbuflevel) {
  string after_info;
  string after_warning;

  {
    CapturedStream capture{fileno(stdout),
                           FLAGS_test_tmpdir + "/logtostdout.out"};

    // stdout now refers to a regular file; make sure it is fully buffered so
    // that messages which are not explicitly flushed stay in the stdio buffer.
    setvbuf(stdout, nullptr, _IOFBF, BUFSIZ);

    FLAGS_logtostdout = true;
    FLAGS_colorlogtostdout = false;
    FLAGS_logbuflevel = GLOG_INFO;  // buffer INFO, flush WARNING and above

    LOG(INFO) << kInfoMessage;
    after_info = PeekCapturedStdout(capture);

    LOG(WARNING) << kWarningMessage;
    after_warning = PeekCapturedStdout(capture);

    capture.StopCapture();
  }
  FLAGS_logtostdout = false;

  // INFO does not exceed --logbuflevel, so it stays in the stdio buffer.
  EXPECT_EQ(after_info.find(kInfoMessage), string::npos);

  // WARNING does, so it must have been written through. Flushing the stream
  // also emits everything buffered before it.
  EXPECT_NE(after_warning.find(kWarningMessage), string::npos);
  EXPECT_NE(after_warning.find(kInfoMessage), string::npos);
}

int main(int argc, char** argv) {
  FLAGS_colorlogtostderr = false;
#ifdef GLOG_USE_GFLAGS
  ParseCommandLineFlags(&argc, &argv, true);
#endif
  // Make sure stderr is not buffered as stderr seems to be buffered
  // on recent windows.
  setbuf(stderr, nullptr);

  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
