// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include <muduo/base/Mutex.h>  // MCHECK

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo {

    template<typename T>
// 对于PDO的数据，我们会直接使用 __thread的关键字
// 而对于非PDO的数据，我们则是使用  pthread_key_create
// 同过 pthread_key_create 创建的key 进程中的每个线程都能看到，但是每个线程可以通过
// pthread_setspecific 来设置线程自己的值
// pthread_getspecific 来获取线程自己的值
// int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));
// destructor 的作用是：当当前线程结束的时候，如果当前的key不为null，那会调用 destructor 来释放资源
    class ThreadLocal : boost::noncopyable {
    public:
        ThreadLocal() {
            // thread-specific data key creation
            MCHECK(pthread_key_create(&pkey_, &ThreadLocal::destructor));
        }

        ~ThreadLocal() {
            MCHECK(pthread_key_delete(pkey_));
        }

        T &value() {
            T *perThreadValue = static_cast<T *>(pthread_getspecific(pkey_));
            if (!perThreadValue) {
                T *newObj = new T();
                MCHECK(pthread_setspecific(pkey_, newObj));
                perThreadValue = newObj;
            }
            return *perThreadValue;
        }

    private:

        static void destructor(void *x) {
            T *obj = static_cast<T *>(x);
            // 技巧检查类型的完整性
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy;
            (void) dummy;
            delete obj;
        }

    private:
        pthread_key_t pkey_;
    };

}
#endif
