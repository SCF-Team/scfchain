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

#include <SupplyFinanceChain/app/main/Application.h>
#include <SupplyFinanceChain/app/ledger/AcceptedLedgerTx.h>
#include <SupplyFinanceChain/basics/Log.h>
#include <SupplyFinanceChain/basics/StringUtilities.h>
#include <SupplyFinanceChain/protocol/jss.h>
#include <SupplyFinanceChain/protocol/UintTypes.h>

namespace SupplyFinanceChain {

AcceptedLedgerTx::AcceptedLedgerTx (
    std::shared_ptr<ReadView const> const& ledger,
    std::shared_ptr<STTx const> const& txn,
    std::shared_ptr<STObject const> const& met,
    AccountIDCache const& accountCache,
    Logs& logs)
    : mLedger (ledger)
    , mTxn (txn)
    , mMeta (std::make_shared<TxMeta> (
        txn->getTransactionID(), ledger->seq(), *met))
    , mAffected (mMeta->getAffectedAccounts (logs.journal("View")))
    , accountCache_ (accountCache)
    , logs_ (logs)
{
    assert (! ledger->open());

    mResult = mMeta->getResultTER ();

    Serializer s;
    met->add(s);
    mRawMeta = std::move (s.modData());

    buildJson ();
}

AcceptedLedgerTx::AcceptedLedgerTx (
    std::shared_ptr<ReadView const> const& ledger,
    std::shared_ptr<STTx const> const& txn,
    TER result,
    AccountIDCache const& accountCache,
    Logs& logs)
    : mLedger (ledger)
    , mTxn (txn)
    , mResult (result)
    , mAffected (txn->getMentionedAccounts ())
    , accountCache_ (accountCache)
    , logs_ (logs)
{
    assert (ledger->open());
    buildJson ();
}

std::string AcceptedLedgerTx::getEscMeta () const
{
    assert (!mRawMeta.empty ());
    return sqlEscape (mRawMeta);
}

void AcceptedLedgerTx::buildJson ()
{
    mJson = Json::objectValue;
    mJson[jss::transaction] = mTxn->getJson (JsonOptions::none);

    if (mMeta)
    {
        mJson[jss::meta] = mMeta->getJson (JsonOptions::none);
        mJson[jss::raw_meta] = strHex (mRawMeta);
    }

    mJson[jss::result] = transHuman (mResult);

    if (! mAffected.empty ())
    {
        Json::Value& affected = (mJson[jss::affected] = Json::arrayValue);
        for (auto const& account: mAffected)
            affected.append (accountCache_.toBase58(account));
    }

    if (mTxn->getTxnType () == ttOFFER_CREATE)
    {
        auto const& account = mTxn->getAccountID(sfAccount);
        auto const amount = mTxn->getFieldAmount (sfTakerGets);

        // If the offer create is not self funded then add the owner balance
        if (account != amount.issue ().account)
        {
            auto const ownerFunds = accountFunds(*mLedger,
                account, amount, fhIGNORE_FREEZE, logs_.journal ("View"));
            mJson[jss::transaction][jss::owner_funds] = ownerFunds.getText ();
        }
    }
}

} // SupplyFinanceChain
