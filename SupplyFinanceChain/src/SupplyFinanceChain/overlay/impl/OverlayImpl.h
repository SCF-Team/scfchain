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

#ifndef SUPPLYFINANCECHAIN_OVERLAY_OVERLAYIMPL_H_INCLUDED
#define SUPPLYFINANCECHAIN_OVERLAY_OVERLAYIMPL_H_INCLUDED

#include <SupplyFinanceChain/app/main/Application.h>
#include <SupplyFinanceChain/core/Job.h>
#include <SupplyFinanceChain/overlay/Overlay.h>
#include <SupplyFinanceChain/overlay/impl/TrafficCount.h>
#include <SupplyFinanceChain/server/Handoff.h>
#include <SupplyFinanceChain/rpc/ServerHandler.h>
#include <SupplyFinanceChain/basics/Resolver.h>
#include <SupplyFinanceChain/basics/chrono.h>
#include <SupplyFinanceChain/basics/UnorderedContainers.h>
#include <SupplyFinanceChain/overlay/impl/TMHello.h>
#include <SupplyFinanceChain/peerfinder/PeerfinderManager.h>
#include <SupplyFinanceChain/resource/ResourceManager.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace SupplyFinanceChain {

class PeerImp;
class BasicConfig;

constexpr std::uint32_t maxTTL = 2;

class OverlayImpl : public Overlay
{
public:
    class Child
    {
    protected:
        OverlayImpl& overlay_;

        explicit
        Child (OverlayImpl& overlay);

        virtual ~Child();

    public:
        virtual void stop() = 0;
    };

private:
    using clock_type = std::chrono::steady_clock;
    using socket_type = boost::asio::ip::tcp::socket;
    using address_type = boost::asio::ip::address;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using error_code = boost::system::error_code;

    struct Timer
        : Child
        , std::enable_shared_from_this<Timer>
    {
        boost::asio::basic_waitable_timer <clock_type> timer_;

        explicit
        Timer (OverlayImpl& overlay);

        void
        stop() override;

        void
        run();

        void
        on_timer (error_code ec);
    };

    Application& app_;
    boost::asio::io_service& io_service_;
    boost::optional<boost::asio::io_service::work> work_;
    boost::asio::io_service::strand strand_;
    std::recursive_mutex mutex_; // VFALCO use std::mutex
    std::condition_variable_any cond_;
    std::weak_ptr<Timer> timer_;
    boost::container::flat_map<
        Child*, std::weak_ptr<Child>> list_;
    Setup setup_;
    beast::Journal journal_;
    ServerHandler& serverHandler_;
    Resource::Manager& m_resourceManager;
    std::unique_ptr <PeerFinder::Manager> m_peerFinder;
    TrafficCount m_traffic;
    hash_map <PeerFinder::Slot::ptr,
        std::weak_ptr <PeerImp>> m_peers;
    hash_map<Peer::id_t, std::weak_ptr<PeerImp>> ids_;
    Resolver& m_resolver;
    std::atomic <Peer::id_t> next_id_;
    int timer_count_;
    std::atomic <uint64_t> jqTransOverflow_ {0};
    std::atomic <uint64_t> peerDisconnects_ {0};
    std::atomic <uint64_t> peerDisconnectsCharges_ {0};

    // Last time we crawled peers for shard info. 'cs' = crawl shards
    std::atomic<std::chrono::seconds> csLast_{std::chrono::seconds{0}};
    std::mutex csMutex_;
    std::condition_variable csCV_;
    // Peer IDs expecting to receive a last link notification
    std::set<std::uint32_t> csIDs_;

    //--------------------------------------------------------------------------

public:
    OverlayImpl (Application& app, Setup const& setup, Stoppable& parent,
        ServerHandler& serverHandler, Resource::Manager& resourceManager,
        Resolver& resolver, boost::asio::io_service& io_service,
        BasicConfig const& config);

    ~OverlayImpl();

    OverlayImpl (OverlayImpl const&) = delete;
    OverlayImpl& operator= (OverlayImpl const&) = delete;

    PeerFinder::Manager&
    peerFinder()
    {
        return *m_peerFinder;
    }

    Resource::Manager&
    resourceManager()
    {
        return m_resourceManager;
    }

    ServerHandler&
    serverHandler()
    {
        return serverHandler_;
    }

    Setup const&
    setup() const
    {
        return setup_;
    }

    Handoff
    onHandoff (std::unique_ptr <beast::asio::ssl_bundle>&& bundle,
        http_request_type&& request,
            endpoint_type remote_endpoint) override;

    void
    connect(beast::IP::Endpoint const& remote_endpoint) override;

    int
    limit() override;

    std::size_t
    size() override;

    Json::Value
    json() override;

    PeerSequence
    getActivePeers() override;

    void
    check () override;

    void
    checkSanity (std::uint32_t) override;

    std::shared_ptr<Peer>
    findPeerByShortID (Peer::id_t const& id) override;

    std::shared_ptr<Peer>
    findPeerByPublicKey (PublicKey const& pubKey) override;

    void
    send (protocol::TMProposeSet& m) override;

    void
    send (protocol::TMValidation& m) override;

    void
    relay (protocol::TMProposeSet& m,
        uint256 const& uid) override;

    void
    relay (protocol::TMValidation& m,
        uint256 const& uid) override;

