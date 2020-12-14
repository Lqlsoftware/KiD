#pragma once

#include <stdint.h>
#include <string.h>

#include "common.h"
#include "util.h"

namespace tavern {

    class PageArena {
        struct Page {
            Page *next;
            int32_t pos;
            char buf[0];
            void *Alloc(const int32_t page_size, const int32_t size) {
                char *ret = 0;
                if ((pos + size) <= page_size) {
                    ret = &buf[pos];
                    pos += size;
                }
                return ret;
            }
        };
        static const int32_t DEFAULT_PAGE_SIZE = 2*1024*1024 - sizeof(Page);
    public:
        PageArena(const int32_t page_size = DEFAULT_PAGE_SIZE)
                : page_size_(page_size),
                  curr_page_(nullptr) {
        }
        ~PageArena() {}
    public:
        void *Alloc(const int32_t size) {
            void *ret = nullptr;
            while (true) {
                if (nullptr == curr_page_
                    || nullptr == (ret = curr_page_->Alloc(page_size_, size))) {
                    Page *page = (Page*)(aligned_alloc(16, page_size_ + sizeof(Page)));
                    memset(page, 0, page_size_ + sizeof(Page));
                    page->next = curr_page_;
                    page->pos = 0;
                    curr_page_ = page;
                    continue;
                }
                break;
            }
            return ret;
        }
        void Reset() {
            while (nullptr != curr_page_) {
                void *page = curr_page_;
                curr_page_ = curr_page_->next;
                free(page);
            }
        }
        int64_t Size() {
            int64_t ret = 0;
            Page *iter = curr_page_;
            while (nullptr != iter) {
                ret += (page_size_ + sizeof(Page));
                iter = iter->next;
            }
            return ret;
        }
    private:
        const int32_t page_size_;
        Page *curr_page_;
    };

}