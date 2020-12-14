#pragma once

#include <pthread.h>

#include "../include/db.hpp"
#include "util.h"
#include "common.h"
#include "hashmap.h"
#include "heaptable.h"

namespace KiD {
    class NvmEngine : DB {
    public:
        /**
         * @param
         * name: file in AEP(exist)
         * dbptr: pointer of db object
         *
         */

        static Status CreateOrOpen(const std::string &name, DB **dbptr, FILE *log_file);
        Status Get(const Slice &key, std::string *value);
        Status Set(const Slice &key, const Slice &value);
        NvmEngine(const std::string &name);
        ~NvmEngine();

    private:
        struct Arg {
            char *base;
            uint64_t size;
        };

        static void *init_thread_func_(void *arg) {
            Arg *ta = (Arg *) arg;
            int64_t timeu = tavern::get_cur_microseconds_time();
            pmem_memset_nodrain(ta->base, 0, ta->size);
            L(INFO, "mem=%p pagefault timeu=%ld", ta->base, tavern::get_cur_microseconds_time() - timeu);
            return nullptr;
        }

        /**
         * Pre hot aep using page fault
         * @param mem
         */
        void aep_init_(char *mem) {
            /* Check already hooked or not
             *   Trick: by compare pmem_drain's address to know whether using Pin tools
             */
            if (0x401600 != (uint64_t) pmem_drain) {
                L(INFO, "drain is hooked, pmem_drain=%p", pmem_drain);
                return;
            }
            L(INFO, "drain is origin, will init aep, pmem_drain=%p", pmem_drain);

            /*
             * Multi-threads init
             */
            static const int64_t PER_THREAD_SIZE = PMEM_SIZE / THREAD_COUNT;
            pthread_t pds[THREAD_COUNT];
            Arg tas[THREAD_COUNT];
            for (int64_t i = 0; i < THREAD_COUNT; i++) {
                tas[i].mem = mem + i * PER_THREAD_SIZE;
                tas[i].size = PER_THREAD_SIZE;
                if (0 != pthread_create(&pds[i], nullptr, init_thread_func_, &tas[i])) {
                    L(WARN, "pthread_create fail, ret=%d", ret);
                }
            }
            for (int64_t i = 0; i < THREAD_COUNT; i++) {
                pthread_join(pds[i], nullptr);
            }
        };

    private:
        void        *map_;
        size_t      mapped_len_;
        int         is_pmem_;

        Hashmap     hm_;
        HeapTable   ht_;

        uint64_t    PMEM_SIZE = 64L * 1024 * 1024 * 1024;
        uint64_t    THREAD_COUNT = 16;
    };
}