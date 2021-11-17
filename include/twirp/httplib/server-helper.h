#pragma once

#include <twirp/rpc-defs.h>
#include <twirp/error-defs.h>
#include <httplib.h>
#include <json/json.h>

namespace trpc {

constexpr auto MalformedError = std::make_pair("malformed",	400);
constexpr auto BadRouteError = std::make_pair("bad_route", 404);

class ServerMiddleware {
public:
    virtual ~ServerMiddleware() = default;
    virtual absl::Status Handle(gp::Arena *arena, trpc::RequestContext *ctx, bool json,
        const httplib::Request &request, httplib::Response &response) = 0;
};

typedef std::vector<std::shared_ptr<ServerMiddleware>> ServerMiddlewares;

inline void SendError(const std::pair<std::string,int> &err, const std::string &msg, httplib::Response &resp) {
    Json::Value value;
    value["code"] = err.first;
    value["msg"] = msg.empty() ? err.first : msg;

    Json::StreamWriterBuilder builder;
    const std::string jsonError = Json::writeString(builder, value);
    resp.status = err.second;
    resp.set_content(jsonError, "application/json");
}

inline void SendError(const absl::Status &status, httplib::Response &resp) {
    Json::Value value;
    Json::Value meta;

    bool hasCodeFromPayload = false;
    status.ForEachPayload([&](absl::string_view nm, const absl::Cord& data) {
        std::string out;
        CopyCordToString(data, &out);
        if (nm == trpc::TwirpStatusKey) {
            resp.status = ErrorCodeToHttpStatus(out);
            value["code"] = out;
            hasCodeFromPayload = true;
        } else {
            meta[std::string(nm)] = out;
        }
    });
    if (!hasCodeFromPayload) {
        auto translated = StatusToErrorCode(status.code());
        value["code"] = std::string(translated.first);
        resp.status = translated.second;
    }
    value["msg"] = std::string(status.message());
    if (!meta.empty()) {
        value["meta"] = meta;
    }

    Json::StreamWriterBuilder builder;
    const std::string jsonError = Json::writeString(builder, value);
    resp.set_content(jsonError, "application/json");
}

inline void RegisterTwirpHandlers(trpc::ServiceHostBase *handler, httplib::Server *srv,
    const ServerMiddlewares &middleware) {

    for (const auto &meth : handler->GetMethods()) {
        std::string pattern = "/twirp/";
        pattern += handler->GetServiceName();
        pattern += "/";
        pattern += meth;
        srv->Post(pattern, [handler, meth, middleware](
            const httplib::Request &req, httplib::Response &res) {

            auto ct = req.get_header_value("content-type");
            bool json;
            if (ct == "application/json") {
                json = true;
            } else if (ct == "application/protobuf") {
                json = false;
            } else {
                return SendError(MalformedError, "Unknown message encoding", res);
            }

            auto arena = std::make_unique<gp::Arena>();
            trpc::RequestContext ctx;

            // Run middlewares
            for (auto &m : middleware) {
                auto status = m->Handle(arena.get(), &ctx, json, req, res);
                if (!status.ok()) {
                    return SendError(status, res);
                }
            }

            auto methodResult = handler->Invoke(arena.get(), meth,
                std::span(req.body.c_str(), req.body.size()), json, &ctx);
            if (!methodResult.ok()) {
                return SendError(methodResult.status(), res);
            }

            //TODO: use zero-copy to avoid two separate copies here.
            absl::StatusOr<std::string> data = trpc::SerializeMessage(methodResult.value().get(), json);
            if (!data.ok()) {
                return SendError(data.status(), res);
            }

            res.status = 200;
            res.set_content(data.value(), json ? "application/json" : "application/protobuf");
        });

        std::string anyPattern = "/twirp/";
        anyPattern += handler->GetServiceName();
        anyPattern += "/.*";
        srv->Post(anyPattern, [](const httplib::Request &req, httplib::Response &res) {
            SendError(BadRouteError, "Method not found", res);
        });
    }
}

} // namespace trpc
