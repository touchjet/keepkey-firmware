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

#include "keepkey/firmware/eos-contracts/generic.h"

#include "eos.h"

#include "keepkey/board/confirm_sm.h"
#include "keepkey/board/keepkey_board.h"
#include "keepkey/firmware/app_confirm.h"
#include "keepkey/firmware/eos.h"
#include "keepkey/firmware/fsm.h"
#include "keepkey/firmware/home_sm.h"
#include "keepkey/firmware/storage.h"
#include "keepkey/firmware/util.h"

#include "messages-eos.pb.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

bool eos_compileActionUnknown(const EosActionCommon *common,
                              const EosActionUnknown *action) {
    switch (common->account) {
    case EOS_eosio_system:
    case EOS_eosio_token: {
        fsm_sendFailure(FailureType_Failure_SyntaxError,
                        "EosActionUnknown cannot be used with supported contracts");
        eos_signingAbort();
        return false;
    }
    }

    if (!storage_isPolicyEnabled("AdvancedMode")) {
        (void)review(ButtonRequestType_ButtonRequest_Other, "Warning",
                     "Signing of arbitrary EOS actions is recommended only for "
                     "experienced users. Enable 'AdvancedMode' policy to dismiss.");
        fsm_sendFailure(FailureType_Failure_ActionCancelled, "Signing cancelled by user");
        eos_signingAbort();
        layoutHome();
        return false;
    }

    char name[EOS_NAME_STR_SIZE];
    CHECK_PARAM_RET(eos_formatName(common->name, name),
                    "Invalid name", false);

    char account[EOS_NAME_STR_SIZE];
    CHECK_PARAM_RET(eos_formatName(common->account, account),
                    "Invalid name", false);

    char title[MEDIUM_STR_BUF];
    snprintf(title, sizeof(title), "%s:%s", account, name);

    char body[54 * 2 + 1];
    memset(body, 0, sizeof(body));
    data2hex(action->data.bytes, MIN((sizeof(body) - 1)/2, action->data.size), body);
    if ((sizeof(body) - 1)/2 < action->data.size) {
        body[sizeof(body) - 1] = 0;
        body[sizeof(body) - 2] = '.';
        body[sizeof(body) - 3] = '.';
    }

    if (!confirm(ButtonRequestType_ButtonRequest_ConfirmEosAction,
                 title, "%d:%s", action->data.size, body)) {
        fsm_sendFailure(FailureType_Failure_ActionCancelled,
                        "Action Cancelled");
        eos_signingAbort();
        layoutHome();
        return false;
    }

    if (!eos_compileActionCommon(common))
        return false;

    eos_hashUInt(&hasher_preimage, action->data.size);
    hasher_Update(&hasher_preimage, (const uint8_t*)action->data.bytes,
                  action->data.size);

    return true;
}

