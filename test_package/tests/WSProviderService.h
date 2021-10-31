#pragma once

#include "service1.pb.h"
#include "arena_helper.h"
#include <absl/status/statusor.h>
#include <set>
#include <span>

namespace weather {
    namespace detail {
        class context_t;
    }
    namespace gp = google::protobuf;

    class WSProviderService {
    public:
        virtual ~WSProviderService() = default;
        virtual absl::StatusOr<WeatherStation*> FindWeatherStation(gp::Arena *arena, detail::context_t *context,
            const WeatherStationId *req) = 0;
    };

    class WSProviderServiceHost {
        std::shared_ptr<WSProviderService> handler_;
    public:
        explicit WSProviderServiceHost(std::shared_ptr<WSProviderService> handler) :
            handler_(std::move(handler)) {}

        static std::string_view getServiceName() {
            return "weather.WSProvider";
        }
        static std::set<std::string_view> getMethods() {
            return {
                "FindWeatherStation",
                "DeleteWeatherStation",
                "UpdateWeatherStation"
            };
        }

        absl::StatusOr<std::string> invokeAndGetData(gp::Arena *arena,
            const std::string_view &method, const std::span<char> &argument1, bool json,
            detail::context_t *context);

        au::status_or_ptr<gp::Message> invokeMethod(gp::Arena *arena,
            const std::string_view &method, const std::span<char> &argument1, bool json,
            detail::context_t *context);
    };
}
