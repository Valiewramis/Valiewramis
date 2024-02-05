/*
*
*   Simple Deque<int> realization with ring buffers
*   without using std containers
*
*/


#pragma once

#include <initializer_list>
#include <algorithm>
#include <deque>

struct RingBufferElement {
    RingBufferElement* left;
    RingBufferElement* right;

    int* arr;

    RingBufferElement() : left(nullptr), right(nullptr), arr(nullptr) {
    }

    void FillArray() {
        if (arr != nullptr) {
            delete[] arr;
            arr = nullptr;
        }

        arr = new int[128];
        for (size_t i = 0; i < 128; ++i) {
            arr[i] = 0;
        }
    }

    ~RingBufferElement() {
        delete[] arr;
        arr = nullptr;
    }
};

class Deque {
public:
    Deque()
        : ind_of_begin_(0),
          el_size_(0),
          size_(0),
          main_array_(nullptr),
          l_edge_(nullptr),
          r_edge_(nullptr),
          lcounter_(0),
          rcounter_(0) {
    }

    explicit Deque(const Deque& rhs)
        : ind_of_begin_(0),
          el_size_(0),
          size_(0),
          main_array_(nullptr),
          l_edge_(nullptr),
          r_edge_(nullptr),
          lcounter_(0),
          rcounter_(0) {

        ind_of_begin_ = rhs.ind_of_begin_;
        el_size_ = rhs.el_size_;
        size_ = rhs.size_;

        lcounter_ = rhs.lcounter_;
        rcounter_ = rhs.rcounter_;

        if (size_ > 0) {
            RingBufferElement** new_array = new RingBufferElement*[size_];

            for (size_t i = 0; i < size_; ++i) {
                new_array[i] = new RingBufferElement();
            }

            for (size_t i = 1; i < size_ - 1; ++i) {
                new_array[i]->left = new_array[i - 1];
                new_array[i]->right = new_array[i + 1];
            }

            new_array[0]->left = new_array[size_ - 1];
            new_array[size_ - 1]->right = new_array[0];

            if (size_ > 1) {
                new_array[0]->right = new_array[1];
                new_array[size_ - 1]->left = new_array[size_ - 2];
            }

            for (size_t i = 0; i < size_; ++i) {
                if (rhs.main_array_[i]->arr != nullptr) {
                    new_array[i]->FillArray();

                    for (int j = 0; j < 128; ++j) {
                        new_array[i]->arr[j] = rhs.main_array_[i]->arr[j];
                    }
                }

                if (rhs.main_array_[i] == rhs.l_edge_) {
                    l_edge_ = new_array[i];
                }
                if (rhs.main_array_[i] == rhs.r_edge_) {
                    r_edge_ = new_array[i];
                }
            }
            main_array_ = new_array;
        }
    }
    explicit Deque(Deque&& rhs)
        : ind_of_begin_(0),
          el_size_(0),
          size_(0),
          main_array_(nullptr),
          l_edge_(nullptr),
          r_edge_(nullptr),
          lcounter_(0),
          rcounter_(0) {

        ind_of_begin_ = std::move(rhs.ind_of_begin_);
        lcounter_ = std::move(rhs.lcounter_);
        rcounter_ = std::move(rhs.rcounter_);

        el_size_ = std::move(rhs.el_size_);
        size_ = std::move(rhs.size_);

        l_edge_ = rhs.l_edge_;
        r_edge_ = rhs.r_edge_;

        main_array_ = rhs.main_array_;

        rhs.l_edge_ = nullptr;
        rhs.r_edge_ = nullptr;
        rhs.main_array_ = nullptr;

        rhs.lcounter_ = 0;
        rhs.rcounter_ = 0;
        rhs.size_ = 0;
        rhs.el_size_ = 0;
    }

    explicit Deque(size_t size)
        : ind_of_begin_(0),
          el_size_(0),
          size_(0),
          main_array_(nullptr),
          l_edge_(nullptr),
          r_edge_(nullptr),
          lcounter_(0),
          rcounter_(0) {

        if (size == 0) {
            return;
        }

        Init();

        while (size > 128 * size_) {
            Relocate();
        }
        size_t needed_buffers = (size / 128) + 1;
        for (size_t i = 0; i < needed_buffers; ++i) {
            main_array_[i]->FillArray();
        }
        el_size_ = size;
    }

    explicit Deque(std::initializer_list<int> list)
        : ind_of_begin_(0),
          el_size_(0),
          size_(0),
          main_array_(nullptr),
          l_edge_(nullptr),
          r_edge_(nullptr),
          lcounter_(0),
          rcounter_(0) {
        for (const int& el : list) {
            PushBack(el);
        }
    }

