#pragma once

#include <stdint.h>
#include <nmmintrin.h>

#include "common.h"
#include "util.h"

namespace KiD {
    struct OccupyHandle;
    struct HashLine {
        Key         keys[KV_PER_LINE];
        uint64_t    vals[KV_PER_LINE];
        volatile    uint8_t count;
        volatile    uint8_t padding[7];

        void Set(const uint8_t index, const uint64_t vpos) {
            vals[index] = vpos;
        }

        uint64_t Get(const uint8_t index) {
            while (true) {
                if (vals[index] == UINT64_MAX) {
                    PAUSE();
                    continue;
                }
                uint64_t ret = vals[index];
                return ret;
            }
        }

        bool atomic_inc_(volatile uint8_t *p, const uint8_t limit, uint8_t &prev) {
            uint8_t oldv = *p;
            while (true) {
                if (oldv >= limit) return false;
                uint8_t cmpv = oldv;
                if (cmpv == (oldv = ATOMIC_VCAS(p, oldv, oldv + 1))) {
                    prev = oldv;
                    return true;
                }
                PAUSE();
            }
        }

        bool Occupy(const Slice &key, OccupyHandle &oh) {
            switch (count) {
                case 5:
                    _mm_prefetch((this->keys + 4), _MM_HINT_T0);
                    if (keys[0] == key) {
                        oh.hline = this;
                        oh.index = 0;
                        oh.is_exist = true;
                        return true;
                    } else if (keys[1] == key) {
                        oh.hline = this;
                        oh.index = 1;
                        oh.is_exist = true;
                        return true;
                    } else if (keys[2] == key) {
                        oh.hline = this;
                        oh.index = 2;
                        oh.is_exist = true;
                        return true;
                    } else if (keys[3] == key) {
                        oh.hline = this;
                        oh.index = 3;
                        oh.is_exist = true;
                        return true;
                    } else if (keys[4] == key) {
                        oh.hline = this;
                        oh.index = 4;
                        oh.is_exist = true;
                        return true;
                    }
                    break;
                case 4:
                    if (keys[3] == key) {
                        oh.hline = this;
                        oh.index = 3;
                        oh.is_exist = true;
                        return true;
                    }
                case 3:
                    if (keys[2] == key) {
                        oh.hline = this;
                        oh.index = 2;
                        oh.is_exist = true;
                        return true;
                    }
                case 2:
                    if (keys[1] == key) {
                        oh.hline = this;
                        oh.index = 1;
                        oh.is_exist = true;
                        return true;
                    }
                case 1:
                    if (keys[0] == key) {
                        oh.hline = this;
                        oh.index = 0;
                        oh.is_exist = true;
                        return true;
                    }
                default:
                    break;
            }

            uint8_t prev = 0;
            if (atomic_inc_(&count, KV_PER_LINE, prev)) {
                vals[prev] = UINT64_MAX;
                keys[prev].Set(key);
                oh.hline = this;
                oh.index = prev;
                oh.is_exist = false;
                return true;
            }

            return false;
        }

        bool Get(const Slice &key, OccupyHandle &oh, bool &is_fullfilled) {

            switch (count) {
                case 5:
                    _mm_prefetch((this->keys + 4), _MM_HINT_T0);
                    if (keys[0] == key) {
                        if (vals[0] == UINT64_MAX) {
                            is_fullfilled = false;
                            return false;
                        }
                        oh.hline = this;
                        oh.index = 0;
                        oh.is_exist = true;
                        return true;
                    } else if (keys[1] == key) {
                        if (vals[1] == UINT64_MAX) {
                            is_fullfilled = false;
                            return false;
                        }
                        oh.hline = this;
                        oh.index = 1;
                        oh.is_exist = true;
                        return true;
                    } else if (keys[2] == key) {
                        if (vals[2] == UINT64_MAX) {
                            is_fullfilled = false;
                            return false;
                        }
                        oh.hline = this;
                        oh.index = 2;
                        oh.is_exist = true;
                        return true;
                    } else if (keys[3] == key) {
                        if (vals[3] == UINT64_MAX) {
                            is_fullfilled = false;
                            return false;
                        }
                        oh.hline = this;
                        oh.index = 3;
                        oh.is_exist = true;
                        return true;
                    } else if (keys[4] == key) {
                        if (vals[4] == UINT64_MAX) {
                            is_fullfilled = false;
                            return false;
                        }
                        oh.hline = this;
                        oh.index = 4;
                        oh.is_exist = true;
                        return true;
                    }
                    break;
                case 4:
                    if (keys[3] == key) {
                        if (vals[3] == UINT64_MAX) {
                            is_fullfilled = false;
                            return false;
                        }
                        oh.hline = this;
                        oh.index = 3;
                        oh.is_exist = true;
                        return true;
                    }
                case 3:
                    if (keys[2] == key) {
                        if (vals[2] == UINT64_MAX) {
                            is_fullfilled = false;
                            return false;
                        }
                        oh.hline = this;
                        oh.index = 2;
                        oh.is_exist = true;
                        return true;
                    }
                case 2:
                    if (keys[1] == key) {
                        if (vals[1] == UINT64_MAX) {
                            is_fullfilled = false;
                            return false;
                        }
                        oh.hline = this;
                        oh.index = 1;
                        oh.is_exist = true;
                        return true;
                    }
                case 1:
                    if (keys[0] == key) {
                        if (vals[0] == UINT64_MAX) {
                            is_fullfilled = false;
                            return false;
                        }
                        oh.hline = this;
                        oh.index = 0;
                        oh.is_exist = true;
                        return true;
                    }
                default:
                    break;
            }
            is_fullfilled = (count >= KV_PER_LINE);
            return false;
        }

        bool Replace(const Slice &key, const uint64_t new_v, uint64_t &old_v) {

            switch (count) {
                case 5:
                    _mm_prefetch((this->keys + 4), _MM_HINT_T0);
                    if (keys[0] == key) {
                        old_v = vals[0];
                        vals[0] = new_v;
                        return true;
                    } else if (keys[1] == key) {
                        old_v = vals[1];
                        vals[1] = new_v;
                        return true;
                    } else if (keys[2] == key) {
                        old_v = vals[2];
                        vals[2] = new_v;
                        return true;
                    } else if (keys[3] == key) {
                        old_v = vals[3];
                        vals[3] = new_v;
                        return true;
                    } else if (keys[4] == key) {
                        old_v = vals[4];
                        vals[4] = new_v;
                        return true;
                    }
                    break;
                case 4:
                    if (keys[3] == key) {
                        old_v = vals[3];
                        vals[3] = new_v;
                        return true;
                    }
                case 3:
                    if (keys[2] == key) {
                        old_v = vals[2];
                        vals[2] = new_v;
                        return true;
                    }
                case 2:
                    if (keys[1] == key) {
                        old_v = vals[1];
                        vals[1] = new_v;
                        return true;
                    }
                case 1:
                    if (keys[0] == key) {
                        old_v = vals[0];
                        vals[0] = new_v;
                        return true;
                    }
                default:
                    break;
            }

            uint8_t prev = 0;
            if (atomic_inc_(&count, KV_PER_LINE, prev)) {
                old_v = UINT64_MAX;
                vals[prev] = new_v;
                keys[prev].Set(key);
                return true;
            }
            return false;
        }
    };

    struct OccupyHandle {
        HashLine    *hline;
        uint8_t     index;
        bool        is_exist;

        void Set(const uint64_t vpos) {
            hline->Set(index, vpos);
        }

        uint64_t Get() {
            return hline->Get(index);
        }
    };
}
