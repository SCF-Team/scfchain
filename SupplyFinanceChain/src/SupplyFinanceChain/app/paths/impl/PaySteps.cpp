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

#include <SupplyFinanceChain/app/paths/impl/Steps.h>
#include <SupplyFinanceChain/basics/contract.h>
#include <SupplyFinanceChain/json/json_writer.h>
#include <SupplyFinanceChain/ledger/ReadView.h>
#include <SupplyFinanceChain/protocol/Feature.h>
#include <SupplyFinanceChain/protocol/IOUAmount.h>
#include <SupplyFinanceChain/protocol/SFCAmount.h>

#include <algorithm>
#include <numeric>
#include <sstream>

namespace SupplyFinanceChain {

// Check equal with tolerance
bool checkNear (IOUAmount const& expected, IOUAmount const& actual)
{
    double const ratTol = 0.001;
    if (abs (expected.exponent () - actual.exponent ()) > 1)
        return false;

    if (actual.exponent () < -20)
        return true;

    auto const a = (expected.exponent () < actual.exponent ())
        ? expected.mantissa () / 10
        : expected.mantissa ();
    auto const b = (actual.exponent () < expected.exponent ())
        ? actual.mantissa () / 10
        : actual.mantissa ();
    if (a == b)
        return true;

    double const diff = std::abs (a - b);
    auto const r = diff / std::max (std::abs (a), std::abs (b));
    return r <= ratTol;
};

bool checkNear (SFCAmount const& expected, SFCAmount const& actual)
{
    return expected == actual;
};

static
bool isSFCAccount (STPathElement const& pe)
{
    if (pe.getNodeType () != STPathElement::typeAccount)
        return false;
    return isSFC (pe.getAccountID ());
};


static
std::pair<TER, std::unique_ptr<Step>>
toStep (
    StrandContext const& ctx,
    STPathElement const* e1,
    STPathElement const* e2,
    Issue const& curIssue)
{
    auto& j = ctx.j;

    if (ctx.isFirst && e1->isAccount () &&
        (e1->getNodeType () & STPathElement::typeCurrency) &&
        isSFC (e1->getCurrency ()))
    {
        return make_SFCEndpointStep (ctx, e1->getAccountID ());
    }

    if (ctx.isLast && isSFCAccount (*e1) && e2->isAccount())
        return make_SFCEndpointStep (ctx, e2->getAccountID());

    if (e1->isAccount() && e2->isAccount())
    {
        return make_DirectStepI (ctx, e1->getAccountID (),
            e2->getAccountID (), curIssue.currency);
    }

    if (e1->isOffer() && e2->isAccount())
    {
        // should already be taken care of
        JLOG (j.error())
            << "Found offer/account payment step. Aborting payment strand.";
        assert (0);
        if (ctx.view.rules().enabled(fix1373))
            return {temBAD_PATH, std::unique_ptr<Step>{}};
        Throw<FlowException> (tefEXCEPTION, "Found offer/account payment step.");
    }

    assert ((e2->getNodeType () & STPathElement::typeCurrency) ||
        (e2->getNodeType () & STPathElement::typeIssuer));
    auto const outCurrency = e2->getNodeType () & STPathElement::typeCurrency
        ? e2->getCurrency ()
        : curIssue.currency;
    auto const outIssuer = e2->getNodeType () & STPathElement::typeIssuer
        ? e2->getIssuerID ()
        : curIssue.account;

    if (isSFC (curIssue.currency) && isSFC (outCurrency))
    {
        JLOG (j.warn()) << "Found sfc/sfc offer payment step";
        return {temBAD_PATH, std::unique_ptr<Step>{}};
    }

    assert (e2->isOffer ());

    if (isSFC (outCurrency))
        return make_BookStepIX (ctx, curIssue);

    if (isSFC (curIssue.currency))
        return make_BookStepXI (ctx, {outCurrency, outIssuer});

    return make_BookStepII (ctx, curIssue, {outCurrency, outIssuer});
}

std::pair<TER, Strand>
toStrandV1 (
    ReadView const& view,
    AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Quality> const& limitQuality,
    boost::optional<Issue> const& sendMaxIssue,
    STPath const& path,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    beast::Journal j)
{
    if (isSFC (src))
    {
        JLOG (j.debug()) << "toStrand with sfcAccount as src";
        return {temBAD_PATH, Strand{}};
    }
    if (isSFC (dst))
    {
        JLOG (j.debug()) << "toStrand with sfcAccount as dst";
        return {temBAD_PATH, Strand{}};
    }
    if (!isConsistent (deliver))
    {
        JLOG (j.debug()) << "toStrand inconsistent deliver issue";
        return {temBAD_PATH, Strand{}};
    }
    if (sendMaxIssue && !isConsistent (*sendMaxIssue))
    {
        JLOG (j.debug()) << "toStrand inconsistent sendMax issue";
        return {temBAD_PATH, Strand{}};
    }

    Issue curIssue = [&]
    {
        auto& currency =
            sendMaxIssue ? sendMaxIssue->currency : deliver.currency;
        if (isSFC (currency))
            return sfcIssue ();
        return Issue{currency, src};
    }();

    STPathElement const firstNode (
        STPathElement::typeAll, src, curIssue.currency, curIssue.account);

    boost::optional<STPathElement> sendMaxPE;
    if (sendMaxIssue && sendMaxIssue->account != src &&
        (path.empty () || !path[0].isAccount () ||
            path[0].getAccountID () != sendMaxIssue->account))
        sendMaxPE.emplace (sendMaxIssue->account, boost::none, boost::none);

    STPathElement const lastNode (dst, boost::none, boost::none);

    auto hasCurrency = [](STPathElement const* pe)
    {
        return pe->getNodeType () & STPathElement::typeCurrency;
    };

    boost::optional<STPathElement> deliverOfferNode;
    boost::optional<STPathElement> deliverAccountNode;

    std::vector<STPathElement const*> pes;
    // reserve enough for the path, the implied source, destination,
    // sendmax and deliver.
    pes.reserve (4 + path.size ());
    pes.push_back (&firstNode);
    if (sendMaxPE)
        pes.push_back (&*sendMaxPE);
    for (auto& i : path)
        pes.push_back (&i);

    // Note that for offer crossing (only) we do use an offer book even if
    // all that is changing is the Issue.account.
    STPathElement const* const lastCurrency =
        *std::find_if (pes.rbegin(), pes.rend(), hasCurrency);
    if ((lastCurrency->getCurrency() != deliver.currency) ||
        (offerCrossing && lastCurrency->getIssuerID() != deliver.account))
    {
        deliverOfferNode.emplace (boost::none, deliver.currency, deliver.account);
        pes.push_back (&*deliverOfferNode);
    }
    if (!((pes.back ()->isAccount() && pes.back ()->getAccountID () == deliver.account) ||
          (lastNode.isAccount() && lastNode.getAccountID () == deliver.account)))
    {
        deliverAccountNode.emplace (deliver.account, boost::none, boost::none);
        pes.push_back (&*deliverAccountNode);
    }
    if (*pes.back() != lastNode)
        pes.push_back (&lastNode);

    auto const strandSrc = firstNode.getAccountID ();
    auto const strandDst = lastNode.getAccountID ();
    bool const isDefaultPath = path.empty();

    Strand result;
    result.reserve (2 * pes.size ());

    /* A strand may not include the same account node more than once
       in the same currency. In a direct step, an account will show up
       at most twice: once as a src and once as a dst (hence the two element array).
       The strandSrc and strandDst will only show up once each.
    */
    std::array<boost::container::flat_set<Issue>, 2> seenDirectIssues;
    // A strand may not include the same offer book more than once
    boost::container::flat_set<Issue> seenBookOuts;
    seenDirectIssues[0].reserve (pes.size());
    seenDirectIssues[1].reserve (pes.size());
    seenBookOuts.reserve (pes.size());
    auto ctx = [&](bool isLast = false)
    {
        return StrandContext{view, result, strandSrc, strandDst, deliver,
            limitQuality, isLast, ownerPaysTransferFee, offerCrossing,
            isDefaultPath, seenDirectIssues, seenBookOuts, j};
    };

    for (int i = 0; i < pes.size () - 1; ++i)
    {
        /* Iterate through the path elements considering them in pairs.
           The first element of the pair is `cur` and the second element is
           `next`. When an offer is one of the pairs, the step created will be for
           `next`. This means when `cur` is an offer and `next` is an
           account then no step is created, as a step has already been created for
           that offer.
        */
        boost::optional<STPathElement> impliedPE;
        auto cur = pes[i];
        auto next = pes[i + 1];

        if (cur->isNone() || next->isNone())
            return {temBAD_PATH, Strand{}};

        /* If both account and issuer are set, use the account
           (and don't insert an implied node for the issuer).
           This matches the behavior of the previous generation payment code
        */
        if (cur->isAccount())
            curIssue.account = cur->getAccountID ();
        else if (cur->hasIssuer())
            curIssue.account = cur->getIssuerID ();

        if (cur->hasCurrency())
            curIssue.currency = cur->getCurrency ();

        if (cur->isAccount() && next->isAccount())
        {
            if (!isSFC (curIssue.currency) &&
                curIssue.account != cur->getAccountID () &&
                curIssue.account != next->getAccountID ())
            {
                JLOG (j.trace()) << "Inserting implied account";
                auto msr = make_DirectStepI (ctx(), cur->getAccountID (),
                    curIssue.account, curIssue.currency);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
                Currency dummy;
                impliedPE.emplace (STPathElement::typeAccount,
                    curIssue.account, dummy, curIssue.account);
                cur = &*impliedPE;
            }
        }
        else if (cur->isAccount() && next->isOffer())
        {
            if (curIssue.account != cur->getAccountID ())
            {
                JLOG (j.trace()) << "Inserting implied account before offer";
                auto msr = make_DirectStepI (ctx(), cur->getAccountID (),
                    curIssue.account, curIssue.currency);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
                Currency dummy;
                impliedPE.emplace (STPathElement::typeAccount,
                    curIssue.account, dummy, curIssue.account);
                cur = &*impliedPE;
            }
        }
        else if (cur->isOffer() && next->isAccount())
        {
            if (curIssue.account != next->getAccountID () &&
                !isSFC (next->getAccountID ()))
            {
                JLOG (j.trace()) << "Inserting implied account after offer";
                auto msr = make_DirectStepI (ctx(), curIssue.account,
                    next->getAccountID (), curIssue.currency);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
            }
            continue;
        }

        if (!next->isOffer() &&
            next->hasCurrency() && next->getCurrency () != curIssue.currency)
        {
            auto const& nextCurrency = next->getCurrency ();
            auto const& nextIssuer =
                next->hasIssuer () ? next->getIssuerID () : curIssue.account;

            if (isSFC (curIssue.currency))
            {
                JLOG (j.trace()) << "Inserting implied XI offer";
                auto msr = make_BookStepXI (
                    ctx(), {nextCurrency, nextIssuer});
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
            }
            else if (isSFC (nextCurrency))
            {
                JLOG (j.trace()) << "Inserting implied IX offer";
                auto msr = make_BookStepIX (ctx(), curIssue);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
            }
            else
            {
                JLOG (j.trace()) << "Inserting implied II offer";
                auto msr = make_BookStepII (
                    ctx(), curIssue, {nextCurrency, nextIssuer});
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
            }
            impliedPE.emplace (
                boost::none, nextCurrency, nextIssuer);
            cur = &*impliedPE;
            curIssue.currency = nextCurrency;
            curIssue.account = nextIssuer;
        }

        auto s =
            toStep (ctx (/*isLast*/ i == pes.size () - 2), cur, next, curIssue);
        if (s.first == tesSUCCESS)
            result.emplace_back (std::move (s.second));
        else
        {
            JLOG (j.debug()) << "toStep failed: " << s.first;
            return {s.first, Strand{}};
        }
    }

    return {tesSUCCESS, std::move (result)};
}


std::pair<TER, Strand>
toStrandV2 (
    ReadView const& view,
    AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Quality> const& limitQuality,
    boost::optional<Issue> const& sendMaxIssue,
    STPath const& path,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    beast::Journal j)
{
    if (isSFC(src) || isSFC(dst) ||
        !isConsistent(deliver) || (sendMaxIssue && !isConsistent(*sendMaxIssue)))
        return {temBAD_PATH, Strand{}};

    if ((sendMaxIssue && sendMaxIssue->account == noAccount()) ||
        (src == noAccount()) ||
        (dst == noAccount()) ||
        (deliver.account == noAccount()))
        return {temBAD_PATH, Strand{}};

    for (auto const& pe : path)
    {
        auto const t = pe.getNodeType();

        if ((t & ~STPathElement::typeAll) || !t)
            return {temBAD_PATH, Strand{}};

        bool const hasAccount = t & STPathElement::typeAccount;
        bool const hasIssuer = t & STPathElement::typeIssuer;
        bool const hasCurrency = t & STPathElement::typeCurrency;

        if (hasAccount && (hasIssuer || hasCurrency))
            return {temBAD_PATH, Strand{}};

        if (hasIssuer && isSFC(pe.getIssuerID()))
            return {temBAD_PATH, Strand{}};

        if (hasAccount && isSFC(pe.getAccountID()))
            return {temBAD_PATH, Strand{}};

        if (hasCurrency && hasIssuer &&
            isSFC(pe.getCurrency()) != isSFC(pe.getIssuerID()))
            return {temBAD_PATH, Strand{}};

        if (hasIssuer && (pe.getIssuerID() == noAccount()))
            return {temBAD_PATH, Strand{}};

        if (hasAccount && (pe.getAccountID() == noAccount()))
            return {temBAD_PATH, Strand{}};
    }

    Issue curIssue = [&]
    {
        auto const& currency =
            sendMaxIssue ? sendMaxIssue->currency : deliver.currency;
        if (isSFC (currency))
            return sfcIssue ();
        return Issue{currency, src};
    }();

    auto hasCurrency = [](STPathElement const pe)
    {
        return pe.getNodeType () & STPathElement::typeCurrency;
    };

    std::vector<STPathElement> normPath;
    // reserve enough for the path, the implied source, destination,
    // sendmax and deliver.
    normPath.reserve(4 + path.size());
    {
        normPath.emplace_back(
            STPathElement::typeAll, src, curIssue.currency, curIssue.account);

        if (sendMaxIssue && sendMaxIssue->account != src &&
            (path.empty() || !path[0].isAccount() ||
             path[0].getAccountID() != sendMaxIssue->account))
        {
            normPath.emplace_back(sendMaxIssue->account, boost::none, boost::none);
        }

        for (auto const& i : path)
            normPath.push_back(i);

        {
            // Note that for offer crossing (only) we do use an offer book
            // even if all that is changing is the Issue.account.
            STPathElement const& lastCurrency =
                *std::find_if (normPath.rbegin(), normPath.rend(),
                    hasCurrency);
            if ((lastCurrency.getCurrency() != deliver.currency) ||
                (offerCrossing &&
                    lastCurrency.getIssuerID() != deliver.account))
            {
                normPath.emplace_back(
                    boost::none, deliver.currency, deliver.account);
            }
        }

        if (!((normPath.back().isAccount() &&
               normPath.back().getAccountID() == deliver.account) ||
              (dst == deliver.account)))
        {
            normPath.emplace_back(deliver.account, boost::none, boost::none);
        }

        if (!normPath.back().isAccount() ||
            normPath.back().getAccountID() != dst)
        {
            normPath.emplace_back(dst, boost::none, boost::none);
        }
    }

    if (normPath.size() < 2)
        return {temBAD_PATH, Strand{}};

    auto const strandSrc = normPath.front().getAccountID ();
    auto const strandDst = normPath.back().getAccountID ();
    bool const isDefaultPath = path.empty();

    Strand result;
    result.reserve (2 * normPath.size ());

    /* A strand may not include the same account node more than once
       in the same currency. In a direct step, an account will show up
       at most twice: once as a src and once as a dst (hence the two element array).
       The strandSrc and strandDst will only show up once each.
    */
    std::array<boost::container::flat_set<Issue>, 2> seenDirectIssues;
    // A strand may not include the same offer book more than once
    boost::container::flat_set<Issue> seenBookOuts;
    seenDirectIssues[0].reserve (normPath.size());
    seenDirectIssues[1].reserve (normPath.size());
    seenBookOuts.reserve (normPath.size());
    auto ctx = [&](bool isLast = false)
    {
        return StrandContext{view, result, strandSrc, strandDst, deliver,
            limitQuality, isLast, ownerPaysTransferFee, offerCrossing,
            isDefaultPath, seenDirectIssues, seenBookOuts, j};
    };

    for (std::size_t i = 0; i < normPath.size () - 1; ++i)
    {
        /* Iterate through the path elements considering them in pairs.
           The first element of the pair is `cur` and the second element is
           `next`. When an offer is one of the pairs, the step created will be for
           `next`. This means when `cur` is an offer and `next` is an
           account then no step is created, as a step has already been created for
           that offer.
        */
        boost::optional<STPathElement> impliedPE;
        auto cur = &normPath[i];
        auto const next = &normPath[i + 1];

        if (cur->isAccount())
            curIssue.account = cur->getAccountID ();
        else if (cur->hasIssuer())
            curIssue.account = cur->getIssuerID ();

        if (cur->hasCurrency())
        {
            curIssue.currency = cur->getCurrency ();
            if (isSFC(curIssue.currency))
                curIssue.account = sfcAccount();
        }

        if (cur->isAccount() && next->isAccount())
        {
            if (!isSFC (curIssue.currency) &&
                curIssue.account != cur->getAccountID () &&
                curIssue.account != next->getAccountID ())
            {
                JLOG (j.trace()) << "Inserting implied account";
                auto msr = make_DirectStepI (ctx(), cur->getAccountID (),
                    curIssue.account, curIssue.currency);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
                impliedPE.emplace(STPathElement::typeAccount,
                    curIssue.account, sfcCurrency(), sfcAccount());
                cur = &*impliedPE;
            }
        }
        else if (cur->isAccount() && next->isOffer())
        {
            if (curIssue.account != cur->getAccountID ())
            {
                JLOG (j.trace()) << "Inserting implied account before offer";
                auto msr = make_DirectStepI (ctx(), cur->getAccountID (),
                    curIssue.account, curIssue.currency);
                if (msr.first != tesSUCCESS)
                    return {msr.first, Strand{}};
                result.push_back (std::move (msr.second));
                impliedPE.emplace(STPathElement::typeAccount,
                    curIssue.account, sfcCurrency(), sfcAccount());
                cur = &*impliedPE;
            }
        }
        else if (cur->isOffer() && next->isAccount())
        {
            if (curIssue.account != next->getAccountID () &&
                !isSFC (next->getAccountID ()))
            {
                if (isSFC(curIssue))
                {
                    if (i != normPath.size() - 2)
                        return {temBAD_PATH, Strand{}};
                    else
                    {
                        // Last step. insert sfc endpoint step
                        auto msr = make_SFCEndpointStep (ctx(), next->getAccountID());
                        if (msr.first != tesSUCCESS)
                            return {msr.first, Strand{}};
                        result.push_back(std::move(msr.second));
                    }
                }
                else
                {
                    JLOG(j.trace()) << "Inserting implied account after offer";
                    auto msr = make_DirectStepI(ctx(),
                        curIssue.account, next->getAccountID(), curIssue.currency);
                    if (msr.first != tesSUCCESS)
                        return {msr.first, Strand{}};
                    result.push_back(std::move(msr.second));
                }
            }
            continue;
        }

        if (!next->isOffer() &&
            next->hasCurrency() && next->getCurrency () != curIssue.currency)
        {
            // Should never happen
            assert(0);
            return {temBAD_PATH, Strand{}};
        }

        auto s =
            toStep (ctx (/*isLast*/ i == normPath.size () - 2), cur, next, curIssue);
        if (s.first == tesSUCCESS)
            result.emplace_back (std::move (s.second));
        else
        {
            JLOG (j.debug()) << "toStep failed: " << s.first;
            return {s.first, Strand{}};
        }
    }

    auto checkStrand = [&]() -> bool {
        auto stepAccts = [](Step const& s) -> std::pair<AccountID, AccountID> {
            if (auto r = s.directStepAccts())
                return *r;
            if (auto const r = s.bookStepBook())
                return std::make_pair(r->in.account, r->out.account);
            Throw<FlowException>(
                tefEXCEPTION, "Step should be either a direct or book step");
            return std::make_pair(sfcAccount(), sfcAccount());
        };

        auto curAccount = src;
        auto curIssue = [&] {
            auto& currency =
                sendMaxIssue ? sendMaxIssue->currency : deliver.currency;
            if (isSFC(currency))
                return sfcIssue();
            return Issue{currency, src};
        }();

        for (auto const& s : result)
        {
            auto const accts = stepAccts(*s);
            if (accts.first != curAccount)
                return false;

            if (auto const b = s->bookStepBook())
            {
                if (curIssue != b->in)
                    return false;
                curIssue = b->out;
            }
            else
            {
                curIssue.account = accts.second;
            }

            curAccount = accts.second;
        }
        if (curAccount != dst)
            return false;
        if (curIssue.currency != deliver.currency)
            return false;
        if (curIssue.account != deliver.account &&
            curIssue.account != dst)
            return false;
        return true;
    };

    if (!checkStrand())
    {
        JLOG (j.warn()) << "Flow check strand failed";
        assert(0);
        return {temBAD_PATH, Strand{}};
    }

    return {tesSUCCESS, std::move (result)};
}

std::pair<TER, Strand>
toStrand (
    ReadView const& view,
    AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Quality> const& limitQuality,
    boost::optional<Issue> const& sendMaxIssue,
    STPath const& path,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    beast::Journal j)
{
    if (view.rules().enabled(fix1373))
        return toStrandV2(view, src, dst, deliver, limitQuality,
            sendMaxIssue, path, ownerPaysTransferFee, offerCrossing, j);
    else
        return toStrandV1(view, src, dst, deliver, limitQuality,
            sendMaxIssue, path, ownerPaysTransferFee, offerCrossing, j);
}

std::pair<TER, std::vector<Strand>>
toStrands (
    ReadView const& view,
    AccountID const& src,
    AccountID const& dst,
    Issue const& deliver,
    boost::optional<Quality> const& limitQuality,
    boost::optional<Issue> const& sendMax,
    STPathSet const& paths,
    bool addDefaultPath,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    beast::Journal j)
{
    std::vector<Strand> result;
    result.reserve (1 + paths.size ());
    // Insert the strand into result if it is not already part of the vector
    auto insert = [&](Strand s)
    {
        bool const hasStrand =
            std::find (result.begin(), result.end(), s) != result.end ();

        if (!hasStrand)
            result.emplace_back (std::move (s));
    };

    if (addDefaultPath)
    {
        auto sp = toStrand (view, src, dst, deliver, limitQuality,
            sendMax, STPath(), ownerPaysTransferFee, offerCrossing, j);
        auto const ter = sp.first;
        auto& strand = sp.second;

        if (ter != tesSUCCESS)
        {
            JLOG (j.trace()) << "failed to add default path";
            if (isTemMalformed (ter) || paths.empty ()) {
                return {ter, std::vector<Strand>{}};
            }
        }
        else if (strand.empty ())
        {
            JLOG (j.trace()) << "toStrand failed";
            Throw<FlowException> (tefEXCEPTION, "toStrand returned tes & empty strand");
        }
        else
        {
            insert(std::move(strand));
        }
    }
    else if (paths.empty ())
    {
        JLOG (j.debug())
            << "Flow: Invalid transaction: No paths and direct SupplyFinanceChain not allowed.";
        return {temSUPPLYFINANCECHAIN_EMPTY, std::vector<Strand>{}};
    }

    TER lastFailTer = tesSUCCESS;
    for (auto const& p : paths)
    {
        auto sp = toStrand (view, src, dst, deliver,
            limitQuality, sendMax, p, ownerPaysTransferFee, offerCrossing, j);
        auto ter = sp.first;
        auto& strand = sp.second;

        if (ter != tesSUCCESS)
        {
            lastFailTer = ter;
            JLOG (j.trace())
                    << "failed to add path: ter: " << ter
                    << "path: " << p.getJson(JsonOptions::none);
            if (isTemMalformed (ter))
                return {ter, std::vector<Strand>{}};
        }
        else if (strand.empty ())
        {
            JLOG (j.trace()) << "toStrand failed";
            Throw<FlowException> (tefEXCEPTION, "toStrand returned tes & empty strand");
        }
        else
        {
            insert(std::move(strand));
        }
    }

    if (result.empty ())
        return {lastFailTer, std::move (result)};

    return {tesSUCCESS, std::move (result)};
}

StrandContext::StrandContext (
    ReadView const& view_,
    std::vector<std::unique_ptr<Step>> const& strand_,
    // A strand may not include an inner node that
    // replicates the source or destination.
    AccountID const& strandSrc_,
    AccountID const& strandDst_,
    Issue const& strandDeliver_,
    boost::optional<Quality> const& limitQuality_,
    bool isLast_,
    bool ownerPaysTransferFee_,
    bool offerCrossing_,
    bool isDefaultPath_,
    std::array<boost::container::flat_set<Issue>, 2>& seenDirectIssues_,
    boost::container::flat_set<Issue>& seenBookOuts_,
    beast::Journal j_)
        : view (view_)
        , strandSrc (strandSrc_)
        , strandDst (strandDst_)
        , strandDeliver (strandDeliver_)
        , limitQuality (limitQuality_)
        , isFirst (strand_.empty ())
        , isLast (isLast_)
        , ownerPaysTransferFee (ownerPaysTransferFee_)
        , offerCrossing (offerCrossing_)
        , isDefaultPath (isDefaultPath_)
        , strandSize (strand_.size ())
        , prevStep (!strand_.empty () ? strand_.back ().get ()
                     : nullptr)
        , seenDirectIssues(seenDirectIssues_)
        , seenBookOuts(seenBookOuts_)
        , j (j_)
{
}

template<class InAmt, class OutAmt>
bool
isDirectSfcToSfc(Strand const& strand)
{
    return false;
}

template<>
bool
isDirectSfcToSfc<SFCAmount, SFCAmount> (Strand const& strand)
{
    return (strand.size () == 2);
}

template
bool
isDirectSfcToSfc<SFCAmount, IOUAmount> (Strand const& strand);
template
bool
isDirectSfcToSfc<IOUAmount, SFCAmount> (Strand const& strand);
template
bool
isDirectSfcToSfc<IOUAmount, IOUAmount> (Strand const& strand);

} // SupplyFinanceChain
