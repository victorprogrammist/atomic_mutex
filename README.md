# atomic_mutex

Мютекс основанный на std::atomic.

Варианты использования:

```
MutexInt mtx;
...
mtx.lockForRead();
mtx.unlockForRead();
...
mtx.lockForWrite();
mtx.unlockForWrite();
```
```
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
```
AtomicMutex<char> mtxChar;
std::cout << sizeof(mtxChar); // 1 байте
AtomicMutexReadLocker<char> locker(mtxChar);
```

Программный код очень не большой и понять его будет не сложно и без комметариев, если знаете что такое мютексы.
