
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

    void lockForWrite();
    void unlockForWrite();
    void lockForRead();
    void unlockForRead();

private:
    std::atomic<TT> counter = 0;
};

//*****************************************************
template <class TT>
struct AtomicMutexReadLocker {
    using Mtx = AtomicMutex<TT>;

    Mtx* ptr = nullptr;

    AtomicMutexReadLocker(Mtx& mtx)
        : ptr(&mtx) { mtx.lockForRead(); }

    ~AtomicMutexReadLocker() {
        ptr->unlockForRead(); }
};

//*****************************************************
template <class TT>
struct AtomicMutexWriteLocker {
    using Mtx = AtomicMutex<TT>;

    Mtx* ptr = nullptr;

    AtomicMutexWriteLocker(Mtx& mtx)
        : ptr(&mtx) { mtx.lockForWrite(); }

    ~AtomicMutexWriteLocker() {
        ptr->unlockForWrite(); }
};

//*****************************************************
template <class TT>
void AtomicMutex<TT>::lockForWrite() {
    TT v = 0;
    while (!counter.compare_exchange_weak(v, -1)) {
        counter.wait(v);
        v = 0;
    }
}

template <class TT>
void AtomicMutex<TT>::unlockForWrite() {
    counter = 0;
    counter.notify_all();
}

template <class TT>
void AtomicMutex<TT>::lockForRead() {
    while (true) {
        TT v = counter.load();
        if (v < 0)
            counter.wait(v);
        else if (counter.compare_exchange_weak(v, v+1))
            break;
    }
}

template <class TT>
void AtomicMutex<TT>::unlockForRead() {
    while (true) {
        TT v = counter.load();
        assert( v > 0 );
        if (counter.compare_exchange_weak(v, v-1)) {
            if (v == 1)
                counter.notify_all();
            break;
        }
    }
}

#endif // __AtomicMutex_h__
