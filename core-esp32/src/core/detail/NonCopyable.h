//
// Created by Ivan Kishchenko on 16/4/24.
//

#pragma once

namespace detail {
    struct base_token {
    };

    class NonCopyable : base_token {
        protected:
        NonCopyable() = default;

        public:
        NonCopyable(const NonCopyable &) = delete;

        NonCopyable &operator=(const NonCopyable &) = delete;
    };
}

