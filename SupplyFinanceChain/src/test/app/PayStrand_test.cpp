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

#include <SupplyFinanceChain/app/paths/Flow.h>
#include <SupplyFinanceChain/app/paths/SupplyFinanceChainCalc.h>
#include <SupplyFinanceChain/app/paths/impl/Steps.h>
#include <SupplyFinanceChain/basics/contract.h>
#include <SupplyFinanceChain/basics/safe_cast.h>
#include <SupplyFinanceChain/core/Config.h>
#include <SupplyFinanceChain/ledger/ApplyViewImpl.h>
#include <SupplyFinanceChain/ledger/PaymentSandbox.h>
#include <SupplyFinanceChain/ledger/Sandbox.h>
#include <SupplyFinanceChain/protocol/Feature.h>
#include <SupplyFinanceChain/protocol/jss.h>
#include <test/jtx.h>
#include <test/jtx/PathSet.h>

namespace SupplyFinanceChain {
namespace test {

struct DirectStepInfo
{
    AccountID src;
    AccountID dst;
    Currency currency;
};

struct SFCEndpointStepInfo
{
    AccountID acc;
};

enum class TrustFlag {freeze, auth, noSupplyFinanceChain};

/*constexpr*/ std::uint32_t trustFlag (TrustFlag f, bool useHigh)
{
    switch(f)
    {
        case TrustFlag::freeze:
            if (useHigh)
                return lsfHighFreeze;
            return lsfLowFreeze;
        case TrustFlag::auth:
            if (useHigh)
                return lsfHighAuth;
            return lsfLowAuth;
        case TrustFlag::noSupplyFinanceChain:
            if (useHigh)
                return lsfHighNoSupplyFinanceChain;
            return lsfLowNoSupplyFinanceChain;
    }
    return 0; // Silence warning about end of non-void function
}

bool
getTrustFlag(
    jtx::Env const& env,
    jtx::Account const& src,
    jtx::Account const& dst,
    Currency const& cur,
    TrustFlag flag)
{
    if (auto sle = env.le(keylet::line(src, dst, cur)))
    {
        auto const useHigh = src.id() > dst.id();
        return sle->isFlag(trustFlag(flag, useHigh));
    }
    Throw<std::runtime_error>("No line in getTrustFlag");
    return false;  // silence warning
}

bool
equal(std::unique_ptr<Step> const& s1, DirectStepInfo const& dsi)
{
    if (!s1)
        return false;
    return test::directStepEqual(*s1, dsi.src, dsi.dst, dsi.currency);
}

bool
equal(std::unique_ptr<Step> const& s1, SFCEndpointStepInfo const& sfcsi)
{
    if (!s1)
        return false;
    return test::sfcEndpointStepEqual(*s1, sfcsi.acc);
}

bool
equal(std::unique_ptr<Step> const& s1, SupplyFinanceChain::Book const& bsi)
{
    if (!s1)
        return false;
    return bookStepEqual(*s1, bsi);
}

template <class Iter>
bool
strandEqualHelper(Iter i)
{
    // base case. all args processed and found equal.
    return true;
}

template <class Iter, class StepInfo, class... Args>
bool
strandEqualHelper(Iter i, StepInfo&& si, Args&&... args)
{
    if (!equal(*i, std::forward<StepInfo>(si)))
        return false;
    return strandEqualHelper(++i, std::forward<Args>(args)...);
}

template <class... Args>
bool
equal(Strand const& strand, Args&&... args)
{
    if (strand.size() != sizeof...(Args))
        return false;
    if (strand.empty())
        return true;
    return strandEqualHelper(strand.begin(), std::forward<Args>(args)...);
}

STPathElement
ape(AccountID const& a)
{
    return STPathElement(
        STPathElement::typeAccount, a, sfcCurrency(), sfcAccount());
};

// Issue path element
STPathElement
ipe(Issue const& iss)
{
    return STPathElement(
        STPathElement::typeCurrency | STPathElement::typeIssuer,
        sfcAccount(),
        iss.currency,
        iss.account);
};

// Issuer path element
STPathElement
iape(AccountID const& account)
{
    return STPathElement(
        STPathElement::typeIssuer, sfcAccount(), sfcCurrency(), account);
};

// Currency path element
STPathElement
cpe(Currency const& c)
{
    return STPathElement(
        STPathElement::typeCurrency, sfcAccount(), c, sfcAccount());
};

// All path element
STPathElement
allpe(AccountID const& a, Issue const& iss)
{
    return STPathElement(
        STPathElement::typeAccount | STPathElement::typeCurrency |
            STPathElement::typeIssuer,
        a,
        iss.currency,
        iss.account);
};

class ElementComboIter
{
    enum class SB /*state bit*/
        : std::uint16_t
    { acc,
      iss,
      cur,
      rootAcc,
      rootIss,
      sfc,
      sameAccIss,
      existingAcc,
      existingCur,
      existingIss,
      prevAcc,
      prevCur,
      prevIss,
      boundary,
      last };

    std::uint16_t state_ = 0;
    static_assert(safe_cast<size_t>(SB::last) <= sizeof(decltype(state_)) * 8, "");
    STPathElement const* prev_ = nullptr;
    // disallow iss and cur to be specified with acc is specified (simplifies some tests)
    bool const allowCompound_ = false;

    bool
    has(SB s) const
    {
        return state_ & (1 << safe_cast<int>(s));
    }

