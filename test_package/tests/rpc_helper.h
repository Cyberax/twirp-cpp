#pragma once

#include <absl/status/statusor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include "arena_helper.h"

namespace rpc_helper {

    namespace gp = google::protobuf;

    inline absl::StatusOr<std::string> serializeMessage(const gp::Message *req, bool toJson) {
        std::string res;
        if (toJson) {
            auto err = gp::util::MessageToJsonString(*req, &res);
            if (!err.ok()) {
                auto txt = std::string("Failed to serialize the request: ") + err.message().ToString();
                return absl::InvalidArgumentError(txt);
            }
        } else {
            bool ok = req->SerializeToString(&res);
            if (!ok) {
                return absl::InvalidArgumentError("Failed to serialize the request");
            }
        }
        return res;
    }

    template<class T> absl::StatusOr<au::owned_ptr<T>> deserializeMessage(
        gp::Arena *arena, const std::span<char> &data, bool json) {
        au::owned_ptr<T> resObj(gp::Arena::CreateMessage<T>(arena));

        if (data.size() >= INT_MAX) {
            return absl::OutOfRangeError("Argument size is too large");
        }

        if (json) {
            auto err = gp::util::JsonStringToMessage(
                gp::StringPiece(data.data(), (int)data.size()), resObj.get());
            if (!err.ok()) {
                return absl::InvalidArgumentError("Can't deserialize json result");
            }
        } else {
            bool ok = resObj->ParseFromString(gp::StringPiece(data.data(), (int)data.size()));
            if (!ok) {
                return absl::InvalidArgumentError("Can't deserialize binary result");
            }
        }

        return resObj;
    }
}
