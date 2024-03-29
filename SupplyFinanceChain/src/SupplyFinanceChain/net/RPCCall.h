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

#ifndef SUPPLYFINANCECHAIN_NET_RPCCALL_H_INCLUDED
#define SUPPLYFINANCECHAIN_NET_RPCCALL_H_INCLUDED

#include <SupplyFinanceChain/basics/Log.h>
#include <SupplyFinanceChain/core/Config.h>
#include <SupplyFinanceChain/json/json_value.h>
#include <boost/asio/io_service.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace SupplyFinanceChain {

// This a trusted interface, the user is expected to provide valid input to
// perform valid requests. Error catching and reporting is not a requirement of
// the command line interface.
//
// Improvements to be more strict and to provide better diagnostics are welcome.

/** Processes SupplyFinanceChain RPC calls. */
namespace RPCCall {

int fromCommandLine (
    Config const& config,
    const std::vector<std::string>& vCmd,
    Logs& logs);

void fromNetwork (
    boost::asio::io_service& io_service,
    std::string const& strIp, const std::uint16_t iPort,
    std::string const& strUsername, std::string const& strPassword,
    std::string const& strPath, std::string const& strMethod,
    Json::Value const& jvParams, const bool bSSL, bool quiet,
    Logs& logs,
    std::function<void (Json::Value const& jvInput)> callbackFuncP = std::function<void (Json::Value const& jvInput)> (),
    std::unordered_map<std::string, std::string> headers = {});
}

/** Given a SupplyFinanceChaind command line, return the corresponding JSON.
*/
Json::Value
cmdLineToJSONRPC (std::vector<std::string> const& args, beast::Journal j);

/** Internal invocation of RPC client.
*/
std::pair<int, Json::Value>
rpcClient(std::vector<std::string> const& args,
    Config const& config, Logs& logs,
    std::unordered_map<std::string, std::string> const& headers = {});

} // SupplyFinanceChain

#endif
