#pragma once

#include <absl/status/statusor.h>

namespace trpc {

struct ErrCodeEntry {
    std::string_view errCode_;
    int httpCode_;
    absl::StatusCode code_;
};

// Mapping of errors to absl::StatusCode as defined in https://twitchtv.github.io/twirp/docs/spec_v7.html
// The mapping is almost one-to-one except for 'malformed' and 'bad_route' codes used in the routing
// code in the server implementation.
constexpr ErrCodeEntry CodeMap[] {
    // The operation was cancelled.
    {"canceled", 408, absl::StatusCode::kCancelled},
    // An unknown error occurred. For example, this can be used when handling errors
    // raised by APIs that do not return any error information.
    {"unknown", 500, absl::StatusCode::kUnknown},
    // The client specified an invalid argument. This indicates arguments that are
    // invalid regardless of the state of the system (i.e. a malformed file name,
    // required argument, number out of range, etc.).
    {"invalid_argument", 400, absl::StatusCode::kInvalidArgument},
    // The client sent a message which could not be decoded. This may mean that
    // the message was encoded improperly or that the client and server have incompatible
    // message definitions.
    {"malformed", 400, absl::StatusCode::kInvalidArgument}, // Internal error, used in Twirp server code
    // Operation expired before completion. For operations that change the state of the
    // system, this error may be returned even if the operation has completed successfully (timeout).
    {"deadline_exceeded", 408, absl::StatusCode::kDeadlineExceeded},
    // Some requested entity was not found.
    {"not_found", 404, absl::StatusCode::kNotFound},
    // The requested URL path wasn't routable to a Twirp service and method. This is returned
    // by generated server code and should not be returned by application code (use "not_found"
    // or "unimplemented" instead).
    {"bad_route", 404, absl::StatusCode::kUnimplemented}, // Internal error, used in Twirp server code
    // An attempt to create an entity failed because one already exists.
    {"already_exists", 409, absl::StatusCode::kAlreadyExists},
    // The caller does not have permission to execute the specified operation. It must
    // not be used if the caller cannot be identified (use "unauthenticated" instead).
    {"permission_denied", 403, absl::StatusCode::kPermissionDenied},
    // The request does not have valid authentication credentials for the operation.
    {"unauthenticated", 401, absl::StatusCode::kUnauthenticated},
    // Some resource has been exhausted or rate-limited, perhaps a per-user quota,
    // or perhaps the entire file system is out of space.
    {"resource_exhausted", 429, absl::StatusCode::kResourceExhausted},
    // The operation was rejected because the system is not in a state required for
    // the operation's execution. For example, doing an rmdir operation on a directory
    // that is non-empty, or on a non-directory object, or when having conflicting
    // read-modify-write on the same resource.
    {"failed_precondition", 412, absl::StatusCode::kFailedPrecondition},
    // The operation was aborted, typically due to a concurrency issue like sequencer
    // check failures, transaction aborts, etc.
    {"aborted", 409, absl::StatusCode::kAborted},
    // The operation was attempted past the valid range. For example, seeking or
    // reading past end of a paginated collection. Unlike "invalid_argument",
    // this error indicates a problem that may be fixed if the system state
    // changes (i.e. adding more items to the collection). There is a fair bit of
    // overlap between "failed_precondition" and "out_of_range".
    // We recommend using "out_of_range" (the more specific error) when it applies
    // so that callers who are iterating through a space can easily look for an
    // "out_of_range" error to detect when they are done.
    {"out_of_range", 400, absl::StatusCode::kOutOfRange},
    // The operation is not implemented or not supported/enabled in this service.
    {"unimplemented", 501, absl::StatusCode::kUnimplemented},
    // When some invariants expected by the underlying system have been broken.
    // In other words, something bad happened in the library or backend service.
    // Twirp specific issues like wire and serialization problems are also reported as
    // "internal" errors.
    {"internal", 500, absl::StatusCode::kInternal},
    // The service is currently unavailable. This is most likely a transient condition
    // and may be corrected by retrying with a backoff.
    {"unavailable", 503, absl::StatusCode::kUnavailable},
    // The operation resulted in unrecoverable data loss or corruption.
    {"dataloss", 500, absl::StatusCode::kDataLoss},
};

// Convert a text error code (e.g. 'unavailable') into the corresponding absl::StatusCode.
inline absl::StatusCode ErrorCodeToStatus(const std::string_view errCode) {
    for(const auto & ent : CodeMap) {
        if (ent.errCode_ == errCode) {
            return ent.code_;
        }
    }
    return absl::StatusCode::kUnknown;
}

// Convert an absl::StatusCode into a tuple of Twirp error code and the HTTP status code.
// All absl::StatusCode entries have mapping. Unknown status codes are mapped to ("unknown", 500).
inline std::pair<std::string_view, int> StatusToErrorCode(const absl::StatusCode &status) {
    for(const auto & ent : CodeMap) {
        if (ent.code_ == status) {
            return std::make_pair(ent.errCode_, ent.httpCode_);
        }
    }
    return std::make_pair("unknown", 500);
}

// Convert a text error code (e.g. 'unavailable') into the corresponding HTTP status code.
inline int ErrorCodeToHttpStatus(const std::string_view errCode) {
    for(const auto & ent : CodeMap) {
        if (ent.errCode_ == errCode) {
            return ent.httpCode_;
        }
    }
    return 500;
}

} // namespace trpc
