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

#include "keepkey/firmware/eos-contracts/eosio.system.h"

#include "eos.h"

#include "keepkey/board/confirm_sm.h"
#include "keepkey/firmware/app_confirm.h"
#include "keepkey/firmware/eos.h"
#include "keepkey/firmware/fsm.h"
#include "keepkey/firmware/home_sm.h"

#include "messages-eos.pb.h"

#include <inttypes.h>
#include <stdbool.h>

#define CHECK_COMMON(ACTION) \
    do { \
        CHECK_PARAM_RET(common->account == EOS_eosio_system, \
                        "Incorrect account name", false); \
        CHECK_PARAM_RET(common->name == (ACTION), \
                        "Incorrect action name", false); \
    } while(0)

bool eos_compileActionDelegate(const EosActionCommon *common,
                               const EosActionDelegate *action) {
    CHECK_COMMON(EOS_Delegate);

    char sender[EOS_NAME_STR_SIZE];
    CHECK_PARAM_RET(eos_formatName(action->sender, sender),
                    "Invalid name", false);

    char receiver[EOS_NAME_STR_SIZE];
    CHECK_PARAM_RET(eos_formatName(action->receiver, receiver),
                    "Invalid name", false);

    char cpu[EOS_ASSET_STR_SIZE];
    CHECK_PARAM_RET(eos_formatAsset(&action->cpu_quantity, cpu),
                    "Invalid asset format", false);

    char net[EOS_ASSET_STR_SIZE];
    CHECK_PARAM_RET(eos_formatAsset(&action->net_quantity, net),
                    "Invalid asset format", false);

    if (!confirm(ButtonRequestType_ButtonRequest_ConfirmEosAction,
                 "Delegate", "Do you want to %s resources from %s to %s?\n"
                 "CPU: %s, NET: %s",
                 (action->has_transfer && action->transfer) ? "transfer" : "delegate",
                 sender, receiver, cpu, net)) {
        fsm_sendFailure(FailureType_Failure_ActionCancelled, "Action Cancelled");
        eos_signingAbort();
        return false;
    }

    if (!eos_compileActionCommon(common))
        return false;

    uint32_t size = 8 + 8 + 16 + 16 + 1;
    eos_hashUInt(&hasher_preimage, size);

    hasher_Update(&hasher_preimage, (const uint8_t*)&action->sender, 8);
    hasher_Update(&hasher_preimage, (const uint8_t*)&action->receiver, 8);

    if (!eos_compileAsset(&action->net_quantity))
        return false;

    if (!eos_compileAsset(&action->cpu_quantity))
        return false;

    uint8_t is_transfer = action->has_transfer ? 1 : 0;
    hasher_Update(&hasher_preimage, &is_transfer, 1);

    return true;
}

bool eos_compileActionUndelegate(const EosActionCommon *common,
                                 const EosActionUndelegate *action) {
    CHECK_COMMON(EOS_Undelegate);

    char sender[EOS_NAME_STR_SIZE];
    CHECK_PARAM_RET(eos_formatName(action->sender, sender),
                    "Invalid name", false);

    char receiver[EOS_NAME_STR_SIZE];
    CHECK_PARAM_RET(eos_formatName(action->receiver, receiver),
                    "Invalid name", false);

    char cpu[EOS_ASSET_STR_SIZE];
    CHECK_PARAM_RET(eos_formatAsset(&action->cpu_quantity, cpu),
                    "Invalid asset format", false);

    char net[EOS_ASSET_STR_SIZE];
    CHECK_PARAM_RET(eos_formatAsset(&action->net_quantity, net),
                    "Invalid asset format", false);

    if (!confirm(ButtonRequestType_ButtonRequest_ConfirmEosAction,
                 "Undelegate", "Do you want to remove delegation of resources from %s to %s?\n"
                 "CPU: %s, NET: %s",
                 sender, receiver, cpu, net)) {
        fsm_sendFailure(FailureType_Failure_ActionCancelled, "Action Cancelled");
        eos_signingAbort();
        return false;
    }

    if (!eos_compileActionCommon(common))
        return false;

    uint32_t size = 8 + 8 + 16 + 16;
    eos_hashUInt(&hasher_preimage, size);

    hasher_Update(&hasher_preimage, (const uint8_t*)&action->sender, 8);
    hasher_Update(&hasher_preimage, (const uint8_t*)&action->receiver, 8);

    if (!eos_compileAsset(&action->net_quantity))
        return false;

    if (!eos_compileAsset(&action->cpu_quantity))
        return false;

    return true;
}

bool eos_compileActionRefund(const EosActionCommon *common,
                             const EosActionRefund *action) {
    CHECK_COMMON(EOS_Refund);

    char owner[EOS_NAME_STR_SIZE];
    CHECK_PARAM_RET(eos_formatName(action->owner, owner),
                    "Invalid name", false);

    if (!confirm(ButtonRequestType_ButtonRequest_ConfirmEosAction,
                 "Refund", "Do you want reclaim all pending unstaked tokens belonging to %s?\n",
                 owner)) {
        fsm_sendFailure(FailureType_Failure_ActionCancelled, "Action Cancelled");
        eos_signingAbort();
        return false;
    }

    if (!eos_compileActionCommon(common))
        return false;

    uint32_t size = 8;
    eos_hashUInt(&hasher_preimage, size);

    hasher_Update(&hasher_preimage, (const uint8_t*)&action->owner, 8);

    return true;
}
