extern "C" {
#include "keepkey/firmware/eos.h"
#include "messages-eos.pb.h"
}

#include "gtest/gtest.h"

#include <string>

TEST(EOS, FormatNameVec) {
    struct {
        uint64_t value;
        const char *name;
        bool ret;
    } vec[] = {
        { 0x5530ea0000000000, "eosio", true },
        { 0x0000000000ea3055, nullptr, true },
    };

    for (const auto &v : vec) {
        char str[EOS_NAME_STR_SIZE];
        ASSERT_EQ(v.ret, eos_formatName(v.value, str));
        if (v.name)
            ASSERT_EQ(v.name, std::string(str));
    }
}

TEST(EOS, FormatAssetVec) {
    struct {
        int64_t amount;
        uint64_t symbol;
        std::string expected;
        bool ret;
    } vec[] = {
        { 7654321L, 0x000000534f4504L, "765.4321 EOS",      true },
        {      42L, 0x004e45584f4600L, "42 FOXEN",          true },
        {      42L, 0x004e45584f4601L, "4.2 FOXEN",         true },
        {      42L, 0x004e45584f4602L, "0.42 FOXEN",        true },
        {      42L, 0x004e45584f4603L, "0.042 FOXEN",       true },
        {      42L, 0x004e45584f4604L, "0.0042 FOXEN",      true },
        {      42L, 0x004e45584f4605L, "0.00042 FOXEN",     true },
        {      42L, 0x004e45584f4606L, "0.000042 FOXEN",    true },
        {      42L, 0x004e45584f4607L, "0.0000042 FOXEN",   true },
        {      42L, 0x004e45584f4608L, "0.00000042 FOXEN",  true },
        {      42L, 0x004e45584f4609L, "0.000000042 FOXEN", true },
    };

    for (const auto &v : vec) {
        char str[EOS_ASSET_STR_SIZE];
        EosAsset asset;
        asset.has_amount = true;
        asset.amount = v.amount;
        asset.has_symbol = true;
        asset.symbol = v.symbol;
        EXPECT_EQ(v.ret, eos_formatAsset(&asset, str));
        EXPECT_EQ(v.expected, str);
    }
}
