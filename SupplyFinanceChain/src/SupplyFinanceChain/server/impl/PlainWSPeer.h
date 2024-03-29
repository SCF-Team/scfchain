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

#ifndef SUPPLYFINANCECHAIN_SERVER_PLAINWSPEER_H_INCLUDED
#define SUPPLYFINANCECHAIN_SERVER_PLAINWSPEER_H_INCLUDED

#include <SupplyFinanceChain/server/impl/BaseWSPeer.h>
#include <SupplyFinanceChain/beast/asio/waitable_timer.h>
#include <memory>

namespace SupplyFinanceChain {

template<class Handler>
class PlainWSPeer
    : public BaseWSPeer<Handler, PlainWSPeer<Handler>>
    , public std::enable_shared_from_this<PlainWSPeer<Handler>>
{
    friend class BasePeer<Handler, PlainWSPeer>;
    friend class BaseWSPeer<Handler, PlainWSPeer>;

    using clock_type = std::chrono::system_clock;
    using error_code = boost::system::error_code;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using waitable_timer = boost::asio::basic_waitable_timer <clock_type>;
    using socket_type = boost::asio::ip::tcp::socket;

    boost::beast::websocket::stream<socket_type> ws_;

public:
    template<class Body, class Headers>
    PlainWSPeer(
        Port const& port,
        Handler& handler,
        endpoint_type remote_address,
        boost::beast::http::request<Body, Headers>&& request,
        socket_type&& socket,
        beast::Journal journal);
};

//------------------------------------------------------------------------------

template <class Handler>
template <class Body, class Headers>
PlainWSPeer<Handler>::PlainWSPeer(
    Port const& port,
    Handler& handler,
    endpoint_type remote_address,
    boost::beast::http::request<Body, Headers>&& request,
    socket_type&& socket,
    beast::Journal journal)
    : BaseWSPeer<Handler, PlainWSPeer>(
          port,
          handler,
          socket.get_executor(),
          beast::create_waitable_timer<waitable_timer>(socket),
          remote_address,
          std::move(request),
          journal)
    , ws_(std::move(socket))
{
}

} // SupplyFinanceChain

#endif
