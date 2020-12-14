#pragma once

#include <stdint.h>
#include <string.h>

#include "page_arena.h"

namespace KiD {
    static const int64_t MEMBLOCK_TYPE_START = 80 + sizeof(KVItem);
    static const int64_t MEMBLOCK_TYPE_END = 1024 + sizeof(KVItem);
    static const int64_t MEMBLOCK_ALIGN_SIZE = 32;
    static const uint64_t MEMBLOCK_ALIGN_SIZE_MASK = 0x000000000000001f;
    static const int64_t MEMBLOCK_TYPE_COUNT = (MEMBLOCK_TYPE_END + MEMBLOCK_ALIGN_SIZE) / MEMBLOCK_ALIGN_SIZE + 1;

    struct ThreadLocalInfo {
        uint64_t gc_append_len;
        uint64_t gc_append_len_sum;
        uint64_t gc_elastic_room;
        uint64_t data_index;
        uint32_t version;
        uint8_t padding[28];
    };

    struct Freelist {
        struct Node {
            uint64_t pos;
            Node *next;
        };
        PageArena arena;
        Node *free_list_head;
        Node *lists[MEMBLOCK_TYPE_COUNT];
        uint64_t padding;

        Freelist() {
            free_list_head = nullptr;
            memset(lists, 0, sizeof(lists));
            for (int64_t i = 0; i < 4194272; ++i) {
                Node *new_node = (Node *) arena.Alloc(sizeof(Node));
                new_node->next = free_list_head;
                free_list_head = new_node;
            }
        }

        int Push(const uint64_t aligned_mem_len, const uint64_t pos) {
            Node *&list = lists[aligned_mem_len / MEMBLOCK_ALIGN_SIZE];

            Node *new_node = nullptr;
            if (nullptr != free_list_head) {
                new_node = free_list_head;
                free_list_head = free_list_head->next;
            } else {
                new_node = (Node *) arena.Alloc(sizeof(Node));
            }
            new_node->pos = pos;

            new_node->next = list;
            list = new_node;

            return 0;
        }

        bool Pop(ThreadLocalInfo *tlinfo, const uint64_t aligned_mem_len, uint64_t &pos,
                 const uint64_t probe_count = UINT64_MAX) {
            if (tlinfo->gc_append_len_sum <= 2147483648) {
                tlinfo->gc_append_len_sum += aligned_mem_len;
                return false;
            }

            for (uint64_t i = aligned_mem_len / MEMBLOCK_ALIGN_SIZE, j = 0;
                 i < MEMBLOCK_TYPE_COUNT && j < probe_count; ++i, ++j) {
                Node *&list = lists[i];
                if (nullptr != list) {
                    Node *pop_node = list;
                    list = list->next;
                    pos = pop_node->pos;
                    pop_node->next = free_list_head;
                    free_list_head = pop_node;
                    return true;
                }
            }
            return false;
        }

        void LogInfo() {
            int64_t free_cnt = 0;
            Node *iter = free_list_head;
            while (nullptr != iter) {
                free_cnt += 1;
                iter = iter->next;
            }

            int64_t using_cnt = 0;
            for (uint64_t i = 0; i < MEMBLOCK_TYPE_COUNT; ++i) {
                Node *iter = lists[i];
                while (nullptr != iter) {
                    using_cnt += 1;
                    iter = iter->next;
                }
            }
        }
    }
}