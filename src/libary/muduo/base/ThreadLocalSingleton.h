// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo {
// 通过 __thread
    template<typename T>
    class ThreadLocalSingleton : boost::noncopyable {
    public:
        // 为什么需要 __thread 和 TSD 两个同时用呢？
        // 我们还是从目的出发：我们希望能够得到每个线程都有单一的对象，
        // 并且对象能够在线程结束的时候自动释放
        // 跟 ThreadLocal 的区别就是 ThreadLocalSingleton 可以不用声明
        // ThreadLocalSingleton<Test>::instance() 方式处处获取
        // 而 ThreadLocal 则必须自己先声明再获取
        static T &instance() {
            if (!t_value_) {
                t_value_ = new T();
                deleter_.set(t_value_);
            }
            return *t_value_;
        }

        static T *pointer() {
            return t_value_;
        }

    private:
        ThreadLocalSingleton();

        ~ThreadLocalSingleton();

        static void destructor(void *obj) {
            assert(obj == t_value_);
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy;
            (void) dummy;
            delete t_value_;
            t_value_ = 0;
        }

        class Deleter {
        public:
            Deleter() {
                pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
            }

            ~Deleter() {
                pthread_key_delete(pkey_);
            }

            void set(T *newObj) {
                assert(pthread_getspecific(pkey_) == NULL);
                pthread_setspecific(pkey_, newObj);
            }

            pthread_key_t pkey_;
        };

        static __thread T *t_value_;
        static Deleter deleter_;
    };

    template<typename T>
    __thread T *ThreadLocalSingleton<T>::t_value_ = 0;

    template<typename T>
    typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}
#endif
