#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <twirp/httplib/client-helper.h>
#include "service_client.hpp"

int main() {
    // Create a requester that will be used by the client.
    auto req = std::make_shared<trpc::HttplibRequester>("http://localhost:8080");
    // And create a client that uses this requester in JSON mode
    twitch::twirp::example::HaberdasherClient cliBad(req, true);

    // Prepare parameters...
    auto size = twitch::twirp::example::Size();
    size.set_inches(32);
    // And run the method
    auto res = cliBad.MakeHat(nullptr, nullptr, &size);
    // This method will fail with Unauthenticated error, since we haven't specified
    // the required 'Authorization' header.
    if (res.ok() || !IsUnauthenticated(res.status())) {
        std::cerr << "Unexpected error status: " << res.status() << std::endl;
        return 1;
    }
    printf("Yay! Got an unauthenticated response!\n");

    // Happy path - with authentication. This requester uses a middleware that adds
    // the required HTTP header.
    req = std::make_shared<trpc::HttplibRequester>("http://localhost:8080",
        trpc::ClientMiddlewares {
            // Set the header!
            trpc::SetHeaderMiddleware::make("Authorization", "PrettyPlease"),
    });
    twitch::twirp::example::HaberdasherClient cli(req, false);

    // And this method will now succeed!
    res = cli.MakeHat(nullptr, nullptr, &size);
    if (!res.ok()) {
        std::cerr << "Error status: " << res.status() << std::endl;
        return 1;
    }
    printf("name = %s\n", res.value()->name().c_str());
    printf("color = %s\n", res.value()->color().c_str());
    printf("size = %d\n", res.value()->size());

    // This will now test the error path. The remote method can't accept sizes
    // that are more than 100 inches.
    size = twitch::twirp::example::Size();
    size.set_inches(3200);
    res = cli.MakeHat(nullptr, nullptr, &size);
    // So we get an OutOfRange error.
    if (!res.ok() && absl::IsOutOfRange(res.status())) {
        printf("Got expected result!\n");
    } else {
        std::cerr << "Unexpected error status: " << res.status() << std::endl;
        return 1;
    }
}
