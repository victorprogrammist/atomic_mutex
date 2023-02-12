# atomic_mutex

Мютекс основанный на std::atomic.

Сделан с целью инстанцирования в элементы множеств для блокировки
данных этого элемента без блокировки остальных элементов множества.

Это не для блокировки множества для добавления или удаления элементов.
Это блокировка данных самого элемента.

В отличии от классических `std::mutex` или `std::shared_mutex` имеет меньший размер.
Например `AtomicMutex<char>` будет иметь размер 1 байт.
В то же время, на gcc `sizeof(std::mutex)` == 40 байт, а `sizeof(std::shared_mutex)` == 56 байт.

Вероятно в момент, когда мютекс переходит в режим ожидания разблокирования, операционкой будут создаваться дополнительные
структуры для оповещений этих ожиданий, но предполагается, что пересечения блокировок для элементов множества
происходят редко. По коду можете посмотреть, для каких случаев проиходят блокировки, код там очень компактный.

Варианты использования:

```cpp
MutexInt mtx; // atomic_mutex.h: using MutexInt = AtomicMutex<int>;
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
   // using MutexIntRead = AtomicMutexReadLocker<int>;
   MutexIntRead locker(mtx);
   ...
}
...
{
   // using MutexIntWrite = AtomicMutexWriteLocker<int>;
   MutexIntWrite locker(mtx);
   ...
}
...
// void AtomicMutex::useForRead(const auto& task);
mtx.useForRead([&] {
   doSomeThing();
});
...
// void AtomicMutex::useForWrite(const auto& task);
mtx.useForWrite([&] {
   doSomeThing();
});
```

```cpp
AtomicMutex<char> mtxChar;
std::cout << sizeof(mtxChar); // 1 байт
AtomicMutexReadLocker<char> locker(mtxChar);
```

