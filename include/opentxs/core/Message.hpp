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

#ifndef OPENTXS_CORE_OTMESSAGE_HPP
#define OPENTXS_CORE_OTMESSAGE_HPP

#include "opentxs/Forward.hpp"

#include "opentxs/core/crypto/OTASCIIArmor.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/NumList.hpp"
#include "opentxs/core/String.hpp"

#include <stdint.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace opentxs
{

class Context;
class Message;
class Nym;
class OTPasswordData;
class ServerContext;
class Tag;

class OTMessageStrategy
{
public:
    virtual int32_t processXml(
        Message& message,
        irr::io::IrrXMLReader*& xml) = 0;
    virtual void writeXml(Message& message, Tag& parent) = 0;
    virtual ~OTMessageStrategy();

    void processXmlSuccess(Message& m, irr::io::IrrXMLReader*& xml);
};

class OTMessageStrategyManager
{
public:
    OTMessageStrategy* findStrategy(std::string name)
    {
        auto strategy = mapping.find(name);
        if (strategy == mapping.end()) return nullptr;
        return strategy->second.get();
    }
    void registerStrategy(std::string name, OTMessageStrategy* strategy)
    {
        mapping[name] = std::unique_ptr<OTMessageStrategy>(strategy);
    }

private:
    std::unordered_map<std::string, std::unique_ptr<OTMessageStrategy>> mapping;
};

class Message : public Contract
{
protected:
    int32_t ProcessXMLNode(irr::io::IrrXMLReader*& xml) override;

    void UpdateContents() override;

    bool m_bIsSigned{false};

private:
    typedef std::map<MessageType, std::string> TypeMap;
    typedef std::map<std::string, MessageType> ReverseTypeMap;

    static const TypeMap message_names_;
    static const ReverseTypeMap message_types_;
    static const std::map<MessageType, MessageType> reply_message_;

    static ReverseTypeMap make_reverse_map();
    static MessageType reply_command(const MessageType& type);

    bool updateContentsByType(Tag& parent);

    int32_t processXmlNodeAckReplies(Message& m, irr::io::IrrXMLReader*& xml);
    int32_t processXmlNodeAcknowledgedReplies(
        Message& m,
        irr::io::IrrXMLReader*& xml);
    int32_t processXmlNodeNotaryMessage(
        Message& m,
        irr::io::IrrXMLReader*& xml);

public:
    EXPORT static std::string Command(const MessageType type);
    EXPORT static MessageType Type(const std::string& type);
    EXPORT static std::string ReplyCommand(const MessageType type);

    EXPORT Message();
    EXPORT virtual ~Message();

    bool VerifyContractID() const override;

    EXPORT bool SignContract(
        const Nym& theNym,
        const OTPasswordData* pPWData = nullptr) override;
    EXPORT bool VerifySignature(
        const Nym& theNym,
        const OTPasswordData* pPWData = nullptr) const override;

    EXPORT bool HarvestTransactionNumbers(
        ServerContext& context,
        bool bHarvestingForRetry,            // false until positively asserted.
        bool bReplyWasSuccess,               // false until positively asserted.
        bool bReplyWasFailure,               // false until positively asserted.
        bool bTransactionWasSuccess,         // false until positively asserted.
        bool bTransactionWasFailure) const;  // false until positively asserted.

    // So the message can get the list of numbers from the Nym, before sending,
    // that should be listed as acknowledged that the server reply has already
    // been seen for those request numbers.
    EXPORT void SetAcknowledgments(const Context& context);
    EXPORT void SetAcknowledgments(const std::set<RequestNumber>& numbers);

    EXPORT static void registerStrategy(
        std::string name,
        OTMessageStrategy* strategy);

    String m_strCommand;   // perhaps @register is the string for "reply to
                           // register" a-ha
    String m_strNotaryID;  // This is sent with every message for security
                           // reasons.
    String m_strNymID;     // The hash of the user's public key... or x509 cert.
    String m_strNymboxHash;  // Sometimes in a server reply as FYI, sometimes
                             // in user message for validation purposes.
    String m_strInboxHash;   // Sometimes in a server reply as FYI, sometimes in
                             // user message for validation purposes.
    String m_strOutboxHash;  // Sometimes in a server reply as FYI, sometimes
                             // in user message for validation purposes.
    String m_strNymID2;      // If the user requests public key of another user.
                             // ALSO used for MARKET ID sometimes.
    String m_strNymPublicKey;  // The user's public key... or x509 cert.
    String m_strInstrumentDefinitionID;  // The hash of the contract for
                                         // whatever
                                         // digital
                                         // asset is referenced.
    String m_strAcctID;                  // The unique ID of an asset account.
    String m_strType;                    // .
    String m_strRequestNum;  // Every user has a request number. This prevents
                             // messages from
                             // being intercepted and repeated by attackers.

    OTASCIIArmor m_ascInReferenceTo;  // If the server responds to a user
                                      // command, he sends
    // it back to the user here in ascii armored format.
    OTASCIIArmor m_ascPayload;  // If the reply needs to include a payload (such
                                // as a new account
    // or a message envelope or request from another user etc) then
    // it can be put here in ascii-armored format.
    OTASCIIArmor m_ascPayload2;  // Sometimes one payload just isn't enough.
    OTASCIIArmor m_ascPayload3;  // Sometimes two payload just isn't enough.

    // This list of request numbers is stored for optimization, so client/server
    // can communicate about
    // which messages have been received, and can avoid certain downloads, such
    // as replyNotice Box Receipts.
    //
    NumList m_AcknowledgedReplies;  // Client request: list of server replies
                                    // client has already seen.
    // Server reply:   list of client-acknowledged replies (so client knows that
    // server knows.)

    int64_t m_lNewRequestNum{0};  // If you are SENDING a message, you set
                                  // m_strRequestNum. (For all msgs.)
    // Server Reply for all messages copies that same number into
    // m_strRequestNum;
    // But if this is a SERVER REPLY to the "getRequestNumber" MESSAGE, the
    // "request number" expected in that reply is stored HERE in
    // m_lNewRequestNum;
    int64_t m_lDepth{0};  // For Market-related messages... (Plus for usage
                          // credits.) Also used by getBoxReceipt
    int64_t m_lTransactionNum{
        0};  // For Market-related messages... Also used by
             // getBoxReceipt

    int32_t keytypeAuthent_ = 0;
    int32_t keytypeEncrypt_ = 0;
    std::uint8_t enum_{0};
    std::uint32_t enum2_{0};

    bool m_bSuccess{
        false};  // When the server replies to the client, this may be true
                 // or false
    bool m_bBool{
        false};  // Some commands need to send a bool. This variable is for
                 // those.
    int64_t m_lTime{0};  // Timestamp when the message was signed.

    static OTMessageStrategyManager messageStrategyManager;
};

class RegisterStrategy
{
public:
    RegisterStrategy(std::string name, OTMessageStrategy* strategy)
    {
        Message::registerStrategy(name, strategy);
    }
};

}  // namespace opentxs

#endif  // OPENTXS_CORE_OTMESSAGE_HPP
