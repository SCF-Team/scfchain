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

#ifndef SUPPLYFINANCECHAIN_PROTOCOL_ISSUE_H_INCLUDED
#define SUPPLYFINANCECHAIN_PROTOCOL_ISSUE_H_INCLUDED

#include <cassert>
#include <functional>
#include <type_traits>

#include <SupplyFinanceChain/protocol/UintTypes.h>

namespace SupplyFinanceChain {

/** A currency issued by an account.
    @see Currency, AccountID, Issue, Book
*/
class Issue
{
public:
    Currency currency;
    AccountID account;

    Issue ()
    {
    }

    Issue (Currency const& c, AccountID const& a)
            : currency (c), account (a)
    {
    }
};

bool
isConsistent (Issue const& ac);

std::string
to_string (Issue const& ac);

std::ostream&
operator<< (std::ostream& os, Issue const& x);

template <class Hasher>
void
hash_append(Hasher& h, Issue const& r)
{
    using beast::hash_append;
    hash_append(h, r.currency, r.account);
}

/** Ordered comparison.
    The assets are ordered first by currency and then by account,
    if the currency is not SFC.
*/
int
compare (Issue const& lhs, Issue const& rhs);

/** Equality comparison. */
/** @{ */
bool
operator== (Issue const& lhs, Issue const& rhs);
bool
operator!= (Issue const& lhs, Issue const& rhs);
/** @} */

/** Strict weak ordering. */
/** @{ */
bool
operator< (Issue const& lhs, Issue const& rhs);
bool
operator> (Issue const& lhs, Issue const& rhs);
bool
operator>= (Issue const& lhs, Issue const& rhs);
bool
operator<= (Issue const& lhs, Issue const& rhs);
/** @} */

//------------------------------------------------------------------------------

/** Returns an asset specifier that represents SFC. */
inline Issue const& sfcIssue ()
{
    static Issue issue {sfcCurrency(), sfcAccount()};
    return issue;
}

/** Returns an asset specifier that represents no account and currency. */
inline Issue const& noIssue ()
{
    static Issue issue {noCurrency(), noAccount()};
    return issue;
}

}

#endif
