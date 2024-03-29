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

#include <SupplyFinanceChain/app/ledger/LedgerMaster.h>
#include <SupplyFinanceChain/app/main/Application.h>
#include <SupplyFinanceChain/app/misc/AmendmentTable.h>
#include <SupplyFinanceChain/protocol/ErrorCodes.h>
#include <SupplyFinanceChain/protocol/jss.h>
#include <SupplyFinanceChain/net/RPCErr.h>
#include <SupplyFinanceChain/rpc/Context.h>
#include <SupplyFinanceChain/beast/core/LexicalCast.h>

namespace SupplyFinanceChain {


// {
//   feature : <feature>
//   vetoed : true/false
// }
Json::Value doFeature (RPC::Context& context)
{

    // Get majority amendment status
    majorityAmendments_t majorities;

    if (auto const valLedger = context.ledgerMaster.getValidatedLedger())
        majorities = getMajorityAmendments (*valLedger);

    auto& table = context.app.getAmendmentTable ();

    if (!context.params.isMember (jss::feature))
    {
        auto features = table.getJson(0);

        for (auto const& m : majorities)
        {
            features[to_string(m.first)][jss::majority] =
                m.second.time_since_epoch().count();
        }

        Json::Value jvReply = Json::objectValue;
        jvReply[jss::features] = features;
        return jvReply;
    }

    auto feature = table.find (
        context.params[jss::feature].asString());

    if (!feature &&
        !feature.SetHexExact (context.params[jss::feature].asString ()))
        return rpcError (rpcBAD_FEATURE);

    if (context.params.isMember (jss::vetoed))
    {
        if (context.params[jss::vetoed].asBool ())
            context.app.getAmendmentTable().veto (feature);
        else
            context.app.getAmendmentTable().unVeto(feature);
    }

    Json::Value jvReply = table.getJson(feature);

    auto m = majorities.find (feature);
    if (m != majorities.end())
        jvReply [jss::majority] =
            m->second.time_since_epoch().count();

    return jvReply;
}


} // SupplyFinanceChain
