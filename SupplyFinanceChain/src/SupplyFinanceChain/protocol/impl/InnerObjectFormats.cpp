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

#include <SupplyFinanceChain/protocol/InnerObjectFormats.h>

namespace SupplyFinanceChain {

InnerObjectFormats::InnerObjectFormats ()
{
    add (sfSignerEntry.jsonName.c_str(), sfSignerEntry.getCode(),
        {
            {sfAccount,       soeREQUIRED},
            {sfSignerWeight,  soeREQUIRED},
        });

    add (sfSigner.jsonName.c_str(), sfSigner.getCode(),
        {
            {sfAccount,       soeREQUIRED},
            {sfSigningPubKey, soeREQUIRED},
            {sfTxnSignature,  soeREQUIRED},
        });
}

InnerObjectFormats const&
InnerObjectFormats::getInstance ()
{
    static InnerObjectFormats instance;
    return instance;
}

SOTemplate const*
InnerObjectFormats::findSOTemplateBySField (SField const& sField) const
{
    auto itemPtr = findByType (sField.getCode ());
    if (itemPtr)
        return &(itemPtr->getSOTemplate());

    return nullptr;
}

} // SupplyFinanceChain
