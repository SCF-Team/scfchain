//------------------------------------------------------------------------------
/*
    This file is part of SupplyFinanceChaind: https://github.com/SupplyFinanceChain/SupplyFinanceChaind
    Copyright (c) 2012-2016 SupplyFinanceChain Labs Inc.

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

#include <SupplyFinanceChain/basics/mulDiv.h>
#include <SupplyFinanceChain/beast/unit_test.h>

namespace SupplyFinanceChain {
namespace test {

struct mulDiv_test : beast::unit_test::suite
{
    void run() override
    {
        const auto max = std::numeric_limits<std::uint64_t>::max();
        const std::uint64_t max32 = std::numeric_limits<std::uint32_t>::max();

        auto result = mulDiv(85, 20, 5);
        BEAST_EXPECT(result.first && result.second == 340);
        result = mulDiv(20, 85, 5);
        BEAST_EXPECT(result.first && result.second == 340);

        result = mulDiv(0, max - 1, max - 3);
        BEAST_EXPECT(result.first && result.second == 0);
        result = mulDiv(max - 1, 0, max - 3);
        BEAST_EXPECT(result.first && result.second == 0);

        result = mulDiv(max, 2, max / 2);
        BEAST_EXPECT(result.first && result.second == 4);
        result = mulDiv(max, 1000, max / 1000);
        BEAST_EXPECT(result.first && result.second == 1000000);
        result = mulDiv(max, 1000, max / 1001);
        BEAST_EXPECT(result.first && result.second == 1001000);
        result = mulDiv(max32 + 1, max32 + 1, 5);
        BEAST_EXPECT(result.first && result.second == 3689348814741910323);

        // Overflow
        result = mulDiv(max - 1, max - 2, 5);
        BEAST_EXPECT(!result.first && result.second == max);
    }
};

BEAST_DEFINE_TESTSUITE(mulDiv, SupplyFinanceChain_basics, SupplyFinanceChain);

}  // namespace test
}  // namespace SupplyFinanceChain
