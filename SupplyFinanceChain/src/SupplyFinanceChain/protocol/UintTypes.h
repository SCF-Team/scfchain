//------------------------------------------------------------------------------
/*
    This file is part of SupplyFinanceChaind: https://github.com/SupplyFinanceChain/SupplyFinanceChaind
    Copyright (c) 2014 SupplyFinanceChain Labs Inc.

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

#ifndef SUPPLYFINANCECHAIN_PROTOCOL_UINTTYPES_H_INCLUDED
#define SUPPLYFINANCECHAIN_PROTOCOL_UINTTYPES_H_INCLUDED

#include <SupplyFinanceChain/basics/UnorderedContainers.h>
#include <SupplyFinanceChain/basics/base_uint.h>
#include <SupplyFinanceChain/protocol/AccountID.h>
#include <SupplyFinanceChain/beast/utility/Zero.h>

namespace SupplyFinanceChain {
namespace detail {

class CurrencyTag
{
public:
    explicit CurrencyTag() = default;
};

class DirectoryTag
{
public:
    explicit DirectoryTag() = default;
};

class NodeIDTag
{
public:
    explicit NodeIDTag() = default;
};

} // detail

/** Directory is an index into the directory of offer books.
    The last 64 bits of this are the quality. */
using Directory = base_uint<256, detail::DirectoryTag>;

/** Currency is a hash representing a specific currency. */
using Currency = base_uint<160, detail::CurrencyTag>;

/** NodeID is a 160-bit hash representing one node. */
using NodeID = base_uint<160, detail::NodeIDTag>;

/** SFC currency. */
Currency const& sfcCurrency();

/** A placeholder for empty currencies. */
Currency const& noCurrency();

/** We deliberately disallow the currency that looks like "SFC" because too
    many people were using it instead of the correct SFC currency. */
Currency const& badCurrency();

inline bool isSFC(Currency const& c)
{
    return c == beast::zero;
}

/** Returns "", "SFC", or three letter ISO code. */
std::string to_string(Currency const& c);

/** Tries to convert a string to a Currency, returns true on success. */
bool to_currency(Currency&, std::string const&);

/** Tries to convert a string to a Currency, returns noCurrency() on failure. */
Currency to_currency(std::string const&);

inline std::ostream& operator<< (std::ostream& os, Currency const& x)
{
    os << to_string (x);
    return os;
}

} // SupplyFinanceChain

namespace std {

template <>
struct hash <SupplyFinanceChain::Currency> : SupplyFinanceChain::Currency::hasher
{
    explicit hash() = default;
};

template <>
struct hash <SupplyFinanceChain::NodeID> : SupplyFinanceChain::NodeID::hasher
{
    explicit hash() = default;
};

template <>
struct hash <SupplyFinanceChain::Directory> : SupplyFinanceChain::Directory::hasher
{
    explicit hash() = default;
};

} // std

#endif
