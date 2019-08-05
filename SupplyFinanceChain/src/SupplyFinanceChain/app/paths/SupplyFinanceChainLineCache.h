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

#ifndef SUPPLYFINANCECHAIN_APP_PATHS_SUPPLYFINANCECHAINLINECACHE_H_INCLUDED
#define SUPPLYFINANCECHAIN_APP_PATHS_SUPPLYFINANCECHAINLINECACHE_H_INCLUDED

#include <SupplyFinanceChain/app/ledger/Ledger.h>
#include <SupplyFinanceChain/app/paths/SupplyFinanceChainState.h>
#include <SupplyFinanceChain/basics/hardened_hash.h>
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace SupplyFinanceChain {

// Used by Pathfinder
class SupplyFinanceChainLineCache
{
public:
    explicit
    SupplyFinanceChainLineCache (
        std::shared_ptr <ReadView const> const& l);

    std::shared_ptr <ReadView const> const&
    getLedger () const
    {
        return mLedger;
    }

    std::vector<SupplyFinanceChainState::pointer> const&
    getSupplyFinanceChainLines (AccountID const& accountID);

private:
    std::mutex mLock;

    SupplyFinanceChain::hardened_hash<> hasher_;
    std::shared_ptr <ReadView const> mLedger;

    struct AccountKey
    {
        AccountID account_;
        std::size_t hash_value_;

        AccountKey (AccountID const& account, std::size_t hash)
            : account_ (account)
            , hash_value_ (hash)
        { }

        AccountKey (AccountKey const& other) = default;

        AccountKey&
        operator=(AccountKey const& other) = default;

        bool operator== (AccountKey const& lhs) const
        {
            return hash_value_ == lhs.hash_value_ && account_ == lhs.account_;
        }

        std::size_t
        get_hash () const
        {
            return hash_value_;
        }

        struct Hash
        {
            explicit Hash() = default;

            std::size_t
            operator () (AccountKey const& key) const noexcept
            {
                return key.get_hash ();
            }
        };
    };

    hash_map <
        AccountKey,
        std::vector <SupplyFinanceChainState::pointer>,
        AccountKey::Hash> lines_;
};

} // SupplyFinanceChain

#endif
