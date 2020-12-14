#include <libpmem.h>
#include <stdint.h>
#include <string.h>

#include "../include/db.hpp"
#include "../include/xxhash.h"

#include "common.h"
#include "util.h"
#include "heaptable.h"
#include "hashmap.h"
#include "allocator.h"


namespace KiD {
    HeapTable::HeapTable()
            : data_(nullptr),
              tlis_(nullptr),
              freelists_(nullptr) {
        tlis_ = (ThreadLocalInfo *) aligned_alloc(64, sizeof(ThreadLocalInfo) * THREAD_CNT);
        void *freelists_buffer = aligned_alloc(64, sizeof(Freelist) * THREAD_CNT);
        memset(tlis_, 0, sizeof(ThreadLocalInfo) * THREAD_CNT);
        freelists_ = new(freelists_buffer) Freelist[THREAD_CNT];
    }

    HeapTable::~HeapTable() {
        // do nothing
    }

    void HeapTable::Init(void *data, Hashmap &index) {
        data_ = (FormattedData *) data;
        for (uint32_t i = 0; i < THREAD_CNT; ++i) {
            /*
             * Init with exit pmem file (Recovery)
             */
            uint64_t pos = 0;
            while (true) {
                KVItem *item = (KVItem *) &(data_->d[i][pos]);
                if (item->IsEOF() || item->checksum != hash_(&item->mem_len, 6 + 16 + item->value_len)) {
                    tlis_[i].data_index = pos;
                    break;
                }
                recovery_(index, *item, Pos(i, pos, item->mem_len));
                pos += item->mem_len;
            }
            tlis_[i].version = i;
            tlis_[i].gc_elastic_room = 1610612736;
        }
    }

    bool HeapTable::atomic_add_(uint64_t *p, const uint64_t delta, uint64_t limit, uint64_t &prev) {
        limit -= delta;
        uint64_t oldv = *p;
        while (true) {
            if (oldv > limit) return false;
            uint64_t cmpv = oldv;
            if (cmpv == (oldv = ATOMIC_VCAS(p, oldv, oldv + delta))) {
                prev = oldv;
                return true;
            }
            PAUSE();
        }
    }

    void HeapTable::PutKV(const Slice &key, const Slice &value, const bool overwrite, uint64_t &pos) {
        int64_t t = gettno() & THREAD_MASK;
        uint64_t data_index = UINT64_MAX;
        uint64_t alloc_size = sizeof(KVItem) + value.size();
        alloc_size = upper_align(alloc_size, MEMBLOCK_ALIGN_SIZE_MASK);

        KVItem *item = nullptr;
        Pos prev_p(pos);

        if (overwrite
            && freelists_[t].Pop(&(tlis_[t]), alloc_size, pos, 256)) {
            item = get_item_(pos);
        } else if (atomic_add_(&tlis_[t].data_index, alloc_size, UINT32_MAX, data_index)) {
            // allocate new kvitem
            Pos p(t, data_index, alloc_size);
            item = get_item_(p.v_);
            item->mem_len = p.GetMemLen();
            item->padding = 0;
            pos = p.v_;
        } else if (freelists_[t].Pop(&(tlis_[t]), alloc_size, pos)) {
            item = get_item_(pos);
        }

        Pos curr_p(pos);
        if (overwrite) {
            curr_p.SetVersion(prev_p.GetVersion() + 1);
            pos = curr_p.v_;
        }
        item->value_len = value.size();
        item->version = curr_p.GetVersion();
        ((uint64_t *) item->key)[0] = ((uint64_t *) (key.data()))[0];
        ((uint64_t *) item->key)[1] = ((uint64_t *) (key.data()))[1];
        pmem_memcpy(item->value, value.data(), value.size(), PMEM_F_MEM_NOFLUSH);
        item->checksum = hash_(&item->mem_len, 22 + item->value_len);
        pmem_persist(item, alloc_size);
    }

    void HeapTable::RetireKV(const uint64_t pos) {
        Pos p(pos);
        freelists_[gettno() & THREAD_MASK].Push(p.GetMemLen(), pos);
    }

    void HeapTable::GetValue(uint64_t &pos, std::string &value) {
        KVItem *item = get_item_(pos);
        if (item->value_len != value.size()) {
            value.resize(item->value_len);
        }
        MEMCPY(const_cast<char *>(value.data()), item->value, item->value_len);
    }

    const HeapTable::KVItem *HeapTable::get_eof_item_() {
        static KVItem eof;
        return &eof;
    }

    void HeapTable::recovery_(Hashmap &index, const KVItem &item, const Pos &pos) {
        Hashmap::OccupyHandle oh;
        index.Occupy(item.GetKey(), oh);
        if (oh.is_exist) {
            KVItem *prev = get_item_(oh.Get());
            if (item.version > prev->version) {
                oh.Set(pos.v_);
            }
        } else {
            oh.Set(pos.v_);
        }
    }

    KVItem *HeapTable::get_item_(const uint64_t pos) {
        Pos p(pos);
        KVItem *item = (KVItem *) &data_->d[p.GetPart()][p.GetPosInPart()];
        return item;
    }

    KVItem *
    HeapTable::get_tc_item_(const uint32_t version, const uint16_t mem_len, const Slice &key, const Slice &value) {
        static __thread char buffer[MEMBLOCK_TYPE_END];
        static __thread KVItem *ret = (KVItem *) buffer;
        ret->version = version;
        ret->mem_len = mem_len;
        ret->value_len = value.size();
        ((uint64_t *) ret->key)[0] = ((uint64_t *) (key.data()))[0];
        ((uint64_t *) ret->key)[1] = ((uint64_t *) (key.data()))[1];
        return ret;
    }

    void HeapTable::LogInfo() {
        for (uint32_t i = 0; i < THREAD_CNT; ++i) {
            freelists_[i].LogInfo();
        }
    }

    inline uint16_t HeapTable::hash_(const void *data, const uint64_t len) {
        /*
         * xxHASH3 64bit version
         */
        return (uint16_t) XXH3_64bits(data, len);
    }
}
