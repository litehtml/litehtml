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

#include "litehtml/url.h"

#include <iostream>

#include <gtest/gtest.h>

using namespace litehtml;

namespace {

struct url_parse_testcase {
    string str;
    string scheme;
    string authority;
    string path;
    string query;
    string fragment;
};

struct url_resolve_testcase {
    string base;
    string reference;
    string expected;
};

} // namespace

TEST(URLTest, DefaultConstructor)
{
    url u;

    EXPECT_TRUE(u.scheme().empty());
    EXPECT_TRUE(u.authority().empty());
    EXPECT_TRUE(u.path().empty());
    EXPECT_TRUE(u.query().empty());
    EXPECT_TRUE(u.fragment().empty());
}

TEST(URLTest, Parse)
{
    std::vector<url_parse_testcase> testcases = {

        // Example from RFC 3986 that includes a scheme, an authority, a path,
        // a query, and a fragment.
        { "foo://example.com:8042/over/there?name=ferret#nose",
            "foo", "example.com:8042", "/over/there", "name=ferret", "nose" },

        // Example from RFC 3986 that only includes a scheme and a path.
        { "urn:example:animal:ferret:nose",
            "urn", "", "example:animal:ferret:nose", "", "" },

        { "http://www.litehtml.com/",
            "http", "www.litehtml.com", "/", "", "" },

        { "https://www.slashdot.org/",
            "https", "www.slashdot.org", "/", "", "" },

        { "https://www.slashdot.org",
            "https", "www.slashdot.org", "", "", "" },

        { "https://news.slashdot.org/story/21/09/24/2157247/",
            "https", "news.slashdot.org", "/story/21/09/24/2157247/", "", "" },

        { "https://www.cbc.ca/news/politics/spavor-kovrig-return-1.6189516",
            "https", "www.cbc.ca", "/news/politics/spavor-kovrig-return-1.6189516", "", "" },

        { "https://twitter.com/geekbench/status/1412433598200823810",
            "https", "twitter.com", "/geekbench/status/1412433598200823810", "", "" },

        { "https://browser.geekbench.com/v5/cpu/search?q=ryzen",
            "https", "browser.geekbench.com", "/v5/cpu/search", "q=ryzen", "" },

        { "https://datatracker.ietf.org/doc/html/rfc3986#section-2.2",
            "https", "datatracker.ietf.org", "/doc/html/rfc3986", "", "section-2.2" },

        { "file:///home/litehtml/build/hipster.html",
            "file", "", "/home/litehtml/build/hipster.html" },

        { "/home/litehtml/Projects/litehtml/build/hipster.html",
            "", "", "/home/litehtml/Projects/litehtml/build/hipster.html" },
    };

    for (auto& testcase : testcases) {
        url u(testcase.str);

        EXPECT_EQ(testcase.scheme, u.scheme());
        EXPECT_EQ(testcase.authority, u.authority());
        EXPECT_EQ(testcase.path, u.path());
        EXPECT_EQ(testcase.query, u.query());
        EXPECT_EQ(testcase.fragment, u.fragment());
    }
}

TEST(URLTest, Build)
{
    std::vector<url_parse_testcase> testcases = {

        // Example from RFC 3986 that includes a scheme, an authority, a path,
        // a query, and a fragment.
        { "foo://example.com:8042/over/there?name=ferret#nose",
            "foo", "example.com:8042", "/over/there", "name=ferret", "nose" },

        // Example from RFC 3986 that only includes a scheme and a path.
        { "urn:example:animal:ferret:nose",
            "urn", "", "example:animal:ferret:nose", "", "" },

        { "http://www.litehtml.com/",
            "http", "www.litehtml.com", "/", "", "" },

        { "https://www.slashdot.org/",
            "https", "www.slashdot.org", "/", "", "" },

        { "https://www.slashdot.org",
            "https", "www.slashdot.org", "", "", "" },

        { "https://news.slashdot.org/story/21/09/24/2157247/",
            "https", "news.slashdot.org", "/story/21/09/24/2157247/", "", "" },

        { "https://www.cbc.ca/news/politics/spavor-kovrig-return-1.6189516",
            "https", "www.cbc.ca", "/news/politics/spavor-kovrig-return-1.6189516", "", "" },

        { "https://twitter.com/geekbench/status/1412433598200823810",
            "https", "twitter.com", "/geekbench/status/1412433598200823810", "", "" },

        { "https://browser.geekbench.com/v5/cpu/search?q=ryzen",
            "https", "browser.geekbench.com", "/v5/cpu/search", "q=ryzen", "" },

        { "https://datatracker.ietf.org/doc/html/rfc3986#section-2.2",
            "https", "datatracker.ietf.org", "/doc/html/rfc3986", "", "section-2.2" },

        // Disabled since the url class does not regenerate the same URL for
        // this test case (it does not emit the double slash at the start of
        // the authority).  How do we determine which schemes require the double
        // slash and which ones do not?

        // { "file:///home/litehtml/build/hipster.html",
        //    "file", "", "/home/litehtml/build/hipster.html" },

        { "/home/litehtml/Projects/litehtml/build/hipster.html",
            "", "", "/home/litehtml/Projects/litehtml/build/hipster.html" },
    };

    for (auto& testcase : testcases) {
        url u(testcase.scheme,
            testcase.authority,
            testcase.path,
            testcase.query,
            testcase.fragment);
        EXPECT_EQ(testcase.str, u.str());
    }

}

TEST(URLTest, Resolve)
{
    std::vector<url_resolve_testcase> testcases = {
        { "https://www.twitter.com/", "/foo",
            "https://www.twitter.com/foo" },

        { "https://www.twitter.com/", "https://www.facebook.com/",
            "https://www.facebook.com/" },

        { "https://www.example.com/index.html", "about.html",
            "https://www.example.com/about.html" },
    };

    for (auto& testcase : testcases) {
        url u = resolve(url(testcase.base), url(testcase.reference));
        url expected(testcase.expected);

        EXPECT_EQ(expected.scheme(), u.scheme());
        EXPECT_EQ(expected.authority(), u.authority());
        EXPECT_EQ(expected.path(), u.path());
        EXPECT_EQ(expected.query(), u.query());
        EXPECT_EQ(expected.fragment(), u.fragment());
    }
}
