/*
*
*   std::unique_ptr<T> implementation using Empty Base Optimisation for Deleter objects
*
*/



#pragma once

#include "compressed_pair.h"
#include "deleters.h"

#include <cstddef>

template <typename T>
class Slug {
public:
    Slug() = default;
    Slug(const Slug& other) = delete;
    Slug(Slug&& other) {
        (void)other;
    }
    Slug& operator=(Slug&& other) {
        (void)other;
        return *this;
    }
    template <typename U>
    operator Slug<U>() {
        return Slug<U>();
    }

    void operator()(T* ptr) {
        if (ptr != nullptr) {
            delete ptr;
        }
    }
};
template <typename T>
class Slug<T[]> {
public:
    Slug() = default;
    Slug(const Slug& other) = delete;
    Slug(Slug&& other) {
        (void)other;
    }
    Slug& operator=(Slug&& other) {
        (void)other;

        return *this;
    }
    template <typename U>
    operator Slug<U>() {
        return Slug<U>();
    }

    void operator()(T* ptr) {
        delete[] ptr;
    }
};

// Primary template

template <typename T, typename DeleterTemp = Slug<T>>
class UniquePtr {
public:
    template <typename U, typename SomeDeleterTemp>
    friend class UniquePtr;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    explicit UniquePtr(T* ptr = nullptr) : t_deleter_pair_(ptr, DeleterTemp()) {
    }
    UniquePtr(T* ptr, DeleterTemp&& deleter) : t_deleter_pair_(ptr, std::move(deleter)) {
    }

    UniquePtr(T* ptr, const DeleterTemp& deleter) noexcept {
        t_deleter_pair_.GetFirst() = ptr;
        t_deleter_pair_.GetSecond() = deleter;
    }

