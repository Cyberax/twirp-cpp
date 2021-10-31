#include <span>
#include <utility>
#include <google/protobuf/util/json_util.h>
#include "WSProviderService.h"
#include "arena_helper.h"
#include "rpc_helper.h"

namespace gp = google::protobuf;

absl::StatusOr<std::string> weather::WSProviderServiceHost::invokeAndGetData(
    gp::Arena *arena, const std::string_view &method, const std::span<char> &argument1,
    bool json, weather::detail::context_t *context) {

    au::status_or_ptr<gp::Message> res = invokeMethod(arena, method, argument1, json, context);
    if (!res.ok()) {
        return res.status();
    }

    return rpc_helper::serializeMessage(res.value().get(), json);
}

au::status_or_ptr<gp::Message> weather::WSProviderServiceHost::invokeMethod(gp::Arena *arena,
    const std::string_view &method, const std::span<char> &argument1, bool json,
    weather::detail::context_t *context) {

    if (argument1.size() >= INT_MAX) {
        return absl::OutOfRangeError("Serialized object's size is out of range");
    }

    if (method == "FindWeatherStation") {
        absl::StatusOr<au::owned_ptr<WeatherStationId>> reqObj =
            rpc_helper::deserializeMessage<WeatherStationId>(arena, argument1, json);
        if (!reqObj.ok()) {
            return reqObj.status();
        }

        absl::StatusOr<WeatherStation*> res = handler_->FindWeatherStation(
            arena, context, reqObj.value().get());
        if (!res.ok()) {
            return res.status();
        }

        return au::status_or_ptr<gp::Message>(res.value());
    } else if (method == "DeleteWeatherStation") {

    } else if (method == "UpdateWeatherStation") {

    }

    return au::status_or_ptr<gp::Message>();
}
