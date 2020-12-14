#include <libpmem.h>

#include "engine.h"
#include "util.h"

namespace KiD {
    Status NvmEngine::CreateOrOpen(const std::string &name, DB **dbptr, FILE *log_file) {
        SetLogFileHandle(log_file);
        if (nullptr != dbptr) {
            NvmEngine *e = new NvmEngine(name);
            *dbptr = e;
            return Ok;
        }
        return OutOfMemory;
    }

    Status NvmEngine::Get(const Slice &key, std::string *value) {
        Hashmap::OccupyHandle oh;
        /*
         * Searching in hash map
         */
        if (LIKELY(hm_.Get(key, oh))) {
            /*
             * Double check for concurrent read/write
             */
            while (true) {
                uint64_t pos = oh.Get();
                ht_.GetValue(pos, *value);
                if (pos == oh.Get()) {
                    return Ok;
                }
            }
        }
        return NotFound;
    }

    Status NvmEngine::Set(const Slice &key, const Slice &value) {
        Hashmap::OccupyHandle oh;
        hm_.Occupy(key, oh);

        uint64_t pos = 0;

        /*
         * Update
         *  retire after put operation
         */
        if (oh.is_exist) {
            pos = oh.Get();
            uint64_t old_pos = pos;
            ht_.PutKV(key, value, oh.is_exist, pos);
            oh.Set(pos);
            ht_.RetireKV(old_pos);
            return Ok;
        }

        /*
         * Put
         */
        ht_.PutKV(key, value, oh.is_exist, pos);
        oh.Set(pos);
        return Ok;
    }

    NvmEngine::NvmEngine(const std::string &name) : map_(nullptr) {
        if (nullptr ==
            (map_ = pmem_map_file(name.c_str(), PMEM_SIZE, PMEM_FILE_CREATE, 0666, &mapped_len_, &is_pmem_))) {
            L(WARN, "pmem_map_file fail, name=%s errno=%u", name.c_str(), errno);
            exit(1);
        }
        L(INFO, "pmem_map_file succ, name=%s mapped_len=%lu is_pmem=%d map=%p", name.c_str(), mapped_len_, is_pmem_,
          map_);

        aep_init_((char *) map_);

        ht_.Init(map_, hm_);
    }

    NvmEngine::~NvmEngine() {
        if (nullptr != map_) {
            pmem_unmap(map_, mapped_len_);
            map_ = nullptr;
        }
    }
}