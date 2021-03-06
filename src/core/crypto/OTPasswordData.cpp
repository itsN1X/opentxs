/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/

#include "opentxs/stdafx.hpp"

#include "opentxs/core/crypto/OTPasswordData.hpp"

#include "opentxs/core/crypto/OTPassword.hpp"
#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/String.hpp"

// Instantiate one of these whenever you do an action that may
// require a passphrase. When you call the OpenSSL private key
// using function, just pass in the address to this instance along
// as one of the parameters. That way when the actual password
// callback is activated, you'll get that pointer as the userdata
// parameter to the callback.
// This enables you to easily pass data to the callback about
// which Nym is doing the action, or what string should be displayed
// on the screen, etc. You'll also be able to use the same mechanism
// for determining whether it's a wallet-Nym doing the action, or
// a real Nym. (Thus making it possible to skip any "password caching"
// code that normally happens for real nyms, when it's the wallet nym.)
//

namespace opentxs
{

bool OTPasswordData::isUsingOldSystem() const { return m_bUsingOldSystem; }

void OTPasswordData::setUsingOldSystem(bool bUsing)
{
    m_bUsingOldSystem = bUsing;
}

bool OTPasswordData::isForNormalNym() const { return (nullptr == m_pMasterPW); }

bool OTPasswordData::isForCachedKey() const { return (nullptr != m_pMasterPW); }

const char* OTPasswordData::GetDisplayString() const
{
    return m_strDisplay.c_str();
}

OTPasswordData::OTPasswordData(
    const char* szDisplay,
    OTPassword* pMasterPW,
    const OTCachedKey* pCachedKey)
    : m_pMasterPW(pMasterPW)
    , m_strDisplay(
          nullptr == szDisplay ? "(Sorry, no user data provided.)" : szDisplay)
    , m_bUsingOldSystem(false)
    , m_pCachedKey(pCachedKey)
{
    // They can both be nullptr, or they can both be not nullptr.
    // But you can't have one nullptr, and the other not.
    OT_ASSERT(
        ((nullptr == pMasterPW) && (nullptr == pCachedKey)) ||
        ((nullptr != pMasterPW) && (nullptr != pCachedKey)));
}

OTPasswordData::OTPasswordData(
    const std::string& str_Display,
    OTPassword* pMasterPW,
    const OTCachedKey* pCachedKey)
    : m_pMasterPW(pMasterPW)
    , m_strDisplay(str_Display)
    , m_bUsingOldSystem(false)
    , m_pCachedKey(pCachedKey)
{
    // They can both be nullptr, or they can both be not nullptr.
    // But you can't have one nullptr, and the other not.
    OT_ASSERT(
        ((nullptr == pMasterPW) && (nullptr == pCachedKey)) ||
        ((nullptr != pMasterPW) && (nullptr != pCachedKey)));
}

OTPasswordData::OTPasswordData(
    const String& strDisplay,
    OTPassword* pMasterPW,
    const OTCachedKey* pCachedKey)
    : m_pMasterPW(pMasterPW)
    , m_strDisplay(strDisplay.Get())
    , m_bUsingOldSystem(false)
    , m_pCachedKey(pCachedKey)
{
    // They can both be nullptr, or they can both be  not nullptr.
    // But you can't have one nullptr, and the other not.
    OT_ASSERT(
        ((nullptr == pMasterPW) && (nullptr == pCachedKey)) ||
        ((nullptr != pMasterPW) && (nullptr != pCachedKey)));
}

OTPasswordData::~OTPasswordData()
{
    m_pMasterPW = nullptr;   // not owned
    m_pCachedKey = nullptr;  // not owned
}

bool OTPasswordData::ClearOverride()
{
    password_override_.reset();

    return true;
}

bool OTPasswordData::SetOverride(const OTPassword& password)
{
    password_override_.reset(new OTPassword(password));

    return true;
}

const std::unique_ptr<OTPassword>& OTPasswordData::Override() const
{
    return password_override_;
}
}  // namespace opentxs
