#include <google/protobuf/util/json_util.h>
#include "WSProviderClient.h"
#include "arena_helper.h"
#include "rpc_helper.h"

namespace gp = google::protobuf;

absl::StatusOr<weather::WeatherStation*> weather::WSProviderClient::FindWeatherStation(
    gp::Arena *arena, void *context,
    const weather::WeatherStationId *req) {

    absl::StatusOr<std::string> msg = rpc_helper::serializeMessage(req, json_);
    if (!msg.ok()) {
        return msg.status();
    }

    // Invoke the remote side!
    absl::StatusOr<std::string> result = requester_->makeRequest(context, msg.value(), json_,
        "weather.WSProvider", "FindWeatherStation");
    if (!result.ok()) {
        return result.status();
    }

    absl::StatusOr<au::owned_ptr<weather::WeatherStation>> res =
        rpc_helper::deserializeMessage<WeatherStation>(arena,result.value(), json_);
    if (!res.ok()) {
        return res.status();
    }

    return res.value().release();
}
