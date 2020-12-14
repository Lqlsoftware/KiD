#pragma once

#include <stdint.h>

#include "../include/db.hpp"
#include "../include/rte/rte_memcpy.h"

#include "common.h"
#include "util.h"

#include "allocator.h"
#include "hashmap.h"

#define MEMCPY rte_memcpy

namespace KiD {
    /*
     * Index of HOT
     */
    struct Pos {
        union {
            struct {
                uint32_t pos_in_part_;
                uint16_t mem_len_;
                uint8_t part_;
                uint8_t version_;
            };
            uint64_t v_;
        };
        Pos(const uint64_t v) : v_(v) {};
        Pos(const uint64_t part, const uint64_t pos_in_part, const uint64_t mem_len)
                : pos_in_part_(pos_in_part),
                  mem_len_(mem_len),
                  part_(part),
                  version_(0) {
        }
        uint32_t GetPosInPart() const {
            return pos_in_part_;
        }
        uint8_t GetPart() const {
            return part_;
        }
        uint16_t GetMemLen() const {
            return mem_len_;
        }
        uint8_t GetVersion() const {
            return version_;
        }
        void SetVersion(const uint8_t version) {
            version_ = version;
        }
    };

    /*
     * KVItem in HOT
     */
    struct KVItem {
        uint16_t checksum;
        uint16_t mem_len;
        uint16_t value_len;
        uint8_t version;
        uint8_t padding;
        char key[16];
        char value[0];
        KVItem() {
            memset(this, 0, sizeof(*this));
        }
        bool IsEOF() {
            return (0 == mem_len && 0 == value_len);
        }
        Slice GetKey() const {
            return Slice((char*)key, sizeof(key));
        }
    };

    class HeapTable {
        static const int64_t    TOTAL_DATA_SIZE = 64L*1024*1024*1024;
        static const int64_t    THREAD_DATA_SIZE = TOTAL_DATA_SIZE / THREAD_CNT;
        struct FormattedData {
            char d[THREAD_CNT][THREAD_DATA_SIZE];
        };
    public:
        void Init(void *data, Hashmap &index);
        void PutKV(const Slice &key, const Slice &value, const bool overwrite, uint64_t &pos);
        void PutNewKV(const Slice &key, const Slice &value, uint64_t &pos);
        void RetireKV(const uint64_t pos);
        void GetValue(uint64_t &pos, std::string &value);
        void LogInfo();

        HeapTable();
        ~HeapTable();
    private:
        const KVItem *get_eof_item_();
        KVItem *get_item_(const uint64_t pos);
        void recovery_(Hashmap &index, const KVItem &item, const Pos &pos);
        bool atomic_add_(uint64_t *p, const uint64_t delta, const uint64_t limit, uint64_t &prev);
        KVItem *get_tc_item_(const uint32_t version, const uint16_t mem_len, const Slice &key, const Slice &value);
        uint16_t hash_(const void *data, const uint64_t len);
    private:
        FormattedData   *data_;
        ThreadLocalInfo *tlis_;
        Freelist        *freelists_;
    };

}

