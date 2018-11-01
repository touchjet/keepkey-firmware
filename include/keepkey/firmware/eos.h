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

#define EOS_ASSET_STR_SIZE 32 /* FIXME: blind guess */
#define EOS_NAME_STR_SIZE  32 /* FIXME: blind guess */

typedef struct _EosAsset EosAsset;
typedef struct _EosSignedTx EosSignedTx;
typedef struct _EosActionCommon EosActionCommon;
typedef struct _EosActionTransfer EosActionTransfer;

void eos_formatAsset(const EosAsset *asset, char str[EOS_ASSET_STR_SIZE]);
void eos_formatName(uint64_t name, char str[EOS_NAME_STR_SIZE]);

void eos_signingInit(void);

bool eos_signingIsInited(void);

void eos_signingAbort(void);

bool eos_signingIsFinished(void);

/// \returns true iff successful.
bool eos_compileActionTransfer(const EosActionCommon *common,
                               const EosActionTransfer *transfer);

void eos_sign(EosSignedTx *sig);

#endif
