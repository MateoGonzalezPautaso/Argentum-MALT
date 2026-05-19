#ifndef COMMON_VISIT_H
#define COMMON_VISIT_H

template <class... Ts>
struct overloaded: Ts... {
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

#endif  // COMMON_VISIT_H
