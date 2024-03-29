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

#include <SupplyFinanceChain/app/consensus/RCLConsensus.h>
#include <SupplyFinanceChain/app/consensus/RCLValidations.h>
#include <SupplyFinanceChain/app/ledger/BuildLedger.h>
#include <SupplyFinanceChain/app/ledger/InboundLedgers.h>
#include <SupplyFinanceChain/app/ledger/InboundTransactions.h>
#include <SupplyFinanceChain/app/ledger/LedgerMaster.h>
#include <SupplyFinanceChain/app/ledger/LocalTxs.h>
#include <SupplyFinanceChain/app/ledger/OpenLedger.h>
#include <SupplyFinanceChain/app/misc/AmendmentTable.h>
#include <SupplyFinanceChain/app/misc/HashRouter.h>
#include <SupplyFinanceChain/app/misc/LoadFeeTrack.h>
#include <SupplyFinanceChain/app/misc/NetworkOPs.h>
#include <SupplyFinanceChain/app/misc/TxQ.h>
#include <SupplyFinanceChain/app/misc/ValidatorKeys.h>
#include <SupplyFinanceChain/app/misc/ValidatorList.h>
#include <SupplyFinanceChain/basics/make_lock.h>
#include <SupplyFinanceChain/beast/core/LexicalCast.h>
#include <SupplyFinanceChain/consensus/LedgerTiming.h>
#include <SupplyFinanceChain/nodestore/DatabaseShard.h>
#include <SupplyFinanceChain/overlay/Overlay.h>
#include <SupplyFinanceChain/overlay/predicates.h>
#include <SupplyFinanceChain/protocol/Feature.h>
#include <SupplyFinanceChain/protocol/digest.h>
#include <algorithm>

