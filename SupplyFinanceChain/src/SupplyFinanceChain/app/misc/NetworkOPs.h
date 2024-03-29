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

#ifndef SUPPLYFINANCECHAIN_APP_MISC_NETWORKOPS_H_INCLUDED
#define SUPPLYFINANCECHAIN_APP_MISC_NETWORKOPS_H_INCLUDED

#include <SupplyFinanceChain/app/ledger/Ledger.h>
#include <SupplyFinanceChain/app/consensus/RCLCxPeerPos.h>
#include <SupplyFinanceChain/core/JobQueue.h>
#include <SupplyFinanceChain/core/Stoppable.h>
#include <SupplyFinanceChain/ledger/ReadView.h>
#include <SupplyFinanceChain/net/InfoSub.h>
#include <SupplyFinanceChain/protocol/STValidation.h>
#include <boost/asio.hpp>
#include <memory>
#include <deque>
#include <tuple>

#include <SupplyFinanceChain/protocol/messages.h>

namespace SupplyFinanceChain {

// Operations that clients may wish to perform against the network
// Master operational handler, server sequencer, network tracker

class Peer;
class LedgerMaster;
class Transaction;
class ValidatorKeys;

// This is the primary interface into the "client" portion of the program.
// Code that wants to do normal operations on the network such as
// creating and monitoring accounts, creating transactions, and so on
// should use this interface. The RPC code will primarily be a light wrapper
// over this code.
//
// Eventually, it will check the node's operating mode (synched, unsynched,
// etectera) and defer to the correct means of processing. The current
// code assumes this node is synched (and will continue to do so until
// there's a functional network.
//
/** Provides server functionality for clients.

    Clients include backend applications, local commands, and connected
    clients. This class acts as a proxy, fulfilling the command with local
    data if possible, or asking the network and returning the results if
    needed.

    A backend application or local client can trust a local instance of
    SupplyFinanceChaind / NetworkOPs. However, client software connecting to non-local
    instances of SupplyFinanceChaind will need to be hardened to protect against hostile
    or unreliable servers.
*/
class NetworkOPs
    : public InfoSub::Source
{
protected:
    explicit NetworkOPs (Stoppable& parent);

public:
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

    enum OperatingMode
    {
        // how we process transactions or account balance requests
        omDISCONNECTED  = 0,    // not ready to process requests
        omCONNECTED     = 1,    // convinced we are talking to the network
        omSYNCING       = 2,    // fallen slightly behind
        omTRACKING      = 3,    // convinced we agree with the network
        omFULL          = 4     // we have the ledger and can even validate
    };

    enum class FailHard : unsigned char
    {
        no,
        yes
    };
    static inline FailHard doFailHard (bool noMeansDont)
    {
        return noMeansDont ? FailHard::yes : FailHard::no;
    }

public:
    ~NetworkOPs () override = default;

    //--------------------------------------------------------------------------
    //
    // Network information
    //

    virtual OperatingMode getOperatingMode () const = 0;
    virtual std::string strOperatingMode (bool admin = false) const = 0;

    //--------------------------------------------------------------------------
    //
    // Transaction processing
    //

    // must complete immediately
    virtual void submitTransaction (std::shared_ptr<STTx const> const&) = 0;

    /**
     * Process transactions as they arrive from the network or which are
     * submitted by clients. Process local transactions synchronously
     *
     * @param transaction Transaction object
     * @param bUnlimited Whether a privileged client connection submitted it.
     * @param bLocal Client submission.
     * @param failType fail_hard setting from transaction submission.
     */
    virtual void processTransaction (std::shared_ptr<Transaction>& transaction,
        bool bUnlimited, bool bLocal, FailHard failType) = 0;

    //--------------------------------------------------------------------------
    //
    // Owner functions
    //

    virtual Json::Value getOwnerInfo (std::shared_ptr<ReadView const> lpLedger,
        AccountID const& account) = 0;

    //--------------------------------------------------------------------------
    //
    // Book functions
    //

    virtual void getBookPage (
        std::shared_ptr<ReadView const>& lpLedger,
        Book const& book,
        AccountID const& uTakerID,
        bool const bProof,
        unsigned int iLimit,
        Json::Value const& jvMarker,
        Json::Value& jvResult) = 0;

    //--------------------------------------------------------------------------

    // ledger proposal/close functions
    virtual void processTrustedProposal (RCLCxPeerPos peerPos,
        std::shared_ptr<protocol::TMProposeSet> set) = 0;

    virtual bool recvValidation (STValidation::ref val,
        std::string const& source) = 0;

    virtual void mapComplete (std::shared_ptr<SHAMap> const& map,
        bool fromAcquire) = 0;

    // network state machine
    virtual bool beginConsensus (uint256 const& netLCL) = 0;
    virtual void endConsensus () = 0;
    virtual void setStandAlone () = 0;
    virtual void setStateTimer () = 0;

    virtual void setNeedNetworkLedger () = 0;
    virtual void clearNeedNetworkLedger () = 0;
    virtual bool isNeedNetworkLedger () = 0;
    virtual bool isFull () = 0;
    virtual bool isAmendmentBlocked () = 0;
    virtual void setAmendmentBlocked () = 0;
    virtual void consensusViewChange () = 0;

    virtual Json::Value getConsensusInfo () = 0;
    virtual Json::Value getServerInfo (
        bool human, bool admin, bool counters) = 0;
    virtual void clearLedgerFetch () = 0;
    virtual Json::Value getLedgerFetchInfo () = 0;

    /** Accepts the current transaction tree, return the new ledger's sequence

        This API is only used via RPC with the server in STANDALONE mode and
        performs a virtual consensus round, with all the transactions we are
        proposing being accepted.
    */
    virtual std::uint32_t acceptLedger (
        boost::optional<std::chrono::milliseconds> consensusDelay = boost::none) = 0;

    virtual uint256 getConsensusLCL () = 0;

    virtual void reportFeeChange () = 0;

    virtual void updateLocalTx (ReadView const& newValidLedger) = 0;
    virtual std::size_t getLocalTxCount () = 0;

    // client information retrieval functions
    using AccountTx  = std::pair<std::shared_ptr<Transaction>, TxMeta::pointer>;
    using AccountTxs = std::vector<AccountTx>;

    virtual AccountTxs getAccountTxs (
        AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger,  bool descending,
        std::uint32_t offset, int limit, bool bUnlimited) = 0;

    virtual AccountTxs getTxsAccount (
        AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger, bool forward,
        Json::Value& token, int limit, bool bUnlimited) = 0;

    using txnMetaLedgerType = std::tuple<std::string, std::string, std::uint32_t>;
    using MetaTxsList       = std::vector<txnMetaLedgerType>;

    virtual MetaTxsList getAccountTxsB (AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger,  bool descending,
            std::uint32_t offset, int limit, bool bUnlimited) = 0;

    virtual MetaTxsList getTxsAccountB (AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger,  bool forward,
        Json::Value& token, int limit, bool bUnlimited) = 0;

    //--------------------------------------------------------------------------
    //
    // Monitoring: publisher side
    //
    virtual void pubLedger (
        std::shared_ptr<ReadView const> const& lpAccepted) = 0;
    virtual void pubProposedTransaction (
        std::shared_ptr<ReadView const> const& lpCurrent,
        std::shared_ptr<STTx const> const& stTxn, TER terResult) = 0;
    virtual void pubValidation (STValidation::ref val) = 0;
};

//------------------------------------------------------------------------------

std::unique_ptr<NetworkOPs>
make_NetworkOPs (Application& app, NetworkOPs::clock_type& clock,
    bool standalone, std::size_t minPeerCount, bool start_valid,
    JobQueue& job_queue, LedgerMaster& ledgerMaster, Stoppable& parent,
    ValidatorKeys const & validatorKeys, boost::asio::io_service& io_svc,
    beast::Journal journal);

} // SupplyFinanceChain

#endif
