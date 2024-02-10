/*
*   Shared and Weak Pointers implementation
*/



#pragma once

#include <exception>
#include <memory>
#include <type_traits>
#include <cstddef>

class BadWeakPtr : public std::exception {};

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

class BaseBlock {
public:
    virtual void IfZeroWeak() = 0;
    virtual void IfZeroStrong() = 0;
    virtual void DecreaseWeak() = 0;
    virtual void DecreaseStrong() = 0;
    virtual void IncreaseStrong() = 0;
    virtual void IncreaseWeak() = 0;
    virtual int GetStrongQ() = 0;
    virtual int GetWeakQ() = 0;

    virtual ~BaseBlock() = default;
};

template <typename T>
class PointerControlBlock : public BaseBlock {
public:
    PointerControlBlock(T* ptr) : ptr_(ptr), weak_counter_(0), destroyed_(false) {
        strong_counter_ = 1;
    }
    T* GetRawPointer() {
        return ptr_;
    }
    ~PointerControlBlock() {
        if (!destroyed_) {
            delete ptr_;
        }
    }

    void IfZeroWeak() override {
        delete this;
    }
    void IfZeroStrong() override {
        if (!destroyed_) {
            delete ptr_;
            destroyed_ = true;
        }
    }
    void DecreaseWeak() override {
        --weak_counter_;
        if (weak_counter_ == 0 && strong_counter_ == 0) {
            IfZeroWeak();
        } else if (strong_counter_ == 0) {
            IfZeroStrong();
        }
    }
    void DecreaseStrong() override {
        --strong_counter_;
        if (weak_counter_ == 0 && strong_counter_ == 0) {
            IfZeroWeak();
        } else if (strong_counter_ == 0) {
            IfZeroStrong();
        }
    }

    void IncreaseWeak() override {
        ++weak_counter_;
    }
    void IncreaseStrong() override {
        ++strong_counter_;
    }

    int GetStrongQ() override {
        return strong_counter_;
    }
    int GetWeakQ() override {
        return weak_counter_;
    }

private:
    T* ptr_;
    bool destroyed_;
    int strong_counter_;
    int weak_counter_;
};

template <typename T>
class ControlBlock : public BaseBlock {
public:
    ControlBlock() : destroyed_(false), strong_counter_(1), weak_counter_(0) {
        new (&holder_) T();
    }

    template <typename... Args>
    ControlBlock(Args&&... args) : destroyed_(false), strong_counter_(1), weak_counter_(0) {
        new (&holder_) T(std::forward<Args>(args)...);
    }

    T* GetRawPointer() {
        return reinterpret_cast<T*>(&holder_);
    }
    ~ControlBlock() override {
        if (!destroyed_) {
            reinterpret_cast<T*>(&holder_)->~T();
            destroyed_ = true;
        }
    }

    void IfZeroWeak() override {
        delete this;
    }

    void IfZeroStrong() override {
        if (!destroyed_) {
            reinterpret_cast<T*>(&holder_)->~T();
            destroyed_ = true;
        }
    }

    void DecreaseWeak() override {
        --weak_counter_;

        if (weak_counter_ == 0 && strong_counter_ == 0) {
            IfZeroWeak();
        } else if (strong_counter_ == 0) {
            IfZeroStrong();
        }
    }
    void DecreaseStrong() override {
        --strong_counter_;
        if (weak_counter_ == 0 && strong_counter_ == 0) {
            IfZeroWeak();
        } else if (strong_counter_ == 0) {
            IfZeroStrong();
        }
    }

    void IncreaseStrong() override {
        ++strong_counter_;
    }
    void IncreaseWeak() override {
        ++weak_counter_;
    }

    int GetStrongQ() override {
        return strong_counter_;
    }

    int GetWeakQ() override {
        return strong_counter_;
    }

private:
    bool destroyed_;
    int strong_counter_;
    int weak_counter_;
    std::aligned_storage_t<sizeof(T), alignof(T)> holder_;
};

