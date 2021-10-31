#pragma once

#include <span>
#include <any>
#include <absl/status/statusor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <absl/container/node_hash_map.h>
#include <concepts>

namespace trpc {

namespace gp = google::protobuf;

static constexpr absl::string_view TwirpStatusKey = "twirp_status";

template<class T> struct ArenaDeleter
{
    void operator()(T *ptr) const {
        if (ptr->GetArena()) {
            // Objects are deleted by the arena
            return;
        }
        delete ptr;
    }
};

template<class T> using OwnedPtr = std::unique_ptr<T, ArenaDeleter<T>>;
template<class T> using StatusOrPtr = absl::StatusOr<OwnedPtr<T>>;

class Requester {
public:
    virtual ~Requester() = default;
    virtual absl::StatusOr<std::string> MakeRequest(gp::Arena *arena,
        void *context, const std::span<char> &data, bool json,
        std::string_view service, std::string_view method) = 0;
};

// A concept for the request context keys. The keys are used to store and
// retrieve context-bound data with type safety. A typical use-case is to set
// the authentication data in middleware and consume it inside the remote method
// implementation.
template<typename T> concept RequestContextKey = requires(T a) {
    typename T::ValueType; // The value type for the key
    requires std::is_move_assignable<typename T::ValueType>::value;
    requires std::is_copy_constructible<typename T::ValueType>::value;

    // There should be a constexpr field 'Name' of type std::string_view
    { T::Name };
    requires std::is_same<const std::string_view, decltype(T::Name)>::value;

    // There should be a static function called Default() that returns
    // the static reference to the default value. Typically it can return
    // a reference to a function-scoped static. This is thread-safe since
    // C++11, see "Magic Statics".
    // Example:
    // static const std::string& Default() {
    //     static std::string val("Hello);
    //     return val;
    // }
    { T::Default() } -> std::same_as<const typename T::ValueType&>;
};

class RequestContext {
    absl::node_hash_map<std::string_view, std::any> data_;
public:
    void ForEachItem(const std::function<void(std::string_view, const std::any&)>& visitor) const {
        for(const auto &k : data_) {
            visitor(k.first, k.second);
        }
    }

    template<class T> requires RequestContextKey<T> void Set(typename T::ValueType &&data) {
        data_[T::Name] = std::move(data);
    }

    template<class T> requires RequestContextKey<T> void Set(const typename T::ValueType &data) {
        data_[T::Name] = data;
    }

    template<class T> requires RequestContextKey<T>
    const typename T::ValueType& GetOrDef() const {
        auto pos = data_.find(T::Name);
        if (pos == data_.end()) {
            return T::Default();
        }
        return *std::any_cast<const typename T::ValueType>(&(pos->second));
    }

    template<class T> requires RequestContextKey<T>
        const typename T::ValueType* GetOrNull() const {
        auto pos = data_.find(T::Name);
        if (pos == data_.end()) {
            return nullptr;
        }
        return std::any_cast<const typename T::ValueType>(&(pos->second));
    }
};

class ServiceHostBase {
public:
    virtual ~ServiceHostBase() = default;

    virtual std::string_view GetServiceName() const = 0;
    virtual const std::set<std::string_view>& GetMethods() const = 0;

    virtual StatusOrPtr<gp::Message> Invoke(gp::Arena *arena,
        const std::string_view &method, const std::span<const char> &argument1, bool json,
        RequestContext *context) = 0;
};

inline absl::StatusOr<std::string> SerializeMessage(const gp::Message *req, bool toJson) {
    std::string res;
    if (toJson) {
        auto err = gp::util::MessageToJsonString(*req, &res);
        if (!err.ok()) {
            auto txt = std::string("Failed to serialize the request: ") + err.message().ToString();
            return absl::InvalidArgumentError(txt);
        }
    } else {
        bool ok = req->SerializeToString(&res);
        if (!ok) {
            return absl::InvalidArgumentError("Failed to serialize the request");
        }
    }
    return std::move(res);
}

template<class T> StatusOrPtr<T> DeserializeMessage(
    gp::Arena *arena, const std::span<const char> &data, bool json) {

    OwnedPtr<T> resObj(gp::Arena::CreateMessage<T>(arena));

    if (data.size() >= INT_MAX) {
        return absl::OutOfRangeError("Argument size is too large");
    }

    if (json) {
        auto err = gp::util::JsonStringToMessage(
            gp::StringPiece(data.data(), (int)data.size()), resObj.get());
        if (!err.ok()) {
            auto res = absl::Status(absl::StatusCode::kInvalidArgument,
                "Can't deserialize JSON request");
            res.SetPayload(TwirpStatusKey, absl::Cord("malformed"));
            return std::move(res);
        }
    } else {
        bool ok = resObj->ParseFromString(gp::StringPiece(data.data(), (int)data.size()));
        if (!ok) {
            auto res = absl::Status(absl::StatusCode::kInvalidArgument,
                "Can't deserialize binary request");
            res.SetPayload(TwirpStatusKey, absl::Cord("malformed"));
            return std::move(res);
        }
    }

    return resObj;
}

} // namespace trpc
