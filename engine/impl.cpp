#include "include/db.hpp"
#include "engine.h"

Status DB::CreateOrOpen(const std::string& name, DB** dbptr, FILE* log_file) {
    return KiD::CreateOrOpen(name, dbptr, log_file);
}

DB::~DB() {}