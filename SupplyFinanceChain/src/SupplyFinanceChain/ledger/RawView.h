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

#ifndef SUPPLYFINANCECHAIN_LEDGER_RAWVIEW_H_INCLUDED
#define SUPPLYFINANCECHAIN_LEDGER_RAWVIEW_H_INCLUDED

#include <SupplyFinanceChain/ledger/ReadView.h>
#include <SupplyFinanceChain/protocol/Serializer.h>
#include <SupplyFinanceChain/protocol/STLedgerEntry.h>
#include <boost/optional.hpp>
#include <cstdint>
#include <memory>
#include <utility>

namespace SupplyFinanceChain {

/** Interface for ledger entry changes.

    Subclasses allow raw modification of ledger entries.
*/
class RawView
{
public:
    virtual ~RawView() = default;
    RawView() = default;
    RawView(RawView const&) = default;
    RawView& operator=(RawView const&) = delete;

    /** Delete an existing state item.

        The SLE is provided so the implementation
        can calculate metadata.
    */
    virtual
    void
    rawErase (std::shared_ptr<SLE> const& sle) = 0;

    /** Unconditionally insert a state item.

        Requirements:
            The key must not already exist.

        Effects:

            The key is associated with the SLE.

        @note The key is taken from the SLE
    */
    virtual
    void
    rawInsert (std::shared_ptr<SLE> const& sle) = 0;

    /** Unconditionally replace a state item.

        Requirements:

            The key must exist.

        Effects:

            The key is associated with the SLE.

        @note The key is taken from the SLE
    */
    virtual
    void
    rawReplace (std::shared_ptr<SLE> const& sle) = 0;

    /** Destroy SFC.

        This is used to pay for transaction fees.
    */
    virtual
    void
    rawDestroySFC (SFCAmount const& fee) = 0;
};

//------------------------------------------------------------------------------

/** Interface for changing ledger entries with transactions.

    Allows raw modification of ledger entries and insertion
    of transactions into the transaction map.
*/
class TxsRawView : public RawView
{
public:
    /** Add a transaction to the tx map.

        Closed ledgers must have metadata,
        while open ledgers omit metadata.
    */
    virtual
    void
    rawTxInsert (ReadView::key_type const& key,
        std::shared_ptr<Serializer const>
            const& txn, std::shared_ptr<
                Serializer const> const& metaData) = 0;
};

} // SupplyFinanceChain

#endif
