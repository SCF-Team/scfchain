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

#ifndef SUPPLYFINANCECHAIN_NET_HTTPCLIENT_H_INCLUDED
#define SUPPLYFINANCECHAIN_NET_HTTPCLIENT_H_INCLUDED

#include <SupplyFinanceChain/basics/ByteUtilities.h>
#include <SupplyFinanceChain/core/Config.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <chrono>

namespace SupplyFinanceChain {

/** Provides an asynchronous HTTP client implementation with optional SSL.
*/
class HTTPClient
{
public:
    explicit HTTPClient() = default;

    static constexpr auto maxClientHeaderBytes = kilobytes(32);

    static void initializeSSLContext (Config const& config, beast::Journal j);

    static void get (
        bool bSSL,
        boost::asio::io_service& io_service,
        std::deque <std::string> deqSites,
        const unsigned short port,
        std::string const& strPath,
        std::size_t responseMax,    // if no Content-Length header
        std::chrono::seconds timeout,
        std::function <bool (const boost::system::error_code& ecResult, int iStatus, std::string const& strData)> complete,
        beast::Journal& j);

    static void get (
        bool bSSL,
        boost::asio::io_service& io_service,
        std::string strSite,
        const unsigned short port,
        std::string const& strPath,
        std::size_t responseMax,    // if no Content-Length header
        std::chrono::seconds timeout,
        std::function <bool (const boost::system::error_code& ecResult, int iStatus, std::string const& strData)> complete,
        beast::Journal& j);

    static void request (
        bool bSSL,
        boost::asio::io_service& io_service,
        std::string strSite,
        const unsigned short port,
        std::function <void (boost::asio::streambuf& sb, std::string const& strHost)> build,
        std::size_t responseMax,    // if no Content-Length header
        std::chrono::seconds timeout,
        std::function <bool (const boost::system::error_code& ecResult, int iStatus, std::string const& strData)> complete,
        beast::Journal& j);
};

} // SupplyFinanceChain

#endif
