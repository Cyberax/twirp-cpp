#pragma once

#include <memory>
#include <absl/status/statusor.h>

namespace au {

    template<class T> struct deleter
    {
        void operator()(T *ptr) const {
            if (ptr->GetArena()) {
                // Objects are deleted by the arena
                return;
            }
            delete ptr;
        }
    };

    template<class T> using owned_ptr = std::unique_ptr<T, deleter<T>>;
    template<class T> using status_or_ptr = absl::StatusOr<au::owned_ptr<T>>;
}
