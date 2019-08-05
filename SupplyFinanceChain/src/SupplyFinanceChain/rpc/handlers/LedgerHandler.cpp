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

#include <SupplyFinanceChain/rpc/handlers/LedgerHandler.h>
#include <SupplyFinanceChain/app/ledger/LedgerToJson.h>
#include <SupplyFinanceChain/app/main/Application.h>
#include <SupplyFinanceChain/app/misc/LoadFeeTrack.h>
#include <SupplyFinanceChain/json/Object.h>
#include <SupplyFinanceChain/protocol/ErrorCodes.h>
#include <SupplyFinanceChain/protocol/jss.h>
#include <SupplyFinanceChain/resource/Fees.h>
#include <SupplyFinanceChain/rpc/impl/RPCHelpers.h>
#include <SupplyFinanceChain/rpc/Role.h>

namespace SupplyFinanceChain {
namespace RPC {

LedgerHandler::LedgerHandler (Context& context) : context_ (context)
{
}

Status LedgerHandler::check()
{
    auto const& params = context_.params;
    bool needsLedger = params.isMember (jss::ledger) ||
            params.isMember (jss::ledger_hash) ||
            params.isMember (jss::ledger_index);
    if (! needsLedger)
        return Status::OK;

    if (auto s = lookupLedger (ledger_, context_, result_))
        return s;

    bool const full = params[jss::full].asBool();
    bool const transactions = params[jss::transactions].asBool();
    bool const accounts = params[jss::accounts].asBool();
    bool const expand = params[jss::expand].asBool();
    bool const binary = params[jss::binary].asBool();
    bool const owner_funds = params[jss::owner_funds].asBool();
    bool const queue = params[jss::queue].asBool();
    auto type = chooseLedgerEntryType(params);
    if (type.first)
        return type.first;
    type_ = type.second;

    options_ = (full ? LedgerFill::full : 0)
            | (expand ? LedgerFill::expand : 0)
            | (transactions ? LedgerFill::dumpTsfc : 0)
            | (accounts ? LedgerFill::dumpState : 0)
            | (binary ? LedgerFill::binary : 0)
            | (owner_funds ? LedgerFill::ownerFunds : 0)
            | (queue ? LedgerFill::dumpQueue : 0);

    if (full || accounts)
    {
        // Until some sane way to get full ledgers has been implemented,
        // disallow retrieving all state nodes.
        if (! isUnlimited (context_.role))
            return rpcNO_PERMISSION;

        if (context_.app.getFeeTrack().isLoadedLocal() &&
            ! isUnlimited (context_.role))
        {
            return rpcTOO_BUSY;
        }
        context_.loadType = binary ? Resource::feeMediumBurdenRPC :
            Resource::feeHighBurdenRPC;
    }
    if (queue)
    {
        if (! ledger_ || ! ledger_->open())
        {
            // It doesn't make sense to request the queue
            // with a non-existant or closed/validated ledger.
            return rpcINVALID_PARAMS;
        }

        queueTxs_ = context_.app.getTxQ().getTxs(*ledger_);
    }

    return Status::OK;
}

} // RPC
} // SupplyFinanceChain
