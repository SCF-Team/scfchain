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
#include <SupplyFinanceChain/json/JsonPropertyStream.h>
#include <SupplyFinanceChain/json/json_value.h>
#include <SupplyFinanceChain/protocol/jss.h>
#include <SupplyFinanceChain/rpc/Context.h>

namespace SupplyFinanceChain {

Json::Value doPrint (RPC::Context& context)
{
    JsonPropertyStream stream;
    if (context.params.isObject()
        && context.params[jss::params].isArray()
        && context.params[jss::params][0u].isString ())
    {
        context.app.write (stream, context.params[jss::params][0u].asString());
    }
    else
    {
        context.app.write (stream);
    }

    return stream.top();
}

} // SupplyFinanceChain
