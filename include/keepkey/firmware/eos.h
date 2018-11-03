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

#ifndef KEEPKEY_FIRMWARE_EOS_H
#define KEEPKEY_FIRMWARE_EOS_H

#include "trezor/crypto/bip32.h"

#include <inttypes.h>
#include <stdbool.h>

#define EOS_NAME_STR_SIZE  (12 + 1 + 1)
#define EOS_ASSET_STR_SIZE (1 + 21 + 1 + 12 + 1)

typedef enum _EosActionName {
    EOS_Transfer = 0xcdcd3c2d57000000L,
    EOS_Owner    = 0xa726ab8000000000L,
    EOS_Active   = 0x3232eda800000000L,
} EosActionName;

typedef struct _EosActionCommon EosActionCommon;
typedef struct _EosActionTransfer EosActionTransfer;
typedef struct _EosAsset EosAsset;
typedef struct _EosPermissionLevel EosPermissionLevel;
typedef struct _EosSignedTx EosSignedTx;
typedef struct _EosTxHeader EosTxHeader;

/// \returns true iff the asset can be correctly decoded.
bool eos_formatAsset(const EosAsset *asset, char str[EOS_ASSET_STR_SIZE]);

/// \returns true iff the name can be correctly decoded.
bool eos_formatName(uint64_t name, char str[EOS_NAME_STR_SIZE]);

/// \returns true iff successful.
bool eos_getPublicKey(const HDNode *node, const curve_info *curve,
                      char *str, size_t len);

void eos_signingInit(const uint8_t *chain_id, uint32_t num_actions,
                     const EosTxHeader *_header, const HDNode *node);

bool eos_signingIsInited(void);

void eos_signingAbort(void);

bool eos_signingIsFinished(void);

void eos_hashUInt(Hasher *hasher, uint64_t val);

/// \returns true iff successful.
bool eos_compileActionCommon(const EosActionCommon *common);

/// \returns true iff successful.
bool eos_compilePermissionLevel(const EosPermissionLevel *auth);

/// \returns true iff successful.
bool eos_compileActionTransfer(const EosActionCommon *common,
                               const EosActionTransfer *transfer);

bool eos_signTx(EosSignedTx *sig);

#endif
