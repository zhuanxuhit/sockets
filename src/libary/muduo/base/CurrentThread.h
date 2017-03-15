// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include <stdint.h>

namespace muduo {
    namespace CurrentThread {
        // internal
        // 在Thread.cc中定义
        extern __thread int t_cachedTid;
        extern __thread char t_tidString[32];
        extern __thread int t_tidStringLength;
        extern __thread const char *t_threadName;

        void cacheTid();

        inline int tid() {
            // 优化方法
            #define  unlikely(x)      __builtin_expect(!!(x), 0)
            // https://my.oschina.net/moooofly/blog/175019
            // #define  likely(x)        __builtin_expect(!!(x), 1)
//            if (__builtin_expect(t_cachedTid == 0, 0)) {
            if (unlikely(t_cachedTid == 0)) {
                cacheTid();
            }
            return t_cachedTid;
        }

        inline const char *tidString() // for logging
        {
            return t_tidString;
        }

        inline int tidStringLength() // for logging
        {
            return t_tidStringLength;
        }

        inline const char *name() {
            return t_threadName;
        }

        bool isMainThread();

        void sleepUsec(int64_t usec);
    }
}

#endif
