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

#ifndef SUPPLYFINANCECHAIN_PROTOCOL_AMOUNTCONVERSION_H_INCLUDED
#define SUPPLYFINANCECHAIN_PROTOCOL_AMOUNTCONVERSION_H_INCLUDED

#include <SupplyFinanceChain/protocol/IOUAmount.h>
#include <SupplyFinanceChain/protocol/SFCAmount.h>
#include <SupplyFinanceChain/protocol/STAmount.h>

namespace SupplyFinanceChain {

inline
STAmount
toSTAmount (IOUAmount const& iou, Issue const& iss)
{
    bool const isNeg = iou.signum() < 0;
    std::uint64_t const umant = isNeg ? - iou.mantissa () : iou.mantissa ();
    return STAmount (iss, umant, iou.exponent (), /*native*/ false, isNeg,
                     STAmount::unchecked ());
}

inline
STAmount
toSTAmount (IOUAmount const& iou)
{
    return toSTAmount (iou, noIssue ());
}

inline
STAmount
toSTAmount (SFCAmount const& sfc)
{
    bool const isNeg = sfc.signum() < 0;
    std::uint64_t const umant = isNeg ? - sfc.drops () : sfc.drops ();
    return STAmount (umant, isNeg);
}

inline
STAmount
toSTAmount (SFCAmount const& sfc, Issue const& iss)
{
    assert (isSFC(iss.account) && isSFC(iss.currency));
    return toSTAmount (sfc);
}

template <class T>
T
toAmount (STAmount const& amt)
{
    static_assert(sizeof(T) == -1, "Must use specialized function");
    return T(0);
}

template <>
inline
STAmount
toAmount<STAmount> (STAmount const& amt)
{
    return amt;
}

template <>
inline
IOUAmount
toAmount<IOUAmount> (STAmount const& amt)
{
    assert (amt.mantissa () < std::numeric_limits<std::int64_t>::max ());
    bool const isNeg = amt.negative ();
    std::int64_t const sMant =
            isNeg ? - std::int64_t (amt.mantissa ()) : amt.mantissa ();

    assert (! isSFC (amt));
    return IOUAmount (sMant, amt.exponent ());
}

template <>
inline
SFCAmount
toAmount<SFCAmount> (STAmount const& amt)
{
    assert (amt.mantissa () < std::numeric_limits<std::int64_t>::max ());
    bool const isNeg = amt.negative ();
    std::int64_t const sMant =
            isNeg ? - std::int64_t (amt.mantissa ()) : amt.mantissa ();

    assert (isSFC (amt));
    return SFCAmount (sMant);
}


}

#endif
