#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include "StringHelpers.h"

// Lowercase (ASCII)
static std::string to_lower_ascii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Trim both ends
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// Collapse multiple spaces to one
static std::string collapse_spaces(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    bool in_space = false;
    for (unsigned char ch : s) {
        bool is_space = std::isspace(ch);
        if (is_space) {
            if (!in_space) {
                out.push_back(' ');
                in_space = true;
            }
        } else {
            out.push_back(ch);
            in_space = false;
        }
    }
    return out;
}

// Remove punctuation but keep letters, digits and spaces
static std::string remove_punct_keep_spaces(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (std::isalnum(c) || std::isspace(c)) out.push_back(c);
    }
    return out;
}

// Full normalize pipeline (ASCII-safe)
static std::string normalize_ascii_name(std::string s) {
    s = to_lower_ascii(std::move(s));
    s = remove_punct_keep_spaces(s);
    s = trim(s);
    s = collapse_spaces(s);
    return s;
}

// Reuse normalize_ascii_name(...) from earlier
static std::vector<std::string> split_ws(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> toks;
    std::string t;
    while (iss >> t) toks.push_back(t);
    return toks;
}

// Tokenize + optional filtering (drop short tokens and titles)
static std::vector<std::string> tokenize_and_filter(const std::string& normalized, size_t min_token_len = 3) {
    auto toks = split_ws(normalized);
    std::vector<std::string> out;
    out.reserve(toks.size());
    for (auto& tok : toks) {
        if (tok.size() < min_token_len) continue;
        out.push_back(tok);
    }
    return out;
}

// Join tokens with a single space
static std::string join_with_space(const std::vector<std::string>& toks) {
    std::string out;
    bool first = true;
    for (auto& t : toks) {
        if (!first) out.push_back(' ');
        out += t;
        first = false;
    }
    return out;
}

// DETOKENIZE: join tokens without spaces
static std::string detokenize_no_space(const std::vector<std::string>& toks) {
    std::string out;
    out.reserve(64);
    for (auto& t : toks) out += t;
    return out;
}

// Example wrapper: produce normalized spaced and detokenized forms
void stringhelpers::build_norm_and_detok(const std::string& raw, std::string& out_norm_raw,
                                         std::string& out_norm_spaced,
                          std::string& out_detok) {
    auto norm = normalize_ascii_name(raw);    
    auto toks = tokenize_and_filter(norm);
    out_norm_raw = raw;                      
    out_norm_spaced = join_with_space(toks);  
    out_detok = detokenize_no_space(toks);    
}

// small helper to split on whitespace
void stringhelpers::split_ws(const std::string& s, std::vector<std::string>& out) {
    std::istringstream iss(s);
    std::string t;
    while (iss >> t) out.push_back(t);

}

