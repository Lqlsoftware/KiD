#include <sys/syscall.h>
#include <stdlib.h>
#include <execinfo.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "util.h"

namespace KiD {

    int64_t gettid() {
        static __thread int64_t tid = -1;
        if (UNLIKELY(-1 == tid)) {
            tid = static_cast<int64_t>(syscall(__NR_gettid));
        }
        return tid;
    }

    int64_t gettno() {
        static __thread int64_t tno = -1;
        static int64_t tno_seq = 0;
        if (UNLIKELY(-1 == tno)) {
            tno = __sync_fetch_and_add(&tno_seq, 1);
        }
        return tno;
    }

    void BT() {
        static const int64_t BT_BUF_SIZE = 128;
        void *buffer[BT_BUF_SIZE] = {nullptr};
        int nptrs = backtrace(buffer, BT_BUF_SIZE);
        char **strings = backtrace_symbols(buffer, nptrs);
        if (nullptr != strings) {
            for (int i = 0; i < nptrs; ++i) {
                L(INFO, "%s", strings[i]);
            }
            free(strings);
        }
    }

    const char *PA(const uint64_t *array, const int64_t length) {
        static const int64_t PA_BUFFER_SIZE = 1024;
        static __thread char BUFFER[MAX_THREAD_COUNT][PA_BUFFER_SIZE];
        static __thread int64_t buffer_index = 0;
        char *buffer = BUFFER[buffer_index++%MAX_THREAD_COUNT];
        int64_t pos = 0;
        for (int64_t i = 0; i < length; ++i) {
            pos += snprintf(buffer + pos, PA_BUFFER_SIZE - pos, "%lu ", array[i]);
        }
        return buffer;
    }

    const char *Sprintf(const char *fmt, ...) {
        static const int64_t BUFFER_SIZE = 4096;
        static const int64_t BUFFER_CNT = 16;
        static __thread char BUFFER[BUFFER_CNT][BUFFER_SIZE];
        static __thread int64_t index = 0;

        char *ret = BUFFER[index++%BUFFER_CNT];
        va_list args;
        va_start(args, fmt);
        vsnprintf(ret, BUFFER_SIZE, fmt, args);
        va_end(args);
        return ret;
    }

    FILE *&LogFileHandle() {
        static FILE *file = nullptr;
        return file;
    }

    void LogRUsage(const char *function, const int64_t line) {
        static __thread struct rusage last_ru;
        static __thread int64_t last_time_us = 0;
        if (0 == last_time_us) {
            memset(&last_ru, 0, sizeof(last_ru));
        }

        struct rusage ru;
        int64_t time_us = get_cur_microseconds_time();

        if (0 != getrusage(RUSAGE_SELF, &ru)) {
            L(WARN, "[%s:%ld] getrusage fail, errno=%u", function, line, errno);
        } else {
            L(INFO, "[%s:%ld] getrusage, ru_utime=%ld ru_stime=%ld ru_maxrss=%ld ru_ixrss=%ld ru_idrss=%ld ru_isrss=%ld ru_minflt=%ld ru_majflt=%ld ru_nswap=%ld ru_inblock=%ld ru_oublock=%ld ru_msgsnd=%ld ru_msgrcv=%ld ru_nsignals=%ld ru_nvcsw=%ld ru_nivcsw=%ld",
              function,
              line,
              tv_to_microseconds(ru.ru_utime),
              tv_to_microseconds(ru.ru_stime),
              ru.ru_maxrss,
              ru.ru_ixrss,
              ru.ru_idrss,
              ru.ru_isrss,
              ru.ru_minflt,
              ru.ru_majflt,
              ru.ru_nswap,
              ru.ru_inblock,
              ru.ru_oublock,
              ru.ru_msgsnd,
              ru.ru_msgrcv,
              ru.ru_nsignals,
              ru.ru_nvcsw,
              ru.ru_nivcsw);

            double timeu = (double)(time_us - last_time_us) / 1000000.0;
            L(INFO, "[%s:%ld] getrusage gradient(per-second), timeu=%0.2f ru_utime=%0.2lf ru_stime=%0.2lf ru_maxrss=%0.2lf ru_ixrss=%0.2lf ru_idrss=%0.2lf ru_isrss=%0.2lf ru_minflt=%0.2lf ru_majflt=%0.2lf ru_nswap=%0.2lf ru_inblock=%0.2lf ru_oublock=%0.2lf ru_msgsnd=%0.2lf ru_msgrcv=%0.2lf ru_nsignals=%0.2lf ru_nvcsw=%0.2lf ru_nivcsw=%0.2lf",
              function,
              line,
              timeu,
              (tv_to_microseconds(ru.ru_utime) - tv_to_microseconds(last_ru.ru_utime)) / timeu,
              (tv_to_microseconds(ru.ru_stime) - tv_to_microseconds(last_ru.ru_stime)) / timeu,
              (ru.ru_maxrss   - last_ru.ru_maxrss)    / timeu,
              (ru.ru_ixrss    - last_ru.ru_ixrss)     / timeu,
              (ru.ru_idrss    - last_ru.ru_idrss)     / timeu,
              (ru.ru_isrss    - last_ru.ru_isrss)     / timeu,
              (ru.ru_minflt   - last_ru.ru_minflt)    / timeu,
              (ru.ru_majflt   - last_ru.ru_majflt)    / timeu,
              (ru.ru_nswap    - last_ru.ru_nswap)     / timeu,
              (ru.ru_inblock  - last_ru.ru_inblock)   / timeu,
              (ru.ru_oublock  - last_ru.ru_oublock)   / timeu,
              (ru.ru_msgsnd   - last_ru.ru_msgsnd)    / timeu,
              (ru.ru_msgrcv   - last_ru.ru_msgrcv)    / timeu,
              (ru.ru_nsignals - last_ru.ru_nsignals)  / timeu,
              (ru.ru_nvcsw    - last_ru.ru_nvcsw)     / timeu,
              (ru.ru_nivcsw   - last_ru.ru_nivcsw)    / timeu);

            memcpy(&last_ru, &ru, sizeof(last_ru));
            last_time_us = time_us;
        }
        return;
    }
}