/*
 * This file is part of the KeepKey project.
 *
 * Copyright (C) 2018 KeepKey
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "keepkey/firmware/eos.h"

#include "trezor/crypto/memzero.h"

static bool inited = false;

void eos_formatAsset(const EosAsset *asset, char str[EOS_ASSET_STR_SIZE]) {
    memzero(str, EOS_ASSET_STR_SIZE);
}

void eos_formatName(uint64_t name, char str[EOS_NAME_STR_SIZE]) {
    memzero(str, EOS_NAME_STR_SIZE);
}

void eos_signingInit(void) {
    inited = true;
}

bool eos_signingIsInited(void) {
    return inited;
}

void eos_signingAbort(void) {
    inited = false;
}

bool eos_compileActionTransfer(const EosActionCommon *common,
                               const EosActionTransfer *transfer) {
    (void)common;
    (void)transfer;

    return true;
}
