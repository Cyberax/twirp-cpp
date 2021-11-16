#include <twirp/httplib/server-helper.h>
#include "service_server.hpp"

// This is an example of a piece of data that is stored in a request context
// and passed from the middleware to the actual service methods.
struct Principal {
    bool is_anonymous_;
    std::string username_;
};

// The key used to store/retrieve the `Principal` objects from the request context.
struct AuthenticationData {
    // The `AuthenticationData` stores `Principal` objects
    typedef Principal ValueType;
    // This is the key name, it can be arbitrary (as long as it's unique)
    static constexpr std::string_view Name = "AuthData";

    // This method returns a reference to the default value. The reference
    // is needed to avoid copies and allocations.
    static const Principal& Default() {
        // This is a static variable that is initialized only once during the
        // first access. It's thread safe since C++11.
        static Principal def {
            .is_anonymous_ = true,
            .username_ = "anonymous"
        };
        return def;
    }

    // Make sure that the key correctly implements the `trpc::RequestContextKey` concept.
    static_assert(trpc::RequestContextKey<AuthenticationData>);
};

// Simple authentication middleware. In reality, you'd probably use some kind of token
// validation here.
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

// Service implementation!
class SimpleImpl : public twitch::twirp::example::HaberdasherService {
public:
    absl::StatusOr<twitch::twirp::example::Hat *> MakeHat(
        google::protobuf::Arena *arena, trpc::RequestContext *context,
        const twitch::twirp::example::Size *req) override {

        // We retrieve the authentication data (or get the default). This is
        // type-safe, and there are no allocations or casts here!
        const Principal &principal = context->GetOrDef<AuthenticationData>();
        if (principal.is_anonymous_) {
            return absl::UnauthenticatedError("Anonymous requests are not allowed");
        }

        // Simple example of error handling
        if (req->inches() > 100) {
            return absl::OutOfRangeError("Index size is out of range");
        }

        // You don't have to use the arena, the Twirp server code will always make sure to
        // deallocate the result correctly, even if it's on the regular heap.
        auto hat = google::protobuf::Arena::CreateMessage<twitch::twirp::example::Hat>(arena);
        hat->set_size(req->inches());
        hat->set_color("beige");
        hat->set_name("Magic Hat");

        return hat;
    }
};

int main() {
    httplib::Server srv;
    srv.set_logger([](const httplib::Request &req, const httplib::Response &resp) -> void {
        printf("Request received: %s, resp=%d\n", req.path.c_str(), resp.status);
    });

    // Create the service implementation
    auto impl = std::make_shared<SimpleImpl>();
    // Create the host (it's an adapter for the service impl)
    twitch::twirp::example::HaberdasherServiceHost host(impl);

    // Register the Twirp service handlers in the HTTP service
    trpc::RegisterTwirpHandlers(&host, &srv,trpc::ServerMiddlewares{
        // Add middleware here. We have the authentication middleware only for now.
        std::make_shared<AuthenticationMiddleware>(),
    });

    // And start listening!
    srv.listen("0.0.0.0", 8080);
}
