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

#ifndef SUPPLYFINANCECHAIN_APP_LEDGER_TRANSACTIONMASTER_H_INCLUDED
#define SUPPLYFINANCECHAIN_APP_LEDGER_TRANSACTIONMASTER_H_INCLUDED

#include <SupplyFinanceChain/shamap/SHAMapItem.h>
#include <SupplyFinanceChain/shamap/SHAMapTreeNode.h>

namespace SupplyFinanceChain {

class Application;
class Transaction;
class STTx;

// Tracks all transactions in memory

class TransactionMaster
{
public:
    TransactionMaster (Application& app);
    TransactionMaster (TransactionMaster const&) = delete;
    TransactionMaster& operator= (TransactionMaster const&) = delete;

    std::shared_ptr<Transaction>
    fetch (uint256 const& , bool checkDisk);

    std::shared_ptr<STTx const>
    fetch (std::shared_ptr<SHAMapItem> const& item,
        SHAMapTreeNode::TNType type, bool checkDisk,
            std::uint32_t uCommitLedger);

    // return value: true = we had the transaction already
    bool inLedger (uint256 const& hash, std::uint32_t ledger);

    void canonicalize (std::shared_ptr<Transaction>* pTransaction);

    void sweep (void);

    TaggedCache <uint256, Transaction>&
    getCache();

private:
    Application& mApp;
    TaggedCache <uint256, Transaction> mCache;
};

} // SupplyFinanceChain

#endif
