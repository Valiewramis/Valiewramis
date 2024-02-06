/**
*
* Pairs with Empty-Base Optimization
*
*/



#pragma once

#include <utility>
#include <type_traits>

template <class V, class U>
inline constexpr bool kStatement0 =
    std::is_empty_v<V> && !std::is_final_v<V> && std::is_empty_v<U> && !std::is_final_v<U>;

template <class V, class U>
inline constexpr bool kStatement1 =
    kStatement0<V, U> && !(std::is_base_of_v<V, U> || std::is_base_of_v<U, V>);

template <class V, class U>
inline constexpr bool kStatement2 =
    !kStatement0<U, V> && (std::is_empty_v<V> && !std::is_final_v<V>);

// Me think, why waste time write lot code, when few code do trick.

template <typename F, typename S, typename = void>

class CompressedPair {
public:
    CompressedPair() : first_(F()), second_(S()) {
    }

    CompressedPair(const F& first, const S& second) : first_(first), second_(second) {
    }
    CompressedPair(const F& first, S&& second) : first_(first), second_(std::move(second)) {
    }
    CompressedPair(F&& first, const S& second) : first_(std::move(first)), second_(second) {
    }
    CompressedPair(F&& first, S&& second) : first_(std::move(first)), second_(std::move(second)) {
    }

    F& GetFirst() {
        return first_;
    }

    const F& GetFirst() const {
        return first_;
    }

    const S& GetSecond() const {
        return second_;
    };

    S& GetSecond() {
        return second_;
    }

private:
    F first_;
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, std::enable_if_t<kStatement1<F, S>>> : F, S {
public:
    CompressedPair() : F(), S() {
    }

    CompressedPair(const F& f, const S& s) : F(f), S(s) {
    }
    CompressedPair(const F& f, S&& s) : F(f), S(std::move(s)) {
    }
    CompressedPair(F&& f, const S& s) : F(std::move(f)), S(s) {
    }
    CompressedPair(F&& f, S&& s) : F(std::move(f)), S(std::move(s)) {
    }
    const F& GetFirst() const {
        const F* f_ptr = static_cast<const F*>(this);
        return *f_ptr;
    }

    F& GetFirst() {
        F* f_ptr = static_cast<F*>(this);
        return *f_ptr;
    }

    const S& GetSecond() const {
        const S* s_ptr = static_cast<const S*>(this);
        return *s_ptr;
    }
    S& GetSecond() {
        S* s_ptr = static_cast<S*>(this);
        return *s_ptr;
    }
};

template <typename F, typename S>
class CompressedPair<F, S, std::enable_if_t<kStatement2<F, S>>> : F {
public:
    CompressedPair() : F(), second_elem_(S()) {
    }
    CompressedPair(const F& f, const S& s) : F(f), second_elem_(s) {
    }
    CompressedPair(const F& f, S&& s) : F(f), second_elem_(std::move(s)) {
    }
    CompressedPair(F&& f, const S& s) : F(std::move(f)), second_elem_(s) {
    }
    CompressedPair(F&& f, S&& s) : F(std::move(f)), second_elem_(std::move(s)) {
    }

    const F& GetFirst() const {
        const F* f_ptr = static_cast<const F*>(this);
        return *f_ptr;
    }

    F& GetFirst() {
        F* f_ptr = static_cast<F*>(this);
        return *f_ptr;
    }

    const S& GetSecond() const {
        return second_elem_;
    }
    S& GetSecond() {
        return second_elem_;
    }

private:
    S second_elem_;
};

template <typename F, typename S>
class CompressedPair<F, S, std::enable_if_t<kStatement2<S, F>>> : S {
public:
    CompressedPair() : S(), first_elem_(F()) {
    }
    CompressedPair(const F& f, const S& s) : first_elem_(f), S(s) {
    }
    CompressedPair(F&& f, const S& s) : first_elem_(std::move(f)), S(s) {
    }
    CompressedPair(const F& f, S&& s) : S(std::move(s)), first_elem_(f) {
    }
    CompressedPair(F&& f, S&& s) : first_elem_(std::move(f)), S(std::move(s)) {
    }

    const F& GetFirst() const {
        return first_elem_;
    }

    F& GetFirst() {
        return first_elem_;
    }

    const S& GetSecond() const {
        const S* s_ptr = static_cast<const S*>(this);
        return *s_ptr;
    }
    S& GetSecond() {
        S* s_ptr = static_cast<S*>(this);
        return *s_ptr;
    }

private:
    F first_elem_;
};
