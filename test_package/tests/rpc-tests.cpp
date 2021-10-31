//
// Created by Besogonov Aleksei on 11/19/20.
//

#include <gtest/gtest.h>
#include "service1.pb.h"
#include "service1_server.hpp"
#include "service1_client.hpp"

using namespace weather;

struct AuthData {
    typedef std::string ValueType;
    static constexpr std::string_view Name = "AuthData";

    static const std::string& Default() {
        static std::string res = "NonePresent";
        return res;
    }
};
static_assert(trpc::RequestContextKey<AuthData>);

class SimpleImpl : public WSProviderService {
public:
    absl::StatusOr<WeatherStation*> FindWeatherStation(gp::Arena *arena, trpc::RequestContext *context,
        const WeatherStationId *req) override {

        if (req->id() == "InjectError") {
            auto st = absl::DataLossError("We lost it!");
            st.SetPayload("details", absl::Cord("a long explanation"));
            return st;
        }

        trpc::OwnedPtr<WeatherStation> res(gp::Arena::CreateMessage<WeatherStation>(arena));
        res->mutable_ws_id()->set_id(std::string("Reflected") + req->id());
        res->set_type(weather::WEATHER_STATION_SAT);
        res->set_contextdata(context->GetOrDef<AuthData>());

        return res.release();
    };

    absl::StatusOr<weather::WeatherStation *> DeleteWeatherStation(
        gp::Arena *arena, trpc::RequestContext *context,
        const weather::WeatherStationId *req) override {
        return absl::UnimplementedError("");
    }

    absl::StatusOr<weather::WeatherStationId *> UpdateWeatherStation(
        gp::Arena *arena, trpc::RequestContext *context,
        const weather::WeatherStation *req) override {
        return absl::UnimplementedError("");
    }
};

class DirectRequester : public trpc::Requester {
    WSProviderServiceHost *delegate_;
public:
    explicit DirectRequester(WSProviderServiceHost *delegate) : delegate_(delegate) {}

    absl::StatusOr<std::string> MakeRequest(gp::Arena *arena, void *context,
        const std::span<char> &data, bool json, std::string_view service,
        std::string_view method) override {

        trpc::RequestContext ctx;
        ctx.Set<AuthData>("TestContextData");

        auto res = delegate_->Invoke(arena, method, data, json, &ctx);
        if (!res.ok()) {
            return res.status();
        }
        return trpc::SerializeMessage(res.value().get(), json);
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

void test_error(gp::Arena *arena, bool json) {
    auto impl = std::make_shared<SimpleImpl>();
    WSProviderServiceHost host(impl);

    auto dr = std::make_shared<DirectRequester>(&host);
    WSProviderClient cli(dr, json);

    auto req = gp::Arena::CreateMessage<WeatherStationId>(arena);
    req->set_id("InjectError");
    auto res = cli.FindWeatherStation(arena, nullptr, req);

    EXPECT_EQ(false, res.ok());
    EXPECT_EQ(true, IsDataLoss(res.status()));
    EXPECT_EQ("a long explanation", res.status().GetPayload("details").value().Flatten());
}

TEST(RpcTests, error_arena_protobuf) {
    auto arena = google::protobuf::Arena();
    test_error(&arena, false);
}

TEST(RpcTests, error_arena_json) {
    auto arena = google::protobuf::Arena();
    test_error(&arena, true);
}

TEST(RpcTests, error_no_arena_protobuf) {
    test_error(nullptr, false);
}

TEST(RpcTests, error_no_arena_json) {
    test_error(nullptr, true);
}
