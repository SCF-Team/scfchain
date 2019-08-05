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

#include <SupplyFinanceChain/app/main/Application.h>
#include <SupplyFinanceChain/app/misc/ValidatorList.h>
#include <SupplyFinanceChain/protocol/jss.h>
#include <SupplyFinanceChain/rpc/Context.h>
#include <SupplyFinanceChain/basics/make_lock.h>

namespace SupplyFinanceChain {

Json::Value doUnlList (RPC::Context& context)
{
    Json::Value obj (Json::objectValue);

    context.app.validators().for_each_listed (
        [&unl = obj[jss::unl]](
            PublicKey const& publicKey,
            bool trusted)
        {
            Json::Value node (Json::objectValue);

            node[jss::pubkey_validator] = toBase58(
                TokenType::NodePublic, publicKey);
            node[jss::trusted] = trusted;

            unl.append (std::move (node));
        });

    return obj;
}

} // SupplyFinanceChain
