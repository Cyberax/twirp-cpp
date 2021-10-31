#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <twirp/httplib/client-helper.h>
#include "service_client.hpp"

int main() {
    auto req = std::make_shared<trpc::HttplibRequester>("http://localhost:8080",
        trpc::ClientMiddlewares {
            trpc::SetHeaderMiddleware::make("Authorization", "PrettyPlease"),
    });

    twitch::twirp::example::HaberdasherClient cli(req, true);

    // Happy path
    auto size = twitch::twirp::example::Size();
    size.set_inches(32);
    auto res = cli.MakeHat(nullptr, nullptr, &size);
    if (!res.ok()) {
        std::cerr << "Error status: " << res.status() << std::endl;
        return 1;
    }
    printf("name = %s\n", res.value()->name().c_str());
    printf("color = %s\n", res.value()->color().c_str());
    printf("size = %d\n", res.value()->size());

    // Error path
    size = twitch::twirp::example::Size();
    size.set_inches(3200);
    res = cli.MakeHat(nullptr, nullptr, &size);
    if (res.ok()) {
        std::cerr << "Expected an error but got OK" << std::endl;
        return 1;
    }
    if (absl::IsOutOfRange(res.status())) {
        printf("Got expected result!\n");
    } else {
        std::cerr << "Unexpected error status: " << res.status() << std::endl;
    }
}
