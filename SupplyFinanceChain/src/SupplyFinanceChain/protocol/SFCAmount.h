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

#ifndef SUPPLYFINANCECHAIN_PROTOCOL_SFCAMOUNT_H_INCLUDED
#define SUPPLYFINANCECHAIN_PROTOCOL_SFCAMOUNT_H_INCLUDED

#include <SupplyFinanceChain/basics/contract.h>
#include <SupplyFinanceChain/protocol/SystemParameters.h>
#include <SupplyFinanceChain/beast/utility/Zero.h>
#include <boost/operators.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <string>
#include <type_traits>

namespace SupplyFinanceChain {

class SFCAmount
    : private boost::totally_ordered <SFCAmount>
    , private boost::additive <SFCAmount>
{
private:
    std::int64_t drops_;

public:
    SFCAmount () = default;
    SFCAmount (SFCAmount const& other) = default;
    SFCAmount& operator= (SFCAmount const& other) = default;

    SFCAmount (beast::Zero)
        : drops_ (0)
    {
    }

    SFCAmount&
    operator= (beast::Zero)
    {
        drops_ = 0;
        return *this;
    }

    template <class Integer,
        class = typename std::enable_if_t <
            std::is_integral<Integer>::value>>
    SFCAmount (Integer drops)
        : drops_ (static_cast<std::int64_t> (drops))
    {
    }

    template <class Integer,
        class = typename std::enable_if_t <
            std::is_integral<Integer>::value>>
    SFCAmount&
    operator= (Integer drops)
    {
        drops_ = static_cast<std::int64_t> (drops);
        return *this;
    }

    SFCAmount&
    operator+= (SFCAmount const& other)
    {
        drops_ += other.drops_;
        return *this;
    }

    SFCAmount&
    operator-= (SFCAmount const& other)
    {
        drops_ -= other.drops_;
        return *this;
    }

    SFCAmount
    operator- () const
    {
        return { -drops_ };
    }

    bool
    operator==(SFCAmount const& other) const
    {
        return drops_ == other.drops_;
    }

    bool
    operator<(SFCAmount const& other) const
    {
        return drops_ < other.drops_;
    }

    /** Returns true if the amount is not zero */
    explicit
    operator bool() const noexcept
    {
        return drops_ != 0;
    }

    /** Return the sign of the amount */
    int
    signum() const noexcept
    {
        return (drops_ < 0) ? -1 : (drops_ ? 1 : 0);
    }

    /** Returns the number of drops */
    std::int64_t
    drops () const
    {
        return drops_;
    }
};

inline
std::string
to_string (SFCAmount const& amount)
{
    return std::to_string (amount.drops ());
}

inline
SFCAmount
mulRatio (
    SFCAmount const& amt,
    std::uint32_t num,
    std::uint32_t den,
    bool roundUp)
{
    using namespace boost::multiprecision;

    if (!den)
        Throw<std::runtime_error> ("division by zero");

    int128_t const amt128 (amt.drops ());
    auto const neg = amt.drops () < 0;
    auto const m = amt128 * num;
    auto r = m / den;
    if (m % den)
    {
        if (!neg && roundUp)
            r += 1;
        if (neg && !roundUp)
            r -= 1;
    }
    if (r > std::numeric_limits<std::int64_t>::max ())
        Throw<std::overflow_error> ("SFC mulRatio overflow");
    return SFCAmount (r.convert_to<std::int64_t> ());
}

/** Returns true if the amount does not exceed the initial SFC in existence. */
inline
bool isLegalAmount (SFCAmount const& amount)
{
    return amount.drops () <= SYSTEM_CURRENCY_START;
}

}

#endif
