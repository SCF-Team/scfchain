//------------------------------------------------------------------------------
/*
    This file is part of SupplyFinanceChaind: https://github.com/SupplyFinanceChain/SupplyFinanceChaind
    Copyright (c) 2018 SupplyFinanceChain Labs Inc.

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

#ifndef SUPPLYFINANCECHAIN_BASICS_FILEUTILITIES_H_INCLUDED
#define SUPPLYFINANCECHAIN_BASICS_FILEUTILITIES_H_INCLUDED

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

namespace SupplyFinanceChain
{

// TODO: Should this one function have its own file, or can it
// be absorbed somewhere else?

std::string getFileContents(boost::system::error_code& ec,
    boost::filesystem::path const& sourcePath,
    boost::optional<std::size_t> maxSize = boost::none);

}

#endif
