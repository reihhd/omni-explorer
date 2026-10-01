// json.h - tiny JSON parser (enough for offsets.json)
#pragma once
#include "common.h"

struct J {
    int t = 0;   // 0 null/bool, 1 string, 2 number, 3 object, 4 array
    std::string s;
    unsigned long long n = 0;
    std::map<std::string, J> o;
    std::vector<J> a;
    bool has(const std::string& k) const { return t == 3 && o.count(k) > 0; }
    const J& at(const std::string& k) const { return o.at(k); }
};

struct JP {
    const std::string& s;
    size_t i = 0;
    explicit JP(const std::string& x) : s(x) {}
    void ws() { while (i < s.size() && isspace((unsigned char)s[i])) i++; }
    bool str(std::string& out) {
        if (i >= s.size() || s[i] != '"') return false;
        i++;
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) {
                i++; char c = s[i];
                if (c == 'n') out += '\n';
                else if (c == 't') out += '\t';
                else if (c == 'r') out += '\r';
                else if (c == 'u') { out += '?'; i += 4; }
                else out += c;
                i++;
            } else out += s[i++];
        }
        if (i >= s.size()) return false;
        i++;
        return true;
    }
    bool parse(J& out) {
        ws(); if (i >= s.size()) return false;
        char c = s[i];
        if (c == '{') {
            i++; out.t = 3; ws();
            if (i < s.size() && s[i] == '}') { i++; return true; }
            while (true) {
                ws(); std::string k; if (!str(k)) return false;
                ws(); if (i >= s.size() || s[i] != ':') return false; i++;
                J v; if (!parse(v)) return false;
                out.o[k] = std::move(v);
                ws(); if (i >= s.size()) return false;
                if (s[i] == ',') { i++; continue; }
                if (s[i] == '}') { i++; return true; }
                return false;
            }
        }
        if (c == '[') {
            i++; out.t = 4; ws();
            if (i < s.size() && s[i] == ']') { i++; return true; }
            while (true) {
                J v; if (!parse(v)) return false;
                out.a.push_back(std::move(v));
                ws(); if (i >= s.size()) return false;
                if (s[i] == ',') { i++; continue; }
                if (s[i] == ']') { i++; return true; }
                return false;
            }
        }
        if (c == '"') { out.t = 1; return str(out.s); }
        size_t st = i;
        while (i < s.size() && !strchr(",}] \t\r\n", s[i])) i++;
        std::string tok = s.substr(st, i - st);
        if (tok == "null" || tok == "true" || tok == "false") { out.t = 0; return true; }
        out.t = 2;
        char* e = nullptr;
        double d = strtod(tok.c_str(), &e);
        if (e == tok.c_str()) return false;
        out.n = d < 0 ? 0 : (unsigned long long)d;
        return true;
    }
};

bool JNum(const J& v, uint64_t& out);
