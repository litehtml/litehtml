// Copyright (C) 2020-2021 Primate Labs Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the names of the copyright holders nor the names of their
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

#include "litehtml/url_path.h"

#include <iostream>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace litehtml;

namespace {

struct url_path_testcase {
    string base;
    string path;
    string expected;
};

} // namespace

TEST(URLPathTest, Absolute)
{
    std::vector<std::pair<string, bool>> testcases = {
        { "", false },
        { "a", false },
        { "a/", false },
        { "a/b", false },
        { "a/b/", false },
        { "a/b/c", false },
        { "a/b/c/", false },

        { "/", true },
        { "/a", true },
        { "/a/", true },
        { "/a/b", true },
        { "/a/b/", true },
        { "/a/b/c", true },
        { "/a/b/c/", true },
    };

    for (auto& testcase : testcases) {
        EXPECT_EQ(testcase.second, is_url_path_absolute(testcase.first));
    }
}

TEST(URLPathTest, DirectoryName)
{
    std::vector<std::pair<string, string>> testcases = {
        { "", "." },
        { "a", "." },
        { "a/", "a/" },
        { "a/b", "a/" },
        { "a/b/", "a/b/" },
        { "a/b/c", "a/b/" },
        { "a/b/c/", "a/b/c/" },

        { "/", "/" },
        { "/a", "/" },
        { "/a/", "/a/" },
        { "/a/b", "/a/" },
        { "/a/b/", "/a/b/" },
        { "/a/b/c", "/a/b/" },
        { "/a/b/c/", "/a/b/c/" },
    };

    for (auto& testcase : testcases) {
        EXPECT_EQ(testcase.second, url_path_directory_name(testcase.first));
    }
}

TEST(URLPathTest, BaseName)
{
    std::vector<std::pair<string, string>> testcases = {
        { "", "" },
        { "a", "a" },
        { "a/", "" },
        { "a/b", "b" },
        { "a/b/", "" },
        { "a/b/c", "c" },
        { "a/b/c/", "" },

        { "/", "" },
        { "/a", "a" },
        { "/a/", "" },
        { "/a/b", "b" },
        { "/a/b/", "" },
        { "/a/b/c", "c" },
        { "/a/b/c/", "" },
    };

    for (auto& testcase : testcases) {
        EXPECT_EQ(testcase.second, url_path_base_name(testcase.first));
    }
}

TEST(URLPathTest, Append)
{
    std::vector<url_path_testcase> testcases = {
        { "", "a", "a" },
        { "/", "a", "/a" },
        { "/a", "", "/a" },
        { "/a", "b", "/a/b" },
    };

    for (auto& testcase : testcases) {
        EXPECT_EQ(testcase.expected, url_path_append(testcase.base, testcase.path));
    }
}

TEST(URLPathTest, Resolve)
{
    std::vector<url_path_testcase> testcases = {
        { "/", "a", "/a" },
        { "/a", "b", "/b" },
        { "/a", "/b", "/b" },
    };

    for (auto& testcase : testcases) {
        EXPECT_EQ(testcase.expected, url_path_resolve(testcase.base, testcase.path));
    }
}