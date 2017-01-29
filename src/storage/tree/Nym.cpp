/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Nym, CLI, GUI
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

#include "opentxs/storage/tree/Nym.hpp"

#include "opentxs/storage/Storage.hpp"

#include <functional>

namespace opentxs
{
namespace storage
{
Nym::Nym(
    const Storage& storage,
    const keyFunction& migrate,
    const std::string& id,
    const std::string& hash,
    const std::string& alias)
    : Node(storage, migrate, hash)
    , alias_(alias)
    , nymid_(id)
{
    if (check_hash(hash)) {
        init(hash);
    } else {
        version_ = 1;
        root_ = Node::BLANK_HASH;
        credentials_ = Node::BLANK_HASH;
        sent_peer_request_ = Node::BLANK_HASH;
        incoming_peer_request_ = Node::BLANK_HASH;
        sent_peer_reply_ = Node::BLANK_HASH;
        incoming_peer_reply_ = Node::BLANK_HASH;
        finished_peer_request_ = Node::BLANK_HASH;
        finished_peer_reply_ = Node::BLANK_HASH;
        processed_peer_request_ = Node::BLANK_HASH;
        processed_peer_reply_ = Node::BLANK_HASH;
    }

    checked_.store(false);
    private_.store(false);
    revision_.store(0);
}

std::string Nym::Alias() const { return alias_; }

PeerReplies* Nym::finished_reply_box() const
{
    std::unique_lock<std::mutex> lock(finished_reply_box_lock_);

    if (!finished_reply_box_) {
        finished_reply_box_.reset(
            new PeerReplies(storage_, migrate_, finished_peer_reply_));

        if (!finished_reply_box_) {
            std::cerr << __FUNCTION__ << ": Unable to instantiate."
                      << std::endl;
            abort();
        }
    }

    lock.unlock();

    return finished_reply_box_.get();
}

PeerRequests* Nym::finished_request_box() const
{
    std::unique_lock<std::mutex> lock(finished_request_box_lock_);

    if (!finished_request_box_) {
        finished_request_box_.reset(
            new PeerRequests(storage_, migrate_, finished_peer_request_));

        if (!finished_request_box_) {
            std::cerr << __FUNCTION__ << ": Unable to instantiate."
                      << std::endl;
            abort();
        }
    }

    lock.unlock();

    return finished_request_box_.get();
}

const PeerRequests& Nym::FinishedRequestBox() const
{
    return *finished_request_box();
}

const PeerReplies& Nym::FinishedReplyBox() const
{
    return *finished_reply_box();
}

PeerReplies* Nym::incoming_reply_box() const
{
    std::unique_lock<std::mutex> lock(incoming_reply_box_lock_);

    if (!incoming_reply_box_) {
        incoming_reply_box_.reset(
            new PeerReplies(storage_, migrate_, incoming_peer_reply_));

        if (!incoming_reply_box_) {
            std::cerr << __FUNCTION__ << ": Unable to instantiate."
                      << std::endl;
            abort();
        }
    }

    lock.unlock();

    return incoming_reply_box_.get();
}

PeerRequests* Nym::incoming_request_box() const
{
    std::unique_lock<std::mutex> lock(incoming_request_box_lock_);

    if (!incoming_request_box_) {
        incoming_request_box_.reset(
            new PeerRequests(storage_, migrate_, incoming_peer_request_));

        if (!incoming_request_box_) {
            std::cerr << __FUNCTION__ << ": Unable to instantiate."
                      << std::endl;
            abort();
        }
    }

    lock.unlock();

    return incoming_request_box_.get();
}

const PeerRequests& Nym::IncomingRequestBox() const
{
    return *incoming_request_box();
}

const PeerReplies& Nym::IncomingReplyBox() const
{
    return *incoming_reply_box();
}

void Nym::init(const std::string& hash)
{
    std::shared_ptr<proto::StorageNym> serialized;
    storage_.LoadProto(hash, serialized);

    if (!serialized) {
        std::cerr << __FUNCTION__ << ": Failed to load nym index file."
                  << std::endl;
        abort();
    }

    version_ = serialized->version();

    // Fix legacy data stores
    if (0 == version_) {
        version_ = 1;
    }

    nymid_ = serialized->nymid();
    credentials_ = serialized->credlist().hash();
    sent_peer_request_ = serialized->sentpeerrequests().hash();
    incoming_peer_request_ = serialized->incomingpeerrequests().hash();
    sent_peer_reply_ = serialized->sentpeerreply().hash();
    incoming_peer_reply_ = serialized->incomingpeerreply().hash();
    finished_peer_request_ = serialized->finishedpeerrequest().hash();
    finished_peer_reply_ = serialized->finishedpeerreply().hash();
    processed_peer_request_ = serialized->processedpeerrequest().hash();
    processed_peer_reply_ = serialized->processedpeerreply().hash();
}

bool Nym::Load(
    std::shared_ptr<proto::CredentialIndex>& output,
    std::string& alias,
    const bool checking) const
{
    std::lock_guard<std::mutex> lock(write_lock_);

    if (!check_hash(credentials_)) {
        if (!checking) {
            std::cerr << __FUNCTION__ << ": Error: nym with id " << nymid_
                      << " does not exist." << std::endl;
        }

        return false;
    }

    alias = alias_;
    checked_.store(storage_.LoadProto(credentials_, output, false));

    if (!checked_.load()) {
        return false;
    }

    private_.store(proto::CREDINDEX_PRIVATE == output->mode());
    revision_.store(output->revision());

    return true;
}

bool Nym::Migrate() const
{
    if (!Node::migrate(credentials_)) {
        return false;
    }

    if (!sent_request_box()->Migrate()) {
        return false;
    }

    if (!incoming_request_box()->Migrate()) {
        return false;
    }

    if (!sent_reply_box()->Migrate()) {
        return false;
    }

    if (!incoming_reply_box()->Migrate()) {
        return false;
    }

    if (!finished_request_box()->Migrate()) {
        return false;
    }

    if (!finished_reply_box()->Migrate()) {
        return false;
    }

    if (!processed_request_box()->Migrate()) {
        return false;
    }

    if (!processed_reply_box()->Migrate()) {
        return false;
    }

    return Node::migrate(root_);
}

Editor<PeerRequests> Nym::mutable_SentRequestBox()
{
    std::function<void(PeerRequests*, std::unique_lock<std::mutex>&)> callback =
        [&](PeerRequests* in, std::unique_lock<std::mutex>& lock) -> void {
        this->save(in, lock, StorageBox::SENTPEERREQUEST);
    };

    return Editor<PeerRequests>(
        write_lock_, sent_request_box(), callback);
}

Editor<PeerRequests> Nym::mutable_IncomingRequestBox()
{
    std::function<void(PeerRequests*, std::unique_lock<std::mutex>&)> callback =
        [&](PeerRequests* in, std::unique_lock<std::mutex>& lock) -> void {
        this->save(in, lock, StorageBox::INCOMINGPEERREQUEST);
    };

    return Editor<PeerRequests>(
        write_lock_, incoming_request_box(), callback);
}

Editor<PeerReplies> Nym::mutable_SentReplyBox()
{
    std::function<void(PeerReplies*, std::unique_lock<std::mutex>&)> callback =
        [&](PeerReplies* in, std::unique_lock<std::mutex>& lock) -> void {
        this->save(in, lock, StorageBox::SENTPEERREPLY);
    };

    return Editor<PeerReplies>(
        write_lock_, sent_reply_box(), callback);
}

Editor<PeerReplies> Nym::mutable_IncomingReplyBox()
{
    std::function<void(PeerReplies*, std::unique_lock<std::mutex>&)> callback =
        [&](PeerReplies* in, std::unique_lock<std::mutex>& lock) -> void {
        this->save(in, lock, StorageBox::INCOMINGPEERREPLY);
    };

    return Editor<PeerReplies>(
        write_lock_, incoming_reply_box(), callback);
}

Editor<PeerRequests> Nym::mutable_FinishedRequestBox()
{
    std::function<void(PeerRequests*, std::unique_lock<std::mutex>&)> callback =
        [&](PeerRequests* in, std::unique_lock<std::mutex>& lock) -> void {
        this->save(in, lock, StorageBox::FINISHEDPEERREQUEST);
    };

    return Editor<PeerRequests>(
        write_lock_, finished_request_box(), callback);
}

Editor<PeerReplies> Nym::mutable_FinishedReplyBox()
{
    std::function<void(PeerReplies*, std::unique_lock<std::mutex>&)> callback =
        [&](PeerReplies* in, std::unique_lock<std::mutex>& lock) -> void {
        this->save(in, lock, StorageBox::FINISHEDPEERREPLY);
    };

    return Editor<PeerReplies>(
        write_lock_, finished_reply_box(), callback);
}

Editor<PeerRequests> Nym::mutable_ProcessedRequestBox()
{
    std::function<void(PeerRequests*, std::unique_lock<std::mutex>&)> callback =
        [&](PeerRequests* in, std::unique_lock<std::mutex>& lock) -> void {
        this->save(in, lock, StorageBox::PROCESSEDPEERREQUEST);
    };

    return Editor<PeerRequests>(
        write_lock_, processed_request_box(), callback);
}

Editor<PeerReplies> Nym::mutable_ProcessedReplyBox()
{
    std::function<void(PeerReplies*, std::unique_lock<std::mutex>&)> callback =
        [&](PeerReplies* in, std::unique_lock<std::mutex>& lock) -> void {
        this->save(in, lock, StorageBox::PROCESSEDPEERREPLY);
    };

    return Editor<PeerReplies>(
        write_lock_, processed_reply_box(), callback);
}

PeerReplies* Nym::processed_reply_box() const
{
    std::unique_lock<std::mutex> lock(processed_reply_box_lock_);

    if (!processed_reply_box_) {
        processed_reply_box_.reset(
            new PeerReplies(storage_, migrate_, processed_peer_reply_));

        if (!processed_reply_box_) {
            std::cerr << __FUNCTION__ << ": Unable to instantiate."
                      << std::endl;
            abort();
        }
    }

    lock.unlock();

    return processed_reply_box_.get();
}

PeerRequests* Nym::processed_request_box() const
{
    std::unique_lock<std::mutex> lock(processed_request_box_lock_);

    if (!processed_request_box_) {
        processed_request_box_.reset(
            new PeerRequests(storage_, migrate_, processed_peer_request_));

        if (!processed_request_box_) {
            std::cerr << __FUNCTION__ << ": Unable to instantiate."
                      << std::endl;
            abort();
        }
    }

    lock.unlock();

    return processed_request_box_.get();
}

const PeerRequests& Nym::ProcessedRequestBox() const
{
    return *processed_request_box();
}

const PeerReplies& Nym::ProcessedReplyBox() const
{
    return *processed_reply_box();
}

bool Nym::save(const std::unique_lock<std::mutex>& lock)
{
    if (!verify_write_lock(lock)) {
        std::cerr << __FUNCTION__ << ": Lock failure." << std::endl;
        abort();
    }

    auto serialized = serialize();

    if (!proto::Check(serialized, version_, version_)) {
        return false;
    }

    return storage_.StoreProto(serialized, root_);
}

void Nym::save(
    PeerReplies* input,
    const std::unique_lock<std::mutex>& lock,
    StorageBox type)
{
    if (!verify_write_lock(lock)) {
        std::cerr << __FUNCTION__ << ": Lock failure." << std::endl;
        abort();
    }

    if (nullptr == input) {
        std::cerr << __FUNCTION__ << ": Null target" << std::endl;
        abort();
    }

    update_hash(type, input->Root());

    if (!save(lock)) {
        std::cerr << __FUNCTION__ << ": Save error" << std::endl;
        abort();
    }
}

void Nym::save(
    PeerRequests* input,
    const std::unique_lock<std::mutex>& lock,
    StorageBox type)
{
    if (!verify_write_lock(lock)) {
        std::cerr << __FUNCTION__ << ": Lock failure." << std::endl;
        abort();
    }

    if (nullptr == input) {
        std::cerr << __FUNCTION__ << ": Null target" << std::endl;
        abort();
    }

    update_hash(type, input->Root());

    if (!save(lock)) {
        std::cerr << __FUNCTION__ << ": Save error" << std::endl;
        abort();
    }
}

void Nym::update_hash(const StorageBox type, const std::string& root)
{
    switch (type) {
        case StorageBox::SENTPEERREQUEST: {
            std::lock_guard<std::mutex> lock(sent_request_box_lock_);
            sent_peer_request_ = root;
        } break;
        case StorageBox::INCOMINGPEERREQUEST: {
            std::lock_guard<std::mutex> lock(incoming_request_box_lock_);
            incoming_peer_request_ = root;
        } break;
        case StorageBox::SENTPEERREPLY: {
            std::lock_guard<std::mutex> lock(sent_reply_box_lock_);
            sent_peer_reply_ = root;
        } break;
        case StorageBox::INCOMINGPEERREPLY: {
            std::lock_guard<std::mutex> lock(incoming_reply_box_lock_);
            incoming_peer_reply_ = root;
        } break;
        case StorageBox::FINISHEDPEERREQUEST: {
            std::lock_guard<std::mutex> lock(finished_request_box_lock_);
            finished_peer_request_ = root;
        } break;
        case StorageBox::FINISHEDPEERREPLY: {
            std::lock_guard<std::mutex> lock(finished_reply_box_lock_);
            finished_peer_reply_ = root;
        } break;
        case StorageBox::PROCESSEDPEERREQUEST: {
            std::lock_guard<std::mutex> lock(finished_reply_box_lock_);
            processed_peer_request_ = root;
        } break;
        case StorageBox::PROCESSEDPEERREPLY: {
            std::lock_guard<std::mutex> lock(processed_reply_box_lock_);
            processed_peer_reply_ = root;
        } break;
        default: {
            std::cerr << __FUNCTION__ << ": Unknown box" << std::endl;
            abort();
        }
    }
}

PeerReplies* Nym::sent_reply_box() const
{
    std::unique_lock<std::mutex> lock(sent_reply_box_lock_);

    if (!sent_reply_box_) {
        sent_reply_box_.reset(
            new PeerReplies(storage_, migrate_, sent_peer_reply_));

        if (!sent_reply_box_) {
            std::cerr << __FUNCTION__ << ": Unable to instantiate."
                      << std::endl;
            abort();
        }
    }

    lock.unlock();

    return sent_reply_box_.get();
}

PeerRequests* Nym::sent_request_box() const
{
    std::unique_lock<std::mutex> lock(sent_request_box_lock_);

    if (!sent_request_box_) {
        sent_request_box_.reset(
            new PeerRequests(storage_, migrate_, sent_peer_request_));

        if (!sent_request_box_) {
            std::cerr << __FUNCTION__ << ": Unable to instantiate."
                      << std::endl;
            abort();
        }
    }

    lock.unlock();

    return sent_request_box_.get();
}

const PeerRequests& Nym::SentRequestBox() const { return *sent_request_box(); }

const PeerReplies& Nym::SentReplyBox() const { return *sent_reply_box(); }

proto::StorageNym Nym::serialize() const
{
    proto::StorageNym serialized;
    serialized.set_version(version_);
    serialized.set_nymid(nymid_);

    set_hash(version_, nymid_, credentials_, *serialized.mutable_credlist());
    set_hash(
        version_,
        nymid_,
        sent_peer_request_,
        *serialized.mutable_sentpeerrequests());
    set_hash(
        version_,
        nymid_,
        incoming_peer_request_,
        *serialized.mutable_incomingpeerrequests());
    set_hash(
        version_,
        nymid_,
        sent_peer_reply_,
        *serialized.mutable_sentpeerreply());
    set_hash(
        version_,
        nymid_,
        incoming_peer_reply_,
        *serialized.mutable_incomingpeerreply());
    set_hash(
        version_,
        nymid_,
        finished_peer_request_,
        *serialized.mutable_finishedpeerrequest());
    set_hash(
        version_,
        nymid_,
        finished_peer_reply_,
        *serialized.mutable_finishedpeerreply());
    set_hash(
        version_,
        nymid_,
        processed_peer_request_,
        *serialized.mutable_processedpeerrequest());
    set_hash(
        version_,
        nymid_,
        processed_peer_reply_,
        *serialized.mutable_processedpeerreply());

    return serialized;
}

bool Nym::SetAlias(const std::string& alias)
{
    std::lock_guard<std::mutex> lock(write_lock_);

    alias_ = alias;

    return true;
}

bool Nym::Store(
    const proto::CredentialIndex& data,
    const std::string& alias,
    std::string& plaintext)
{
    std::unique_lock<std::mutex> lock(write_lock_);

    const std::uint64_t revision = data.revision();
    bool saveOk = false;
    const bool incomingPublic = (proto::CREDINDEX_PUBLIC == data.mode());
    const bool existing = check_hash(credentials_);

    if (existing) {
        if (incomingPublic) {
            if (checked_.load()) {
                saveOk = !private_.load();
            } else {
                std::shared_ptr<proto::CredentialIndex> serialized;
                storage_.LoadProto(credentials_, serialized, true);
                saveOk = !private_.load();
            }
        } else {
            saveOk = true;
        }
    } else {
        saveOk = true;
    }

    const bool keyUpgrade = (!incomingPublic) && (!private_.load());
    const bool revisionUpgrade = revision > revision_.load();
    const bool upgrade = keyUpgrade || revisionUpgrade;

    if (saveOk) {
        if (upgrade) {
            const bool saved = storage_.StoreProto<proto::CredentialIndex>(
                data, credentials_, plaintext);

            if (!saved) {
                return false;
            }

            revision_.store(revision);

            if (!alias.empty()) {
                alias_ = alias;
            }
        }
    }

    checked_.store(true);
    private_.store(!incomingPublic);

    return save(lock);
}
}  // namespace storage
}  // namespace opentxs