    bool
    hasAny(std::initializer_list<SB> sb) const
    {
        for (auto const s : sb)
            if (has(s))
                return true;
        return false;
    }

    size_t
    count(std::initializer_list<SB> sb) const
    {
        size_t result=0;

        for (auto const s : sb)
            if (has(s))
                result++;
        return result;
    }

public:
    explicit ElementComboIter(STPathElement const* prev = nullptr) : prev_(prev)
    {
    }

    bool
    valid() const
    {
        return
            (allowCompound_ || !(has(SB::acc) && hasAny({SB::cur, SB::iss}))) &&
            (!hasAny({SB::prevAcc, SB::prevCur, SB::prevIss}) || prev_) &&
            (!hasAny({SB::rootAcc, SB::sameAccIss, SB::existingAcc, SB::prevAcc}) || has(SB::acc)) &&
            (!hasAny({SB::rootIss, SB::sameAccIss, SB::existingIss, SB::prevIss}) || has(SB::iss)) &&
            (!hasAny({SB::sfc, SB::existingCur, SB::prevCur}) || has(SB::cur)) &&
            // These will be duplicates
            (count({SB::sfc, SB::existingCur, SB::prevCur}) <= 1) &&
            (count({SB::rootAcc, SB::existingAcc, SB::prevAcc}) <= 1) &&
            (count({SB::rootIss, SB::existingIss, SB::rootIss}) <= 1);
    }
    bool
    next()
    {
        if (!(has(SB::last)))
        {
            do
            {
                ++state_;
            } while (!valid());
        }
        return !has(SB::last);
    }

    template <
        class Col,
        class AccFactory,
        class IssFactory,
        class CurrencyFactory>
    void
    emplace_into(
        Col& col,
        AccFactory&& accF,
        IssFactory&& issF,
        CurrencyFactory&& currencyF,
        boost::optional<AccountID> const& existingAcc,
        boost::optional<Currency> const& existingCur,
        boost::optional<AccountID> const& existingIss)
    {
        assert(!has(SB::last));

        auto const acc = [&]() -> boost::optional<AccountID> {
            if (!has(SB::acc))
                return boost::none;
            if (has(SB::rootAcc))
                return sfcAccount();
            if (has(SB::existingAcc) && existingAcc)
                return existingAcc;
            return accF().id();
        }();
        auto const iss = [&]() -> boost::optional<AccountID> {
            if (!has(SB::iss))
                return boost::none;
            if (has(SB::rootIss))
                return sfcAccount();
            if (has(SB::sameAccIss))
                return acc;
            if (has(SB::existingIss) && existingIss)
                return *existingIss;
            return issF().id();
        }();
        auto const cur = [&]() -> boost::optional<Currency> {
            if (!has(SB::cur))
                return boost::none;
            if (has(SB::sfc))
                return sfcCurrency();
            if (has(SB::existingCur) && existingCur)
                return *existingCur;
            return currencyF();
        }();
        if (!has(SB::boundary))
            col.emplace_back(acc, cur, iss);
        else
            col.emplace_back(
                STPathElement::Type::typeBoundary,
                acc.value_or(AccountID{}),
                cur.value_or(Currency{}),
                iss.value_or(AccountID{}));
    }
};

struct ExistingElementPool
{
    std::vector<jtx::Account> accounts;
    std::vector<SupplyFinanceChain::Currency> currencies;
    std::vector<std::string> currencyNames;

    jtx::Account
    getAccount(size_t id)
    {
        assert(id < accounts.size());
        return accounts[id];
    }

    SupplyFinanceChain::Currency
    getCurrency(size_t id)
    {
        assert(id < currencies.size());
        return currencies[id];
    }

    // ids from 0 through (nextAvail -1) have already been used in the
    // path
    size_t nextAvailAccount = 0;
    size_t nextAvailCurrency = 0;

    using ResetState = std::tuple<size_t, size_t>;
    ResetState
    getResetState() const
    {
        return std::make_tuple(nextAvailAccount, nextAvailCurrency);
    }

    void
    resetTo(ResetState const& s)
    {
        std::tie(nextAvailAccount, nextAvailCurrency) = s;
    }

    struct StateGuard
    {
        ExistingElementPool& p_;
        ResetState state_;

        explicit StateGuard(ExistingElementPool& p) : p_{p}, state_{p.getResetState()}
        {
        }
        ~StateGuard()
        {
            p_.resetTo(state_);
        }
    };

