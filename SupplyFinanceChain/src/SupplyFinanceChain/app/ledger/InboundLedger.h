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

#ifndef SUPPLYFINANCECHAIN_APP_LEDGER_INBOUNDLEDGER_H_INCLUDED
#define SUPPLYFINANCECHAIN_APP_LEDGER_INBOUNDLEDGER_H_INCLUDED

#include <SupplyFinanceChain/app/main/Application.h>
#include <SupplyFinanceChain/app/ledger/Ledger.h>
#include <SupplyFinanceChain/overlay/PeerSet.h>
#include <SupplyFinanceChain/basics/CountedObject.h>
#include <mutex>
#include <set>
#include <utility>

namespace SupplyFinanceChain {

// A ledger we are trying to acquire
class InboundLedger
    : public PeerSet
    , public std::enable_shared_from_this <InboundLedger>
    , public CountedObject <InboundLedger>
{
public:
    static char const* getCountedObjectName () { return "InboundLedger"; }

    using PeerDataPairType = std::pair<
        std::weak_ptr<Peer>,
        std::shared_ptr<protocol::TMLedgerData>>;

    // These are the reasons we might acquire a ledger
    enum class Reason
    {
        HISTORY,  // Acquiring past ledger
        SHARD,    // Acquiring for shard
        GENERIC,  // Generic other reasons
        CONSENSUS // We believe the consensus round requires this ledger
    };

    InboundLedger(Application& app, uint256 const& hash,
        std::uint32_t seq, Reason reason, clock_type&);

    ~InboundLedger ();

    // Called when the PeerSet timer expires
    void execute () override;

    // Called when another attempt is made to fetch this same ledger
    void update (std::uint32_t seq);

    std::shared_ptr<Ledger const>
    getLedger() const
    {
        return mLedger;
    }

    std::uint32_t getSeq () const
    {
        return mSeq;
    }

    Reason
    getReason() const
    {
        return mReason;
    }

    bool checkLocal ();
    void init (ScopedLockType& collectionLock);

    bool
    gotData(std::weak_ptr<Peer>,
        std::shared_ptr<protocol::TMLedgerData> const&);

    using neededHash_t =
        std::pair <protocol::TMGetObjectByHash::ObjectType, uint256>;

    /** Return a Json::objectValue. */
    Json::Value getJson (int);

    void runData ();

    static
    LedgerInfo
    deserializeHeader(Slice data, bool hasPrefix);

private:
    enum class TriggerReason
    {
        added,
        reply,
        timeout
    };

    void filterNodes (
        std::vector<std::pair<SHAMapNodeID, uint256>>& nodes,
        TriggerReason reason);

    void trigger (std::shared_ptr<Peer> const&, TriggerReason);

    std::vector<neededHash_t> getNeededHashes ();

    void addPeers ();
    void tryDB (Family& f);

    void done ();

    void onTimer (bool progress, ScopedLockType& peerSetLock) override;

    void newPeer (std::shared_ptr<Peer> const& peer) override
    {
        // For historical nodes, do not trigger too soon
        // since a fetch pack is probably coming
        if (mReason != Reason::HISTORY)
            trigger (peer, TriggerReason::added);
    }

    std::weak_ptr <PeerSet> pmDowncast () override;

    int processData (std::shared_ptr<Peer> peer, protocol::TMLedgerData& data);

    bool takeHeader (std::string const& data);
    bool takeTxNode (const std::vector<SHAMapNodeID>& IDs,
                     const std::vector<Blob>& data,
                     SHAMapAddNode&);
    bool takeTxRootNode (Slice const& data, SHAMapAddNode&);

    // VFALCO TODO Rename to receiveAccountStateNode
    //             Don't use acronyms, but if we are going to use them at least
    //             capitalize them correctly.
    //
    bool takeAsNode (const std::vector<SHAMapNodeID>& IDs,
                     const std::vector<Blob>& data,
                     SHAMapAddNode&);
    bool takeAsRootNode (Slice const& data, SHAMapAddNode&);

    std::vector<uint256>
    neededTxHashes (
        int max, SHAMapSyncFilter* filter) const;

    std::vector<uint256>
    neededStateHashes (
        int max, SHAMapSyncFilter* filter) const;

    std::shared_ptr<Ledger> mLedger;
    bool mHaveHeader;
    bool mHaveState;
    bool mHaveTransactions;
    bool mSignaled;
    bool mByHash;
    std::uint32_t mSeq;
    Reason const mReason;

    std::set <uint256> mRecentNodes;

    SHAMapAddNode mStats;

    // Data we have received from peers
    std::mutex mReceivedDataLock;
    std::vector <PeerDataPairType> mReceivedData;
    bool mReceiveDispatched;
};

} // SupplyFinanceChain

#endif
