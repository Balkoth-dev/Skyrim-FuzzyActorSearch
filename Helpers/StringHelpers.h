#pragma once

#include <string>

namespace stringhelpers {

    // Build normalized spaced and detokenized forms from raw input.
    // - out_norm_spaced: tokens joined with single spaces after normalization/filtering
    // - out_detok: tokens joined without spaces
    void build_norm_and_detok(const std::string& raw, std::string& out_norm_raw, std::string& out_norm_spaced,
                              std::string& out_detok);

    void split_ws(const std::string& s, std::vector<std::string>& out);

}  // namespace stringhelpers