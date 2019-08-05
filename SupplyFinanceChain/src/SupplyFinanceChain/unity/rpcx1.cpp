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

#include <SupplyFinanceChain/rpc/handlers/AccountCurrenciesHandler.cpp>
#include <SupplyFinanceChain/rpc/handlers/AccountInfo.cpp>
#include <SupplyFinanceChain/rpc/handlers/AccountLines.cpp>
#include <SupplyFinanceChain/rpc/handlers/AccountChannels.cpp>
#include <SupplyFinanceChain/rpc/handlers/AccountObjects.cpp>
#include <SupplyFinanceChain/rpc/handlers/AccountOffers.cpp>
#include <SupplyFinanceChain/rpc/handlers/AccountTx.cpp>
#include <SupplyFinanceChain/rpc/handlers/AccountTxOld.cpp>
#include <SupplyFinanceChain/rpc/handlers/AccountTxSwitch.cpp>
#include <SupplyFinanceChain/rpc/handlers/BlackList.cpp>
#include <SupplyFinanceChain/rpc/handlers/BookOffers.cpp>
#include <SupplyFinanceChain/rpc/handlers/CanDelete.cpp>
#include <SupplyFinanceChain/rpc/handlers/Connect.cpp>
#include <SupplyFinanceChain/rpc/handlers/ConsensusInfo.cpp>
#include <SupplyFinanceChain/rpc/handlers/CrawlShards.cpp>
#include <SupplyFinanceChain/rpc/handlers/DepositAuthorized.cpp>
#include <SupplyFinanceChain/rpc/handlers/DownloadShard.cpp>
#include <SupplyFinanceChain/rpc/handlers/Feature1.cpp>
#include <SupplyFinanceChain/rpc/handlers/Fee1.cpp>
#include <SupplyFinanceChain/rpc/handlers/FetchInfo.cpp>
#include <SupplyFinanceChain/rpc/handlers/GatewayBalances.cpp>
#include <SupplyFinanceChain/rpc/handlers/GetCounts.cpp>
#include <SupplyFinanceChain/rpc/handlers/LedgerHandler.cpp>
#include <SupplyFinanceChain/rpc/handlers/LedgerAccept.cpp>
#include <SupplyFinanceChain/rpc/handlers/LedgerCleanerHandler.cpp>
#include <SupplyFinanceChain/rpc/handlers/LedgerClosed.cpp>
#include <SupplyFinanceChain/rpc/handlers/LedgerCurrent.cpp>
#include <SupplyFinanceChain/rpc/handlers/LedgerData.cpp>
#include <SupplyFinanceChain/rpc/handlers/LedgerEntry.cpp>
#include <SupplyFinanceChain/rpc/handlers/LedgerHeader.cpp>
#include <SupplyFinanceChain/rpc/handlers/LedgerRequest.cpp>
#include <SupplyFinanceChain/rpc/handlers/LogLevel.cpp>
#include <SupplyFinanceChain/rpc/handlers/LogRotate.cpp>
#include <SupplyFinanceChain/rpc/handlers/NoSupplyFinanceChainCheck.cpp>
#include <SupplyFinanceChain/rpc/handlers/OwnerInfo.cpp>
