//------------------------------------------------------------------------------
/*
    This file is part of SupplyFinanceChaind: https://github.com/SupplyFinanceChain/SupplyFinanceChaind
    Copyright (c) 2012-2014 SupplyFinanceChain Labs Inc.

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

#include <SupplyFinanceChain/app/main/Application.h>
#include <SupplyFinanceChain/app/misc/LoadFeeTrack.h>
#include <SupplyFinanceChain/core/TimeKeeper.h>
#include <SupplyFinanceChain/overlay/Cluster.h>
#include <SupplyFinanceChain/overlay/Overlay.h>
#include <SupplyFinanceChain/protocol/jss.h>
#include <SupplyFinanceChain/rpc/Context.h>
#include <SupplyFinanceChain/basics/make_lock.h>

namespace SupplyFinanceChain {

Json::Value doPeers (RPC::Context& context)
{
    Json::Value jvResult (Json::objectValue);

    {
        jvResult[jss::peers] = context.app.overlay ().json ();

        auto const now = context.app.timeKeeper().now();
        auto const self = context.app.nodeIdentity().first;

        Json::Value& cluster = (jvResult[jss::cluster] = Json::objectValue);
        std::uint32_t ref = context.app.getFeeTrack().getLoadBase();

        context.app.cluster().for_each ([&cluster, now, ref, &self]
            (ClusterNode const& node)
            {
                if (node.identity() == self)
                    return;

                Json::Value& json = cluster[
                    toBase58(
                        TokenType::NodePublic,
                        node.identity())];

                if (!node.name().empty())
                    json[jss::tag] = node.name();

                if ((node.getLoadFee() != ref) && (node.getLoadFee() != 0))
                    json[jss::fee] = static_cast<double>(node.getLoadFee()) / ref;

                if (node.getReportTime() != NetClock::time_point{})
                    json[jss::age] = (node.getReportTime() >= now)
                        ? 0
                        : (now - node.getReportTime()).count();
            });
    }

    return jvResult;
}

} // SupplyFinanceChain