    //--------------------------------------------------------------------------
    //
    // OverlayImpl
    //

    void
    add_active (std::shared_ptr<PeerImp> const& peer);

    void
    remove (PeerFinder::Slot::ptr const& slot);

    /** Called when a peer has connected successfully
        This is called after the peer handshake has been completed and during
        peer activation. At this point, the peer address and the public key
        are known.
    */
    void
    activate (std::shared_ptr<PeerImp> const& peer);

    // Called when an active peer is destroyed.
    void
    onPeerDeactivate (Peer::id_t id);

    // UnaryFunc will be called as
    //  void(std::shared_ptr<PeerImp>&&)
    //
    template <class UnaryFunc>
    void
    for_each (UnaryFunc&& f)
    {
        std::vector<std::weak_ptr<PeerImp>> wp;
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);

            // Iterate over a copy of the peer list because peer
            // destruction can invalidate iterators.
            wp.reserve(ids_.size());

            for (auto& x : ids_)
                wp.push_back(x.second);
        }

        for (auto& w : wp)
        {
            if (auto p = w.lock())
                f(std::move(p));
        }
    }

    std::size_t
    selectPeers (PeerSet& set, std::size_t limit, std::function<
        bool(std::shared_ptr<Peer> const&)> score) override;

    // Called when TMManifests is received from a peer
    void
    onManifests (
        std::shared_ptr<protocol::TMManifests> const& m,
            std::shared_ptr<PeerImp> const& from);

    static
    bool
    isPeerUpgrade (http_request_type const& request);

    template<class Body>
    static
    bool
    isPeerUpgrade (boost::beast::http::response<Body> const& response)
    {
        if (! is_upgrade(response))
            return false;
        if(response.result() != boost::beast::http::status::switching_protocols)
            return false;
        auto const versions = parse_ProtocolVersions(
            response["Upgrade"]);
        if (versions.size() == 0)
            return false;
        return true;
    }

    template<class Fields>
    static
    bool
    is_upgrade(boost::beast::http::header<true, Fields> const& req)
    {
        if(req.version() < 11)
            return false;
        if(req.method() != boost::beast::http::verb::get)
            return false;
        if(! boost::beast::http::token_list{req["Connection"]}.exists("upgrade"))
            return false;
        return true;
    }

    template<class Fields>
    static
    bool
    is_upgrade(boost::beast::http::header<false, Fields> const& req)
    {
        if(req.version() < 11)
            return false;
        if(! boost::beast::http::token_list{req["Connection"]}.exists("upgrade"))
            return false;
        return true;
    }

    static
    std::string
    makePrefix (std::uint32_t id);

    void
    reportTraffic (
        TrafficCount::category cat,
        bool isInbound,
        int bytes);

    void
    incJqTransOverflow() override
    {
        ++jqTransOverflow_;
    }

    std::uint64_t
    getJqTransOverflow() const override
    {
        return jqTransOverflow_;
    }

    void
    incPeerDisconnect() override
    {
        ++peerDisconnects_;
    }

    std::uint64_t
    getPeerDisconnect() const override
    {
        return peerDisconnects_;
    }

    void
    incPeerDisconnectCharges() override
    {
        ++peerDisconnectsCharges_;
    }

    std::uint64_t
    getPeerDisconnectCharges() const override
    {
        return peerDisconnectsCharges_;
    }

    Json::Value
    crawlShards(bool pubKey, std::uint32_t hops) override;


    /** Called when the last link from a peer chain is received.

        @param id peer id that received the shard info.
    */
    void
    lastLink(std::uint32_t id);

private:
    std::shared_ptr<Writer>
    makeRedirectResponse (PeerFinder::Slot::ptr const& slot,
        http_request_type const& request, address_type remote_address);

    std::shared_ptr<Writer>
    makeErrorResponse (PeerFinder::Slot::ptr const& slot,
        http_request_type const& request, address_type remote_address,
        std::string msg);

    bool
    processRequest (http_request_type const& req,
        Handoff& handoff);

    /** Returns information about peers on the overlay network.
        Reported through the /crawl API
        Controlled through the config section [crawl] overlay=[0|1]
    */
    Json::Value
    getOverlayInfo();

    /** Returns information about the local server.
        Reported through the /crawl API
        Controlled through the config section [crawl] server=[0|1]
    */
    Json::Value
    getServerInfo();

    /** Returns information about the local server's performance counters.
        Reported through the /crawl API
        Controlled through the config section [crawl] counts=[0|1]
    */
    Json::Value
    getServerCounts();

    /** Returns information about the local server's UNL.
        Reported through the /crawl API
        Controlled through the config section [crawl] unl=[0|1]
    */
    Json::Value
    getUnlInfo();

    //--------------------------------------------------------------------------

    //
    // Stoppable
    //

    void
    checkStopped();

    void
    onPrepare() override;

    void
    onStart() override;

    void
    onStop() override;

    void
    onChildrenStopped() override;

    //
    // PropertyStream
    //

    void
    onWrite (beast::PropertyStream::Map& stream) override;

    //--------------------------------------------------------------------------

    void
    remove (Child& child);

    void
    stop();

    void
    autoConnect();

    void
    sendEndpoints();
};

} // SupplyFinanceChain

#endif
