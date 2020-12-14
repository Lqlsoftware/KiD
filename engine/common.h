#pragma once

#include "../include/db.hpp"

#include "util.h"

namespace KiD {
    /*
     * Hash line
     *  Total kv count: 229037390
     *  Prime about 5% over size
     */
    static const int64_t    KV_PER_LINE = 5;
    static const int64_t    HASH_LINE_CNT = 48097859;
    static const uint64_t   VPOS_MASK = 0x000000ffffffffff;

    /*
     * Heap table
     */
    static const int64_t    KEY_CNT_IN_LINE = 16;
    static const int64_t    KEY_LINE_CNT = 51729744;
    static const int64_t    VALUE_CNT = (KEY_CNT_IN_LINE * KEY_LINE_CNT);
    static const int64_t    THREAD_CNT = 16;
    static const uint64_t   THREAD_MASK = 0x000000000000000f;
    static const int64_t    VALUE_CNT_PER_THREAD = 48*1024*1024;
    static const int64_t    THREAD_BUFFER_VCNT = 16;


    struct Key {
        uint64_t k[2];

        bool operator== (const Key &key) const {
            return k[0] == key.k[0] && k[1] == key.k[1];
        }

        bool operator== (const Slice &key) const {
            return k[0] == ((uint64_t*)(key.data()))[0] && k[1] == ((uint64_t*)(key.data()))[1]);
        }

        void Set(const Slice &key) {
            k[0] = ((uint64_t*)(key.data()))[0];
            k[1] = ((uint64_t*)(key.data()))[1];
        }

        Slice GetSliceKey() {
            return Slice((char*)k, 16);
        };
    };

    struct Value {
        char v[80];

        Value &operator= (const Value &value) {
            memcpy(v, value.v, 80);
            return *this;
        }
    };
}
