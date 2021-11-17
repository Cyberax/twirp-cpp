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

// absl::Status payload key that you can set to override the returned Twirp error code.
static constexpr absl::string_view TwirpStatusKey = "twirp_status";

// Deleter for the objects that MIGHT be allocated inside an arena. It does nothing for these objects
// while deleting normally-allocated objects with `delete`.
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

// A variant of unique_ptr that is safe to use with possibly arena-allocated objects
template<class T> using OwnedPtr = std::unique_ptr<T, ArenaDeleter<T>>;

// Alias for absl::StatusOr<trpc::OwnerPtr<T>>
template<class T> using StatusOrPtr = absl::StatusOr<OwnedPtr<T>>;

// Interface for the HTTP (or other) request handlers.
class Requester {
public:
    virtual ~Requester() = default;

    // Make a request.
    // arena - optional arena for the intermediate objects. Arena is guaranteed to stay alive
    // until the method returns.
    // context - user-specified context pointer.
    // data - request body.
    // json - a flag indicating if the body is JSON-encoded or is Protobuf-binary
    // service - the service name for the request
    // method - the method name within the service
    // returns: error status or response body (JSON or Protobuf encoded depending on the `json` flag)
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

// A storage for request-specific information, its primarily used to pass authentication
// data from middleware to service methods.
class RequestContext {
    absl::node_hash_map<std::string_view, std::any> data_;
public:
    // Enumerate the stored data, the key order is undefined.
    // Return "false" from the visitor to stop the enumeration.
    void ForEachItem(const std::function<bool(std::string_view, const std::any&)>& visitor) const {
        for(const auto &k : data_) {
            if (!visitor(k.first, k.second)) {
                break;
            }
        }
    }

    // Set the data for the specified RequestContextKey.
    template<class T> requires RequestContextKey<T> void Set(typename T::ValueType &&data) {
        data_[T::Name] = std::move(data);
    }

    // Set the data for the specified RequestContextKey.
    template<class T> requires RequestContextKey<T> void Set(const typename T::ValueType &data) {
        data_[T::Name] = data;
    }

    // Get the const reference to the data for for the specified RequestContextKey. If there's no data
    // available then a reference to the default value inside the RequestContextKey is returned.
    // The reference will remain valid for the duration of the request, it survives mutations of
    // RequestContext that don't involve the specified RequestContextKey.
    template<class T> requires RequestContextKey<T> const typename T::ValueType& GetOrDef() const {
        auto pos = data_.find(T::Name);
        if (pos == data_.end()) {
            return T::Default();
        }
        return *std::any_cast<const typename T::ValueType>(&(pos->second));
    }

    // Get the const reference to the data for for the specified RequestContextKey. If there's no data
    // available then nullptr is returned.
    // The pointer will remain valid for the duration of the request, it survives mutations of
    // RequestContext that don't involve the specified RequestContextKey.
    template<class T> requires RequestContextKey<T>
        const typename T::ValueType* GetOrNull() const {
        auto pos = data_.find(T::Name);
        if (pos == data_.end()) {
            return nullptr;
        }
        return std::any_cast<const typename T::ValueType>(&(pos->second));
    }
};

// The pure virtual class representing the service host. Implementations of it are generated by the
// `protoc-gen-twirpcpp` generator based on the Protobuf schema.
class ServiceHostBase {
public:
    virtual ~ServiceHostBase() = default;

    virtual std::string_view GetServiceName() const = 0;
    virtual const std::set<std::string_view>& GetMethods() const = 0;

    virtual StatusOrPtr<gp::Message> Invoke(gp::Arena *arena,
        const std::string_view &method, const std::span<const char> &argument1, bool json,
        RequestContext *context) = 0;
};

// Serialize the given message to std::string. The encoding is either binary protobuf or
// json protobuf, depending on the `toJson` flag.
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

// Deserialize the given protobuf message of type `T`. `json` specifies the encoding
// of the message (protobuf binary or protobuf JSON). The arena is optional and can be `nullptr`.
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
