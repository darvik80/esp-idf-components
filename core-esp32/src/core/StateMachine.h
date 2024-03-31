//
// Created by Ivan Kishchenko on 28/3/24.
//

#pragma once

#include <tuple>
#include <variant>
#include <functional>

template<typename State>
struct TransitionTo {
    template<typename Machine>
    void execute(Machine &machine) {
        machine.template transitionTo<State>();
    }
};

struct Nothing {
    template<typename Machine>
    void execute(Machine &) {
    }
};

namespace detail {

        template<typename...>
        struct is_one_of : std::false_type {
        };
        template<typename F, typename... T>
        struct is_one_of<F, F, T...> : std::true_type {
        };
        template<typename F, typename S, typename... T>
        struct is_one_of<F, S, T...> : is_one_of<F, T...> {
        };

        template<typename ClassType, typename ArgType, typename... ReturnTypes>
        class has_handle {
            template<typename U, typename = std::enable_if_t<is_one_of<decltype(std::declval<U>().handle(
                    std::declval<ArgType>())), Nothing, TransitionTo<ReturnTypes>...>::value>>
            static std::true_type test(int);

            template<typename>
            static std::false_type test(...);

            public:
            static constexpr bool value = decltype(test<ClassType>(0))::value;
        };
}

template<typename... States>
class StateMachine {
        public:
        template<typename State>
        void transitionTo() {
            currentState = &std::get<State>(states);
        }

        template<typename Event>
        void handle(const Event &event) {
            auto passEventToState = [this, &event](auto statePtr) {
                if constexpr (detail::has_handle<decltype(*statePtr), Event, States...>::value) {
                    statePtr->handle(event).execute(*this);
                }
            };

            std::visit(passEventToState, currentState);
        }

        private:
        std::tuple<States...> states;
        std::variant<States *...> currentState{&std::get<0>(states)};
};
