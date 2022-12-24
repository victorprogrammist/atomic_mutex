# atomic_mutex

Мютекс основанный на std::atomic.

Варианты использования:

```cpp
MutexInt mtx;
...
mtx.lockForRead();
mtx.unlockForRead();
...
mtx.lockForWrite();
mtx.unlockForWrite();
```

```cpp
MutexInt mtx;
...
{
   MutexIntReadLocker locker(mtx);
}
...
{
   MutexIntWriteLocker locker(mtx);
}
...
```

```cpp
AtomicMutex<char> mtxChar;
std::cout << sizeof(mtxChar); // 1 байт
AtomicMutexReadLocker<char> locker(mtxChar);
```

Программный код очень не большой и понять его будет не сложно и без комметариев, если знаете что такое мютексы.
