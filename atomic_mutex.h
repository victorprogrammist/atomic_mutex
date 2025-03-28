
/*
 * Author Telnov Victor, v-telnov@yandex.ru
 *
 * licensed under GNU GPL3
 *
 */

#ifndef __AtomicMutex_h__
#define __AtomicMutex_h__

#include <atomic>
#include <cassert>

template <class TT> struct AtomicMutex;
template <class TT> struct AtomicMutexReadLocker;
template <class TT> struct AtomicMutexWriteLocker;
template <class TT> struct AtomicMutexWriteLazyLocker;

using MutexInt = AtomicMutex<int>;
using MutexIntRead = AtomicMutexReadLocker<int>;
using MutexIntWrite = AtomicMutexWriteLocker<int>;
using MutexIntWriteLazy = AtomicMutexWriteLazyLocker<int>;

using MutexChar = AtomicMutex<char>;
using MutexCharRead = AtomicMutexReadLocker<char>;
using MutexCharWrite = AtomicMutexWriteLocker<char>;
using MutexCharWriteLazy = AtomicMutexWriteLazyLocker<char>;

//*****************************************************
template <class TT>
struct AtomicMutex {
    static_assert( TT(-1) < 0, "Need signed numeric type");

    static auto o() {
        return std::memory_order_relaxed;
    }

    void lockForWriteGreedy();
    void lockForWriteLazy();
    void lockForWrite() { lockForWriteGreedy(); }

    void unlockForWrite();

    bool tryLockForRead();
    void lockForRead();
    void unlockForRead();

    void lock() { lockForWrite(); }
    void unlock() { unlockForWrite(); }
    void lock_shared() { lockForRead(); }
    void unlock_shared() { unlockForRead(); }

    auto useForRead(const auto& task);
    auto useForWrite(const auto& task);

private:
    std::atomic<TT> counter = 0;

    inline void waitWhile(TT v) const noexcept {
        counter.wait(v, o());
    }

    inline bool exchWeak(TT& vWas, TT vNew) noexcept {
        return counter.compare_exchange_weak(vWas, vNew, o());
    }

    inline TT load() const noexcept { return counter.load(o()); }
};

//*****************************************************
template <class TT>
struct AtomicMutexReadLocker {
    using Mtx = AtomicMutex<TT>;

    AtomicMutexReadLocker(Mtx& mtx)
        : ptr(&mtx) { mtx.lockForRead(); }

    ~AtomicMutexReadLocker() {
        ptr->unlockForRead(); }

    auto run(const auto& task) const {
        return task();
    }

private:
    Mtx* ptr = nullptr;
};

//*****************************************************
template <class TT>
struct AtomicMutexWriteLocker {
    using Mtx = AtomicMutex<TT>;

    AtomicMutexWriteLocker(Mtx& mtx)
        : ptr(&mtx) { mtx.lockForWrite(); }

    ~AtomicMutexWriteLocker() {
        ptr->unlockForWrite(); }

    auto run(const auto& task) const {
        return task();
    }

private:
    Mtx* ptr = nullptr;
};

template <class TT>
struct AtomicMutexWriteLazyLocker {
    using Mtx = AtomicMutex<TT>;

    AtomicMutexWriteLazyLocker(Mtx& mtx)
        : ptr(&mtx) { mtx.lockForWriteLazy(); }

    ~AtomicMutexWriteLazyLocker() {
        ptr->unlockForWrite(); }

    auto run(const auto& task) const {
        return task();
    }

private:
    Mtx* ptr = nullptr;
};

//*****************************************************

template <class TT>
auto AtomicMutex<TT>::useForRead(const auto& task) {
    return AtomicMutexReadLocker<TT>(*this).run(task);
}

template <class TT>
auto AtomicMutex<TT>::useForWrite(const auto& task) {
    return AtomicMutexWriteLocker<TT>(*this).run(task);
}

//*****************************************************

template <class TT>
void AtomicMutex<TT>::lockForWriteGreedy() {

    TT vv = load();

    while (true) {
        if (vv > 0) {
            if (exchWeak(vv, -2 - vv))
                break;

        } else if (vv == 0) {
            if (exchWeak(vv, -1))
                return;

        } else {
            waitWhile(vv);
            vv = load();
        }
    }

    vv = load();

    while (true) {

        if (vv == -2) {
            if (exchWeak(vv, -1))
                return;

        } else {
            assert( vv < -2 );
            waitWhile(vv);
            vv = load();
        }
    }
}

template <class TT>
void AtomicMutex<TT>::lockForWriteLazy() {

    TT vv = load();

    while (true) {
        if (vv == 0) {
            if (exchWeak(vv, -1))
                return;
        } else {
            waitWhile(vv);
            vv = load();
            continue;
        }
    }
}

template <class TT>
void AtomicMutex<TT>::unlockForWrite() {

    TT v = -1;
    while (!exchWeak(v, 0));

    counter.notify_all(); // кто первый, того и тапки
}

template <class TT>
bool AtomicMutex<TT>::tryLockForRead() {

    TT vv = load();

    while (true) {
        if (vv >= 0) {
            if (exchWeak(vv, vv+1))
                return true;
        } else
            return false;
    }
}

template <class TT>
void AtomicMutex<TT>::lockForRead() {

    TT vv = load();

    while (true) {

        if (vv >= 0) {
            if (exchWeak(vv, vv+1)) {
                return;

            }

        } else {
            waitWhile(vv);
            vv = load();
        }
    }
}

template <class TT>
void AtomicMutex<TT>::unlockForRead() {

    TT vv = load();

    while (vv > 0) {
        if (exchWeak(vv, vv-1)) {

            // если был один читатель, и успешное завершение, тогда vv не изменяется...
            if (vv == 1) // ждать могут только на запись
                counter.notify_one(); // здесь одного достаточно, но при разблокировки из записи уже нотифи всех.

            return;
        }
    }

    assert( vv <= -3 );

    while (true) {
        if (exchWeak(vv, vv+1)) {
            if (vv == -3)
                // такой на запись один, но который из них неизвестно
                counter.notify_all();
            return;
        }
    }

}


#endif // __AtomicMutex_h__

