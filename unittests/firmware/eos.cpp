extern "C" {
#include "keepkey/firmware/eos.h"
}

#include "gtest/gtest.h"

#include <string>

TEST(EOS, FormatNameVec) {
    struct {
        uint64_t value;
        const char *name;
    } vec[] = {
        { 0x5530ea0000000000, "eosio" },
        { 0x0000000000ea3055, nullptr },
    };

    for (const auto &v : vec) {
        char str[EOS_NAME_STR_SIZE];
        char *res = eos_formatName(v.value, str);
        ASSERT_EQ(res, str);
        if (v.name)
            ASSERT_EQ(res, std::string(v.name));
    }
}
