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

#include <SupplyFinanceChain/basics/chrono.h>
#include <SupplyFinanceChain/ledger/BookDirs.h>
#include <SupplyFinanceChain/ledger/ReadView.h>
#include <SupplyFinanceChain/ledger/View.h>
#include <SupplyFinanceChain/basics/contract.h>
#include <SupplyFinanceChain/basics/Log.h>
#include <SupplyFinanceChain/basics/StringUtilities.h>
#include <SupplyFinanceChain/protocol/Feature.h>
#include <SupplyFinanceChain/protocol/st.h>
#include <SupplyFinanceChain/protocol/Protocol.h>
#include <SupplyFinanceChain/protocol/Quality.h>
#include <boost/algorithm/string.hpp>
#include <cassert>

namespace SupplyFinanceChain {

NetClock::time_point const& fix1141Time ()
{
    using namespace std::chrono_literals;
    // Fri July 1, 2016 17:00:00 UTC
    static NetClock::time_point const soTime{520707600s};
    return soTime;
}

bool fix1141 (NetClock::time_point const closeTime)
{
    return closeTime > fix1141Time();
}

NetClock::time_point const& fix1274Time ()
{
    using namespace std::chrono_literals;
    // Fri Sep 30, 2016 17:00:00 UTC
    static NetClock::time_point const soTime{528570000s};

    return soTime;
}

bool fix1274 (NetClock::time_point const closeTime)
{
    return closeTime > fix1274Time();
}

NetClock::time_point const& fix1298Time ()
{
    using namespace std::chrono_literals;
    // Wed Dec 21, 2016 18:00:00 UTC
    static NetClock::time_point const soTime{535658400s};

    return soTime;
}

bool fix1298 (NetClock::time_point const closeTime)
{
    return closeTime > fix1298Time();
}

NetClock::time_point const& fix1443Time ()
{
    using namespace std::chrono_literals;
    // Sun Mar 12, 2017 01:00:00 UTC
    static NetClock::time_point const soTime{542595600s};

    return soTime;
}

bool fix1443 (NetClock::time_point const closeTime)
{
    return closeTime > fix1443Time();
}

NetClock::time_point const& fix1449Time ()
{
    using namespace std::chrono_literals;
    // Thurs, Mar 30, 2017 20:00:00 UTC
    static NetClock::time_point const soTime{544219200s};

    return soTime;
}

bool fix1449 (NetClock::time_point const closeTime)
{
    return closeTime > fix1449Time();
}

//------------------------------------------------------------------------------
//
// Observers
//
//------------------------------------------------------------------------------

void addRaw (LedgerInfo const& info, Serializer& s)
{
    s.add32 (info.seq);
    s.add64 (info.drops.drops ());
    s.add256 (info.parentHash);
    s.add256 (info.txHash);
    s.add256 (info.accountHash);
    s.add32 (info.parentCloseTime.time_since_epoch().count());
    s.add32 (info.closeTime.time_since_epoch().count());
    s.add8 (info.closeTimeResolution.count());
    s.add8 (info.closeFlags);
}

bool
isGlobalFrozen (ReadView const& view,
    AccountID const& issuer)
{
    // VFALCO Perhaps this should assert
    if (isSFC (issuer))
        return false;
    auto const sle =
        view.read(keylet::account(issuer));
    if (sle && sle->isFlag (lsfGlobalFreeze))
        return true;
    return false;
}

// Can the specified account spend the specified currency issued by
// the specified issuer or does the freeze flag prohibit it?
bool
isFrozen (ReadView const& view, AccountID const& account,
    Currency const& currency, AccountID const& issuer)
{
    if (isSFC (currency))
        return false;
    auto sle =
        view.read(keylet::account(issuer));
    if (sle && sle->isFlag (lsfGlobalFreeze))
        return true;
    if (issuer != account)
    {
        // Check if the issuer froze the line
        sle = view.read(keylet::line(
            account, issuer, currency));
        if (sle && sle->isFlag(
            (issuer > account) ?
                lsfHighFreeze : lsfLowFreeze))
            return true;
    }
    return false;
}

STAmount
accountHolds (ReadView const& view,
    AccountID const& account, Currency const& currency,
        AccountID const& issuer, FreezeHandling zeroIfFrozen,
              beast::Journal j)
{
    STAmount amount;
    if (isSFC(currency))
    {
        return {sfcLiquid (view, account, 0, j)};
    }

    // IOU: Return balance on trust line modulo freeze
    auto const sle = view.read(keylet::line(
        account, issuer, currency));
    if (! sle)
    {
        amount.clear ({currency, issuer});
    }
    else if ((zeroIfFrozen == fhZERO_IF_FROZEN) &&
        isFrozen(view, account, currency, issuer))
    {
        amount.clear (Issue (currency, issuer));
    }
    else
    {
        amount = sle->getFieldAmount (sfBalance);
        if (account > issuer)
        {
            // Put balance in account terms.
            amount.negate();
        }
        amount.setIssuer (issuer);
    }
    JLOG (j.trace()) << "accountHolds:" <<
        " account=" << to_string (account) <<
        " amount=" << amount.getFullText ();

    return view.balanceHook(
        account, issuer, amount);
}

STAmount
accountFunds (ReadView const& view, AccountID const& id,
    STAmount const& saDefault, FreezeHandling freezeHandling,
        beast::Journal j)
{
    STAmount saFunds;

    if (!saDefault.native () &&
        saDefault.getIssuer () == id)
    {
        saFunds = saDefault;
        JLOG (j.trace()) << "accountFunds:" <<
            " account=" << to_string (id) <<
            " saDefault=" << saDefault.getFullText () <<
            " SELF-FUNDED";
    }
    else
    {
        saFunds = accountHolds(view, id,
            saDefault.getCurrency(), saDefault.getIssuer(),
                freezeHandling, j);
        JLOG (j.trace()) << "accountFunds:" <<
            " account=" << to_string (id) <<
            " saDefault=" << saDefault.getFullText () <<
            " saFunds=" << saFunds.getFullText ();
    }
    return saFunds;
}

// Prevent ownerCount from wrapping under error conditions.
//
// adjustment allows the ownerCount to be adjusted up or down in multiple steps.
// If id != boost.none, then do error reporting.
//
// Returns adjusted owner count.
static
std::uint32_t
confineOwnerCount (std::uint32_t current, std::int32_t adjustment,
    boost::optional<AccountID> const& id = boost::none,
    beast::Journal j = beast::Journal {beast::Journal::getNullSink()})
{
    std::uint32_t adjusted {current + adjustment};
    if (adjustment > 0)
    {
        // Overflow is well defined on unsigned
        if (adjusted < current)
        {
            if (id)
            {
                JLOG (j.fatal()) <<
                    "Account " << *id <<
                    " owner count exceeds max!";
            }
            adjusted = std::numeric_limits<std::uint32_t>::max ();
        }
    }
    else
    {
        // Underflow is well defined on unsigned
        if (adjusted > current)
        {
            if (id)
            {
                JLOG (j.fatal()) <<
                    "Account " << *id <<
                    " owner count set below 0!";
            }
            adjusted = 0;
            assert(!id);
        }
    }
    return adjusted;
}

SFCAmount
sfcLiquid (ReadView const& view, AccountID const& id,
    std::int32_t ownerCountAdj, beast::Journal j)
{
    auto const sle = view.read(keylet::account(id));
    if (sle == nullptr)
        return beast::zero;

    // Return balance minus reserve
    if (fix1141 (view.info ().parentCloseTime))
    {
        std::uint32_t const ownerCount = confineOwnerCount (
            view.ownerCountHook (id, sle->getFieldU32 (sfOwnerCount)),
                ownerCountAdj);

        auto const reserve =
            view.fees().accountReserve(ownerCount);

        auto const fullBalance =
            sle->getFieldAmount(sfBalance);

        auto const balance = view.balanceHook(id, sfcAccount(), fullBalance);

        STAmount amount = balance - reserve;
        if (balance < reserve)
            amount.clear ();

        JLOG (j.trace()) << "accountHolds:" <<
            " account=" << to_string (id) <<
            " amount=" << amount.getFullText() <<
            " fullBalance=" << fullBalance.getFullText() <<
            " balance=" << balance.getFullText() <<
            " reserve=" << to_string (reserve) <<
            " ownerCount=" << to_string (ownerCount) <<
            " ownerCountAdj=" << to_string (ownerCountAdj);

        return amount.sfc();
    }
    else
    {
        // pre-switchover
        // SFC: return balance minus reserve
        std::uint32_t const ownerCount =
            confineOwnerCount (sle->getFieldU32 (sfOwnerCount), ownerCountAdj);
        auto const reserve =
            view.fees().accountReserve(sle->getFieldU32(sfOwnerCount));
        auto const balance = sle->getFieldAmount(sfBalance);

        STAmount amount = balance - reserve;
        if (balance < reserve)
            amount.clear ();

        JLOG (j.trace()) << "accountHolds:" <<
            " account=" << to_string (id) <<
            " amount=" << amount.getFullText() <<
            " balance=" << balance.getFullText() <<
            " reserve=" << to_string (reserve) <<
            " ownerCount=" << to_string (ownerCount) <<
            " ownerCountAdj=" << to_string (ownerCountAdj);

        return view.balanceHook(id, sfcAccount(), amount).sfc();
    }
}

void
forEachItem (ReadView const& view, AccountID const& id,
    std::function<void(std::shared_ptr<SLE const> const&)> f)
{
    auto const root = keylet::ownerDir(id);
    auto pos = root;
    for(;;)
    {
        auto sle = view.read(pos);
        if (! sle)
            return;
        // VFALCO NOTE We aren't checking field exists?
        for (auto const& key : sle->getFieldV256(sfIndexes))
            f(view.read(keylet::child(key)));
        auto const next =
            sle->getFieldU64 (sfIndexNext);
        if (! next)
            return;
        pos = keylet::page(root, next);
    }
}

bool
forEachItemAfter (ReadView const& view, AccountID const& id,
    uint256 const& after, std::uint64_t const hint,
        unsigned int limit, std::function<
            bool (std::shared_ptr<SLE const> const&)> f)
{
    auto const rootIndex = keylet::ownerDir(id);
    auto currentIndex = rootIndex;

    // If startAfter is not zero try jumping to that page using the hint
    if (after.isNonZero ())
    {
        auto const hintIndex = keylet::page(rootIndex, hint);
        auto hintDir = view.read(hintIndex);
        if (hintDir)
        {
            for (auto const& key : hintDir->getFieldV256 (sfIndexes))
            {
                if (key == after)
                {
                    // We found the hint, we can start here
                    currentIndex = hintIndex;
                    break;
                }
            }
        }

        bool found = false;
        for (;;)
        {
            auto const ownerDir = view.read(currentIndex);
            if (! ownerDir)
                return found;
            for (auto const& key : ownerDir->getFieldV256 (sfIndexes))
            {
                if (! found)
                {
                    if (key == after)
                        found = true;
                }
                else if (f (view.read(keylet::child(key))) && limit-- <= 1)
                {
                    return found;
                }
            }

            auto const uNodeNext =
                ownerDir->getFieldU64(sfIndexNext);
            if (uNodeNext == 0)
                return found;
            currentIndex = keylet::page(rootIndex, uNodeNext);
        }
    }
    else
    {
        for (;;)
        {
            auto const ownerDir = view.read(currentIndex);
            if (! ownerDir)
                return true;
            for (auto const& key : ownerDir->getFieldV256 (sfIndexes))
                if (f (view.read(keylet::child(key))) && limit-- <= 1)
                    return true;
            auto const uNodeNext =
                ownerDir->getFieldU64 (sfIndexNext);
            if (uNodeNext == 0)
                return true;
            currentIndex = keylet::page(rootIndex, uNodeNext);
        }
    }
}

Rate
transferRate (ReadView const& view,
    AccountID const& issuer)
{
    auto const sle = view.read(keylet::account(issuer));

    if (sle && sle->isFieldPresent (sfTransferRate))
        return Rate{ sle->getFieldU32 (sfTransferRate) };

    return parityRate;
}

bool
areCompatible (ReadView const& validLedger, ReadView const& testLedger,
    beast::Journal::Stream& s, const char* reason)
{
    bool ret = true;

    if (validLedger.info().seq < testLedger.info().seq)
    {
        // valid -> ... -> test
        auto hash = hashOfSeq (testLedger, validLedger.info().seq,
            beast::Journal {beast::Journal::getNullSink()});
        if (hash && (*hash != validLedger.info().hash))
        {
            JLOG(s) << reason << " incompatible with valid ledger";

            JLOG(s) << "Hash(VSeq): " << to_string (*hash);

            ret = false;
        }
    }
    else if (validLedger.info().seq > testLedger.info().seq)
    {
        // test -> ... -> valid
        auto hash = hashOfSeq (validLedger, testLedger.info().seq,
            beast::Journal {beast::Journal::getNullSink()});
        if (hash && (*hash != testLedger.info().hash))
        {
            JLOG(s) << reason << " incompatible preceding ledger";

            JLOG(s) << "Hash(NSeq): " << to_string (*hash);

            ret = false;
        }
    }
    else if ((validLedger.info().seq == testLedger.info().seq) &&
         (validLedger.info().hash != testLedger.info().hash))
    {
        // Same sequence number, different hash
        JLOG(s) << reason << " incompatible ledger";

        ret = false;
    }

    if (! ret)
    {
        JLOG(s) << "Val: " << validLedger.info().seq <<
            " " << to_string (validLedger.info().hash);

        JLOG(s) << "New: " << testLedger.info().seq <<
            " " << to_string (testLedger.info().hash);
    }

    return ret;
}

bool areCompatible (uint256 const& validHash, LedgerIndex validIndex,
    ReadView const& testLedger, beast::Journal::Stream& s, const char* reason)
{
    bool ret = true;

    if (testLedger.info().seq > validIndex)
    {
        // Ledger we are testing follows last valid ledger
        auto hash = hashOfSeq (testLedger, validIndex,
            beast::Journal {beast::Journal::getNullSink()});
        if (hash && (*hash != validHash))
        {
            JLOG(s) << reason << " incompatible following ledger";
            JLOG(s) << "Hash(VSeq): " << to_string (*hash);

            ret = false;
        }
    }
    else if ((validIndex == testLedger.info().seq) &&
        (testLedger.info().hash != validHash))
    {
        JLOG(s) << reason << " incompatible ledger";

        ret = false;
    }

    if (! ret)
    {
        JLOG(s) << "Val: " << validIndex <<
            " " << to_string (validHash);

        JLOG(s) << "New: " << testLedger.info().seq <<
            " " << to_string (testLedger.info().hash);
    }

    return ret;
}

bool
dirIsEmpty (ReadView const& view,
    Keylet const& k)
{
    auto const sleNode = view.read(k);
    if (! sleNode)
        return true;
    if (! sleNode->getFieldV256 (sfIndexes).empty ())
        return false;
    // If there's another page, it must be non-empty
    return sleNode->getFieldU64 (sfIndexNext) == 0;
}

bool
cdirFirst (ReadView const& view,
    uint256 const& uRootIndex,  // --> Root of directory.
    std::shared_ptr<SLE const>& sleNode,      // <-> current node
    unsigned int& uDirEntry,    // <-- next entry
    uint256& uEntryIndex,       // <-- The entry, if available. Otherwise, zero.
    beast::Journal j)
{
    sleNode = view.read(keylet::page(uRootIndex));
    uDirEntry   = 0;
    assert (sleNode);           // Never probe for directories.
    return cdirNext (view, uRootIndex, sleNode, uDirEntry, uEntryIndex, j);
}

bool
cdirNext (ReadView const& view,
    uint256 const& uRootIndex,  // --> Root of directory
    std::shared_ptr<SLE const>& sleNode,      // <-> current node
    unsigned int& uDirEntry,    // <-> next entry
    uint256& uEntryIndex,       // <-- The entry, if available. Otherwise, zero.
    beast::Journal j)
{
    auto const& svIndexes = sleNode->getFieldV256 (sfIndexes);
    assert (uDirEntry <= svIndexes.size ());
    if (uDirEntry >= svIndexes.size ())
    {
        auto const uNodeNext =
            sleNode->getFieldU64 (sfIndexNext);
        if (! uNodeNext)
        {
            uEntryIndex.zero ();
            return false;
        }
        auto const sleNext = view.read(
            keylet::page(uRootIndex, uNodeNext));
        uDirEntry = 0;
        if (!sleNext)
        {
            // This should never happen
            JLOG (j.fatal())
                    << "Corrupt directory: index:"
                    << uRootIndex << " next:" << uNodeNext;
            return false;
        }
        sleNode = sleNext;
        return cdirNext (view, uRootIndex,
            sleNode, uDirEntry, uEntryIndex, j);
    }
    uEntryIndex = svIndexes[uDirEntry++];
    JLOG (j.trace()) << "dirNext:" <<
        " uDirEntry=" << uDirEntry <<
        " uEntryIndex=" << uEntryIndex;
    return true;
}

std::set <uint256>
getEnabledAmendments (ReadView const& view)
{
    std::set<uint256> amendments;

    if (auto const sle = view.read(keylet::amendments()))
    {
        if (sle->isFieldPresent (sfAmendments))
        {
            auto const& v = sle->getFieldV256 (sfAmendments);
            amendments.insert (v.begin(), v.end());
        }
    }

    return amendments;
}

majorityAmendments_t
getMajorityAmendments (ReadView const& view)
{
    majorityAmendments_t ret;

    if (auto const sle = view.read(keylet::amendments()))
    {
        if (sle->isFieldPresent (sfMajorities))
        {
            using tp = NetClock::time_point;
            using d = tp::duration;

            auto const majorities = sle->getFieldArray (sfMajorities);

            for (auto const& m : majorities)
                ret[m.getFieldH256 (sfAmendment)] =
                    tp(d(m.getFieldU32(sfCloseTime)));
        }
    }

    return ret;
}

boost::optional<uint256>
hashOfSeq (ReadView const& ledger, LedgerIndex seq,
    beast::Journal journal)
{
    // Easy cases...
    if (seq > ledger.seq())
    {
        JLOG (journal.warn()) <<
            "Can't get seq " << seq <<
            " from " << ledger.seq() << " future";
        return boost::none;
    }
    if (seq == ledger.seq())
        return ledger.info().hash;
    if (seq == (ledger.seq() - 1))
        return ledger.info().parentHash;

    // Within 256...
    {
        int diff = ledger.seq() - seq;
        if (diff <= 256)
        {
            auto const hashIndex =
                ledger.read(keylet::skip());
            if (hashIndex)
            {
                assert (hashIndex->getFieldU32 (sfLastLedgerSequence) ==
                        (ledger.seq() - 1));
                STVector256 vec = hashIndex->getFieldV256 (sfHashes);
                if (vec.size () >= diff)
                    return vec[vec.size () - diff];
                JLOG (journal.warn()) <<
                    "Ledger " << ledger.seq() <<
                    " missing hash for " << seq <<
                    " (" << vec.size () << "," << diff << ")";
            }
            else
            {
                JLOG (journal.warn()) <<
                    "Ledger " << ledger.seq() <<
                    ":" << ledger.info().hash << " missing normal list";
            }
        }
        if ((seq & 0xff) != 0)
        {
            JLOG (journal.debug()) <<
                "Can't get seq " << seq <<
                " from " << ledger.seq() << " past";
            return boost::none;
        }
    }

    // in skiplist
    auto const hashIndex =
        ledger.read(keylet::skip(seq));
    if (hashIndex)
    {
        auto const lastSeq =
            hashIndex->getFieldU32 (sfLastLedgerSequence);
        assert (lastSeq >= seq);
        assert ((lastSeq & 0xff) == 0);
        auto const diff = (lastSeq - seq) >> 8;
        STVector256 vec = hashIndex->getFieldV256 (sfHashes);
        if (vec.size () > diff)
            return vec[vec.size () - diff - 1];
    }
    JLOG (journal.warn()) <<
        "Can't get seq " << seq <<
        " from " << ledger.seq() << " error";
    return boost::none;
}

//------------------------------------------------------------------------------
//
// Modifiers
//
//------------------------------------------------------------------------------

void
adjustOwnerCount (ApplyView& view,
    std::shared_ptr<SLE> const& sle,
        std::int32_t amount, beast::Journal j)
{
    assert(amount != 0);
    std::uint32_t const current {sle->getFieldU32 (sfOwnerCount)};
    AccountID const id = (*sle)[sfAccount];
    std::uint32_t const adjusted = confineOwnerCount (current, amount, id, j);
    view.adjustOwnerCountHook (id, current, adjusted);
    sle->setFieldU32 (sfOwnerCount, adjusted);
    view.update(sle);
}

bool
dirFirst (ApplyView& view,
    uint256 const& uRootIndex,  // --> Root of directory.
    std::shared_ptr<SLE>& sleNode,      // <-> current node
    unsigned int& uDirEntry,    // <-- next entry
    uint256& uEntryIndex,       // <-- The entry, if available. Otherwise, zero.
    beast::Journal j)
{
    sleNode = view.peek(keylet::page(uRootIndex));
    uDirEntry   = 0;
    assert (sleNode);           // Never probe for directories.
    return dirNext (view, uRootIndex, sleNode, uDirEntry, uEntryIndex, j);
}

bool
dirNext (ApplyView& view,
    uint256 const& uRootIndex,  // --> Root of directory
    std::shared_ptr<SLE>& sleNode,      // <-> current node
    unsigned int& uDirEntry,    // <-> next entry
    uint256& uEntryIndex,       // <-- The entry, if available. Otherwise, zero.
    beast::Journal j)
{
    auto const& svIndexes = sleNode->getFieldV256 (sfIndexes);
    assert (uDirEntry <= svIndexes.size ());
    if (uDirEntry >= svIndexes.size ())
    {
        auto const uNodeNext =
            sleNode->getFieldU64 (sfIndexNext);
        if (! uNodeNext)
        {
            uEntryIndex.zero ();
            return false;
        }
        auto const sleNext = view.peek(
            keylet::page(uRootIndex, uNodeNext));
        uDirEntry = 0;
        if (!sleNext)
        {
            // This should never happen
            JLOG (j.fatal())
                    << "Corrupt directory: index:"
                    << uRootIndex << " next:" << uNodeNext;
            return false;
        }
        sleNode = sleNext;
        return dirNext (view, uRootIndex,
            sleNode, uDirEntry, uEntryIndex, j);
    }
    uEntryIndex = svIndexes[uDirEntry++];
    JLOG (j.trace()) << "dirNext:" <<
        " uDirEntry=" << uDirEntry <<
        " uEntryIndex=" << uEntryIndex;
    return true;
}

std::function<void (SLE::ref)>
describeOwnerDir(AccountID const& account)
{
    return [&account](std::shared_ptr<SLE> const& sle)
    {
        (*sle)[sfOwner] = account;
    };
}

boost::optional<std::uint64_t>
dirAdd (ApplyView& view,
    Keylet const&                           dir,
    uint256 const&                          uLedgerIndex,
    bool                                    strictOrder,
    std::function<void (SLE::ref)>          fDescriber,
    beast::Journal j)
{
    if (view.rules().enabled(featureSortedDirectories))
    {
        if (strictOrder)
            return view.dirAppend(dir, uLedgerIndex, fDescriber);
        else
            return view.dirInsert(dir, uLedgerIndex, fDescriber);
    }

    JLOG (j.trace()) << "dirAdd:" <<
        " dir=" << to_string (dir.key) <<
        " uLedgerIndex=" << to_string (uLedgerIndex);

    auto sleRoot = view.peek(dir);
    std::uint64_t uNodeDir = 0;

    if (! sleRoot)
    {
        // No root, make it.
        sleRoot = std::make_shared<SLE>(dir);
        sleRoot->setFieldH256 (sfRootIndex, dir.key);
        view.insert (sleRoot);
        fDescriber (sleRoot);

        STVector256 v;
        v.push_back (uLedgerIndex);
        sleRoot->setFieldV256 (sfIndexes, v);

        JLOG (j.trace()) <<
            "dirAdd: created root " << to_string (dir.key) <<
            " for entry " << to_string (uLedgerIndex);

        return uNodeDir;
    }

    SLE::pointer sleNode;
    STVector256 svIndexes;

    uNodeDir = sleRoot->getFieldU64 (sfIndexPrevious);       // Get index to last directory node.

    if (uNodeDir)
    {
        // Try adding to last node.
        sleNode = view.peek (keylet::page(dir, uNodeDir));
        assert (sleNode);
    }
    else
    {
        // Try adding to root.  Didn't have a previous set to the last node.
        sleNode     = sleRoot;
    }

    svIndexes = sleNode->getFieldV256 (sfIndexes);

    if (dirNodeMaxEntries != svIndexes.size ())
    {
        // Add to current node.
        view.update(sleNode);
    }
    // Add to new node.
    else if (!++uNodeDir)
    {
        return boost::none;
    }
    else
    {
        // Have old last point to new node
        sleNode->setFieldU64 (sfIndexNext, uNodeDir);
        view.update(sleNode);

        // Have root point to new node.
        sleRoot->setFieldU64 (sfIndexPrevious, uNodeDir);
        view.update (sleRoot);

        // Create the new node.
        sleNode = std::make_shared<SLE>(
            keylet::page(dir, uNodeDir));
        sleNode->setFieldH256 (sfRootIndex, dir.key);
        view.insert (sleNode);

        if (uNodeDir != 1)
            sleNode->setFieldU64 (sfIndexPrevious, uNodeDir - 1);

        fDescriber (sleNode);

        svIndexes   = STVector256 ();
    }

    svIndexes.push_back (uLedgerIndex); // Append entry.
    sleNode->setFieldV256 (sfIndexes, svIndexes);   // Save entry.

    JLOG (j.trace()) <<
        "dirAdd:   creating: root: " << to_string (dir.key);
    JLOG (j.trace()) <<
        "dirAdd:  appending: Entry: " << to_string (uLedgerIndex);
    JLOG (j.trace()) <<
        "dirAdd:  appending: Node: " << strHex (uNodeDir);

    return uNodeDir;
}

TER
trustCreate (ApplyView& view,
    const bool      bSrcHigh,
    AccountID const&  uSrcAccountID,
    AccountID const&  uDstAccountID,
    uint256 const&  uIndex,             // --> SupplyFinanceChain state entry
    SLE::ref        sleAccount,         // --> the account being set.
    const bool      bAuth,              // --> authorize account.
    const bool      bNoSupplyFinanceChain,          // --> others cannot SupplyFinanceChain through
    const bool      bFreeze,            // --> funds cannot leave
    STAmount const& saBalance,          // --> balance of account being set.
                                        // Issuer should be noAccount()
    STAmount const& saLimit,            // --> limit for account being set.
                                        // Issuer should be the account being set.
    std::uint32_t uQualityIn,
    std::uint32_t uQualityOut,
    beast::Journal j)
{
    JLOG (j.trace())
        << "trustCreate: " << to_string (uSrcAccountID) << ", "
        << to_string (uDstAccountID) << ", " << saBalance.getFullText ();

    auto const& uLowAccountID   = !bSrcHigh ? uSrcAccountID : uDstAccountID;
    auto const& uHighAccountID  =  bSrcHigh ? uSrcAccountID : uDstAccountID;

    auto const sleSupplyFinanceChainState = std::make_shared<SLE>(
        ltSUPPLYFINANCECHAIN_STATE, uIndex);
    view.insert (sleSupplyFinanceChainState);

    auto lowNode = dirAdd (view, keylet::ownerDir (uLowAccountID),
        sleSupplyFinanceChainState->key(), false, describeOwnerDir (uLowAccountID), j);

    if (!lowNode)
        return tecDIR_FULL;

    auto highNode = dirAdd (view, keylet::ownerDir (uHighAccountID),
        sleSupplyFinanceChainState->key(), false, describeOwnerDir (uHighAccountID), j);

    if (!highNode)
        return tecDIR_FULL;

    const bool bSetDst = saLimit.getIssuer () == uDstAccountID;
    const bool bSetHigh = bSrcHigh ^ bSetDst;

    assert (sleAccount->getAccountID (sfAccount) ==
        (bSetHigh ? uHighAccountID : uLowAccountID));
    auto slePeer = view.peek (keylet::account(
        bSetHigh ? uLowAccountID : uHighAccountID));
    assert (slePeer);

    // Remember deletion hints.
    sleSupplyFinanceChainState->setFieldU64 (sfLowNode, *lowNode);
    sleSupplyFinanceChainState->setFieldU64 (sfHighNode, *highNode);

    sleSupplyFinanceChainState->setFieldAmount (
        bSetHigh ? sfHighLimit : sfLowLimit, saLimit);
    sleSupplyFinanceChainState->setFieldAmount (
        bSetHigh ? sfLowLimit : sfHighLimit,
        STAmount ({saBalance.getCurrency (),
                   bSetDst ? uSrcAccountID : uDstAccountID}));

    if (uQualityIn)
        sleSupplyFinanceChainState->setFieldU32 (
            bSetHigh ? sfHighQualityIn : sfLowQualityIn, uQualityIn);

    if (uQualityOut)
        sleSupplyFinanceChainState->setFieldU32 (
            bSetHigh ? sfHighQualityOut : sfLowQualityOut, uQualityOut);

    std::uint32_t uFlags = bSetHigh ? lsfHighReserve : lsfLowReserve;

    if (bAuth)
    {
        uFlags |= (bSetHigh ? lsfHighAuth : lsfLowAuth);
    }
    if (bNoSupplyFinanceChain)
    {
        uFlags |= (bSetHigh ? lsfHighNoSupplyFinanceChain : lsfLowNoSupplyFinanceChain);
    }
    if (bFreeze)
    {
        uFlags |= (!bSetHigh ? lsfLowFreeze : lsfHighFreeze);
    }

    if ((slePeer->getFlags() & lsfDefaultSupplyFinanceChain) == 0)
    {
        // The other side's default is no rippling
        uFlags |= (bSetHigh ? lsfLowNoSupplyFinanceChain : lsfHighNoSupplyFinanceChain);
    }

    sleSupplyFinanceChainState->setFieldU32 (sfFlags, uFlags);
    adjustOwnerCount(view, sleAccount, 1, j);

    // ONLY: Create SupplyFinanceChain balance.
    sleSupplyFinanceChainState->setFieldAmount (sfBalance, bSetHigh ? -saBalance : saBalance);

    view.creditHook (uSrcAccountID,
        uDstAccountID, saBalance, saBalance.zeroed());

    return tesSUCCESS;
}

TER
trustDelete (ApplyView& view,
    std::shared_ptr<SLE> const& sleSupplyFinanceChainState,
        AccountID const& uLowAccountID,
            AccountID const& uHighAccountID,
                 beast::Journal j)
{
    // Detect legacy dirs.
    std::uint64_t uLowNode    = sleSupplyFinanceChainState->getFieldU64 (sfLowNode);
    std::uint64_t uHighNode   = sleSupplyFinanceChainState->getFieldU64 (sfHighNode);

    JLOG (j.trace())
        << "trustDelete: Deleting SupplyFinanceChain line: low";

    if (! view.dirRemove(
            keylet::ownerDir(uLowAccountID),
            uLowNode,
            sleSupplyFinanceChainState->key(),
            false))
    {
        return tefBAD_LEDGER;
    }

    JLOG (j.trace())
            << "trustDelete: Deleting SupplyFinanceChain line: high";

    if (! view.dirRemove(
            keylet::ownerDir(uHighAccountID),
            uHighNode,
            sleSupplyFinanceChainState->key(),
            false))
    {
        return tefBAD_LEDGER;
    }

    JLOG (j.trace()) << "trustDelete: Deleting SupplyFinanceChain line: state";
    view.erase(sleSupplyFinanceChainState);

    return tesSUCCESS;
}

TER
offerDelete (ApplyView& view,
    std::shared_ptr<SLE> const& sle,
    beast::Journal j)
{
    if (! sle)
        return tesSUCCESS;
    auto offerIndex = sle->key();
    auto owner = sle->getAccountID  (sfAccount);

    // Detect legacy directories.
    uint256 uDirectory = sle->getFieldH256 (sfBookDirectory);

    if (! view.dirRemove(
        keylet::ownerDir(owner),
        sle->getFieldU64(sfOwnerNode),
        offerIndex,
        false))
    {
        return tefBAD_LEDGER;
    }

    if (! view.dirRemove(
        keylet::page(uDirectory),
        sle->getFieldU64(sfBookNode),
        offerIndex,
        false))
    {
        return tefBAD_LEDGER;
    }

    adjustOwnerCount(view, view.peek(
        keylet::account(owner)), -1, j);

    view.erase(sle);

    return tesSUCCESS;
}

// Direct send w/o fees:
// - Redeeming IOUs and/or sending sender's own IOUs.
// - Create trust line if needed.
// --> bCheckIssuer : normally require issuer to be involved.
TER
SupplyFinanceChainCredit (ApplyView& view,
    AccountID const& uSenderID, AccountID const& uReceiverID,
    STAmount const& saAmount, bool bCheckIssuer,
    beast::Journal j)
{
    auto issuer = saAmount.getIssuer ();
    auto currency = saAmount.getCurrency ();

    // Make sure issuer is involved.
    assert (
        !bCheckIssuer || uSenderID == issuer || uReceiverID == issuer);
    (void) issuer;

    // Disallow sending to self.
    assert (uSenderID != uReceiverID);

    bool bSenderHigh = uSenderID > uReceiverID;
    uint256 uIndex = getSupplyFinanceChainStateIndex (
        uSenderID, uReceiverID, saAmount.getCurrency ());
    auto sleSupplyFinanceChainState  = view.peek (keylet::line(uIndex));

    TER terResult;

    assert (!isSFC (uSenderID) && uSenderID != noAccount());
    assert (!isSFC (uReceiverID) && uReceiverID != noAccount());

    if (!sleSupplyFinanceChainState)
    {
        STAmount saReceiverLimit({currency, uReceiverID});
        STAmount saBalance = saAmount;

        saBalance.setIssuer (noAccount());

        JLOG (j.debug()) << "SupplyFinanceChainCredit: "
            "create line: " << to_string (uSenderID) <<
            " -> " << to_string (uReceiverID) <<
            " : " << saAmount.getFullText ();

        auto const sleAccount =
            view.peek(keylet::account(uReceiverID));

        bool noSupplyFinanceChain = (sleAccount->getFlags() & lsfDefaultSupplyFinanceChain) == 0;

        terResult = trustCreate (view,
            bSenderHigh,
            uSenderID,
            uReceiverID,
            uIndex,
            sleAccount,
            false,
            noSupplyFinanceChain,
            false,
            saBalance,
            saReceiverLimit,
            0,
            0,
            j);
    }
    else
    {
        STAmount saBalance   = sleSupplyFinanceChainState->getFieldAmount (sfBalance);

        if (bSenderHigh)
            saBalance.negate ();    // Put balance in sender terms.

        view.creditHook (uSenderID,
            uReceiverID, saAmount, saBalance);

        STAmount    saBefore    = saBalance;

        saBalance   -= saAmount;

        JLOG (j.trace()) << "SupplyFinanceChainCredit: " <<
            to_string (uSenderID) <<
            " -> " << to_string (uReceiverID) <<
            " : before=" << saBefore.getFullText () <<
            " amount=" << saAmount.getFullText () <<
            " after=" << saBalance.getFullText ();

        std::uint32_t const uFlags (sleSupplyFinanceChainState->getFieldU32 (sfFlags));
        bool bDelete = false;

        // YYY Could skip this if rippling in reverse.
        if (saBefore > beast::zero
            // Sender balance was positive.
            && saBalance <= beast::zero
            // Sender is zero or negative.
            && (uFlags & (!bSenderHigh ? lsfLowReserve : lsfHighReserve))
            // Sender reserve is set.
            && static_cast <bool> (uFlags & (!bSenderHigh ? lsfLowNoSupplyFinanceChain : lsfHighNoSupplyFinanceChain)) !=
               static_cast <bool> (view.read (keylet::account(uSenderID))->getFlags() & lsfDefaultSupplyFinanceChain)
            && !(uFlags & (!bSenderHigh ? lsfLowFreeze : lsfHighFreeze))
            && !sleSupplyFinanceChainState->getFieldAmount (
                !bSenderHigh ? sfLowLimit : sfHighLimit)
            // Sender trust limit is 0.
            && !sleSupplyFinanceChainState->getFieldU32 (
                !bSenderHigh ? sfLowQualityIn : sfHighQualityIn)
            // Sender quality in is 0.
            && !sleSupplyFinanceChainState->getFieldU32 (
                !bSenderHigh ? sfLowQualityOut : sfHighQualityOut))
            // Sender quality out is 0.
        {
            // Clear the reserve of the sender, possibly delete the line!
            adjustOwnerCount(view,
                view.peek(keylet::account(uSenderID)), -1, j);

            // Clear reserve flag.
            sleSupplyFinanceChainState->setFieldU32 (
                sfFlags,
                uFlags & (!bSenderHigh ? ~lsfLowReserve : ~lsfHighReserve));

            // Balance is zero, receiver reserve is clear.
            bDelete = !saBalance        // Balance is zero.
                && !(uFlags & (bSenderHigh ? lsfLowReserve : lsfHighReserve));
            // Receiver reserve is clear.
        }

        if (bSenderHigh)
            saBalance.negate ();

        // Want to reflect balance to zero even if we are deleting line.
        sleSupplyFinanceChainState->setFieldAmount (sfBalance, saBalance);
        // ONLY: Adjust SupplyFinanceChain balance.

        if (bDelete)
        {
            terResult   = trustDelete (view,
                sleSupplyFinanceChainState,
                bSenderHigh ? uReceiverID : uSenderID,
                !bSenderHigh ? uReceiverID : uSenderID, j);
        }
        else
        {
            view.update (sleSupplyFinanceChainState);
            terResult   = tesSUCCESS;
        }
    }

    return terResult;
}

// Calculate the fee needed to transfer IOU assets between two parties.
static
STAmount
SupplyFinanceChainTransferFee (ReadView const& view,
    AccountID const& from,
    AccountID const& to,
    AccountID const& issuer,
    STAmount const& amount,
    beast::Journal j)
{
    if (from != issuer && to != issuer)
    {
        Rate const rate = transferRate (view, issuer);

        if (parityRate != rate)
        {
            auto const fee = multiply (amount, rate) - amount;

            JLOG (j.debug()) << "SupplyFinanceChainTransferFee:" <<
                " amount=" << amount.getFullText () <<
                " fee=" << fee.getFullText ();

            return fee;
        }
    }

    return amount.zeroed();
}

// Send regardless of limits.
// --> saAmount: Amount/currency/issuer to deliver to reciever.
// <-- saActual: Amount actually cost.  Sender pays fees.
static
TER
SupplyFinanceChainSend (ApplyView& view,
    AccountID const& uSenderID, AccountID const& uReceiverID,
    STAmount const& saAmount, STAmount& saActual, beast::Journal j)
{
    auto const issuer   = saAmount.getIssuer ();

    assert (!isSFC (uSenderID) && !isSFC (uReceiverID));
    assert (uSenderID != uReceiverID);

    if (uSenderID == issuer || uReceiverID == issuer || issuer == noAccount())
    {
        // Direct send: redeeming IOUs and/or sending own IOUs.
        SupplyFinanceChainCredit (view, uSenderID, uReceiverID, saAmount, false, j);
        saActual = saAmount;
        return tesSUCCESS;
    }

    // Sending 3rd party IOUs: transit.

    // Calculate the amount to transfer accounting
    // for any transfer fees:
    if (!fix1141 (view.info ().parentCloseTime))
    {
        STAmount const saTransitFee = SupplyFinanceChainTransferFee (
            view, uSenderID, uReceiverID, issuer, saAmount, j);

        saActual = !saTransitFee ? saAmount : saAmount + saTransitFee;
        saActual.setIssuer (issuer);  // XXX Make sure this done in + above.
    }
    else
    {
        saActual = multiply (saAmount,
            transferRate (view, issuer));
    }

    JLOG (j.debug()) << "SupplyFinanceChainSend> " <<
        to_string (uSenderID) <<
        " - > " << to_string (uReceiverID) <<
        " : deliver=" << saAmount.getFullText () <<
        " cost=" << saActual.getFullText ();

    TER terResult   = SupplyFinanceChainCredit (view, issuer, uReceiverID, saAmount, true, j);

    if (tesSUCCESS == terResult)
        terResult = SupplyFinanceChainCredit (view, uSenderID, issuer, saActual, true, j);

    return terResult;
}

TER
accountSend (ApplyView& view,
    AccountID const& uSenderID, AccountID const& uReceiverID,
    STAmount const& saAmount, beast::Journal j)
{
    assert (saAmount >= beast::zero);

    /* If we aren't sending anything or if the sender is the same as the
     * receiver then we don't need to do anything.
     */
    if (!saAmount || (uSenderID == uReceiverID))
        return tesSUCCESS;

    if (!saAmount.native ())
    {
        STAmount saActual;

        JLOG (j.trace()) << "accountSend: " <<
            to_string (uSenderID) << " -> " << to_string (uReceiverID) <<
            " : " << saAmount.getFullText ();

        return SupplyFinanceChainSend (view, uSenderID, uReceiverID, saAmount, saActual, j);
    }

    auto const fv2Switch = fix1141 (view.info ().parentCloseTime);
    if (!fv2Switch)
    {
        auto const dummyBalance = saAmount.zeroed();
        view.creditHook (uSenderID, uReceiverID, saAmount, dummyBalance);
    }

    /* SFC send which does not check reserve and can do pure adjustment.
     * Note that sender or receiver may be null and this not a mistake; this
     * setup is used during pathfinding and it is carefully controlled to
     * ensure that transfers are balanced.
     */

    TER terResult (tesSUCCESS);

    SLE::pointer sender = uSenderID != beast::zero
        ? view.peek (keylet::account(uSenderID))
        : SLE::pointer ();
    SLE::pointer receiver = uReceiverID != beast::zero
        ? view.peek (keylet::account(uReceiverID))
        : SLE::pointer ();

    if (auto stream = j.trace())
    {
        std::string sender_bal ("-");
        std::string receiver_bal ("-");

        if (sender)
            sender_bal = sender->getFieldAmount (sfBalance).getFullText ();

        if (receiver)
            receiver_bal = receiver->getFieldAmount (sfBalance).getFullText ();

       stream << "accountSend> " <<
            to_string (uSenderID) << " (" << sender_bal <<
            ") -> " << to_string (uReceiverID) << " (" << receiver_bal <<
            ") : " << saAmount.getFullText ();
    }

    if (sender)
    {
        if (sender->getFieldAmount (sfBalance) < saAmount)
        {
            // VFALCO Its laborious to have to mutate the
            //        TER based on params everywhere
            terResult = view.open()
                ? TER {telFAILED_PROCESSING}
                : TER {tecFAILED_PROCESSING};
        }
        else
        {
            auto const sndBal = sender->getFieldAmount (sfBalance);
            if (fv2Switch)
                view.creditHook (uSenderID, sfcAccount (), saAmount, sndBal);

            // Decrement SFC balance.
            sender->setFieldAmount (sfBalance, sndBal - saAmount);
            view.update (sender);
        }
    }

    if (tesSUCCESS == terResult && receiver)
    {
        // Increment SFC balance.
        auto const rcvBal = receiver->getFieldAmount (sfBalance);
        receiver->setFieldAmount (sfBalance, rcvBal + saAmount);

        if (fv2Switch)
            view.creditHook (sfcAccount (), uReceiverID, saAmount, -rcvBal);

        view.update (receiver);
    }

    if (auto stream = j.trace())
    {
        std::string sender_bal ("-");
        std::string receiver_bal ("-");

        if (sender)
            sender_bal = sender->getFieldAmount (sfBalance).getFullText ();

        if (receiver)
            receiver_bal = receiver->getFieldAmount (sfBalance).getFullText ();

        stream << "accountSend< " <<
            to_string (uSenderID) << " (" << sender_bal <<
            ") -> " << to_string (uReceiverID) << " (" << receiver_bal <<
            ") : " << saAmount.getFullText ();
    }

    return terResult;
}

static
bool
updateTrustLine (
    ApplyView& view,
    SLE::pointer state,
    bool bSenderHigh,
    AccountID const& sender,
    STAmount const& before,
    STAmount const& after,
    beast::Journal j)
{
    std::uint32_t const flags (state->getFieldU32 (sfFlags));

    auto sle = view.peek (keylet::account(sender));
    assert (sle);

    // YYY Could skip this if rippling in reverse.
    if (before > beast::zero
        // Sender balance was positive.
        && after <= beast::zero
        // Sender is zero or negative.
        && (flags & (!bSenderHigh ? lsfLowReserve : lsfHighReserve))
        // Sender reserve is set.
        && static_cast <bool> (flags & (!bSenderHigh ? lsfLowNoSupplyFinanceChain : lsfHighNoSupplyFinanceChain)) !=
           static_cast <bool> (sle->getFlags() & lsfDefaultSupplyFinanceChain)
        && !(flags & (!bSenderHigh ? lsfLowFreeze : lsfHighFreeze))
        && !state->getFieldAmount (
            !bSenderHigh ? sfLowLimit : sfHighLimit)
        // Sender trust limit is 0.
        && !state->getFieldU32 (
            !bSenderHigh ? sfLowQualityIn : sfHighQualityIn)
        // Sender quality in is 0.
        && !state->getFieldU32 (
            !bSenderHigh ? sfLowQualityOut : sfHighQualityOut))
        // Sender quality out is 0.
    {
        // VFALCO Where is the line being deleted?
        // Clear the reserve of the sender, possibly delete the line!
        adjustOwnerCount(view, sle, -1, j);

        // Clear reserve flag.
        state->setFieldU32 (sfFlags,
            flags & (!bSenderHigh ? ~lsfLowReserve : ~lsfHighReserve));

        // Balance is zero, receiver reserve is clear.
        if (!after        // Balance is zero.
            && !(flags & (bSenderHigh ? lsfLowReserve : lsfHighReserve)))
            return true;
    }

    return false;
}

TER
issueIOU (ApplyView& view,
    AccountID const& account,
        STAmount const& amount, Issue const& issue, beast::Journal j)
{
    assert (!isSFC (account) && !isSFC (issue.account));

    // Consistency check
    assert (issue == amount.issue ());

    // Can't send to self!
    assert (issue.account != account);

    JLOG (j.trace()) << "issueIOU: " <<
        to_string (account) << ": " <<
        amount.getFullText ();

    bool bSenderHigh = issue.account > account;
    uint256 const index = getSupplyFinanceChainStateIndex (
        issue.account, account, issue.currency);
    auto state = view.peek (keylet::line(index));

    if (!state)
    {
        // NIKB TODO: The limit uses the receiver's account as the issuer and
        // this is unnecessarily inefficient as copying which could be avoided
        // is now required. Consider available options.
        STAmount limit({issue.currency, account});
        STAmount final_balance = amount;

        final_balance.setIssuer (noAccount());

        auto receiverAccount = view.peek (keylet::account(account));

        bool noSupplyFinanceChain = (receiverAccount->getFlags() & lsfDefaultSupplyFinanceChain) == 0;

        return trustCreate (view, bSenderHigh, issue.account, account, index,
            receiverAccount, false, noSupplyFinanceChain, false, final_balance, limit, 0, 0, j);
    }

    STAmount final_balance = state->getFieldAmount (sfBalance);

    if (bSenderHigh)
        final_balance.negate ();    // Put balance in sender terms.

    STAmount const start_balance = final_balance;

    final_balance -= amount;

    auto const must_delete = updateTrustLine(view, state, bSenderHigh, issue.account,
        start_balance, final_balance, j);

    view.creditHook (issue.account, account, amount, start_balance);

    if (bSenderHigh)
        final_balance.negate ();

    // Adjust the balance on the trust line if necessary. We do this even if we
    // are going to delete the line to reflect the correct balance at the time
    // of deletion.
    state->setFieldAmount (sfBalance, final_balance);
    if (must_delete)
        return trustDelete (view, state,
            bSenderHigh ? account : issue.account,
            bSenderHigh ? issue.account : account, j);

    view.update (state);

    return tesSUCCESS;
}

TER
redeemIOU (ApplyView& view,
    AccountID const& account,
    STAmount const& amount,
    Issue const& issue,
    beast::Journal j)
{
    assert (!isSFC (account) && !isSFC (issue.account));

    // Consistency check
    assert (issue == amount.issue ());

    // Can't send to self!
    assert (issue.account != account);

    JLOG (j.trace()) << "redeemIOU: " <<
        to_string (account) << ": " <<
        amount.getFullText ();

    bool bSenderHigh = account > issue.account;
    uint256 const index = getSupplyFinanceChainStateIndex (
        account, issue.account, issue.currency);
    auto state  = view.peek (keylet::line(index));

    if (!state)
    {
        // In order to hold an IOU, a trust line *MUST* exist to track the
        // balance. If it doesn't, then something is very wrong. Don't try
        // to continue.
        JLOG (j.fatal()) << "redeemIOU: " <<
            to_string (account) << " attempts to redeem " <<
            amount.getFullText () << " but no trust line exists!";

        return tefINTERNAL;
    }

    STAmount final_balance = state->getFieldAmount (sfBalance);

    if (bSenderHigh)
        final_balance.negate ();    // Put balance in sender terms.

    STAmount const start_balance = final_balance;

    final_balance -= amount;

    auto const must_delete = updateTrustLine (view, state, bSenderHigh, account,
        start_balance, final_balance, j);

    view.creditHook (account, issue.account, amount, start_balance);

    if (bSenderHigh)
        final_balance.negate ();


    // Adjust the balance on the trust line if necessary. We do this even if we
    // are going to delete the line to reflect the correct balance at the time
    // of deletion.
    state->setFieldAmount (sfBalance, final_balance);

    if (must_delete)
    {
        return trustDelete (view, state,
            bSenderHigh ? issue.account : account,
            bSenderHigh ? account : issue.account, j);
    }

    view.update (state);
    return tesSUCCESS;
}

TER
transferSFC (ApplyView& view,
    AccountID const& from,
    AccountID const& to,
    STAmount const& amount,
    beast::Journal j)
{
    assert (from != beast::zero);
    assert (to != beast::zero);
    assert (from != to);
    assert (amount.native ());

    SLE::pointer sender = view.peek (keylet::account(from));
    SLE::pointer receiver = view.peek (keylet::account(to));

    JLOG (j.trace()) << "transferSFC: " <<
        to_string (from) <<  " -> " << to_string (to) <<
        ") : " << amount.getFullText ();

    if (sender->getFieldAmount (sfBalance) < amount)
    {
        // VFALCO Its unfortunate we have to keep
        //        mutating these TER everywhere
        // FIXME: this logic should be moved to callers maybe?
        return view.open()
            ? TER {telFAILED_PROCESSING}
            : TER {tecFAILED_PROCESSING};
    }

    // Decrement SFC balance.
    sender->setFieldAmount (sfBalance,
        sender->getFieldAmount (sfBalance) - amount);
    view.update (sender);

    receiver->setFieldAmount (sfBalance,
        receiver->getFieldAmount (sfBalance) + amount);
    view.update (receiver);

    return tesSUCCESS;
}

} // SupplyFinanceChain
