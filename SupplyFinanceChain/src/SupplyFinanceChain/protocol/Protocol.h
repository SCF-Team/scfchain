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

#ifndef SUPPLYFINANCECHAIN_PROTOCOL_PROTOCOL_H_INCLUDED
#define SUPPLYFINANCECHAIN_PROTOCOL_PROTOCOL_H_INCLUDED

#include <SupplyFinanceChain/basics/base_uint.h>
#include <SupplyFinanceChain/basics/ByteUtilities.h>
#include <cstdint>

namespace SupplyFinanceChain {

/** Protocol specific constants, types, and data.

    This information is, implicitly, part of the SupplyFinanceChain
    protocol.

    @note Changing these values without adding code to the
          server to detect "pre-change" and "post-change"
          will result in a hard fork.
*/
/** Smallest legal byte size of a transaction. */
std::size_t constexpr txMinSizeBytes = 32;

/** Largest legal byte size of a transaction. */
std::size_t constexpr txMaxSizeBytes = megabytes(1);

/** The maximum number of unfunded offers to delete at once */
std::size_t constexpr unfundedOfferRemoveLimit = 1000;

/** The maximum number of metadata entries allowed in one transaction */
std::size_t constexpr oversizeMetaDataCap = 5200;

/** The maximum number of entries per directory page */
std::size_t constexpr dirNodeMaxEntries = 32;

/** The maximum number of pages allowed in a directory */
std::uint64_t constexpr dirNodeMaxPages = 262144;

/** A ledger index. */
using LedgerIndex = std::uint32_t;

/** A transaction identifier.
    The value is computed as the hash of the
    canonicalized, serialized transaction object.
*/
using TxID = uint256;

using TxSeq = std::uint32_t;

} // SupplyFinanceChain

#endif
