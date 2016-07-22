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

#ifndef OPENTXS_CORE_CRYPTO_NYMPARAMETERS_HPP
#define OPENTXS_CORE_CRYPTO_NYMPARAMETERS_HPP

#include "opentxs/core/Proto.hpp"
#include "opentxs/core/Types.hpp"
#include "opentxs/core/crypto/Credential.hpp"
#include "opentxs/core/crypto/OTAsymmetricKey.hpp"

#include <cstdint>
#include <string>
#include <memory>

namespace opentxs
{

class NymParameters
{
public:
    NymParameterType nymParameterType();

    proto::AsymmetricKeyType AsymmetricKeyType() const;

    void setNymParameterType(NymParameterType theKeytype);

    proto::CredentialType credentialType() const;

    void setCredentialType(proto::CredentialType theCredentialtype);

    inline proto::SourceType SourceType() const { return sourceType_; }

    inline void SetSourceType(proto::SourceType sType) { sourceType_ = sType; }

    inline proto::SourceProofType SourceProofType() const
    {
        return sourceProofType_;
    }

    inline void SetSourceProofType(proto::SourceProofType sType)
    {
        sourceProofType_ = sType;
    }

    inline std::shared_ptr<proto::ContactData> ContactData() const
    {
        return contact_data_;
    }

    inline std::shared_ptr<proto::VerificationSet> VerificationSet() const
    {
        return verification_set_;
    }

    void SetContactData(const proto::ContactData& contactData);
    void SetVerificationSet(const proto::VerificationSet& verificationSet);

#if defined(OT_CRYPTO_SUPPORTED_KEY_RSA)
    std::int32_t keySize();

    void setKeySize(std::int32_t keySize);
    explicit NymParameters(const std::int32_t keySize);
#endif

    explicit NymParameters(proto::CredentialType theCredentialtype);
    NymParameters() = default;
    ~NymParameters() = default;

#if defined(OT_CRYPTO_WITH_BIP32)
    inline std::string Seed() const { return seed_; }
    inline void SetSeed(const std::string seed) { seed_ = seed; }

    inline std::uint32_t Nym() const { return nym_; }
    inline void SetNym(const std::uint32_t path) { nym_ = path; }

    inline std::uint32_t Credset() const { return credset_; }
    inline void SetCredset(const std::uint32_t path) { credset_ = path; }

    inline std::uint32_t CredIndex() const { return cred_index_; }
    inline void SetCredIndex(const std::uint32_t path) { cred_index_ = path; }
#endif

private:
    proto::SourceType sourceType_ = proto::SOURCETYPE_PUBKEY;
    proto::SourceProofType sourceProofType_ =
        proto::SOURCEPROOFTYPE_SELF_SIGNATURE;
    std::shared_ptr<proto::ContactData> contact_data_;
    std::shared_ptr<proto::VerificationSet> verification_set_;

    NymParameterType nymType_ = NymParameterType::ED25519;
#if defined(OT_CRYPTO_WITH_BIP32)
    proto::CredentialType credentialType_ = proto::CREDTYPE_HD;
#else
    proto::CredentialType credentialType_ = proto::CREDTYPE_LEGACY;
#endif

//----------------------------------------
// CRYPTO ALGORITHMS
//----------------------------------------
#if defined(OT_CRYPTO_SUPPORTED_KEY_RSA)
    std::int32_t nBits_ = 1024;
#endif
#if defined(OT_CRYPTO_WITH_BIP32)
    std::string seed_;
    std::uint32_t nym_ = 0;
    std::uint32_t credset_ = 0;
    std::uint32_t cred_index_ = 0;
#endif
};
}  // namespace opentxs
#endif  // OPENTXS_CORE_CRYPTO_NYMPARAMETERS_HPP