    Deque& operator=(Deque& rhs) {
        if (this == &rhs) {
            return *this;
        }

        Delete();

        ind_of_begin_ = rhs.ind_of_begin_;
        el_size_ = rhs.el_size_;
        size_ = rhs.size_;

        lcounter_ = rhs.lcounter_;
        rcounter_ = rhs.rcounter_;

        if (size_ > 0) {
            RingBufferElement** new_array = new RingBufferElement*[size_];

            for (size_t i = 0; i < size_; ++i) {
                new_array[i] = new RingBufferElement();
            }

            for (size_t i = 1; i < size_ - 1; ++i) {
                new_array[i]->left = new_array[i - 1];
                new_array[i]->right = new_array[i + 1];
            }

            new_array[0]->left = new_array[size_ - 1];
            new_array[size_ - 1]->right = new_array[0];

            if (size_ > 1) {
                new_array[0]->right = new_array[1];
                new_array[size_ - 1]->left = new_array[size_ - 2];
            }

            for (size_t i = 0; i < size_; ++i) {
                if (rhs.main_array_[i]->arr != nullptr) {
                    new_array[i]->FillArray();

                    for (int j = 0; j < 128; ++j) {
                        new_array[i]->arr[j] = rhs.main_array_[i]->arr[j];
                    }
                }

                if (rhs.main_array_[i] == rhs.l_edge_) {
                    l_edge_ = new_array[i];
                }
                if (rhs.main_array_[i] == rhs.r_edge_) {
                    r_edge_ = new_array[i];
                }
            }

            main_array_ = new_array;
        }
        return *this;
    }

    Deque& operator=(Deque&& rhs) {
        if (this == &rhs) {
            return *this;
        }

        Delete();

        lcounter_ = std::move(rhs.lcounter_);
        rcounter_ = std::move(rhs.rcounter_);
        ind_of_begin_ = std::move(rhs.ind_of_begin_);

        el_size_ = std::move(rhs.el_size_);
        size_ = std::move(rhs.size_);

        l_edge_ = rhs.l_edge_;
        r_edge_ = rhs.r_edge_;

        main_array_ = rhs.main_array_;

        rhs.l_edge_ = nullptr;
        rhs.r_edge_ = nullptr;
        rhs.main_array_ = nullptr;

        rhs.lcounter_ = 0;
        rhs.rcounter_ = 0;
        rhs.size_ = 0;
        rhs.el_size_ = 0;

        return *this;
    }

    void Swap(Deque& rhs) {
        std::swap(main_array_, rhs.main_array_);
        std::swap(l_edge_, rhs.l_edge_);
        std::swap(r_edge_, rhs.r_edge_);
        std::swap(ind_of_begin_, rhs.ind_of_begin_);

        std::swap(lcounter_, rhs.lcounter_);
        std::swap(rcounter_, rhs.rcounter_);

        std::swap(el_size_, rhs.el_size_);
        std::swap(size_, rhs.size_);
    }

    void PushBack(int value) {
        ++el_size_;
        if (size_ == 0) {
            Init();
            r_edge_->arr[rcounter_] = value;
            return;
        }
        ++rcounter_;
        if (rcounter_ == 128) {
            if (r_edge_->right == l_edge_) {
                Relocate();
            }
            r_edge_ = r_edge_->right;
            r_edge_->FillArray();
            rcounter_ = 0;
        }
        r_edge_->arr[rcounter_] = value;
    }

    void PopBack() {
        --el_size_;

        if (el_size_ == 0) {
            Delete();
            return;
        }

        if (rcounter_ > 0) {
            r_edge_->arr[rcounter_] = 0;
            --rcounter_;
            return;
        }

        delete[] r_edge_->arr;
        r_edge_->arr = nullptr;

        r_edge_ = r_edge_->left;

        rcounter_ = 127;
    }

    void PushFront(int value) {
        ++el_size_;

        if (size_ == 0) {
            Init();
            l_edge_->arr[lcounter_] = value;
            return;
        }

        if (lcounter_ > 0) {
            --lcounter_;
            l_edge_->arr[lcounter_] = value;
            return;
        }

        if (l_edge_->left == r_edge_) {
            Relocate();
        }
        l_edge_ = l_edge_->left;
        if (ind_of_begin_ == 0) {
            ind_of_begin_ = size_ - 1;
        } else {
            --ind_of_begin_;
        }
        l_edge_->FillArray();
        lcounter_ = 127;

        l_edge_->arr[lcounter_] = value;
    }

