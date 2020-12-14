#include <stdint.h>
#include <string.h>
#include <nmmintrin.h>

#include "common.h"
#include "util.h"
#include "hashmap.h"

namespace KiD {

    Hashmap::Hashmap() : hlines_(nullptr) {
        hlines_ = (HashLine *) aligned_alloc(sizeof(HashLine), sizeof(HashLine) * HASH_LINE_CNT);
        memset(hlines_, 0, sizeof(HashLine) * HASH_LINE_CNT);
    }

    Hashmap::~Hashmap() {
        if (nullptr != hlines_) {
            free(hlines_);
            hlines_ = nullptr;
        }
    }

    void Hashmap::Occupy(const Slice &key, OccupyHandle &oh) {
        uint32_t hash = CRC32Hash(key) % HASH_LINE_CNT;
        for (uint32_t i = 0; i < HASH_LINE_CNT; ++i) {
            if (hlines_[hash].Occupy(key, oh)) {
                return;
            }
            hash++;
            hash = UNLIKELY(hash == HASH_LINE_CNT) ? 0 : hash;
            _mm_prefetch(hlines_[hash].keys, _MM_HINT_T0);
        }
    }

    void Hashmap::Replace(const Slice &key, const uint64_t new_v, uint64_t &old_v) {
        uint32_t hash = CRC32Hash(key) % HASH_LINE_CNT;
        for (uint32_t i = 0; i < HASH_LINE_CNT; ++i) {
            if (hlines_[hash].Replace(key, new_v, old_v)) {
                return;
            }
            hash++;
            hash = UNLIKELY(hash == HASH_LINE_CNT) ? 0 : hash;
            _mm_prefetch(hlines_[hash].keys, _MM_HINT_T0);
        }
    }

    bool Hashmap::Get(const Slice &key, OccupyHandle &oh) {
        uint32_t hash = CRC32Hash(key) % HASH_LINE_CNT;
        for (uint32_t i = 0; i < HASH_LINE_CNT; ++i) {
            bool is_fullfilled = false;
            if (hlines_[hash].Get(key, oh, is_fullfilled)) {
                return true;
            }
            if (!is_fullfilled) {
                return false;
            }
            hash++;
            hash = UNLIKELY(hash == HASH_LINE_CNT) ? 0 : hash;
            _mm_prefetch(hlines_[hash].keys, _MM_HINT_T0);
        }
        return false;
    }
}