//------------------------------------------------------------------------------
/*
    This file is part of SupplyFinanceChaind: https://github.com/SupplyFinanceChain/SupplyFinanceChaind
    Copyright (c) 2012, 2013 SupplyFinanceChain Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <SupplyFinanceChain/json/json_reader.h>
#include <SupplyFinanceChain/json/json_writer.h>
#include <test/json/TestOutputSuite.h>

namespace Json {

struct Output_test : SupplyFinanceChain::test::TestOutputSuite
{
    void runTest (std::string const& name, std::string const& valueDesc)
    {
        setup (name);
        Json::Value value;
        BEAST_EXPECT(Json::Reader().parse (valueDesc, value));
        auto out = stringOutput (output_);
        outputJson (value, out);

        // Compare with the original version.
        auto expected = Json::FastWriter().write (value);
        expectResult (expected);
        expectResult (valueDesc);
        expectResult (jsonAsString (value));
    }

    void runTest (std::string const& name)
    {
        runTest (name, name);
    }

    void run () override
    {
        runTest ("empty dict", "{}");
        runTest ("empty array", "[]");
        runTest ("array", "[23,4.25,true,null,\"string\"]");
        runTest ("dict", "{\"hello\":\"world\"}");
        runTest ("array dict", "[{}]");
        runTest ("array array", "[[]]");
        runTest ("more complex",
                 "{\"array\":[{\"12\":23},{},null,false,0.5]}");
    }
};

BEAST_DEFINE_TESTSUITE(Output, SupplyFinanceChain_basics, SupplyFinanceChain);

} // Json
