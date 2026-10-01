#include "json.h"

bool JNum(const J& v, uint64_t& out) {
    if (v.t == 2) { out = v.n; return true; }
    if (v.t == 1) { char* e = nullptr; out = strtoull(v.s.c_str(), &e, 0); return e != v.s.c_str(); }
    return false;
}
