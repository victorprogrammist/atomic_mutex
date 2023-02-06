
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

using MutexInt = AtomicMutex<int>;
using MutexIntRead = AtomicMutexReadLocker<int>;
using MutexIntWrite = AtomicMutexWriteLocker<int>;

//*****************************************************
template <class TT>
struct AtomicMutex {
    static_assert( TT(-1) < 0, "Need signed numeric type");

    static auto o() {
        return std::memory_order_seq_cst; }

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

    void useForRead(const auto& task);
    void useForWrite(const auto& task);

private:
    std::atomic<TT> counter = 0;

    void waitWhile(TT v) const noexcept {
        counter.wait(v, o());
    }

    bool exch(TT& vWas, TT vNew) volatile noexcept {
        return counter.compare_exchange_weak(vWas, vNew, o());
    }

    void exchAss(TT vWas, TT vNew) volatile noexcept {
        bool r = exch(vWas, vNew);
        assert( r );
    }

    TT load() const volatile noexcept { return counter.load(o()); }
};

//*****************************************************
template <class TT>
struct AtomicMutexReadLocker {
    using Mtx = AtomicMutex<TT>;

    AtomicMutexReadLocker(Mtx& mtx)
        : ptr(&mtx) { mtx.lockForRead(); }

    ~AtomicMutexReadLocker() {
        ptr->unlockForRead(); }

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

private:
    Mtx* ptr = nullptr;
};

//*****************************************************

template <class TT>
void AtomicMutex<TT>::useForRead(const auto& task) {
    AtomicMutexReadLocker<TT> locker(*this);
    task();
}

template <class TT>
void AtomicMutex<TT>::useForWrite(const auto& task) {
    AtomicMutexWriteLocker<TT> locker(*this);
    task();
}

//*****************************************************

template <class TT>
void AtomicMutex<TT>::lockForWriteGreedy() {

    while (true) {
        volatile TT vv = load();
        TT v = vv;

        if (v < 0) {
            waitWhile(v);
            continue;
        }

        if (v > 0) {
            if (exch(v, -2 - v))
                break;
            continue;
        }

        if (exch(v, -1))
            return;
    }

    while (true) {
        volatile TT vv = load();
        TT v = vv;

        if (v < -2) {
            waitWhile(v);
            continue;
        }

        assert( v == -2 );

        if (exch(v, -1))
            break;
    }
}

template <class TT>
void AtomicMutex<TT>::lockForWriteLazy() {

    while (true) {
        volatile TT vv = load();
        TT v = vv;

        if (v) {
            waitWhile(v);
            continue;
        }

        if (exch(v, -1))
            return;
    }
}

template <class TT>
void AtomicMutex<TT>::unlockForWrite() {
    exchAss(-1, 0);
    counter.notify_all(); // кто первый, того и тапки
}

template <class TT>
bool AtomicMutex<TT>::tryLockForRead() {

    while (true) {
        volatile TT vv = load();
        TT v = vv;

        if (v < 0)
            return false;

        if (exch(v, v+1))
            return true;
    }
}

template <class TT>
void AtomicMutex<TT>::lockForRead() {

    while (true) {
        volatile TT vv = load();
        TT v = vv;

        if (v < 0)
            waitWhile(v);
        else if (exch(v, v+1))
            break;
    }
}

template <class TT>
void AtomicMutex<TT>::unlockForRead() {

    while (true) {

        volatile TT vv = load();
        TT v = vv;

        if (v < 0) {
            assert( v <= -3 );
            if (!exch(v, v+1))
                continue;
            if (v == -3)
                // такой на запись один, но который из них неизвестно
                counter.notify_all();
            return;
        }

        assert( v > 0 );

        if (!exch(v, v-1))
            continue;

        if (v == 1) // ждать могут только на запись
            counter.notify_one(); // одного достаточно

        return;
    }
}

#endif // __AtomicMutex_h__
