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
#include <SupplyFinanceChain/basics/StringUtilities.h>
#include <SupplyFinanceChain/ledger/ReadView.h>
#include <SupplyFinanceChain/net/RPCErr.h>
#include <SupplyFinanceChain/protocol/ErrorCodes.h>
#include <SupplyFinanceChain/protocol/jss.h>
#include <SupplyFinanceChain/protocol/PayChan.h>
#include <SupplyFinanceChain/protocol/STAccount.h>
#include <SupplyFinanceChain/resource/Fees.h>
#include <SupplyFinanceChain/rpc/Context.h>
#include <SupplyFinanceChain/rpc/impl/RPCHelpers.h>
#include <SupplyFinanceChain/rpc/impl/Tuning.h>

#include <boost/optional.hpp>

namespace SupplyFinanceChain {

// {
//   secret_key: <signing_secret_key>
//   channel_id: 256-bit channel id
//   drops: 64-bit uint (as string)
// }
Json::Value doChannelAuthorize (RPC::Context& context)
{
    auto const& params (context.params);
    for (auto const& p : {jss::secret, jss::channel_id, jss::amount})
        if (!params.isMember (p))
            return RPC::missing_field_error (p);

    Json::Value result;
    auto const keypair = RPC::keypairForSignature (params, result);
    if (RPC::contains_error (result))
        return result;

    uint256 channelId;
    if (!channelId.SetHexExact (params[jss::channel_id].asString ()))
        return rpcError (rpcCHANNEL_MALFORMED);

    boost::optional<std::uint64_t> const optDrops =
        params[jss::amount].isString()
        ? to_uint64(params[jss::amount].asString())
        : boost::none;

    if (!optDrops)
        return rpcError (rpcCHANNEL_AMT_MALFORMED);

    std::uint64_t const drops = *optDrops;

    Serializer msg;
    serializePayChanAuthorization (msg, channelId, SFCAmount (drops));

    try
    {
        auto const buf = sign (keypair.first, keypair.second, msg.slice ());
        result[jss::signature] = strHex (buf);
    }
    catch (std::exception&)
    {
        result =
            RPC::make_error (rpcINTERNAL, "Exception occurred during signing.");
    }
    return result;
}

// {
//   public_key: <public_key>
//   channel_id: 256-bit channel id
//   drops: 64-bit uint (as string)
//   signature: signature to verify
// }
Json::Value doChannelVerify (RPC::Context& context)
{
    auto const& params (context.params);
    for (auto const& p :
         {jss::public_key, jss::channel_id, jss::amount, jss::signature})
        if (!params.isMember (p))
            return RPC::missing_field_error (p);

    boost::optional<PublicKey> pk;
    {
        std::string const strPk = params[jss::public_key].asString();
        pk = parseBase58<PublicKey>(TokenType::AccountPublic, strPk);

        if (!pk)
        {
            std::pair<Blob, bool> pkHex(strUnHex (strPk));
            if (!pkHex.second)
                return rpcError(rpcPUBLIC_MALFORMED);
            auto const pkType = publicKeyType(makeSlice(pkHex.first));
            if (!pkType)
                return rpcError(rpcPUBLIC_MALFORMED);
            pk.emplace(makeSlice(pkHex.first));
        }
    }

    uint256 channelId;
    if (!channelId.SetHexExact (params[jss::channel_id].asString ()))
        return rpcError (rpcCHANNEL_MALFORMED);

    boost::optional<std::uint64_t> const optDrops =
        params[jss::amount].isString()
        ? to_uint64(params[jss::amount].asString())
        : boost::none;

    if (!optDrops)
        return rpcError (rpcCHANNEL_AMT_MALFORMED);

    std::uint64_t const drops = *optDrops;

    std::pair<Blob, bool> sig(strUnHex (params[jss::signature].asString ()));
    if (!sig.second || !sig.first.size ())
        return rpcError (rpcINVALID_PARAMS);

    Serializer msg;
    serializePayChanAuthorization (msg, channelId, SFCAmount (drops));

    Json::Value result;
    result[jss::signature_verified] =
        verify (*pk, msg.slice (), makeSlice (sig.first), /*canonical*/ true);
    return result;
}

} // SupplyFinanceChain
