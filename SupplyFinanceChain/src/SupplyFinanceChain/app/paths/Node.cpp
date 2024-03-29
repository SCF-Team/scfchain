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

#include <SupplyFinanceChain/app/paths/Node.h>
#include <SupplyFinanceChain/app/paths/PathState.h>
#include <SupplyFinanceChain/protocol/jss.h>

namespace SupplyFinanceChain {
namespace path {

// Compare the non-calculated fields.
bool Node::operator== (const Node& other) const
{
    return other.uFlags == uFlags
            && other.account_ == account_
            && other.issue_ == issue_;
}

// This is for debugging not end users. Output names can be changed without
// warning.
Json::Value Node::getJson () const
{
    Json::Value jvNode (Json::objectValue);
    Json::Value jvFlags (Json::arrayValue);

    jvNode[jss::type]  = uFlags;

    bool const hasCurrency = !isSFC (issue_.currency);
    bool const hasAccount = !isSFC (account_);
    bool const hasIssuer = !isSFC (issue_.account);

    if (isAccount() || hasAccount)
        jvFlags.append (!isAccount() == hasAccount ? "account" : "-account");

    if (uFlags & STPathElement::typeCurrency || hasCurrency)
    {
        jvFlags.append ((uFlags & STPathElement::typeCurrency) && hasCurrency
            ? "currency"
            : "-currency");
    }

    if (uFlags & STPathElement::typeIssuer || hasIssuer)
    {
        jvFlags.append ((uFlags & STPathElement::typeIssuer) && hasIssuer
            ? "issuer"
            : "-issuer");
    }

    jvNode["flags"] = jvFlags;

    if (!isSFC (account_))
        jvNode[jss::account] = to_string (account_);

    if (!isSFC (issue_.currency))
        jvNode[jss::currency] = to_string (issue_.currency);

    if (!isSFC (issue_.account))
        jvNode[jss::issuer] = to_string (issue_.account);

    if (saRevRedeem)
        jvNode["rev_redeem"] = saRevRedeem.getFullText ();

    if (saRevIssue)
        jvNode["rev_issue"] = saRevIssue.getFullText ();

    if (saRevDeliver)
        jvNode["rev_deliver"] = saRevDeliver.getFullText ();

    if (saFwdRedeem)
        jvNode["fwd_redeem"] = saFwdRedeem.getFullText ();

    if (saFwdIssue)
        jvNode["fwd_issue"] = saFwdIssue.getFullText ();

    if (saFwdDeliver)
        jvNode["fwd_deliver"] = saFwdDeliver.getFullText ();

    return jvNode;
}

} // path
} // SupplyFinanceChain
