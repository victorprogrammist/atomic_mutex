# atomic_mutex

Мютекс основанный на std::atomic.

Сделан с целью инстанцирования в элементы множеств для блокировки
данных этого элемента без блокировки остальных элементов множества.

Это не для блокировки множества для добавления или удаления элементов,
это блокировка данных самого элемента.

В отличии от классических std::mutex или std::shared_mutex имеет меньший размер.
Например AtomicMutex<char> будет иметь размер 1 байт.
В то же время, на gcc sizeof(std::mutex) == 40 байт, а sizeof(std::shared_mutex) == 56 байт.

Вероятно в момент ожидания разблокирования будут системой создаваться дополнительные
структуры, но предполагается, что пересечения блокировок для элементов множества
происходят редко.

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
