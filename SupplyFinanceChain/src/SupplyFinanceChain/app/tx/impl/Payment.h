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

#ifndef SUPPLYFINANCECHAIN_TX_PAYMENT_H_INCLUDED
#define SUPPLYFINANCECHAIN_TX_PAYMENT_H_INCLUDED

#include <SupplyFinanceChain/app/paths/SupplyFinanceChainCalc.h>
#include <SupplyFinanceChain/app/tx/impl/Transactor.h>
#include <SupplyFinanceChain/basics/Log.h>
#include <SupplyFinanceChain/protocol/TxFlags.h>

namespace SupplyFinanceChain {

// See https://SupplyFinanceChain.com/wiki/Transaction_Format#Payment_.280.29

class Payment
    : public Transactor
{
    /* The largest number of paths we allow */
    static std::size_t const MaxPathSize = 6;

    /* The longest path we allow */
    static std::size_t const MaxPathLength = 8;

public:
    explicit Payment (ApplyContext& ctx)
        : Transactor(ctx)
    {
    }

    static
    SFCAmount
    calculateMaxSpend(STTx const& tx);

    static
    NotTEC
    preflight (PreflightContext const& ctx);

    static
    TER
    preclaim(PreclaimContext const& ctx);

    TER doApply () override;
};

} // SupplyFinanceChain

#endif