    // Create the given number of accounts, and add trust lines so every
    // account trusts every other with every currency
    // Create an offer from every currency/account to every other
    // currency/account; the offer owner is either the specified
    // account or the issuer of the "taker gets" account
    void
    setupEnv(
        jtx::Env& env,
        size_t numAct,
        size_t numCur,
        boost::optional<size_t> const& offererIndex)
    {
        using namespace jtx;

        assert(!offererIndex || offererIndex < numAct);

        accounts.clear();
        accounts.reserve(numAct);
        currencies.clear();
        currencies.reserve(numCur);
        currencyNames.clear();
        currencyNames.reserve(numCur);

        constexpr size_t bufSize = 32;
        char buf[bufSize];

        for (size_t id = 0; id < numAct; ++id)
        {
            snprintf(buf, bufSize, "A%zu", id);
            accounts.emplace_back(buf);
        }

        for (size_t id = 0; id < numCur; ++id)
        {
            if (id < 10)
                snprintf(buf, bufSize, "CC%zu", id);
            else if (id < 100)
                snprintf(buf, bufSize, "C%zu", id);
            else
                snprintf(buf, bufSize, "%zu", id);
            currencies.emplace_back(to_currency(buf));
            currencyNames.emplace_back(buf);
        }

        for (auto const& a : accounts)
            env.fund(SFC(100000), a);

        // Every account trusts every other account with every currency
        for (auto ai1 = accounts.begin(), aie = accounts.end(); ai1 != aie;
             ++ai1)
        {
            for (auto ai2 = accounts.begin(); ai2 != aie; ++ai2)
            {
                if (ai1 == ai2)
                    continue;
                for (auto const& cn : currencyNames)
                {
                    env.trust((*ai1)[cn](1'000'000), *ai2);
                    if (ai1 > ai2)
                    {
                        // accounts with lower indexes hold balances from
                        // accounts
                        // with higher indexes
                        auto const& src = *ai1;
                        auto const& dst = *ai2;
                        env(pay(src, dst, src[cn](500000)));
                    }
                }
                env.close();
            }
        }

        std::vector<IOU> ious;
        ious.reserve(numAct * numCur);
        for (auto const& a : accounts)
            for (auto const& cn : currencyNames)
                ious.emplace_back(a[cn]);

        // create offers from every currency to every other currency
        for (auto takerPays = ious.begin(), ie = ious.end(); takerPays != ie;
             ++takerPays)
        {
            for (auto takerGets = ious.begin(); takerGets != ie; ++takerGets)
            {
                if (takerPays == takerGets)
                    continue;
                auto const owner =
                    offererIndex ? accounts[*offererIndex] : takerGets->account;
                if (owner.id() != takerGets->account.id())
                    env(pay(takerGets->account, owner, (*takerGets)(1000)));

                env(offer(owner, (*takerPays)(1000), (*takerGets)(1000)),
                    txflags(tfPassive));
            }
            env.close();
        }

        // create offers to/from sfc to every other ious
        for (auto const& iou : ious)
        {
            auto const owner =
                offererIndex ? accounts[*offererIndex] : iou.account;
            env(offer(owner, iou(1000), SFC(1000)), txflags(tfPassive));
            env(offer(owner, SFC(1000), iou(1000)), txflags(tfPassive));
            env.close();
        }
    }

    std::int64_t
    totalSFC(ReadView const& v, bool incRoot)
    {
        std::uint64_t totalSFC = 0;
        auto add = [&](auto const& a) {
            // SFC balance
            auto const sle = v.read(keylet::account(a));
            if (!sle)
                return;
            auto const b = (*sle)[sfBalance];
            totalSFC += b.mantissa();
        };
        for (auto const& a : accounts)
            add(a);
        if (incRoot)
            add(sfcAccount());
        return totalSFC;
    }

    // Check that the balances for all accounts for all currencies & SFC are the
    // same
    bool
    checkBalances(ReadView const& v1, ReadView const& v2)
    {
        std::vector<std::tuple<STAmount, STAmount, AccountID, AccountID>> diffs;

        auto sfcBalance = [](ReadView const& v, SupplyFinanceChain::Keylet const& k) {
            auto const sle = v.read(k);
            if (!sle)
                return STAmount{};
            return (*sle)[sfBalance];
        };
        auto lineBalance = [](ReadView const& v, SupplyFinanceChain::Keylet const& k) {
            auto const sle = v.read(k);
            if (!sle)
                return STAmount{};
            return (*sle)[sfBalance];
        };
        std::uint64_t totalSFC[2];
        for (auto ai1 = accounts.begin(), aie = accounts.end(); ai1 != aie;
             ++ai1)
        {
            {
                // SFC balance
                auto const ak = keylet::account(*ai1);
                auto const b1 = sfcBalance(v1, ak);
                auto const b2 = sfcBalance(v2, ak);
                totalSFC[0] += b1.mantissa();
                totalSFC[1] += b2.mantissa();
                if (b1 != b2)
                    diffs.emplace_back(b1, b2, sfcAccount(), *ai1);
            }
            for (auto ai2 = accounts.begin(); ai2 != aie; ++ai2)
            {
                if (ai1 >= ai2)
                    continue;
                for (auto const& c : currencies)
                {
                    // Line balance
                    auto const lk = keylet::line(*ai1, *ai2, c);
                    auto const b1 = lineBalance(v1, lk);
                    auto const b2 = lineBalance(v2, lk);
                    if (b1 != b2)
                        diffs.emplace_back(b1, b2, *ai1, *ai2);
                }
            }
        }
        return diffs.empty();
    }

    jtx::Account
    getAvailAccount()
    {
        return getAccount(nextAvailAccount++);
    }

    SupplyFinanceChain::Currency
    getAvailCurrency()
    {
        return getCurrency(nextAvailCurrency++);
    }

