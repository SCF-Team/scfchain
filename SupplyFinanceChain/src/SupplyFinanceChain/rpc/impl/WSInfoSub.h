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

#ifndef SUPPLYFINANCECHAIN_RPC_WSINFOSUB_H
#define SUPPLYFINANCECHAIN_RPC_WSINFOSUB_H

#include <SupplyFinanceChain/server/WSSession.h>
#include <SupplyFinanceChain/net/InfoSub.h>
#include <SupplyFinanceChain/beast/net/IPAddressConversion.h>
#include <SupplyFinanceChain/json/json_writer.h>
#include <SupplyFinanceChain/rpc/Role.h>
#include <boost/utility/string_view.hpp>
#include <memory>
#include <string>

namespace SupplyFinanceChain {

class WSInfoSub : public InfoSub
{
    std::weak_ptr<WSSession> ws_;
    std::string user_;
    std::string fwdfor_;

public:
    WSInfoSub(Source& source, std::shared_ptr<WSSession> const& ws)
        : InfoSub(source)
        , ws_(ws)
    {
        auto const& h = ws->request();
        if (ipAllowed(beast::IPAddressConversion::from_asio(
            ws->remote_endpoint()).address(), ws->port().secure_gateway_ip))
        {
            auto it = h.find("X-User");
            if (it != h.end())
                user_ = it->value().to_string();
            fwdfor_ = std::string(forwardedFor(h));
        }
    }

    boost::string_view
    user() const
    {
        return user_;
    }

    boost::string_view
    forwarded_for() const
    {
        return fwdfor_;
    }

    void
    send(Json::Value const& jv, bool) override
    {
        auto sp = ws_.lock();
        if(! sp)
            return;
        boost::beast::multi_buffer sb;
        Json::stream(jv,
            [&](void const* data, std::size_t n)
            {
                sb.commit(boost::asio::buffer_copy(
                    sb.prepare(n), boost::asio::buffer(data, n)));
            });
        auto m = std::make_shared<
            StreambufWSMsg<decltype(sb)>>(
                std::move(sb));
        sp->send(m);
    }
};

} // SupplyFinanceChain

#endif
