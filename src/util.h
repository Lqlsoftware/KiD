#pragma once

#include <sys/syscall.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <linux/aio_abi.h>
#include <string.h>
#include <errno.h>
#include <nmmintrin.h>

#include "../include/xxhash.h"
#include "../include/db.hpp"

#define MAX_THREAD_COUNT 1024

////////////////////////////////////////////////////////////////////////////////////////////////////

#define DIRECT_OPEN_FLAG (O_RDWR | O_CREAT | O_DIRECT | O_NOATIME)
#define OPEN_FLAG (O_RDWR | O_CREAT | O_NOATIME)
#define OPEN_MODE (S_IRWXU)

////////////////////////////////////////////////////////////////////////////////////////////////////

#define LIKELY(x) __builtin_expect(!!(x),1)
#define UNLIKELY(x) __builtin_expect(!!(x),0)
#define PAUSE() asm("pause\n")
#define UNUSED(v) ((void)(v))

#define CACHE_ALIGNED __attribute__((aligned(64)))
#define MEM_BARRIER() __sync_synchronize()
#define ATOMIC_FAA(val, addv) __sync_fetch_and_add((val), (addv))
#define ATOMIC_AAF(val, addv) __sync_add_and_fetch((val), (addv))
#define ATOMIC_SET(val, newv) __sync_lock_test_and_set((val), (newv))
#define ATOMIC_VCAS(val, cmpv, newv) __sync_val_compare_and_swap((val), (cmpv), (newv))
#define ATOMIC_BCAS(val, cmpv, newv) __sync_bool_compare_and_swap((val), (cmpv), (newv))

namespace KiD {

    static inline int64_t tv_to_microseconds(const timeval &tp) {
        return (((int64_t) tp.tv_sec) * 1000000 + (int64_t) tp.tv_usec);
    }

    static inline timespec microseconds_to_ts(const int64_t microseconds) {
        struct timespec ts;
        ts.tv_sec = microseconds / 1000000;
        ts.tv_nsec = (microseconds % 1000000) * 1000;
        return ts;
    }

    static inline int64_t get_cur_microseconds_time(void) {
        struct timeval tp;
        gettimeofday(&tp, NULL);
        return tv_to_microseconds(tp);
    }

    static inline const struct tm *get_cur_tm() {
        static __thread struct tm cur_tms[MAX_THREAD_COUNT];
        static __thread int64_t pos = 0;
        time_t t = time(NULL);
        return localtime_r(&t, &cur_tms[pos++ % MAX_THREAD_COUNT]);
    }

    static inline void get_cur_tm(struct tm &cur_tm) {
        time_t t = time(NULL);
        localtime_r(&t, &cur_tm);
    }

    int64_t gettid();

    int64_t gettno();

    void BT();

    const char *PA(const uint64_t *array, const int64_t length);

    static inline int64_t lower_align(const int64_t input, const int64_t align)
    {
        int64_t ret = input;
        ret = (input + align - 1) & ~(align - 1);
        ret = ret - ((ret - input + align - 1) & ~(align - 1));
        return ret;
    }

    static inline int64_t upper_align(const int64_t input, const int64_t mask)
    {
//    int64_t ret = input;
//    ret = (input | mask) + 1;
//    ret = (input + align - 1) & ~(align - 1);
        return (input | mask) + 1;
    }

    static inline int io_setup(unsigned nr, aio_context_t *ctxp)
    {
        return syscall(__NR_io_setup, nr, ctxp);
    }

    static inline int io_destroy(aio_context_t ctx)
    {
        return syscall(__NR_io_destroy, ctx);
    }

    static inline int io_submit(aio_context_t ctx, long nr,  struct iocb **iocbpp)
    {
        return syscall(__NR_io_submit, ctx, nr, iocbpp);
    }

