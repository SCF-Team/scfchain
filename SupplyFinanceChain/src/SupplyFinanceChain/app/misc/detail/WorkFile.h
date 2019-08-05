//------------------------------------------------------------------------------
/*
    This file is part of SupplyFinanceChaind: https://github.com/SupplyFinanceChain/SupplyFinanceChaind
    Copyright (c) 2018 SupplyFinanceChain Labs Inc.

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

#ifndef SUPPLYFINANCECHAIN_APP_MISC_DETAIL_WORKFILE_H_INCLUDED
#define SUPPLYFINANCECHAIN_APP_MISC_DETAIL_WORKFILE_H_INCLUDED

#include <SupplyFinanceChain/app/misc/detail/Work.h>
#include <SupplyFinanceChain/basics/ByteUtilities.h>
#include <SupplyFinanceChain/basics/FileUtilities.h>
#include <cassert>
#include <cerrno>

namespace SupplyFinanceChain {

namespace detail {

// Work with files
class WorkFile: public Work
    , public std::enable_shared_from_this<WorkFile>
{
protected:
    using error_code = boost::system::error_code;
    // Override the definition in Work.h
    using response_type = std::string;

public:
    using callback_type =
        std::function<void(error_code const&, response_type const&)>;
public:
    WorkFile(
        std::string const& path,
        boost::asio::io_service& ios, callback_type cb);
    ~WorkFile();

    void run() override;

    void cancel() override;

private:
    std::string path_;
    callback_type cb_;
    boost::asio::io_service& ios_;
    boost::asio::io_service::strand strand_;

};

//------------------------------------------------------------------------------

WorkFile::WorkFile(
    std::string const& path,
    boost::asio::io_service& ios, callback_type cb)
    : path_(path)
    , cb_(std::move(cb))
    , ios_(ios)
    , strand_(ios)
{
}

WorkFile::~WorkFile()
{
    if (cb_)
        cb_ (make_error_code(boost::system::errc::interrupted), {});
}

void
WorkFile::run()
{
    if (! strand_.running_in_this_thread())
        return ios_.post(strand_.wrap (std::bind(
            &WorkFile::run, shared_from_this())));

    error_code ec;
    auto const fileContents = getFileContents(ec, path_, megabytes(1));

    assert(cb_);
    cb_(ec, fileContents);
    cb_ = nullptr;
}

void
WorkFile::cancel()
{
    // Nothing to do. Either it finished in run, or it didn't start.
}


} // detail

} // SupplyFinanceChain

#endif
