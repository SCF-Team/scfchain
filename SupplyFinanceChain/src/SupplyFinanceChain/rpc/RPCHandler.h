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

#ifndef SUPPLYFINANCECHAIN_RPC_RPCHANDLER_H_INCLUDED
#define SUPPLYFINANCECHAIN_RPC_RPCHANDLER_H_INCLUDED

#include <SupplyFinanceChain/core/Config.h>
#include <SupplyFinanceChain/net/InfoSub.h>
#include <SupplyFinanceChain/rpc/Context.h>
#include <SupplyFinanceChain/rpc/Status.h>

namespace SupplyFinanceChain {
namespace RPC {

struct Context;

/** Execute an RPC command and store the results in a Json::Value. */
Status doCommand (RPC::Context&, Json::Value&);

Role roleRequired (std::string const& method );

} // RPC
} // SupplyFinanceChain

#endif
