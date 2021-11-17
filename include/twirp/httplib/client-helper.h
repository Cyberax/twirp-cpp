// This file contains the httplib-based implementation for the client infrastructure for a Twirp service.
// Please see the https://twitchtv.github.io/twirp/docs/spec_v7.html for details on Twirp specification
// and https://github.com/Cyberax/twirp-cpp/blob/development/example/cpp/tw-client.cpp for the usage example.
#pragma once

#include <httplib.h>
#include <twirp/rpc-defs.h>
#include <twirp/error-defs.h>
#include <json/json.h>

namespace trpc {

// Read and decode the Twirp error contained in the request. Returns the absl::Status with the
// corresponding absl::StatusCode and preserves the additional metadata payloads.
// See https://twitchtv.github.io/twirp/docs/spec_v7.html for details on error encoding.
inline absl::Status DecodeError(const httplib::Response &response) {
    // An error. We must have application/json data for Twirp errors
    auto ct = response.get_header_value("content-type");
    if (ct != "application/json") {
        return absl::UnavailableError("Expected 'application/json' content type for error data");
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    auto reader = std::unique_ptr<Json::CharReader>(builder.newCharReader());

    bool ok = reader->parse(response.body.data(),
        response.body.data()+response.body.length(), &root, nullptr);
    if (!ok) {
        return absl::UnavailableError("Received malformed JSON error response");
    }

    auto errCode = root["code"];
    if (errCode.empty() || !errCode.isString()) {
        return absl::UnavailableError("Expected 'code' entry");
    }
    auto errMsg = root["msg"];
    if (errMsg.empty() || !errMsg.isString()) {
        return absl::UnavailableError("Expected 'msg' entry");
    }
    absl::Status res(trpc::ErrorCodeToStatus(errCode.asString()), errMsg.asString());

    auto errMeta = root["meta"];
    if (!errMeta.empty() && errMeta.isObject()) {
        // Deconstruct meta
        for(Json::ValueIterator itr = errMeta.begin() ; itr != errMeta.end() ; itr++) {
            if (!itr->isString()) {
                return absl::UnavailableError("Only strings are accepted as metadata");
            }
            res.SetPayload(itr.key().asString(), absl::Cord(itr->asString()));
        }
    }

    return res;
}

// The pure virtual base class for client middleware, typically used to set additional
// authentication data.
class ClientMiddleware {
public:
    virtual ~ClientMiddleware() = default;
    // This method is called just before the request is ready to be sent.
    // arena - optional arena that can be used for the request-specific allocations. It's specified by the client,
    // so it might be nullptr.
    // and alive for the duration of the request.
    // context - pointer to user-specified data
    // data - the serialized request
    // json - the flag that specifies the request encoding (binary or JSON)
    // service - the service name
    // method - the method name
    // headers - the HTTP headers for request, can be mutated within this method
    // return - any status but StatusOk() will stop further processing and will be returned to the caller
    virtual absl::Status Handle(gp::Arena *arena, void *context, const std::span<char> &data,
        bool json, std::string_view service, std::string_view method, httplib::Headers *headers) = 0;
};

typedef std::vector<std::shared_ptr<ClientMiddleware>> ClientMiddlewares;

// Middleware that sets the specified HTTP header
class SetHeaderMiddleware : public ClientMiddleware {
    std::string name_, value_;
public:
    // name - the header name
    // value - the header value
    SetHeaderMiddleware(std::string &&name, std::string &&value) :
        name_(std::move(name)), value_(std::move(value)) {};

    static std::shared_ptr<SetHeaderMiddleware> make(std::string &&name, std::string &&value) {
        return std::make_shared<SetHeaderMiddleware>(std::move(name), std::move(value));
    }

    absl::Status Handle(gp::Arena *arena, void *context, const std::span<char> &data,
        bool json, std::string_view service, std::string_view method, httplib::Headers *headers) override {
        headers->insert(std::make_pair(name_, value_));
        return absl::OkStatus();
    }
};

// Implementation of trpc::Requester that uses the httplib
class HttplibRequester : public trpc::Requester {
    httplib::Client client_;
    ClientMiddlewares middlewares_;
public:
    // Create the requester using the specified httplib client. This client needs to have the base URL for the
    // service and any other settings you with to use (e.g. custom SSL CA storage). The base URL needs to have
    // the schema and the path set, but not the "twirp/" suffix. E.g.: "https://handler.someservice.com"
    // middlewares - can be used to customize the request before it's sent
    HttplibRequester(httplib::Client &&client, ClientMiddlewares &&middlewares = ClientMiddlewares()) :
        client_(std::move(client)), middlewares_(std::move(middlewares)){}

    // Create the requester using the specified base URL client. The base URL needs to have
    // the schema and the path set, but not the "twirp/" suffix. E.g.: "https://handler.someservice.com"
    // middlewares - can be used to customize the request before it's sent
    HttplibRequester(const std::string &url, ClientMiddlewares &&middlewares = ClientMiddlewares()) :
        client_(url), middlewares_(std::move(middlewares)) {}

    HttplibRequester(const HttplibRequester&) = delete; // non construction-copyable
    HttplibRequester& operator = (const HttplibRequester&) = delete; // non copyable

    // Implements the requester interface
    absl::StatusOr<std::string> MakeRequest(gp::Arena *arena, void *context, const std::span<char> &data,
        bool json, std::string_view service, std::string_view method) override {

        httplib::Headers headers;

        std::string url = std::string("/twirp/");
        url += service;
        url += "/";
        url += method;

        for(const auto &m : middlewares_) {
            // Will likely mutate the headers
            auto st = m->Handle(arena, context, data, json, service, method, &headers);
            if (!st.ok()) {
                return st;
            }
        }

        auto res = client_.Post(url.c_str(), headers, data.data(), data.size(),
            json ? "application/json" : "application/protobuf");
        if (!res) {
            // Return the error
            return absl::UnavailableError(to_string(res.error()));
        }

        const httplib::Response &response = res.value();
        if (response.status != 200) {
            return DecodeError(response);
        }

        return response.body;
    }
};

} // namespace trpc