namespace SupplyFinanceChain {

RCLConsensus::RCLConsensus(
    Application& app,
    std::unique_ptr<FeeVote>&& feeVote,
    LedgerMaster& ledgerMaster,
    LocalTxs& localTxs,
    InboundTransactions& inboundTransactions,
    Consensus<Adaptor>::clock_type const& clock,
    ValidatorKeys const& validatorKeys,
    beast::Journal journal)
    : adaptor_(
          app,
          std::move(feeVote),
          ledgerMaster,
          localTxs,
          inboundTransactions,
          validatorKeys,
          journal)
    , consensus_(clock, adaptor_, journal)
    , j_(journal)

{
}

RCLConsensus::Adaptor::Adaptor(
    Application& app,
    std::unique_ptr<FeeVote>&& feeVote,
    LedgerMaster& ledgerMaster,
    LocalTxs& localTxs,
    InboundTransactions& inboundTransactions,
    ValidatorKeys const& validatorKeys,
    beast::Journal journal)
    : app_(app)
        , feeVote_(std::move(feeVote))
        , ledgerMaster_(ledgerMaster)
        , localTxs_(localTxs)
        , inboundTransactions_{inboundTransactions}
        , j_(journal)
        , nodeID_{validatorKeys.nodeID}
        , valPublic_{validatorKeys.publicKey}
        , valSecret_{validatorKeys.secretKey}
{
}

boost::optional<RCLCxLedger>
RCLConsensus::Adaptor::acquireLedger(LedgerHash const& hash)
{
    // we need to switch the ledger we're working from
    auto built = ledgerMaster_.getLedgerByHash(hash);
    if (!built)
    {
        if (acquiringLedger_ != hash)
        {
            // need to start acquiring the correct consensus LCL
            JLOG(j_.warn()) << "Need consensus ledger " << hash;

            // Tell the ledger acquire system that we need the consensus ledger
            acquiringLedger_ = hash;

            app_.getJobQueue().addJob(jtADVANCE, "getConsensusLedger",
                [id = hash, &app = app_](Job&)
                {
                    app.getInboundLedgers().acquire(id, 0,
                        InboundLedger::Reason::CONSENSUS);
                });
        }
        return boost::none;
    }

    assert(!built->open() && built->isImmutable());
    assert(built->info().hash == hash);

    // Notify inbound transactions of the new ledger sequence number
    inboundTransactions_.newRound(built->info().seq);

    // Use the ledger timing rules of the acquired ledger
    parms_.useRoundedCloseTime = built->rules().enabled(fix1528);

    return RCLCxLedger(built);
}

void
RCLConsensus::Adaptor::share(RCLCxPeerPos const& peerPos)
{
    protocol::TMProposeSet prop;

    auto const& proposal = peerPos.proposal();

    prop.set_proposeseq(proposal.proposeSeq());
    prop.set_closetime(proposal.closeTime().time_since_epoch().count());

    prop.set_currenttxhash(
        proposal.position().begin(), proposal.position().size());
    prop.set_previousledger(
        proposal.prevLedger().begin(), proposal.position().size());

    auto const pk = peerPos.publicKey().slice();
    prop.set_nodepubkey(pk.data(), pk.size());

    auto const sig = peerPos.signature();
    prop.set_signature(sig.data(), sig.size());

    app_.overlay().relay(prop, peerPos.suppressionID());
}

void
RCLConsensus::Adaptor::share(RCLCxTx const& tx)
{
    // If we didn't relay this transaction recently, relay it to all peers
    if (app_.getHashRouter().shouldRelay(tx.id()))
    {
        JLOG(j_.debug()) << "Relaying disputed tx " << tx.id();
        auto const slice = tx.tx_.slice();
        protocol::TMTransaction msg;
        msg.set_rawtransaction(slice.data(), slice.size());
        msg.set_status(protocol::tsNEW);
        msg.set_receivetimestamp(
            app_.timeKeeper().now().time_since_epoch().count());
        app_.overlay().foreach (send_always(
            std::make_shared<Message>(msg, protocol::mtTRANSACTION)));
    }
    else
    {
        JLOG(j_.debug()) << "Not relaying disputed tx " << tx.id();
    }
}
void
RCLConsensus::Adaptor::propose(RCLCxPeerPos::Proposal const& proposal)
{
    JLOG(j_.trace()) << "We propose: "
                     << (proposal.isBowOut()
                             ? std::string("bowOut")
                             : SupplyFinanceChain::to_string(proposal.position()));

    protocol::TMProposeSet prop;

    prop.set_currenttxhash(
        proposal.position().begin(), proposal.position().size());
    prop.set_previousledger(
        proposal.prevLedger().begin(), proposal.position().size());
    prop.set_proposeseq(proposal.proposeSeq());
    prop.set_closetime(proposal.closeTime().time_since_epoch().count());

    prop.set_nodepubkey(valPublic_.data(), valPublic_.size());

    auto signingHash = sha512Half(
        HashPrefix::proposal,
        std::uint32_t(proposal.proposeSeq()),
        proposal.closeTime().time_since_epoch().count(),
        proposal.prevLedger(),
        proposal.position());

    auto sig = signDigest(valPublic_, valSecret_, signingHash);

    prop.set_signature(sig.data(), sig.size());

    auto const suppression = proposalUniqueId(
        proposal.position(),
        proposal.prevLedger(),
        proposal.proposeSeq(),
        proposal.closeTime(),
        valPublic_,
        sig);

    app_.getHashRouter ().addSuppression (suppression);

    app_.overlay().send(prop);
}

void
RCLConsensus::Adaptor::share(RCLTxSet const& txns)
{
    inboundTransactions_.giveSet(txns.id(), txns.map_, false);
}

boost::optional<RCLTxSet>
RCLConsensus::Adaptor::acquireTxSet(RCLTxSet::ID const& setId)
{
    if (auto txns = inboundTransactions_.getSet(setId, true))
    {
        return RCLTxSet{std::move(txns)};
    }
    return boost::none;
}

bool
RCLConsensus::Adaptor::hasOpenTransactions() const
{
    return !app_.openLedger().empty();
}

std::size_t
RCLConsensus::Adaptor::proposersValidated(LedgerHash const& h) const
{
    return app_.getValidations().numTrustedForLedger(h);
}

std::size_t
RCLConsensus::Adaptor::proposersFinished(
    RCLCxLedger const& ledger,
    LedgerHash const& h) const
{
    RCLValidations& vals = app_.getValidations();
    return vals.getNodesAfter(
        RCLValidatedLedger(ledger.ledger_, vals.adaptor().journal()), h);
}

uint256
RCLConsensus::Adaptor::getPrevLedger(
    uint256 ledgerID,
    RCLCxLedger const& ledger,
    ConsensusMode mode)
{
    RCLValidations& vals = app_.getValidations();
    uint256 netLgr = vals.getPreferred(
        RCLValidatedLedger{ledger.ledger_, vals.adaptor().journal()},
        ledgerMaster_.getValidLedgerIndex());

    if (netLgr != ledgerID)
    {
        if (mode != ConsensusMode::wrongLedger)
            app_.getOPs().consensusViewChange();

        JLOG(j_.debug())<< Json::Compact(app_.getValidations().getJsonTrie());
    }

    return netLgr;
}

auto
RCLConsensus::Adaptor::onClose(
    RCLCxLedger const& ledger,
    NetClock::time_point const& closeTime,
    ConsensusMode mode) -> Result
{
    const bool wrongLCL = mode == ConsensusMode::wrongLedger;
    const bool proposing = mode == ConsensusMode::proposing;

    notify(protocol::neCLOSING_LEDGER, ledger, !wrongLCL);

    auto const& prevLedger = ledger.ledger_;

    ledgerMaster_.applyHeldTransactions();
    // Tell the ledger master not to acquire the ledger we're probably building
    ledgerMaster_.setBuildingLedger(prevLedger->info().seq + 1);

    auto initialLedger = app_.openLedger().current();

    auto initialSet = std::make_shared<SHAMap>(
        SHAMapType::TRANSACTION, app_.family(), SHAMap::version{1});
    initialSet->setUnbacked();

    // Build SHAMap containing all transactions in our open ledger
    for (auto const& tx : initialLedger->txs)
    {
        JLOG(j_.trace()) << "Adding open ledger TX " <<
            tx.first->getTransactionID();
        Serializer s(2048);
        tx.first->add(s);
        initialSet->addItem(
            SHAMapItem(tx.first->getTransactionID(), std::move(s)),
            true,
            false);
    }

    // Add pseudo-transactions to the set
    if ((app_.config().standalone() || (proposing && !wrongLCL)) &&
        ((prevLedger->info().seq % 256) == 0))
    {
        // previous ledger was flag ledger, add pseudo-transactions
        auto const validations =
            app_.getValidations().getTrustedForLedger (
                prevLedger->info().parentHash);

        if (validations.size() >= app_.validators ().quorum ())
        {
            feeVote_->doVoting(prevLedger, validations, initialSet);
            app_.getAmendmentTable().doVoting(
                prevLedger, validations, initialSet);
        }
    }

    // Now we need an immutable snapshot
    initialSet = initialSet->snapShot(false);

    if (!wrongLCL)
    {
        LedgerIndex const seq = prevLedger->info().seq + 1;
        RCLCensorshipDetector<TxID, LedgerIndex>::TxIDSeqVec proposed;

        initialSet->visitLeaves(
            [&proposed, seq](std::shared_ptr<SHAMapItem const> const& item)
            {
                proposed.emplace_back(item->key(), seq);
            });

        censorshipDetector_.propose(std::move(proposed));
    }

    // Needed because of the move below.
    auto const setHash = initialSet->getHash().as_uint256();

    return Result{
        std::move(initialSet),
        RCLCxPeerPos::Proposal{
            initialLedger->info().parentHash,
            RCLCxPeerPos::Proposal::seqJoin,
            setHash,
            closeTime,
            app_.timeKeeper().closeTime(),
            nodeID_}};
}

void
RCLConsensus::Adaptor::onForceAccept(
    Result const& result,
    RCLCxLedger const& prevLedger,
    NetClock::duration const& closeResolution,
    ConsensusCloseTimes const& rawCloseTimes,
    ConsensusMode const& mode,
    Json::Value && consensusJson)
{
    doAccept(
        result,
        prevLedger,
        closeResolution,
        rawCloseTimes,
        mode,
        std::move(consensusJson));
}

void
RCLConsensus::Adaptor::onAccept(
    Result const& result,
    RCLCxLedger const& prevLedger,
    NetClock::duration const& closeResolution,
    ConsensusCloseTimes const& rawCloseTimes,
    ConsensusMode const& mode,
    Json::Value && consensusJson)
{
    app_.getJobQueue().addJob(
        jtACCEPT,
        "acceptLedger",
        [=, cj = std::move(consensusJson) ](auto&) mutable {
            // Note that no lock is held or acquired during this job.
            // This is because generic Consensus guarantees that once a ledger
            // is accepted, the consensus results and capture by reference state
            // will not change until startRound is called (which happens via
            // endConsensus).
            this->doAccept(
                result,
                prevLedger,
                closeResolution,
                rawCloseTimes,
                mode,
                std::move(cj));
            this->app_.getOPs().endConsensus();
        });
}

void
RCLConsensus::Adaptor::doAccept(
    Result const& result,
    RCLCxLedger const& prevLedger,
    NetClock::duration closeResolution,
    ConsensusCloseTimes const& rawCloseTimes,
    ConsensusMode const& mode,
    Json::Value && consensusJson)
{
    prevProposers_ = result.proposers;
    prevRoundTime_ = result.roundTime.read();

    bool closeTimeCorrect;

    const bool proposing = mode == ConsensusMode::proposing;
    const bool haveCorrectLCL = mode != ConsensusMode::wrongLedger;
    const bool consensusFail = result.state == ConsensusState::MovedOn;

    auto consensusCloseTime = result.position.closeTime();

    if (consensusCloseTime == NetClock::time_point{})
    {
        // We agreed to disagree on the close time
        using namespace std::chrono_literals;
        consensusCloseTime = prevLedger.closeTime() + 1s;
        closeTimeCorrect = false;
    }
    else
    {
        // We agreed on a close time
        consensusCloseTime = effCloseTime(
            consensusCloseTime, closeResolution, prevLedger.closeTime());
        closeTimeCorrect = true;
    }

    JLOG(j_.debug()) << "Report: Prop=" << (proposing ? "yes" : "no")
                     << " val=" << (validating_ ? "yes" : "no")
                     << " corLCL=" << (haveCorrectLCL ? "yes" : "no")
                     << " fail=" << (consensusFail ? "yes" : "no");
    JLOG(j_.debug()) << "Report: Prev = " << prevLedger.id() << ":"
                     << prevLedger.seq();

    //--------------------------------------------------------------------------
    std::set<TxID> failed;

    // We want to put transactions in an unpredictable but deterministic order:
    // we use the hash of the set.
    //
    // FIXME: Use a std::vector and a custom sorter instead of CanonicalTXSet?
    CanonicalTXSet retriableTxs{ result.txns.map_->getHash().as_uint256() };

    JLOG(j_.debug()) << "Building canonical tx set: " << retriableTxs.key();

    for (auto const& item : *result.txns.map_)
    {
        try
        {
            retriableTxs.insert(std::make_shared<STTx const>(SerialIter{item.slice()}));
            JLOG(j_.debug()) << "    Tx: " << item.key();
        }
        catch (std::exception const&)
        {
            failed.insert(item.key());
            JLOG(j_.warn()) << "    Tx: " << item.key() << " throws!";
        }
    }

    auto built = buildLCL(prevLedger, retriableTxs, consensusCloseTime,
        closeTimeCorrect, closeResolution, result.roundTime.read(), failed);

    auto const newLCLHash = built.id();
    JLOG(j_.debug()) << "Built ledger #" << built.seq() << ": " << newLCLHash;

    // Tell directly connected peers that we have a new LCL
    notify(protocol::neACCEPTED_LEDGER, built, haveCorrectLCL);

    // As long as we're in sync with the network, attempt to detect attempts
    // at censorship of transaction by tracking which ones don't make it in
    // after a period of time.
    if (haveCorrectLCL && result.state == ConsensusState::Yes)
    {
        std::vector<TxID> accepted;

        result.txns.map_->visitLeaves (
            [&accepted](std::shared_ptr<SHAMapItem const> const& item)
            {
                accepted.push_back(item->key());
            });

        // Track all the transactions which failed or were marked as retriable
        for (auto const& r : retriableTxs)
            failed.insert (r.first.getTXID());

        censorshipDetector_.check(std::move(accepted),
            [curr = built.seq(), j = app_.journal("CensorshipDetector"), &failed]
            (uint256 const& id, LedgerIndex seq)
            {
                if (failed.count(id))
                    return true;

                auto const wait = curr - seq;

                if (wait && (wait % censorshipWarnInternal == 0))
                {
                    std::ostringstream ss;
                    ss << "Potential Censorship: Eligible tx " << id
                       << ", which we are tracking since ledger " << seq
                       << " has not been included as of ledger " << curr
                       << ".";

                    JLOG(j.warn()) << ss.str();
                }

                return false;
            });
    }

    if (validating_)
        validating_ = ledgerMaster_.isCompatible(
            *built.ledger_, j_.warn(), "Not validating");

    if (validating_ && !consensusFail &&
        app_.getValidations().canValidateSeq(built.seq()))
    {
        validate(built, result.txns, proposing);
        JLOG(j_.info()) << "CNF Val " << newLCLHash;
    }
    else
        JLOG(j_.info()) << "CNF buildLCL " << newLCLHash;

    // See if we can accept a ledger as fully-validated
    ledgerMaster_.consensusBuilt(
        built.ledger_, result.txns.id(), std::move(consensusJson));

    //-------------------------------------------------------------------------
    {
        // Apply disputed transactions that didn't get in
        //
        // The first crack of transactions to get into the new
        // open ledger goes to transactions proposed by a validator
        // we trust but not included in the consensus set.
        //
        // These are done first because they are the most likely
        // to receive agreement during consensus. They are also
        // ordered logically "sooner" than transactions not mentioned
        // in the previous consensus round.
        //
        bool anyDisputes = false;
        for (auto& it : result.disputes)
        {
            if (!it.second.getOurVote())
            {
                // we voted NO
                try
                {
                    JLOG(j_.debug())
                        << "Test applying disputed transaction that did"
                        << " not get in " << it.second.tx().id();

                    SerialIter sit(it.second.tx().tx_.slice());
                    auto txn = std::make_shared<STTx const>(sit);

                    // Disputed pseudo-transactions that were not accepted
                    // can't be succesfully applied in the next ledger
                    if (isPseudoTx(*txn))
                        continue;

                    retriableTxs.insert(txn);

                    anyDisputes = true;
                }
                catch (std::exception const&)
                {
                    JLOG(j_.debug())
                        << "Failed to apply transaction we voted NO on";
                }
            }
        }

        // Build new open ledger
        auto lock = make_lock(app_.getMasterMutex(), std::defer_lock);
        auto sl = make_lock(ledgerMaster_.peekMutex(), std::defer_lock);
        std::lock(lock, sl);

        auto const lastVal = ledgerMaster_.getValidatedLedger();
        boost::optional<Rules> rules;
        if (lastVal)
            rules.emplace(*lastVal, app_.config().features);
        else
            rules.emplace(app_.config().features);
        app_.openLedger().accept(
            app_,
            *rules,
            built.ledger_,
            localTxs_.getTxSet(),
            anyDisputes,
            retriableTxs,
            tapNONE,
            "consensus",
            [&](OpenView& view, beast::Journal j) {
                // Stuff the ledger with transactions from the queue.
                return app_.getTxQ().accept(app_, view);
            });

        // Signal a potential fee change to subscribers after the open ledger
        // is created
        app_.getOPs().reportFeeChange();
    }

    //-------------------------------------------------------------------------
    {
        ledgerMaster_.switchLCL(built.ledger_);

        // Do these need to exist?
        assert(ledgerMaster_.getClosedLedger()->info().hash == built.id());
        assert(
            app_.openLedger().current()->info().parentHash == built.id());
    }

    //-------------------------------------------------------------------------
    // we entered the round with the network,
    // see how close our close time is to other node's
    //  close time reports, and update our clock.
    if ((mode == ConsensusMode::proposing || mode == ConsensusMode::observing) && !consensusFail)
    {
        auto closeTime = rawCloseTimes.self;

        JLOG(j_.info()) << "We closed at "
                        << closeTime.time_since_epoch().count();
        using usec64_t = std::chrono::duration<std::uint64_t>;
        usec64_t closeTotal =
            std::chrono::duration_cast<usec64_t>(closeTime.time_since_epoch());
        int closeCount = 1;

        for (auto const& p : rawCloseTimes.peers)
        {
            // FIXME: Use median, not average
            JLOG(j_.info())
                << std::to_string(p.second) << " time votes for "
                << std::to_string(p.first.time_since_epoch().count());
            closeCount += p.second;
            closeTotal += std::chrono::duration_cast<usec64_t>(
                              p.first.time_since_epoch()) *
                p.second;
        }

        closeTotal += usec64_t(closeCount / 2);  // for round to nearest
        closeTotal /= closeCount;

        // Use signed times since we are subtracting
        using duration = std::chrono::duration<std::int32_t>;
        using time_point = std::chrono::time_point<NetClock, duration>;
        auto offset = time_point{closeTotal} -
            std::chrono::time_point_cast<duration>(closeTime);
        JLOG(j_.info()) << "Our close offset is estimated at " << offset.count()
                        << " (" << closeCount << ")";

        app_.timeKeeper().adjustCloseTime(offset);
    }
}

void
RCLConsensus::Adaptor::notify(
    protocol::NodeEvent ne,
    RCLCxLedger const& ledger,
    bool haveCorrectLCL)
{
    protocol::TMStatusChange s;

    if (!haveCorrectLCL)
        s.set_newevent(protocol::neLOST_SYNC);
    else
        s.set_newevent(ne);

    s.set_ledgerseq(ledger.seq());
    s.set_networktime(app_.timeKeeper().now().time_since_epoch().count());
    s.set_ledgerhashprevious(
        ledger.parentID().begin(),
        std::decay_t<decltype(ledger.parentID())>::bytes);
    s.set_ledgerhash(
        ledger.id().begin(), std::decay_t<decltype(ledger.id())>::bytes);

    std::uint32_t uMin, uMax;
    if (!ledgerMaster_.getFullValidatedRange(uMin, uMax))
    {
        uMin = 0;
        uMax = 0;
    }
    else
    {
        // Don't advertise ledgers we're not willing to serve
        uMin = std::max(uMin, ledgerMaster_.getEarliestFetch());
    }
    s.set_firstseq(uMin);
    s.set_lastseq(uMax);
    app_.overlay ().foreach (send_always (
        std::make_shared <Message> (
            s, protocol::mtSTATUS_CHANGE)));
    JLOG (j_.trace()) << "send status change to peer";
}

RCLCxLedger
RCLConsensus::Adaptor::buildLCL(
    RCLCxLedger const& previousLedger,
    CanonicalTXSet& retriableTxs,
    NetClock::time_point closeTime,
    bool closeTimeCorrect,
    NetClock::duration closeResolution,
    std::chrono::milliseconds roundTime,
    std::set<TxID>& failedTxs)
{
    std::shared_ptr<Ledger> built = [&]()
    {
        if (auto const replayData = ledgerMaster_.releaseReplay())
        {
            assert(replayData->parent()->info().hash == previousLedger.id());
            return buildLedger(*replayData, tapNONE, app_, j_);
        }
        return buildLedger(previousLedger.ledger_, closeTime, closeTimeCorrect,
            closeResolution, app_, retriableTxs, failedTxs, j_);
    }();

    // Update fee computations based on accepted txs
    using namespace std::chrono_literals;
    app_.getTxQ().processClosedLedger(app_, *built, roundTime > 5s);

    // And stash the ledger in the ledger master
    if (ledgerMaster_.storeLedger(built))
        JLOG(j_.debug()) << "Consensus built ledger we already had";
    else if (app_.getInboundLedgers().find(built->info().hash))
        JLOG(j_.debug()) << "Consensus built ledger we were acquiring";
    else
        JLOG(j_.debug()) << "Consensus built new ledger";
    return RCLCxLedger{std::move(built)};
}

void
RCLConsensus::Adaptor::validate(RCLCxLedger const& ledger,
    RCLTxSet const& txns,
    bool proposing)
{
    using namespace std::chrono_literals;
    auto validationTime = app_.timeKeeper().closeTime();
    if (validationTime <= lastValidationTime_)
        validationTime = lastValidationTime_ + 1s;
    lastValidationTime_ = validationTime;

    STValidation::FeeSettings fees;
    std::vector<uint256> amendments;

    auto const& feeTrack = app_.getFeeTrack();
    std::uint32_t fee =
        std::max(feeTrack.getLocalFee(), feeTrack.getClusterFee());

    if (fee > feeTrack.getLoadBase())
        fees.loadFee = fee;

    // next ledger is flag ledger
    if (((ledger.seq() + 1) % 256) == 0)
    {
        // Suggest fee changes and new features
        feeVote_->doValidation(ledger.ledger_, fees);
        amendments = app_.getAmendmentTable().doValidation (getEnabledAmendments(*ledger.ledger_));
    }

    auto v = std::make_shared<STValidation>(
        ledger.id(),
        ledger.seq(),
        txns.id(),
        validationTime,
        valPublic_,
        valSecret_,
        nodeID_,
        proposing /* full if proposed */,
        fees,
        amendments);

    // suppress it if we receive it
    app_.getHashRouter().addSuppression(
        sha512Half(makeSlice(v->getSerialized())));
    handleNewValidation(app_, v, "local");
    Blob validation = v->getSerialized();
    protocol::TMValidation val;
    val.set_validation(&validation[0], validation.size());
    // Send signed validation to all of our directly connected peers
    app_.overlay().send(val);
}

void
RCLConsensus::Adaptor::onModeChange(
    ConsensusMode before,
    ConsensusMode after)
{
    JLOG(j_.info()) << "Consensus mode change before=" << to_string(before)
                    << ", after=" << to_string(after);

    // If we were proposing but aren't any longer, we need to reset the
    // censorship tracking to avoid bogus warnings.
    if ((before == ConsensusMode::proposing || before == ConsensusMode::observing) && before != after)
        censorshipDetector_.reset();

    mode_ = after;
}

Json::Value
RCLConsensus::getJson(bool full) const
{
    Json::Value ret;
    {
      ScopedLockType _{mutex_};
      ret = consensus_.getJson(full);
    }
    ret["validating"] = adaptor_.validating();
    return ret;
}

void
RCLConsensus::timerEntry(NetClock::time_point const& now)
{
    try
    {
        ScopedLockType _{mutex_};
        consensus_.timerEntry(now);
    }
    catch (SHAMapMissingNode const& mn)
    {
        // This should never happen
        JLOG(j_.error()) << "Missing node during consensus process " << mn;
        Rethrow();
    }
}

void
RCLConsensus::gotTxSet(NetClock::time_point const& now, RCLTxSet const& txSet)
{
    try
    {
        ScopedLockType _{mutex_};
        consensus_.gotTxSet(now, txSet);
    }
    catch (SHAMapMissingNode const& mn)
    {
        // This should never happen
        JLOG(j_.error()) << "Missing node during consensus process " << mn;
        Rethrow();
    }
}


//! @see Consensus::simulate

void
RCLConsensus::simulate(
    NetClock::time_point const& now,
    boost::optional<std::chrono::milliseconds> consensusDelay)
{
    ScopedLockType _{mutex_};
    consensus_.simulate(now, consensusDelay);
}

bool
RCLConsensus::peerProposal(
    NetClock::time_point const& now,
    RCLCxPeerPos const& newProposal)
{
    ScopedLockType _{mutex_};
    return consensus_.peerProposal(now, newProposal);
}

bool
RCLConsensus::Adaptor::preStartRound(RCLCxLedger const & prevLgr)
{
    // We have a key, we do not want out of sync validations after a restart
    // and are not amendment blocked.
    validating_ = valPublic_.size() != 0 &&
                  prevLgr.seq() >= app_.getMaxDisallowedLedger() &&
                  !app_.getOPs().isAmendmentBlocked();

    // If we are not running in standalone mode and there's a configured UNL,
    // check to make sure that it's not expired.
    if (validating_ && !app_.config().standalone() && app_.validators().count())
    {
        auto const when = app_.validators().expires();

        if (!when || *when < app_.timeKeeper().now())
        {
            JLOG(j_.error()) << "Voluntarily bowing out of consensus process "
                                "because of an expired validator list.";
            validating_ = false;
        }
    }

    const bool synced = app_.getOPs().getOperatingMode() == NetworkOPs::omFULL;

    if (validating_)
    {
        JLOG(j_.info()) << "Entering consensus process, validating, synced="
                        << (synced ? "yes" : "no");
    }
    else
    {
        // Otherwise we just want to monitor the validation process.
        JLOG(j_.info()) << "Entering consensus process, watching, synced="
                        << (synced ? "yes" : "no");
    }

    // Notify inbound ledgers that we are starting a new round
    inboundTransactions_.newRound(prevLgr.seq());

    // Use parent ledger's rules to determine whether to use rounded close time
    parms_.useRoundedCloseTime = prevLgr.ledger_->rules().enabled(fix1528);

    // propose only if we're in sync with the network (and validating)
    return validating_ && synced;
}

bool
RCLConsensus::Adaptor::haveValidated() const
{
    return ledgerMaster_.haveValidated();
}

LedgerIndex
RCLConsensus::Adaptor::getValidLedgerIndex() const
{
    return ledgerMaster_.getValidLedgerIndex();
}

std::pair<std::size_t, hash_set<RCLConsensus::Adaptor::NodeKey_t>>
RCLConsensus::Adaptor::getQuorumKeys() const
{
    return app_.validators().getQuorumKeys();
}

std::size_t
RCLConsensus::Adaptor::laggards(Ledger_t::Seq const seq,
    hash_set<RCLConsensus::Adaptor::NodeKey_t>& trustedKeys) const
{
    return app_.getValidations().laggards(seq, trustedKeys);
}

bool
RCLConsensus::Adaptor::validator() const
{
    return !valPublic_.empty();
}

void
RCLConsensus::startRound(
    NetClock::time_point const& now,
    RCLCxLedger::ID const& prevLgrId,
    RCLCxLedger const& prevLgr,
    hash_set<NodeID> const& nowUntrusted)
{
    ScopedLockType _{mutex_};
    consensus_.startRound(
        now, prevLgrId, prevLgr, nowUntrusted, adaptor_.preStartRound(prevLgr));
}
}
