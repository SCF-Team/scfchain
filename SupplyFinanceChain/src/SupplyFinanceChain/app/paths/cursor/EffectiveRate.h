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

#ifndef SUPPLYFINANCECHAIN_APP_PATHS_CURSOR_EFFECTIVERATE_H_INCLUDED
#define SUPPLYFINANCECHAIN_APP_PATHS_CURSOR_EFFECTIVERATE_H_INCLUDED

#include <SupplyFinanceChain/protocol/AccountID.h>
#include <SupplyFinanceChain/protocol/Issue.h>
#include <SupplyFinanceChain/protocol/Rate.h>
#include <boost/optional.hpp>

namespace SupplyFinanceChain {
namespace path {

Rate
effectiveRate(
    Issue const& issue,
    AccountID const& account1,
    AccountID const& account2,
    boost::optional<Rate> const& rate);

} // path
} // SupplyFinanceChain

#endif
