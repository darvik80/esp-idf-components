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
    TransitionTo& execute(Machine &machine) {
        machine.template transitionTo<State>();
        return *this;
    }
};

struct Nothing {
    template<typename Machine>
    Nothing& execute(Machine &) {
        return *this;
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

template<typename T, typename... States>
class StateMachine {
public:
    template<typename State>
    void transitionTo() {
        prevState = currentState;
        currentState = &std::get<State>(states);
    }

    template<typename Event>
    void handle(const Event &event) {
        auto passEventToState = [this, &event](auto statePtr) {
            if constexpr (detail::has_handle<decltype(*statePtr), Event, States...>::value) {
            static_cast<T*>(this)->onStateChanged(statePtr->handle(event).execute(*this));
        }
        };

        std::visit(passEventToState, currentState);
    }

    template<typename Event, typename Ctx>
    void handle(const Event &event, const Ctx &ctx) {
        auto passEventToState = [this, &event, &ctx](auto statePtr) {
            if constexpr (detail::has_handle<decltype(*statePtr), Event, States...>::value) {
                static_cast<T *>(this)->onStateChanged(statePtr->handle(event).execute(*this), ctx);
            }
        };

        std::visit(passEventToState, currentState);
    }

    const std::variant<States *...> &getCurrentState() const {
        return currentState;
    }
    const std::variant<States *...> &getPrevState() const {
        return prevState;
    }
private:
    std::tuple<States...> states;
    std::variant<States *...> currentState{&std::get<0>(states)};
    std::variant<States *...> prevState{&std::get<0>(states)};
};

template<uint8_t event>
struct StateEvent {
};

template<typename E1, typename S1, typename E2 = void, typename S2 = void, typename E3 = void, typename S3 = void, typename E4 = void, typename S4 = void, typename E5 = void, typename S5 = void>
struct State {
    [[nodiscard]] TransitionTo<S1> handle(const E1 &) const {
        return {};
    }
    [[nodiscard]] TransitionTo<S2> handle(const E2 &) const {
        return {};
    }
    [[nodiscard]] TransitionTo<S3> handle(const E3 &) const {
        return {};
    }
    [[nodiscard]] TransitionTo<S4> handle(const E4 &) const {
        return {};
    }
    [[nodiscard]] TransitionTo<S5> handle(const E5 &) const {
        return {};
    }
};

template<typename E1, typename S1, typename E2, typename S2, typename E3, typename S3, typename E4, typename S4>
struct State<E1, S1, E2, S2, E3, S3, E4, S4> {
    [[nodiscard]] TransitionTo<S1> handle(const E1 &) const {
        return {};
    }
    [[nodiscard]] TransitionTo<S2> handle(const E2 &) const {
        return {};
    }
    [[nodiscard]] TransitionTo<S3> handle(const E3 &) const {
        return {};
    }
    [[nodiscard]] TransitionTo<S4> handle(const E4 &) const {
        return {};
    }
};

template<typename E1, typename S1, typename E2, typename S2, typename E3, typename S3>
struct State<E1, S1, E2, S2, E3, S3> {
    [[nodiscard]] TransitionTo<S1> handle(const E1 &) const {
        return {};
    }
    [[nodiscard]] TransitionTo<S2> handle(const E2 &) const {
        return {};
    }
    [[nodiscard]] TransitionTo<S3> handle(const E3 &) const {
        return {};
    }
};

template<typename E1, typename S1, typename E2, typename S2>
struct State<E1, S1, E2, S2> {
    [[nodiscard]] TransitionTo<S1> handle(const E1 &) const {
        return {};
    }
    [[nodiscard]] TransitionTo<S2> handle(const E2 &) const {
        return {};
    }
};

template<typename E1, typename S1>
struct State<E1, S1> {
    [[nodiscard]] TransitionTo<S1> handle(const E1 &) const {
        return {};
    }
};