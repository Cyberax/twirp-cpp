#pragma once

#include "service1.pb.h"
#include <absl/status/statusor.h>
#include <span>

namespace rpc {

    class Requester {
    public:
        virtual ~Requester() = default;
        virtual absl::StatusOr<std::string> makeRequest(void *context, const std::span<char> &data,
            bool json, std::string_view service, std::string_view method) = 0;
    };
}

namespace weather {
    namespace gp = google::protobuf;

    class WSProviderClient {
        std::shared_ptr<rpc::Requester> requester_;
        bool json_;
    public:
        WSProviderClient(std::shared_ptr<rpc::Requester> requester, bool json) :
            requester_(std::move(requester)), json_(json) {}

        absl::StatusOr<WeatherStation*> FindWeatherStation(gp::Arena *arena, void *context,
            const WeatherStationId *req) ;
    };
}