    template <class F>
    void
    for_each_element_pair(
        STAmount const& sendMax,
        STAmount const& deliver,
        std::vector<STPathElement> const& prefix,
        std::vector<STPathElement> const& suffix,
        boost::optional<AccountID> const& existingAcc,
        boost::optional<Currency> const& existingCur,
        boost::optional<AccountID> const& existingIss,
        F&& f)
    {
        auto accF = [&] { return this->getAvailAccount(); };
        auto issF = [&] { return this->getAvailAccount(); };
        auto currencyF = [&] { return this->getAvailCurrency(); };

        STPathElement const* prevOuter =
            prefix.empty() ? nullptr : &prefix.back();
        ElementComboIter outer(prevOuter);

        std::vector<STPathElement> outerResult;
        std::vector<STPathElement> result;
        auto const resultSize = prefix.size() + suffix.size() + 2;
        outerResult.reserve(resultSize);
        result.reserve(resultSize);
        while (outer.next())
        {
            StateGuard og{*this};
            outerResult = prefix;
            outer.emplace_into(
                outerResult,
                accF,
                issF,
                currencyF,
                existingAcc,
                existingCur,
                existingIss);
            STPathElement const* prevInner = &outerResult.back();
            ElementComboIter inner(prevInner);
            while (inner.next())
            {
                StateGuard ig{*this};
                result = outerResult;
                inner.emplace_into(
                    result,
                    accF,
                    issF,
                    currencyF,
                    existingAcc,
                    existingCur,
                    existingIss);
                result.insert(result.end(), suffix.begin(), suffix.end());
                f(sendMax, deliver, result);
            }
        };
    }
};

struct PayStrandAllPairs_test : public beast::unit_test::suite
{
    // Test every combination of element type pairs on a path
    void
    testAllPairs(FeatureBitset features)
    {
        testcase("All pairs");
        using namespace jtx;
        using SupplyFinanceChainCalc = ::SupplyFinanceChain::path::SupplyFinanceChainCalc;

        ExistingElementPool eep;
        Env env(*this, features);

        auto const closeTime = fix1298Time() +
            100 * env.closed()->info().closeTimeResolution;
        env.close(closeTime);
        eep.setupEnv(env, /*numAcc*/ 9, /*numCur*/ 6, boost::none);
        env.close();

        auto const src = eep.getAvailAccount();
        auto const dst = eep.getAvailAccount();

        SupplyFinanceChainCalc::Input inputs;
        inputs.defaultPathsAllowed = false;

        auto callback = [&](
            STAmount const& sendMax,
            STAmount const& deliver,
            std::vector<STPathElement> const& p) {
            std::array<PaymentSandbox, 2> sbs{
                {PaymentSandbox{env.current().get(), tapNONE},
                 PaymentSandbox{env.current().get(), tapNONE}}};
            std::array<SupplyFinanceChainCalc::Output, 2> rcOutputs;
            // pay with both env1 and env2
            // check all result and account balances match
            // save results so can see if run out of funds or somesuch
            STPathSet paths;
            paths.emplace_back(p);
            for (auto i = 0; i < 2; ++i)
            {
                if (i == 0)
                    env.app().config().features.insert(featureFlow);
                else
                    env.app().config().features.erase(featureFlow);

                try
                {
                    rcOutputs[i] = SupplyFinanceChainCalc::SupplyFinanceChainCalculate(
                        sbs[i],
                        sendMax,
                        deliver,
                        dst,
                        src,
                        paths,
                        env.app().logs(),
                        &inputs);
                }
                catch (...)
                {
                    this->fail();
                }
            }

            // check combinations of src and dst currencies (inc sfc)
            // Check the results
            auto const terMatch = [&] {
                if (rcOutputs[0].result() == rcOutputs[1].result())
                    return true;

                // handle some know error code mismatches
                if (p.empty() ||
                    !(rcOutputs[0].result() == temBAD_PATH ||
                      rcOutputs[0].result() == temBAD_PATH_LOOP))
                    return false;

                if (rcOutputs[1].result() == temBAD_PATH)
                    return true;

                if (rcOutputs[1].result() == terNO_LINE)
                    return true;

                for (auto const& pe : p)
                {
                    auto const t = pe.getNodeType();
                    if ((t & STPathElement::typeAccount) &&
                        t != STPathElement::typeAccount)
                    {
                        return true;
                    }
                }

                // sfc followed by offer that doesn't specify both currency and
                // issuer (and currency is not sfc, if specifyed)
                if (isSFC(sendMax) &&
                    !(p[0].hasCurrency() && isSFC(p[0].getCurrency())) &&
                    !(p[0].hasCurrency() && p[0].hasIssuer()))
                {
                    return true;
                }

                for (size_t i = 0; i < p.size() - 1; ++i)
                {
                    auto const tCur = p[i].getNodeType();
                    auto const tNext = p[i + 1].getNodeType();
                    if ((tCur & STPathElement::typeCurrency) &&
                        isSFC(p[i].getCurrency()) &&
                        (tNext & STPathElement::typeAccount) &&
                        !isSFC(p[i + 1].getAccountID()))
                    {
                        return true;
                    }
                }
                return false;
            }();

            this->BEAST_EXPECT(
                terMatch && (rcOutputs[0].result() == tesSUCCESS ||
                             rcOutputs[0].result() == temBAD_PATH ||
                             rcOutputs[0].result() == temBAD_PATH_LOOP));
            if (terMatch && rcOutputs[0].result() == tesSUCCESS)
                this->BEAST_EXPECT(eep.checkBalances(sbs[0], sbs[1]));
        };

        std::vector<STPathElement> prefix;
        std::vector<STPathElement> suffix;

        for (auto const srcAmtIsSFC : {false, true})
        {
            for (auto const dstAmtIsSFC : {false, true})
            {
                for (auto const hasPrefix : {false, true})
                {
                    ExistingElementPool::StateGuard esg{eep};
                    prefix.clear();
                    suffix.clear();

                    STAmount const sendMax{
                        srcAmtIsSFC ? sfcIssue() : Issue{eep.getAvailCurrency(),
                                                         eep.getAvailAccount()},
                        -1,  // (-1 == no limit)
                        0};

                    STAmount const deliver{
                        dstAmtIsSFC ? sfcIssue() : Issue{eep.getAvailCurrency(),
                                                         eep.getAvailAccount()},
                        1,
                        0};

                    if (hasPrefix)
                    {
                        for(auto const e0IsAccount : {false, true})
                        {
                            for (auto const e1IsAccount : {false, true})
                            {
                                ExistingElementPool::StateGuard presg{eep};
                                prefix.clear();
                                auto pushElement =
                                    [&prefix, &eep](bool isAccount) mutable {
                                        if (isAccount)
                                            prefix.emplace_back(
                                                eep.getAvailAccount().id(),
                                                boost::none,
                                                boost::none);
                                        else
                                            prefix.emplace_back(
                                                boost::none,
                                                eep.getAvailCurrency(),
                                                eep.getAvailAccount().id());
                                    };
                                pushElement(e0IsAccount);
                                pushElement(e1IsAccount);
                                boost::optional<AccountID> existingAcc;
                                boost::optional<Currency> existingCur;
                                boost::optional<AccountID> existingIss;
                                if (e0IsAccount)
                                {
                                    existingAcc = prefix[0].getAccountID();
                                }
                                else
                                {
                                    existingIss = prefix[0].getIssuerID();
                                    existingCur = prefix[0].getCurrency();
                                }
                                if (e1IsAccount)
                                {
                                    if (!existingAcc)
                                        existingAcc = prefix[1].getAccountID();
                                }
                                else
                                {
                                    if (!existingIss)
                                        existingIss = prefix[1].getIssuerID();
                                    if (!existingCur)
                                        existingCur = prefix[1].getCurrency();
                                }
                                eep.for_each_element_pair(
                                    sendMax,
                                    deliver,
                                    prefix,
                                    suffix,
                                    existingAcc,
                                    existingCur,
                                    existingIss,
                                    callback);
                            }
                        }
                    }
                    else
                    {
                        eep.for_each_element_pair(
                            sendMax,
                            deliver,
                            prefix,
                            suffix,
                            /*existingAcc*/ boost::none,
                            /*existingCur*/ boost::none,
                            /*existingIss*/ boost::none,
                            callback);
                    }
                }
            }
        }
    }
    void
    run() override
    {
        auto const sa = jtx::supported_amendments();
        testAllPairs(sa - featureFlowCross);
        testAllPairs(sa);
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(PayStrandAllPairs, app, SupplyFinanceChain, 12);

struct PayStrand_test : public beast::unit_test::suite
{
    void
    testToStrand(FeatureBitset features)
    {
        testcase("To Strand");

        using namespace jtx;

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");
        auto const gw = Account("gw");

        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        auto const eurC = EUR.currency;
        auto const usdC = USD.currency;

        using D = DirectStepInfo;
        using B = SupplyFinanceChain::Book;
        using SFCS = SFCEndpointStepInfo;

        auto test = [&, this](
            jtx::Env& env,
            Issue const& deliver,
            boost::optional<Issue> const& sendMaxIssue,
            STPath const& path,
            TER expTer,
            auto&&... expSteps) {
            auto r = toStrand(
                *env.current(),
                alice,
                bob,
                deliver,
                boost::none,
                sendMaxIssue,
                path,
                true,
                false,
                env.app().logs().journal("Flow"));
            BEAST_EXPECT(r.first == expTer);
            if (sizeof...(expSteps) !=0 )
                BEAST_EXPECT(equal(
                    r.second, std::forward<decltype(expSteps)>(expSteps)...));
        };

        {
            Env env(*this, features);
            env.fund(SFC(10000), alice, bob, gw);
            env.trust(USD(1000), alice, bob);
            env.trust(EUR(1000), alice, bob);
            env(pay(gw, alice, EUR(100)));

            {
                STPath const path =
                    STPath({ipe(bob["USD"]), cpe(EUR.currency)});
                auto r = toStrand(
                    *env.current(),
                    alice,
                    alice,
                    /*deliver*/ sfcIssue(),
                    /*limitQuality*/ boost::none,
                    /*sendMaxIssue*/ EUR.issue(),
                    path,
                    true,
                    false,
                    env.app().logs().journal("Flow"));
                BEAST_EXPECT(r.first == tesSUCCESS);
            }
            {
                STPath const path = STPath({ipe(USD), cpe(sfcCurrency())});
                auto r = toStrand(
                    *env.current(),
                    alice,
                    alice,
                    /*deliver*/ sfcIssue(),
                    /*limitQuality*/ boost::none,
                    /*sendMaxIssue*/ sfcIssue(),
                    path,
                    true,
                    false,
                    env.app().logs().journal("Flow"));
                BEAST_EXPECT(r.first == tesSUCCESS);
            }
            return;
        };

        {
            Env env(*this, features);
            env.fund(SFC(10000), alice, bob, carol, gw);

            test(env, USD, boost::none, STPath(), terNO_LINE);

            env.trust(USD(1000), alice, bob, carol);
            test(env, USD, boost::none, STPath(), tecPATH_DRY);

            env(pay(gw, alice, USD(100)));
            env(pay(gw, carol, USD(100)));

            // Insert implied account
            test(
                env,
                USD,
                boost::none,
                STPath(),
                tesSUCCESS,
                D{alice, gw, usdC},
                D{gw, bob, usdC});
            env.trust(EUR(1000), alice, bob);

            // Insert implied offer
            test(
                env,
                EUR,
                USD.issue(),
                STPath(),
                tesSUCCESS,
                D{alice, gw, usdC},
                B{USD, EUR},
                D{gw, bob, eurC});

            // Path with explicit offer
            test(
                env,
                EUR,
                USD.issue(),
                STPath({ipe(EUR)}),
                tesSUCCESS,
                D{alice, gw, usdC},
                B{USD, EUR},
                D{gw, bob, eurC});

            // Path with offer that changes issuer only
            env.trust(carol["USD"](1000), bob);
            test(
                env,
                carol["USD"],
                USD.issue(),
                STPath({iape(carol)}),
                tesSUCCESS,
                D{alice, gw, usdC},
                B{USD, carol["USD"]},
                D{carol, bob, usdC});

            // Path with SFC src currency
            test(
                env,
                USD,
                sfcIssue(),
                STPath({ipe(USD)}),
                tesSUCCESS,
                SFCS{alice},
                B{SFC, USD},
                D{gw, bob, usdC});

            // Path with SFC dst currency
            test(
                env,
                sfcIssue(),
                USD.issue(),
                STPath({ipe(SFC)}),
                tesSUCCESS,
                D{alice, gw, usdC},
                B{USD, SFC},
                SFCS{bob});

            // Path with SFC cross currency bridged payment
            test(
                env,
                EUR,
                USD.issue(),
                STPath({cpe(sfcCurrency())}),
                tesSUCCESS,
                D{alice, gw, usdC},
                B{USD, SFC},
                B{SFC, EUR},
                D{gw, bob, eurC});

            // SFC -> SFC transaction can't include a path
            test(env, SFC, boost::none, STPath({ape(carol)}), temBAD_PATH);

            {
                // The root account can't be the src or dst
                auto flowJournal = env.app().logs().journal("Flow");
                {
                    // The root account can't be the dst
                    auto r = toStrand(
                        *env.current(),
                        alice,
                        sfcAccount(),
                        SFC,
                        boost::none,
                        USD.issue(),
                        STPath(),
                        true,
                        false,
                        flowJournal);
                    BEAST_EXPECT(r.first == temBAD_PATH);
                }
                {
                    // The root account can't be the src
                    auto r = toStrand(
                        *env.current(),
                        sfcAccount(),
                        alice,
                        SFC,
                        boost::none,
                        boost::none,
                        STPath(),
                        true,
                        false,
                        flowJournal);
                    BEAST_EXPECT(r.first == temBAD_PATH);
                }
                {
                    // The root account can't be the src
                    auto r = toStrand(
                        *env.current(),
                        noAccount(),
                        bob,
                        USD,
                        boost::none,
                        boost::none,
                        STPath(),
                        true,
                        false,
                        flowJournal);
                    BEAST_EXPECT(r.first == terNO_ACCOUNT);
                }
            }

            // Create an offer with the same in/out issue
            test(
                env,
                EUR,
                USD.issue(),
                STPath({ipe(USD), ipe(EUR)}),
                temBAD_PATH);

            // Path element with type zero
            test(
                env,
                USD,
                boost::none,
                STPath({STPathElement(
                    0, sfcAccount(), sfcCurrency(), sfcAccount())}),
                temBAD_PATH);

            // The same account can't appear more than once on a path
            // `gw` will be used from alice->carol and implied between carol
            // and bob
            test(
                env,
                USD,
                boost::none,
                STPath({ape(gw), ape(carol)}),
                temBAD_PATH_LOOP);

            // The same offer can't appear more than once on a path
            test(
                env,
                EUR,
                USD.issue(),
                STPath({ipe(EUR), ipe(USD), ipe(EUR)}),
                temBAD_PATH_LOOP);
        }

        {
            // cannot have more than one offer with the same output issue

            using namespace jtx;
            Env env(*this, features);

            env.fund(SFC(10000), alice, bob, carol, gw);
            env.trust(USD(10000), alice, bob, carol);
            env.trust(EUR(10000), alice, bob, carol);

            env(pay(gw, bob, USD(100)));
            env(pay(gw, bob, EUR(100)));

            env(offer(bob, SFC(100), USD(100)));
            env(offer(bob, USD(100), EUR(100)), txflags(tfPassive));
            env(offer(bob, EUR(100), USD(100)), txflags(tfPassive));

            // payment path: SFC -> SFC/USD -> USD/EUR -> EUR/USD
            env(pay(alice, carol, USD(100)),
                path(~USD, ~EUR, ~USD),
                sendmax(SFC(200)),
                txflags(tfNoSupplyFinanceChainDirect),
                ter(temBAD_PATH_LOOP));
        }

        {
            Env env(*this, features);
            env.fund(SFC(10000), alice, bob, noSupplyFinanceChain(gw));
            env.trust(USD(1000), alice, bob);
            env(pay(gw, alice, USD(100)));
            test(env, USD, boost::none, STPath(), terNO_SUPPLYFINANCECHAIN);
        }

        {
            // check global freeze
            Env env(*this, features);
            env.fund(SFC(10000), alice, bob, gw);
            env.trust(USD(1000), alice, bob);
            env(pay(gw, alice, USD(100)));

            // Account can still issue payments
            env(fset(alice, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), tesSUCCESS);
            env(fclear(alice, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), tesSUCCESS);

            // Account can not issue funds
            env(fset(gw, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), terNO_LINE);
            env(fclear(gw, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), tesSUCCESS);

            // Account can not receive funds
            env(fset(bob, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), terNO_LINE);
            env(fclear(bob, asfGlobalFreeze));
            test(env, USD, boost::none, STPath(), tesSUCCESS);
        }
        {
            // Freeze between gw and alice
            Env env(*this, features);
            env.fund(SFC(10000), alice, bob, gw);
            env.trust(USD(1000), alice, bob);
            env(pay(gw, alice, USD(100)));
            test(env, USD, boost::none, STPath(), tesSUCCESS);
            env(trust(gw, alice["USD"](0), tfSetFreeze));
            BEAST_EXPECT(getTrustFlag(env, gw, alice, usdC, TrustFlag::freeze));
            test(env, USD, boost::none, STPath(), terNO_LINE);
        }
        {
            // check no auth
            // An account may require authorization to receive IOUs from an
            // issuer
            Env env(*this, features);
            env.fund(SFC(10000), alice, bob, gw);
            env(fset(gw, asfRequireAuth));
            env.trust(USD(1000), alice, bob);
            // Authorize alice but not bob
            env(trust(gw, alice["USD"](1000), tfSetfAuth));
            BEAST_EXPECT(getTrustFlag(env, gw, alice, usdC, TrustFlag::auth));
            env(pay(gw, alice, USD(100)));
            env.require(balance(alice, USD(100)));
            test(env, USD, boost::none, STPath(), terNO_AUTH);

            // Check pure issue redeem still works
            auto r = toStrand(
                *env.current(),
                alice,
                gw,
                USD,
                boost::none,
                boost::none,
                STPath(),
                true,
                false,
                env.app().logs().journal("Flow"));
            BEAST_EXPECT(r.first == tesSUCCESS);
            BEAST_EXPECT(equal(r.second, D{alice, gw, usdC}));
        }
        {
            // Check path with sendMax and node with correct sendMax already set
            Env env(*this, features);
            env.fund(SFC(10000), alice, bob, gw);
            env.trust(USD(1000), alice, bob);
            env.trust(EUR(1000), alice, bob);
            env(pay(gw, alice, EUR(100)));
            auto const path = STPath({STPathElement(
                STPathElement::typeAll,
                EUR.account,
                EUR.currency,
                EUR.account)});
            test(env, USD, EUR.issue(), path, tesSUCCESS);
        }

        {
            // last step sfc from offer
            Env env(*this, features);
            env.fund(SFC(10000), alice, bob, gw);
            env.trust(USD(1000), alice, bob);
            env(pay(gw, alice, USD(100)));

            // alice -> USD/SFC -> bob
            STPath path;
            path.emplace_back(boost::none, USD.currency, USD.account.id());
            path.emplace_back(boost::none, sfcCurrency(), boost::none);

            auto r = toStrand(
                *env.current(),
                alice,
                bob,
                SFC,
                boost::none,
                USD.issue(),
                path,
                false,
                false,
                env.app().logs().journal("Flow"));
            BEAST_EXPECT(r.first == tesSUCCESS);
            BEAST_EXPECT(equal(r.second, D{alice, gw, usdC}, B{USD.issue(), sfcIssue()}, SFCS{bob}));
        }
    }

    void
    testRIPD1373(FeatureBitset features)
    {
        using namespace jtx;
        testcase("RIPD1373");

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        if (features[fix1373])
        {
            Env env(*this, features);
            env.fund(SFC(10000), alice, bob, gw);

            env.trust(USD(1000), alice, bob);
            env.trust(EUR(1000), alice, bob);
            env.trust(bob["USD"](1000), alice, gw);
            env.trust(bob["EUR"](1000), alice, gw);

            env(offer(bob, SFC(100), bob["USD"](100)), txflags(tfPassive));
            env(offer(gw, SFC(100), USD(100)), txflags(tfPassive));

            env(offer(bob, bob["USD"](100), bob["EUR"](100)),
                txflags(tfPassive));
            env(offer(gw, USD(100), EUR(100)), txflags(tfPassive));

            Path const p = [&] {
                Path result;
                result.push_back(allpe(gw, bob["USD"]));
                result.push_back(cpe(EUR.currency));
                return result;
            }();

            PathSet paths(p);

            env(pay(alice, alice, EUR(1)),
                json(paths.json()),
                sendmax(SFC(10)),
                txflags(tfNoSupplyFinanceChainDirect | tfPartialPayment),
                ter(temBAD_PATH));
        }

        {
            Env env(*this, features);

            env.fund(SFC(10000), alice, bob, carol, gw);
            env.trust(USD(10000), alice, bob, carol);

            env(pay(gw, bob, USD(100)));

            env(offer(bob, SFC(100), USD(100)), txflags(tfPassive));
            env(offer(bob, USD(100), SFC(100)), txflags(tfPassive));

            // payment path: SFC -> SFC/USD -> USD/SFC
            env(pay(alice, carol, SFC(100)),
                path(~USD, ~SFC),
                txflags(tfNoSupplyFinanceChainDirect),
                ter(temBAD_SEND_SFC_PATHS));
        }

        {
            Env env(*this, features);

            env.fund(SFC(10000), alice, bob, carol, gw);
            env.trust(USD(10000), alice, bob, carol);

            env(pay(gw, bob, USD(100)));

            env(offer(bob, SFC(100), USD(100)), txflags(tfPassive));
            env(offer(bob, USD(100), SFC(100)), txflags(tfPassive));

            // payment path: SFC -> SFC/USD -> USD/SFC
            env(pay(alice, carol, SFC(100)),
                path(~USD, ~SFC),
                sendmax(SFC(200)),
                txflags(tfNoSupplyFinanceChainDirect),
                ter(temBAD_SEND_SFC_MAX));
        }
    }

    void
    testLoop(FeatureBitset features)
    {
        testcase("test loop");
        using namespace jtx;

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];
        auto const CNY = gw["CNY"];

        {
            Env env(*this, features);

            env.fund(SFC(10000), alice, bob, carol, gw);
            env.trust(USD(10000), alice, bob, carol);

            env(pay(gw, bob, USD(100)));
            env(pay(gw, alice, USD(100)));

            env(offer(bob, SFC(100), USD(100)), txflags(tfPassive));
            env(offer(bob, USD(100), SFC(100)), txflags(tfPassive));

            auto const expectedResult = [&] () -> TER {
                if (features[featureFlow] && !features[fix1373])
                    return tesSUCCESS;
                return temBAD_PATH_LOOP;
            }();
            // payment path: USD -> USD/SFC -> SFC/USD
            env(pay(alice, carol, USD(100)),
                sendmax(USD(100)),
                path(~SFC, ~USD),
                txflags(tfNoSupplyFinanceChainDirect),
                ter(expectedResult));
        }
        {
            Env env(*this, features);

            env.fund(SFC(10000), alice, bob, carol, gw);
            env.trust(USD(10000), alice, bob, carol);
            env.trust(EUR(10000), alice, bob, carol);
            env.trust(CNY(10000), alice, bob, carol);

            env(pay(gw, bob, USD(100)));
            env(pay(gw, bob, EUR(100)));
            env(pay(gw, bob, CNY(100)));

            env(offer(bob, SFC(100), USD(100)), txflags(tfPassive));
            env(offer(bob, USD(100), EUR(100)), txflags(tfPassive));
            env(offer(bob, EUR(100), CNY(100)), txflags(tfPassive));

            // payment path: SFC->SFC/USD->USD/EUR->USD/CNY
            env(pay(alice, carol, CNY(100)),
                sendmax(SFC(100)),
                path(~USD, ~EUR, ~USD, ~CNY),
                txflags(tfNoSupplyFinanceChainDirect),
                ter(temBAD_PATH_LOOP));
        }
    }

    void
    testNoAccount(FeatureBitset features)
    {
        testcase("test no account");
        using namespace jtx;

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const gw = Account("gw");
        auto const USD = gw["USD"];

        Env env(*this, features);
        env.fund(SFC(10000), alice, bob, gw);

        STAmount sendMax{USD.issue(), 100, 1};
        STAmount noAccountAmount{Issue{USD.currency, noAccount()}, 100, 1};
        STAmount deliver;
        AccountID const srcAcc = alice.id();
        AccountID dstAcc = bob.id();
        STPathSet pathSet;
        ::SupplyFinanceChain::path::SupplyFinanceChainCalc::Input inputs;
        inputs.defaultPathsAllowed = true;
        try
        {
            PaymentSandbox sb{env.current().get(), tapNONE};
            {
                auto const r = ::SupplyFinanceChain::path::SupplyFinanceChainCalc::SupplyFinanceChainCalculate(
                    sb, sendMax, deliver, dstAcc, noAccount(), pathSet,
                    env.app().logs(), &inputs);
                BEAST_EXPECT(r.result() == temBAD_PATH);
            }
            {
                auto const r = ::SupplyFinanceChain::path::SupplyFinanceChainCalc::SupplyFinanceChainCalculate(
                    sb, sendMax, deliver, noAccount(), srcAcc, pathSet,
                    env.app().logs(), &inputs);
                BEAST_EXPECT(r.result() == temBAD_PATH);
            }
            {
                auto const r = ::SupplyFinanceChain::path::SupplyFinanceChainCalc::SupplyFinanceChainCalculate(
                    sb, noAccountAmount, deliver, dstAcc, srcAcc, pathSet,
                    env.app().logs(), &inputs);
                BEAST_EXPECT(r.result() == temBAD_PATH);
            }
            {
                auto const r = ::SupplyFinanceChain::path::SupplyFinanceChainCalc::SupplyFinanceChainCalculate(
                    sb, sendMax, noAccountAmount, dstAcc, srcAcc, pathSet,
                    env.app().logs(), &inputs);
                BEAST_EXPECT(r.result() == temBAD_PATH);
            }
        }
        catch (...)
        {
            this->fail();
        }
    }

    void
    run() override
    {
        using namespace jtx;
        auto const sa = supported_amendments();
        testToStrand(sa - fix1373 - featureFlowCross);
        testToStrand(sa           - featureFlowCross);
        testToStrand(sa);

        testRIPD1373(sa - featureFlow - fix1373 - featureFlowCross);
        testRIPD1373(sa                         - featureFlowCross);
        testRIPD1373(sa);

        testLoop(sa - featureFlow - fix1373 - featureFlowCross);
        testLoop(sa               - fix1373 - featureFlowCross);
        testLoop(sa                         - featureFlowCross);
        testLoop(sa);

        testNoAccount(sa);
    }
};

BEAST_DEFINE_TESTSUITE(PayStrand, app, SupplyFinanceChain);

}  // test
}  // SupplyFinanceChain
