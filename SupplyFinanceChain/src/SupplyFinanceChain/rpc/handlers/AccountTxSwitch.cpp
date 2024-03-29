//------------------------------------------------------------------------------
/*
    This file is part of SupplyFinanceChaind: https://github.com/SupplyFinanceChain/SupplyFinanceChaind
    Copyright (c) 2012-2014 SupplyFinanceChain Labs Inc.

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

#include <SupplyFinanceChain/json/json_value.h>
#include <SupplyFinanceChain/net/RPCErr.h>
#include <SupplyFinanceChain/protocol/jss.h>
#include <SupplyFinanceChain/resource/Fees.h>
#include <SupplyFinanceChain/rpc/Context.h>

namespace SupplyFinanceChain {

Json::Value doAccountTxOld (RPC::Context& context);
        Json::Value doAccountTx (RPC::Context& context);

// Temporary switching code until the old account_tx is removed
Json::Value doAccountTxSwitch (RPC::Context& context)
{
    if (context.params.isMember(jss::offset) ||
        context.params.isMember(jss::count) ||
        context.params.isMember(jss::descending) ||
        context.params.isMember(jss::ledger_max) ||
        context.params.isMember(jss::ledger_min))
    {
        return doAccountTxOld(context);
    }
    return doAccountTx(context);
}

} // SupplyFinanceChain
