// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <stdlib.h> // atexit
#include <pthread.h>

namespace muduo {

    namespace detail {
// This doesn't detect inherited member functions!
// http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
        template<typename T>
        struct has_no_destroy {
#ifdef __GXX_EXPERIMENTAL_CXX0X__

            template<typename C>
            static char test(decltype(&C::no_destroy));

#else
            template <typename C> static char test(typeof(&C::no_destroy));
#endif

            template<typename C>
            static int32_t test(...);

            const static bool value = sizeof(test<T>(0)) == 1;
        };
    }

    template<typename T>
    class Singleton : boost::noncopyable {
    public:
        static T &instance() {
            // pthread_once
            pthread_once(&ponce_, &Singleton::init);
            assert(value_ != NULL);
            return *value_;
        }

    private:
        // 构造和析构函数都是私有的
        Singleton();

        ~Singleton();

        static void init() {
            value_ = new T();
            if (!detail::has_no_destroy<T>::value) {
                // 进程正常结束的时候会调用，包括 exit 和 从main return
                ::atexit(destroy);
            }
        }

        static void destroy() {
            // 检查class T 的声明一定要完整
            // 因为前向声明 class T;
            // delete T;  也是能通过编译的,但是运行时会出错
            typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
            T_must_be_complete_type dummy;
            (void) dummy;

            delete value_;
            value_ = NULL;
        }

    private:
        // 只有第一次调用 pthread_once 的线程才会调用 init_routine
        static pthread_once_t ponce_;
        static T *value_;
    };

    template<typename T>
    pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

    template<typename T>
    T *Singleton<T>::value_ = NULL;

}
#endif

