#include <twirp/httplib/server-helper.h>
#include "service_server.hpp"

struct Principal {
    bool is_anonymous_;
    std::string username_;
};

struct AuthenticationData {
    typedef Principal ValueType;
    static constexpr std::string_view Name = "AuthData";

    static const Principal& Default() {
        // Thread safe since C++11
        static Principal def {
            .is_anonymous_ = true,
            .username_ = "anonymous"
        };
        return def;
    }
};
static_assert(trpc::RequestContextKey<AuthenticationData>);

class AuthenticationMiddleware : public trpc::ServerMiddleware {
public:
    absl::Status Handle(google::protobuf::Arena *arena, trpc::RequestContext *ctx, bool json,
        const httplib::Request &request, httplib::Response &response) override {

        auto auth = request.get_header_value("Authorization");
        if (auth == "PrettyPlease") {
            Principal p {
                .is_anonymous_ = false,
                .username_ = "VeryImportantUser"
            };
            ctx->Set<AuthenticationData>(std::move(p));
        }

        return absl::OkStatus();
    }
};

class SimpleImpl : public twitch::twirp::example::HaberdasherService {
public:
    absl::StatusOr<twitch::twirp::example::Hat *> MakeHat(
        google::protobuf::Arena *arena, trpc::RequestContext *context,
        const twitch::twirp::example::Size *req) override {

        const Principal &principal = context->GetOrDef<AuthenticationData>();
        if (principal.is_anonymous_) {
            return absl::UnauthenticatedError("Anonymous requests are not allowed");
        }

        if (req->inches() > 100) {
            return absl::OutOfRangeError("Index size is out of range");
        }

        auto hat = google::protobuf::Arena::CreateMessage<twitch::twirp::example::Hat>(arena);
        hat->set_size(req->inches());
        hat->set_color("beige");
        hat->set_name("Magic Hat");

        return hat;
    }
};

int main() {
    httplib::Server srv;

    auto impl = std::make_shared<SimpleImpl>();
    twitch::twirp::example::HaberdasherServiceHost host(impl);
    trpc::RegisterTwirpHandlers(&host, &srv,trpc::ServerMiddlewares{
        std::make_shared<AuthenticationMiddleware>(),
    });

    srv.set_logger([](const httplib::Request &req, const httplib::Response &resp) -> void {
        printf("Request received: %s, resp=%d\n", req.path.c_str(), resp.status);
    });
    srv.listen("0.0.0.0", 8080);
}
