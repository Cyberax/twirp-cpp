//
// Created by Besogonov Aleksei on 11/19/20.
//

#include <gtest/gtest.h>
#include "service1.pb.h"
#include "WSProviderService.h"
#include "WSProviderClient.h"
#include "arena_helper.h"

using namespace weather;

namespace weather::detail {
    class context_t {
    public:
        std::string req_data;
    };
};

class SimpleImpl : public WSProviderService {
public:
    absl::StatusOr<WeatherStation*> FindWeatherStation(gp::Arena *arena, detail::context_t *context,
        const WeatherStationId *req) override {

        au::owned_ptr<WeatherStation> res(gp::Arena::CreateMessage<WeatherStation>(arena));
        res->mutable_ws_id()->set_id(std::string("Reflected") + req->id());
        res->set_type(weather::WEATHER_STATION_SAT);
        res->set_contextdata(context->req_data);

        return res.release();
    };
};

class DirectRequester : public rpc::Requester {
    WSProviderServiceHost *delegate_;
public:
    explicit DirectRequester(WSProviderServiceHost *delegate) : delegate_(delegate) {}

    absl::StatusOr<std::string> makeRequest(void *context,
        const std::span<char> &data, bool json, std::string_view service,
        std::string_view method) override {

        auto arena = google::protobuf::Arena();
        detail::context_t ctx;
        ctx.req_data = "TestContextData";

        auto res = delegate_->invokeAndGetData(&arena, method,
            data, json, &ctx);
        return res;
    }
};

void test_good_path(gp::Arena *arena, bool json) {
    auto impl = std::make_shared<SimpleImpl>();
    WSProviderServiceHost host(impl);

    auto dr = std::make_shared<DirectRequester>(&host);
    WSProviderClient cli(dr, json);

    auto req = gp::Arena::CreateMessage<WeatherStationId>(arena);
    req->set_id("ThisIsAnId");
    auto res = cli.FindWeatherStation(arena, nullptr, req);

    EXPECT_EQ(true, res.ok());
    EXPECT_EQ("TestContextData", res.value()->contextdata());
    EXPECT_EQ("ReflectedThisIsAnId", res.value()->ws_id().id());
    EXPECT_EQ(weather::WEATHER_STATION_SAT, res.value()->type());
}

TEST(RpcTests, arena_protobuf) {
	auto arena = google::protobuf::Arena();
    test_good_path(&arena, false);
}

TEST(RpcTests, arena_json) {
    auto arena = google::protobuf::Arena();
    test_good_path(&arena, true);
}

TEST(RpcTests, no_arena_protobuf) {
    test_good_path(nullptr, false);
}

TEST(RpcTests, no_arena_json) {
    test_good_path(nullptr, true);
}
