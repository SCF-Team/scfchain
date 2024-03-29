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

#ifndef SUPPLYFINANCECHAIN_OVERLAY_PEER_H_INCLUDED
#define SUPPLYFINANCECHAIN_OVERLAY_PEER_H_INCLUDED

#include <SupplyFinanceChain/overlay/Message.h>
#include <SupplyFinanceChain/basics/base_uint.h>
#include <SupplyFinanceChain/json/json_value.h>
#include <SupplyFinanceChain/protocol/PublicKey.h>
#include <SupplyFinanceChain/beast/net/IPEndpoint.h>

namespace SupplyFinanceChain {

namespace Resource {
class Charge;
}

// Maximum hops to attempt when crawling shards. cs = crawl shards
static constexpr std::uint32_t csHopLimit = 3;

/** Represents a peer connection in the overlay. */
class Peer
{
public:
    using ptr = std::shared_ptr<Peer>;

    /** Uniquely identifies a peer.
        This can be stored in tables to find the peer later. Callers
        can discover if the peer is no longer connected and make
        adjustments as needed.
    */
    using id_t = std::uint32_t;

    virtual ~Peer() = default;

    //
    // Network
    //

    virtual
    void
    send (Message::pointer const& m) = 0;

    virtual
    beast::IP::Endpoint
    getRemoteAddress() const = 0;

    /** Adjust this peer's load balance based on the type of load imposed. */
    virtual
    void
    charge (Resource::Charge const& fee) = 0;

    //
    // Identity
    //

    virtual
    id_t
    id() const = 0;

    /** Returns `true` if this connection is a member of the cluster. */
    virtual
    bool
    cluster() const = 0;

    virtual
    bool
    isHighLatency() const = 0;

    virtual
    int
    getScore (bool) const = 0;

    virtual
    PublicKey const&
    getNodePublic() const = 0;

    virtual
    Json::Value json() = 0;

    //
    // Ledger
    //

    virtual uint256 const& getClosedLedgerHash () const = 0;
    virtual bool hasLedger (uint256 const& hash, std::uint32_t seq) const = 0;
    virtual void ledgerRange (std::uint32_t& minSeq, std::uint32_t& maxSeq) const = 0;
    virtual bool hasShard (std::uint32_t shardIndex) const = 0;
    virtual bool hasTxSet (uint256 const& hash) const = 0;
    virtual void cycleStatus () = 0;
    virtual bool supportsVersion (int version) = 0;
    virtual bool hasRange (std::uint32_t uMin, std::uint32_t uMax) = 0;
};

}

#endif