    template <typename U, typename OtherDeleter>
    UniquePtr(UniquePtr<U, OtherDeleter>&& other) noexcept {
        T* upcast = static_cast<U*>(other.t_deleter_pair_.GetFirst());
        t_deleter_pair_.GetFirst() = upcast;
        other.t_deleter_pair_.GetFirst() = nullptr;

        t_deleter_pair_.GetSecond() = std::move(other.t_deleter_pair_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        t_deleter_pair_.GetSecond()(t_deleter_pair_.GetFirst());
        t_deleter_pair_.GetFirst() = nullptr;

        t_deleter_pair_.GetFirst() = other.t_deleter_pair_.GetFirst();
        other.t_deleter_pair_.GetFirst() = nullptr;

        t_deleter_pair_.GetSecond() = std::move(other.t_deleter_pair_.GetSecond());

        return *this;
    }

    template <typename U, typename OtherDeleteTemp>
    UniquePtr& operator=(UniquePtr<U, OtherDeleteTemp>&& other) noexcept {
        t_deleter_pair_.GetSecond()(t_deleter_pair_.GetFirst());
        t_deleter_pair_.GetFirst() = nullptr;

        T* upcast = static_cast<U*>(other.t_deleter_pair_.GetFirst());
        t_deleter_pair_.GetFirst() = upcast;
        other.t_deleter_pair_.GetFirst() = nullptr;

        t_deleter_pair_.GetSecond() = std::move(other.t_deleter_pair_.GetSecond());

        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        t_deleter_pair_.GetSecond()(t_deleter_pair_.GetFirst());
        t_deleter_pair_.GetFirst() = nullptr;

        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        t_deleter_pair_.GetSecond()(t_deleter_pair_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* buffer_data = t_deleter_pair_.GetFirst();
        t_deleter_pair_.GetFirst() = nullptr;
        return buffer_data;
    }
    void Reset(T* ptr = nullptr) {
        T* old_ptr = t_deleter_pair_.GetFirst();
        t_deleter_pair_.GetFirst() = ptr;
        if (old_ptr != nullptr) {
            t_deleter_pair_.GetSecond()(old_ptr);
        }
    }
    void Swap(UniquePtr& other) {
        T* buff_data = t_deleter_pair_.GetFirst();
        t_deleter_pair_.GetFirst() = other.t_deleter_pair_.GetFirst();
        other.t_deleter_pair_.GetFirst() = buff_data;

        std::swap(t_deleter_pair_.GetSecond(), other.t_deleter_pair_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return t_deleter_pair_.GetFirst();
    }

    DeleterTemp& GetDeleter() {
        return t_deleter_pair_.GetSecond();
    }

    const DeleterTemp& GetDeleter() const {
        return t_deleter_pair_.GetSecond();
    }

    explicit operator bool() const {
        return t_deleter_pair_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    T& operator*() const {
        return *t_deleter_pair_.GetFirst();
    }
    T* operator->() const {
        return t_deleter_pair_.GetFirst();
    }

private:
    CompressedPair<T*, DeleterTemp> t_deleter_pair_;
};

template <typename Deleter>
class UniquePtr<void, Deleter> {
public:
    explicit UniquePtr(void* ptr = nullptr) : deleter_() {
        data_ = ptr;
    }
    UniquePtr(void* ptr, Deleter&& deleter) : deleter_(std::move(deleter)) {
        data_ = ptr;
    }
    UniquePtr(void* ptr, const Deleter& deleter) : deleter_(deleter) {
        data_ = ptr;
    }

    UniquePtr(UniquePtr&& other) noexcept {
        data_ = other.data_;
        other.data_ = nullptr;
        deleter_ = std::move(other.deleter_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        deleter_(data_);
        data_ = other.data_;
        deleter_ = std::move(other.deleter_);
    }
    UniquePtr& operator=(std::nullptr_t) {
        deleter_(data_);
        data_ = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        deleter_(data_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void* Release() {
        void* buffer_data = data_;
        data_ = nullptr;
        return buffer_data;
    }
    void Reset(void* ptr = nullptr) {
        void* old_ptr = data_;
        data_ = ptr;
        if (old_ptr != nullptr) {
            deleter_(old_ptr);
        }
    }
    void Swap(UniquePtr& other) {
        void* buff_data = data_;
        data_ = other.data_;
        other.data_ = buff_data;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    void* Get() const {
        return data_;
    }

    Deleter& GetDeleter() {
        return deleter_;
    }

    const Deleter& GetDeleter() const {
        return deleter_;
    }

    explicit operator bool() const {
        return data_ != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    void operator*() const {
    }
    void* operator->() const {
        return data_;
    }

private:
    void* data_;
    Deleter deleter_;
};

// Specialization for arrays
template <typename T, typename DeleterTemp>
class UniquePtr<T[], DeleterTemp> {
public:
    explicit UniquePtr(T* ptr = nullptr) : t_deleter_pair_(ptr, DeleterTemp()) {
    }
    UniquePtr(T* ptr, DeleterTemp&& deleter) : t_deleter_pair_(ptr, std::move(deleter)) {
    }
    UniquePtr(T* ptr, const DeleterTemp& deleter) : t_deleter_pair_(ptr, deleter) {
    }

    UniquePtr(UniquePtr&& other) noexcept {
        t_deleter_pair_.GetFirst() = other.t_deleter_pair_.GetFirst();
        other.t_deleter_pair_.GetFirst() = nullptr;
        t_deleter_pair_.GetSecond() = std::move(other.t_deleter_pair_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        t_deleter_pair_.GetSecond()(t_deleter_pair_.GetFirst());
        t_deleter_pair_.GetFirst() = other.t_deleter_pair_.GetFirst();
        t_deleter_pair_.GetSecond() = std::move(other.t_deleter_pair_.GetSecond());
    }
    UniquePtr& operator=(std::nullptr_t) {
        t_deleter_pair_.GetSecond()(t_deleter_pair_.GetFirst());
        t_deleter_pair_.GetFirst() = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        t_deleter_pair_.GetSecond()(t_deleter_pair_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* buffer_data = t_deleter_pair_.GetFirst();
        t_deleter_pair_.GetFirst() = nullptr;
        return buffer_data;
    }
    void Reset(T* ptr = nullptr) {
        T* old_ptr = t_deleter_pair_.GetFirst();
        t_deleter_pair_.GetFirst() = ptr;
        if (old_ptr != nullptr) {
            t_deleter_pair_.GetSecond()(old_ptr);
        }
    }
    void Swap(UniquePtr& other) {
        T* buff_data = t_deleter_pair_.GetFirst();
        t_deleter_pair_.GetFirst() = other.t_deleter_pair_.GetFirst();
        other.t_deleter_pair_.GetFirst() = buff_data;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return t_deleter_pair_.GetFirst();
    }

    DeleterTemp& GetDeleter() {
        return t_deleter_pair_.GetSecond();
    }

    const DeleterTemp& GetDeleter() const {
        return t_deleter_pair_.GetSecond();
    }

    explicit operator bool() const {
        return t_deleter_pair_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    T& operator*() const {
        return *t_deleter_pair_.GetFirst();
    }
    T* operator->() const {
        return t_deleter_pair_.GetFirst();
    }

    T& operator[](const size_t& i) {
        return t_deleter_pair_.GetFirst()[i];
    }
    const T& operator[](const size_t i) const {
        return t_deleter_pair_.GetFirst()[i];
    }

private:
    CompressedPair<T*, DeleterTemp> t_deleter_pair_;
};