    static inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
                                   struct io_event *events, struct timespec *timeout)
    {
        return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
    }

    static inline void io_prep_pread(struct iocb *iocb, int fd, void *buf, size_t count, long long offset)
    {
        memset(iocb, 0, sizeof(*iocb));
        iocb->aio_fildes = fd;
        iocb->aio_lio_opcode = IOCB_CMD_PREAD;
        iocb->aio_reqprio = 0;
        iocb->aio_buf = (uint64_t)buf;
        iocb->aio_nbytes = count;
        iocb->aio_offset = offset;
    }

    static inline void io_prep_pwrite(struct iocb *iocb, int fd, void *buf, size_t count, long long offset)
    {
        memset(iocb, 0, sizeof(*iocb));
        iocb->aio_fildes = fd;
        iocb->aio_lio_opcode = IOCB_CMD_PWRITE;
        iocb->aio_reqprio = 0;
        iocb->aio_buf = (uint64_t)buf;
        iocb->aio_nbytes = count;
        iocb->aio_offset = offset;
    }

    const char *Sprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

    FILE *&LogFileHandle();

    static inline void SetLogFileHandle(FILE *file) {
        LogFileHandle() = file;
    }

    static inline FILE *GetLogFileHandle() {
        return LogFileHandle();
    }

    void LogRUsage(const char *function, const int64_t line);

    static inline uint16_t BKDRHash(const char *buf, const int64_t size)
    {
        uint16_t seed = 1313; // 31 131 1313 13131 131313 etc..
        uint16_t hash = 0;

        char *str = (char *)buf;
        for(int64_t i = 0; i < size; i++)
            hash = hash * seed + (*str++);

        return (hash & 0x7FFF);
    }

    static inline uint32_t CRC32Hash(const Slice &key) {
        uint32_t *ikey = (uint32_t *)key.data();
        uint32_t tmp = 0;
        tmp = _mm_crc32_u32(tmp, ikey[0]);
        tmp = _mm_crc32_u32(tmp, ikey[1]);
        tmp = _mm_crc32_u32(tmp, ikey[2]);
        tmp = _mm_crc32_u32(tmp, ikey[3]);
        return tmp;
    }

    static inline uint64_t CRC64Hash(const Slice &key) {
        uint64_t *ikey = (uint64_t *)key.data();
        uint64_t tmp = 0;
        tmp = _mm_crc32_u64(tmp, ikey[0]);
        tmp = _mm_crc32_u64(tmp, ikey[1]);
        return tmp;
    }

    static inline uint64_t XXHASH(const Slice &key) {
        return (uint64_t)XXH3_64bits((char *)key.data(), 16);
    }

    static inline uint64_t XXHASH_64(const Slice &key) {
        return (uint64_t)XXH64((char *)key.data(), 16, 131313);
    }

    static inline uint32_t XXHASH_32(const Slice &key) {
        return (uint32_t)XXH32((char *)key.data(), 16, 131313);
    }
}

#define _DEBUG_ "DEBUG"
#define _INFO_  "INFO"
#define _WARN_  "WARN"
#define _ERROR_ "ERROR"

#define DEBUG
#define INFO
#define WARN
#define ERROR

#define _LOG_(_level_, _file_, _line_, _function_, __fmt__, args...) \
    do { \
        const struct tm *cur_tm = KiD::get_cur_tm(); \
        int64_t usec = KiD::get_cur_microseconds_time() % 1000000; \
        fprintf(KiD::GetLogFileHandle(), "[TAIR] [%04d-%02d-%02d %02d:%02d:%02d.%06ld] %s %s:%d:%s [%ld:%ld] " __fmt__ "\n", \
            cur_tm->tm_year + 1900, \
            cur_tm->tm_mon + 1, \
            cur_tm->tm_mday, \
            cur_tm->tm_hour, \
            cur_tm->tm_min, \
            cur_tm->tm_sec, \
            usec, \
            _level_, \
            _file_, \
            _line_, \
            _function_, \
            KiD::gettid(), \
            KiD::gettno(), \
            ##args); \
        fflush(KiD::GetLogFileHandle()); \
    } while(0)

#define L(__level__, __fmt__, args...) _LOG_(_##__level__##_, __FILE__, __LINE__, __FUNCTION__, __fmt__, ##args)

#define LS(ptr) ((NULL == ptr) ? "nil" : ptr)

#define THREAD_ONCE(f) \
    do { \
        static __thread bool flag = false; \
        if (!flag) { \
            L(INFO, "THREAD_ONCE"); \
            f; \
            flag = true; \
        } \
    } while(0)

#define RU() KiD::LogRUsage(__FUNCTION__, __LINE__)