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


// This has to be included early to prevent an obscure MSVC compile error
#include <boost/asio/deadline_timer.hpp>

#include <SupplyFinanceChain/protocol/jss.h>
#include <SupplyFinanceChain/rpc/RPCHandler.h>
#include <SupplyFinanceChain/rpc/handlers/Handlers.h>

#include <SupplyFinanceChain/rpc/handlers/PathFind.cpp>
#include <SupplyFinanceChain/rpc/handlers/PayChanClaim.cpp>
#include <SupplyFinanceChain/rpc/handlers/Peers.cpp>
#include <SupplyFinanceChain/rpc/handlers/Ping.cpp>
#include <SupplyFinanceChain/rpc/handlers/Print.cpp>
#include <SupplyFinanceChain/rpc/handlers/Random.cpp>
#include <SupplyFinanceChain/rpc/handlers/SupplyFinanceChainPathFind.cpp>
#include <SupplyFinanceChain/rpc/handlers/ServerInfo.cpp>
#include <SupplyFinanceChain/rpc/handlers/ServerState.cpp>
#include <SupplyFinanceChain/rpc/handlers/SignFor.cpp>
#include <SupplyFinanceChain/rpc/handlers/SignHandler.cpp>
#include <SupplyFinanceChain/rpc/handlers/Stop.cpp>
#include <SupplyFinanceChain/rpc/handlers/Submit.cpp>
#include <SupplyFinanceChain/rpc/handlers/SubmitMultiSigned.cpp>
#include <SupplyFinanceChain/rpc/handlers/Subscribe.cpp>
#include <SupplyFinanceChain/rpc/handlers/TransactionEntry.cpp>
#include <SupplyFinanceChain/rpc/handlers/Tx.cpp>
#include <SupplyFinanceChain/rpc/handlers/TxHistory.cpp>
#include <SupplyFinanceChain/rpc/handlers/UnlList.cpp>
#include <SupplyFinanceChain/rpc/handlers/Unsubscribe.cpp>
#include <SupplyFinanceChain/rpc/handlers/ValidationCreate.cpp>
#include <SupplyFinanceChain/rpc/handlers/Validators.cpp>
#include <SupplyFinanceChain/rpc/handlers/ValidatorListSites.cpp>
#include <SupplyFinanceChain/rpc/handlers/WalletPropose.cpp>

#include <SupplyFinanceChain/rpc/impl/DeliveredAmount.cpp>
#include <SupplyFinanceChain/rpc/impl/Handler.cpp>
#include <SupplyFinanceChain/rpc/impl/LegacyPathFind.cpp>
#include <SupplyFinanceChain/rpc/impl/Role.cpp>
#include <SupplyFinanceChain/rpc/impl/RPCHandler.cpp>
#include <SupplyFinanceChain/rpc/impl/RPCHelpers.cpp>
#include <SupplyFinanceChain/rpc/impl/ServerHandlerImp.cpp>
#include <SupplyFinanceChain/rpc/impl/ShardArchiveHandler.cpp>
#include <SupplyFinanceChain/rpc/impl/Status.cpp>
#include <SupplyFinanceChain/rpc/impl/TransactionSign.cpp>