    void PopFront() {
        --el_size_;

        if (el_size_ == 0) {
            Delete();
            return;
        }

        if (lcounter_ < 127) {
            l_edge_->arr[lcounter_] = 0;
            ++lcounter_;
            return;
        }

        delete[] l_edge_->arr;
        l_edge_->arr = nullptr;

        l_edge_ = l_edge_->right;

        if (ind_of_begin_ == size_ - 1) {
            ind_of_begin_ = 0;
        } else {
            ++ind_of_begin_;
        }
        lcounter_ = 0;
    }

    int& operator[](size_t ind) {

        if (lcounter_ + ind <= 127) {
            return l_edge_->arr[lcounter_ + ind];
        }

        ind -= (128 - lcounter_);

        RingBufferElement** curr_buff;

        if (ind_of_begin_ == size_ - 1) {
            curr_buff = &main_array_[0];
        } else {
            curr_buff = &main_array_[ind_of_begin_ + 1];
        }

        size_t steps_thr_buff = ind / 128;
        if (static_cast<size_t>(&main_array_[size_ - 1] - curr_buff) >= steps_thr_buff) {
            curr_buff += steps_thr_buff;
        } else {
            size_t bebra = static_cast<size_t>(&main_array_[size_ - 1] - curr_buff) + 1;
            curr_buff = &(main_array_[0]);
            steps_thr_buff = steps_thr_buff - bebra;

            curr_buff += steps_thr_buff;
        }

        return (*curr_buff)->arr[ind % 128];
    }

    int operator[](size_t ind) const {
        if (lcounter_ + ind <= 127) {
            return l_edge_->arr[lcounter_ + ind];
        }

        ind -= (128 - lcounter_);

        RingBufferElement** curr_buff;

        if (ind_of_begin_ == size_ - 1) {
            curr_buff = &main_array_[0];
        } else {
            curr_buff = &main_array_[ind_of_begin_ + 1];
        }

        size_t steps_thr_buff = ind / 128;
        if (static_cast<size_t>(&main_array_[size_ - 1] - curr_buff) >= steps_thr_buff) {
            curr_buff += steps_thr_buff;
        } else {
            size_t bebra = static_cast<size_t>(&main_array_[size_ - 1] - curr_buff) + 1;
            curr_buff = &(main_array_[0]);
            steps_thr_buff = steps_thr_buff - bebra;

            curr_buff += steps_thr_buff;
        }

        return (*curr_buff)->arr[ind % 128];
    }

    size_t Size() const {
        return el_size_;
    }

    void Clear() {
        Delete();
    }

    ~Deque() {
        if (main_array_ != nullptr) {
            Delete();
        }
    }

private:
    size_t ind_of_begin_;
    size_t el_size_;
    size_t size_;
    RingBufferElement** main_array_;

    RingBufferElement* l_edge_;
    RingBufferElement* r_edge_;

    size_t lcounter_;
    size_t rcounter_;

    void Delete() {
        for (size_t i = 0; i < size_; ++i) {
            delete main_array_[i];
            main_array_[i] = nullptr;
        }
        delete[] main_array_;
        main_array_ = nullptr;

        el_size_ = 0;
        size_ = 0;
        ind_of_begin_ = 0;
        r_edge_ = nullptr;
        l_edge_ = nullptr;
        rcounter_ = 0;
        lcounter_ = 0;
    }

    void Init() {
        size_ = 1;
        ind_of_begin_ = 0;

        main_array_ = new RingBufferElement*[1];
        main_array_[0] = new RingBufferElement();
        main_array_[0]->left = main_array_[0];
        main_array_[0]->right = main_array_[0];

        r_edge_ = main_array_[0];
        l_edge_ = main_array_[0];

        main_array_[0]->FillArray();
        rcounter_ = 0;
        lcounter_ = 0;
    }

    void Relocate() {

        RingBufferElement** new_main_array = new RingBufferElement*[2 * size_];

        auto curr_ptr = l_edge_;

        for (size_t i = 0; i < size_; ++i) {
            new_main_array[i] = curr_ptr;
            curr_ptr = curr_ptr->right;
        }

        for (size_t i = size_; i < 2 * size_; ++i) {
            new_main_array[i] = new RingBufferElement();
        }

        for (size_t i = 1; i < 2 * size_ - 1; ++i) {
            new_main_array[i]->left = new_main_array[i - 1];
            new_main_array[i]->right = new_main_array[i + 1];
        }

        size_ = 2 * size_;

        delete[] main_array_;
        main_array_ = nullptr;

        new_main_array[size_ - 1]->left = new_main_array[size_ - 2];
        new_main_array[size_ - 1]->right = new_main_array[0];
        new_main_array[0]->left = new_main_array[size_ - 1];
        new_main_array[0]->right = new_main_array[1];

        ind_of_begin_ = 0;
        main_array_ = new_main_array;
    }
};
