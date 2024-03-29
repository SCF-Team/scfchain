//------------------------------------------------------------------------------
/*
    This file is part of SupplyFinanceChaind: https://github.com/SupplyFinanceChain/SupplyFinanceChaind
    Copyright (c) 2019 SupplyFinanceChain Labs Inc.

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

#ifndef SUPPLYFINANCECHAIN_RPC_DELIVEREDAMOUNT_H_INCLUDED
#define SUPPLYFINANCECHAIN_RPC_DELIVEREDAMOUNT_H_INCLUDED

#include <memory>

namespace Json {
class Value;
}

namespace SupplyFinanceChain {

class ReadView;
class Transaction;
class TxMeta;
class STTx;

namespace RPC {

struct Context;

/**
   Add a `delivered_amount` field to the `meta` input/output parameter.
   The field is only added to successful payment and check cash transactions.
   If a delivered amount field is available in the TxMeta parameter, that value
   is used. Otherwise, the transaction's `Amount` field is used. If neither is available,
   then the delivered amount is set to "unavailable".

   @{
 */
void
insertDeliveredAmount(
    Json::Value& meta,
    ReadView const&,
    std::shared_ptr<STTx const> serializedTx,
    TxMeta const&);

void
insertDeliveredAmount(
    Json::Value& meta,
    Context&,
    std::shared_ptr<Transaction>,
    TxMeta const&);

/** @} */

} // RPC
} // SupplyFinanceChain

#endif