template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    friend class WeakPtr<T>;

    template <typename U>
    friend class SharedPtr;

    template <typename U, typename... Args>
    friend SharedPtr<U> MakeShared(Args&&... args);

    template <typename K, typename U>
    friend inline bool operator==(const SharedPtr<K>& left, const SharedPtr<U>& right);

    SharedPtr() : alias_ptr_(nullptr), control_block_(nullptr), if_pointer_type_(false) {
    }

    SharedPtr(std::nullptr_t)
        : alias_ptr_(nullptr), control_block_(nullptr), if_pointer_type_(false) {
    }

    template <typename U>
    explicit SharedPtr(U* ptr) {

        if (std::is_same<U, T>::value || (std::is_base_of<T, U>::value && std::has_virtual_destructor<U>::value)) {
            control_block_ = new PointerControlBlock<T>(ptr);
        } else if (std::is_base_of<T, U>::value) {
            control_block_ = new PointerControlBlock<U>(ptr);
        }

        alias_ptr_ = nullptr;
        if_pointer_type_ = true;
    }

    SharedPtr(const SharedPtr& other) {
        control_block_ = other.control_block_;
        alias_ptr_ = other.alias_ptr_;
        if (control_block_ != nullptr) {
            control_block_->IncreaseStrong();
        }

        if_pointer_type_ = other.if_pointer_type_;
    }
    SharedPtr(SharedPtr&& other) {
        if_pointer_type_ = other.if_pointer_type_;

        control_block_ = other.control_block_;
        other.control_block_ = nullptr;
        alias_ptr_ = other.alias_ptr_;
        other.alias_ptr_ = nullptr;
    }

    template <typename U>
    SharedPtr(const SharedPtr<U>& other) {
        control_block_ = other.control_block_;
        if_pointer_type_ = other.if_pointer_type_;

        if (control_block_ != nullptr) {
            control_block_->IncreaseStrong();
        }

        if (other.alias_ptr_ != nullptr) {
            alias_ptr_ = static_cast<T*>(other.alias_ptr_);
        } else {
            alias_ptr_ = nullptr;
        }
    }

    template <typename U>
    SharedPtr(SharedPtr<U>&& other) {
        control_block_ = other.control_block_;
        if_pointer_type_ = other.if_pointer_type_;

        if (other.alias_ptr_ != nullptr) {
            alias_ptr_ = static_cast<T*>(other.alias_ptr_);
        } else {
            alias_ptr_ = nullptr;
        }
        other.alias_ptr_ = nullptr;
        other.control_block_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        control_block_ = other.control_block_;
        if_pointer_type_ = other.if_pointer_type_;

        if (control_block_ != nullptr) {
            control_block_->IncreaseStrong();
        }

        alias_ptr_ = ptr;
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this == &other) {
            return *this;
        }

        if_pointer_type_ = other.if_pointer_type_;

        if (control_block_ != nullptr) {
            control_block_->DecreaseStrong();
        }

        control_block_ = other.control_block_;

        if (control_block_ != nullptr) {
            control_block_->IncreaseStrong();
        }
        alias_ptr_ = other.alias_ptr_;

        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        if (this == &other) {
            return *this;
        }

        if_pointer_type_ = other.if_pointer_type_;

        if (control_block_ != nullptr) {
            control_block_->DecreaseStrong();
        }

        control_block_ = other.control_block_;
        alias_ptr_ = other.alias_ptr_;

        other.control_block_ = nullptr;
        other.alias_ptr_ = nullptr;

        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (control_block_ != nullptr) {
            control_block_->DecreaseStrong();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers
    void Reset() {
        if (control_block_ != nullptr) {
            control_block_->DecreaseStrong();
        }

        delete alias_ptr_;
        alias_ptr_ = nullptr;
        control_block_ = nullptr;
    }

    template <typename U>
    void Reset(U* ptr) {
        if (control_block_ != nullptr) {
            control_block_->DecreaseStrong();
        }
        if (std::is_same<U, T>::value || (std::is_base_of<T, U>::value && std::has_virtual_destructor<U>::value)) {
            control_block_ = new PointerControlBlock<T>(ptr);
        } else if (std::is_base_of<T, U>::value) {
            control_block_ = new PointerControlBlock<U>(ptr);
        }
        if_pointer_type_ = true;

        delete alias_ptr_;
        alias_ptr_ = nullptr;
    }

    void Swap(SharedPtr& other) {
        std::swap(control_block_, other.control_block_);
        std::swap(alias_ptr_, alias_ptr_);
        std::swap(if_pointer_type_, other.if_pointer_type_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        if (alias_ptr_ != nullptr) {
            return alias_ptr_;
        }

        if (control_block_ == nullptr) {
            return nullptr;
        }

        if (if_pointer_type_) {
            auto cb_pointer = dynamic_cast<PointerControlBlock<T>*>(control_block_);
            return cb_pointer->GetRawPointer();
        } else {
            auto cb_pointer = dynamic_cast<ControlBlock<T>*>(control_block_);
            return cb_pointer->GetRawPointer();
        }
    }
    T& operator*() const {
        if (alias_ptr_ != nullptr) {
            return *alias_ptr_;
        }

        if (if_pointer_type_) {
            auto cb_pointer = dynamic_cast<PointerControlBlock<T>*>(control_block_);
            return *(cb_pointer->GetRawPointer());
        } else {
            auto cb_pointer = dynamic_cast<ControlBlock<T>*>(control_block_);
            return *(cb_pointer->GetRawPointer());
        }
    }

    T* operator->() const {
        if (alias_ptr_ != nullptr) {
            return alias_ptr_;
        }

        if (if_pointer_type_) {
            auto cb_pointer = dynamic_cast<PointerControlBlock<T>*>(control_block_);
            return cb_pointer->GetRawPointer();
        } else {
            auto cb_pointer = dynamic_cast<ControlBlock<T>*>(control_block_);
            return cb_pointer->GetRawPointer();
        }
    }
    size_t UseCount() const {
        if (control_block_ == nullptr) {
            return 0;
        } else {
            return control_block_->GetStrongQ();
        }
    }
    explicit operator bool() const {
        return control_block_ != nullptr;
    }

private:
    T* alias_ptr_;
    BaseBlock* control_block_;
    bool if_pointer_type_;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.control_block_ == right.control_block_;
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    SharedPtr<T> sp;
    sp.if_pointer_type_ = false;
    sp.control_block_ = new ControlBlock<T>(std::forward<Args>(args)...);

    return sp;
}

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    friend class SharedPtr<T>;

    WeakPtr() : control_block_(nullptr), if_pointer_type_(false) {
    }

    WeakPtr(const WeakPtr& other) {
        control_block_ = other.control_block_;
        if (control_block_ != nullptr) {
            control_block_->IncreaseWeak();
        }
        if_pointer_type_ = other.if_pointer_type_;
    }
    WeakPtr(WeakPtr&& other) {
        control_block_ = other.control_block_;
        other.control_block_ = nullptr;

        if_pointer_type_ = other.if_pointer_type_;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) {
        control_block_ = other.control_block_;
        if (control_block_ != nullptr) {
            control_block_->IncreaseWeak();
        }
        if_pointer_type_ = other.if_pointer_type_;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (control_block_ != nullptr) {
            control_block_->DecreaseWeak();
        }
        control_block_ = other.control_block_;
        if (control_block_ != nullptr) {
            control_block_->IncreaseWeak();
        }
        if_pointer_type_ = other.if_pointer_type_;

        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) {
        if (control_block_ != nullptr) {
            control_block_->DecreaseWeak();
        }

        control_block_ = other.control_block_;
        other.control_block_ = nullptr;

        if_pointer_type_ = other.if_pointer_type_;

        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        if (control_block_ != nullptr) {
            control_block_->DecreaseWeak();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (control_block_ != nullptr) {
            control_block_->DecreaseWeak();
        }
        control_block_ = nullptr;
    }
    void Swap(WeakPtr& other) {
        std::swap(control_block_, other.control_block_);
        std::swap(if_pointer_type_, other.if_pointer_type_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (control_block_ == nullptr) {
            return 0;
        }
        return control_block_->GetStrongQ();
    }
    bool Expired() const {
        if (control_block_ != nullptr) {
            return control_block_->GetStrongQ() == 0;
        } else {
            return true;
        }
    }
    SharedPtr<T> Lock() const {
        SharedPtr<T> new_sp;
        if (!Expired()) {
            new_sp.control_block_ = control_block_;
            control_block_->IncreaseStrong();
            new_sp.if_pointer_type_ = if_pointer_type_;
        }
        return new_sp;
    }

private:
    bool if_pointer_type_;
    BaseBlock* control_block_;
};

template <typename T>
SharedPtr<T>::SharedPtr(const WeakPtr<T>& other) {
    alias_ptr_ = nullptr;
    if_pointer_type_ = false;
    if (!other.Expired()) {
        control_block_ = other.control_block_;
        if (control_block_ != nullptr) {
            control_block_->IncreaseStrong();
        }
        if_pointer_type_ = other.if_pointer_type_;
    } else {
        throw BadWeakPtr();
    }
}
