#pragma once

#include <stdint.h>

#include "common.h"
#include "util.h"

#include "hashline.h"

namespace KiD {
    class Hashmap {
    public:
        Hashmap();

        ~Hashmap();

        bool Get(const Slice &key, OccupyHandle &oh);

        void Occupy(const Slice &key, OccupyHandle &oh);

        void Replace(const Slice &key, const uint64_t new_v, uint64_t &old_v);

    private:
        HashLine *hlines_;
    }
}