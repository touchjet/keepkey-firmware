/*
 * This file is part of the TREZOR project.
 *
 * Copyright (C) 2014 Pavol Rusnak <stick@satoshilabs.com>
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

/* === Includes ============================================================ */

#include "scm_revision.h"
#include "variant.h"
#include "u2f_knownapps.h"

#include "keepkey/board/check_bootloader.h"
#include "keepkey/board/confirm_sm.h"
#include "keepkey/board/keepkey_board.h"
#include "keepkey/board/keepkey_flash.h"
#include "keepkey/board/layout.h"
#include "keepkey/board/memory.h"
#include "keepkey/board/msg_dispatch.h"
#include "keepkey/board/resources.h"
#include "keepkey/board/timer.h"
#include "keepkey/board/u2f.h"
#include "keepkey/board/variant.h"
#include "keepkey/firmware/app_confirm.h"
#include "keepkey/firmware/app_layout.h"
#include "keepkey/firmware/coins.h"
#include "keepkey/firmware/crypto.h"
#include "keepkey/firmware/eos.h"
#include "keepkey/firmware/eos-contracts.h"
#include "keepkey/firmware/ethereum.h"
#include "keepkey/firmware/ethereum_tokens.h"
#include "keepkey/firmware/exchange.h"
#include "keepkey/firmware/fsm.h"
#include "keepkey/firmware/home_sm.h"
#include "keepkey/firmware/passphrase_sm.h"
#include "keepkey/firmware/pin_sm.h"
#include "keepkey/firmware/policy.h"
#include "keepkey/firmware/recovery.h"
#include "keepkey/firmware/recovery_cipher.h"
#include "keepkey/firmware/reset.h"
#include "keepkey/firmware/signing.h"
#include "keepkey/firmware/storage.h"
#include "keepkey/firmware/transaction.h"
#include "keepkey/firmware/util.h"
#include "keepkey/rand/rng.h"
#include "trezor/crypto/address.h"
#include "trezor/crypto/aes/aes.h"
#include "trezor/crypto/base58.h"
#include "trezor/crypto/bip39.h"
#include "trezor/crypto/curves.h"
#include "trezor/crypto/ecdsa.h"
#include "trezor/crypto/hmac.h"
#include "trezor/crypto/memzero.h"
#include "trezor/crypto/rand.h"
#include "trezor/crypto/ripemd160.h"
#include "trezor/crypto/secp256k1.h"

#include "messages.pb.h"
#include "messages-eos.pb.h"

#include <stdio.h>

#define _(X) (X)

static uint8_t msg_resp[MAX_FRAME_SIZE] __attribute__((aligned(4)));

#define CHECK_INITIALIZED \
    if (!storage_isInitialized()) { \
        fsm_sendFailure(FailureType_Failure_NotInitialized, "Device not initialized"); \
        return; \
    }

#define CHECK_NOT_INITIALIZED \
    if (storage_isInitialized()) { \
        fsm_sendFailure(FailureType_Failure_UnexpectedMessage, "Device is already initialized. Use Wipe first."); \
        return; \
    }

#define CHECK_PIN \
    if (!pin_protect_cached()) { \
        layoutHome(); \
        return; \
    }

#define CHECK_PIN_TXSIGN \
    if (!pin_protect_txsign()) { \
        layoutHome(); \
        return; \
    }

#define CHECK_PARAM_RET(cond, errormsg, retval) \
    if (!(cond)) { \
        fsm_sendFailure(FailureType_Failure_Other, (errormsg)); \
        layoutHome(); \
        return retval; \
    }

#define CHECK_PARAM(cond, errormsg) \
    CHECK_PARAM_RET(cond, errormsg, )

static const MessagesMap_t MessagesMap[] = {
#include "messagemap.def"
};

#undef MSG_IN
#define MSG_IN(ID, STRUCT_NAME, PROCESS_FUNC, MSG_PERMS) \
  _Static_assert(sizeof(STRUCT_NAME) <= MAX_DECODE_SIZE, "Message too big");

#undef MSG_OUT
#define MSG_OUT(ID, STRUCT_NAME, PROCESS_FUNC, MSG_PERMS)

#undef RAW_IN
#define RAW_IN(ID, STRUCT_NAME, PROCESS_FUNC, MSG_PERMS) \
  _Static_assert(sizeof(STRUCT_NAME) <= MAX_DECODE_SIZE, "Message too big");

#undef DEBUG_IN
#define DEBUG_IN(ID, STRUCT_NAME, PROCESS_FUNC, MSG_PERMS) \
  _Static_assert(sizeof(STRUCT_NAME) <= MAX_DECODE_SIZE, "Message too big");

#undef DEBUG_OUT
#define DEBUG_OUT(ID, STRUCT_NAME, PROCESS_FUNC, MSG_PERMS)

#include "messagemap.def"

extern bool reset_msg_stack;

static const CoinType *fsm_getCoin(bool has_name, const char *name)
{
    const CoinType *coin;
    if (has_name) {
        coin = coinByName(name);
    } else {
        coin = coinByName("Bitcoin");
    }
    if(!coin)
    {
        fsm_sendFailure(FailureType_Failure_Other, "Invalid coin name");
        layoutHome();
        return 0;
    }

    return coin;
}

static HDNode *fsm_getDerivedNode(const char *curve, const uint32_t *address_n, size_t address_n_count, uint32_t *fingerprint)
{
    static HDNode CONFIDENTIAL node;
    if (fingerprint) {
        *fingerprint = 0;
    }

    if(!storage_getRootNode(curve, true, &node))
    {
        fsm_sendFailure(FailureType_Failure_NotInitialized,
                        "Device not initialized or passphrase request cancelled");
        layoutHome();
        return 0;
    }

    if(!address_n || address_n_count == 0)
    {
        return &node;
    }

    if(hdnode_private_ckd_cached(&node, address_n, address_n_count, fingerprint) == 0)
    {
        fsm_sendFailure(FailureType_Failure_Other, "Failed to derive private key");
        layoutHome();
        return 0;
    }

    return &node;
}

#if DEBUG_LINK
static void sendFailureWrapper(FailureType code, const char *text) {
    fsm_sendFailure(code, text);
}
#endif

static void u2f_filtered_usb_rx(UsbMessage *msg,
                                const U2F_AUTHENTICATE_REQ *req) {
    if (!storage_isPolicyEnabled("Experimental") &&
#if DEBUG_LINK
        memcmp(req->appId, u2f_localhost.appid,           sizeof(u2f_localhost.appid))           != 0 &&
#endif
        memcmp(req->appId, U2F_SHAPESHIFT_COM->appid,     sizeof(U2F_SHAPESHIFT_COM->appid))     != 0 &&
        memcmp(req->appId, U2F_SHAPESHIFT_IO->appid,      sizeof(U2F_SHAPESHIFT_IO->appid))      != 0 &&
        memcmp(req->appId, U2F_SHAPESHIFT_COM_STG->appid, sizeof(U2F_SHAPESHIFT_COM_STG->appid)) != 0 &&
        memcmp(req->appId, U2F_SHAPESHIFT_IO_STG->appid,  sizeof(U2F_SHAPESHIFT_IO_STG->appid))  != 0 &&
        memcmp(req->appId, U2F_SHAPESHIFT_COM_DEV->appid, sizeof(U2F_SHAPESHIFT_COM_DEV->appid)) != 0 &&
        memcmp(req->appId, U2F_SHAPESHIFT_IO_DEV->appid,  sizeof(U2F_SHAPESHIFT_IO_DEV->appid))  != 0) {
        // Ignore the request
        return;
    }

    handle_usb_rx(msg);
}

#if DEBUG_LINK
static void u2f_filtered_debug_usb_rx(UsbMessage *msg,
                                      const U2F_AUTHENTICATE_REQ *req) {
    (void)req; // DEBUG_LINK doesn't care who talks to it.
    handle_debug_usb_rx(msg);
}
#endif

void fsm_init(void)
{
    msg_map_init(MessagesMap, sizeof(MessagesMap) / sizeof(MessagesMap_t));
#if DEBUG_LINK
    set_msg_failure_handler(&sendFailureWrapper);
#else
    set_msg_failure_handler(&fsm_sendFailure);
#endif

    /* set leaving handler for layout to help with determine home state */
    set_leaving_handler(&leave_home);

#if DEBUG_LINK
    set_msg_debug_link_get_state_handler(&fsm_msgDebugLinkGetState);
#endif

    msg_init();

    u2f_set_rx_callback(u2f_filtered_usb_rx);
#if DEBUG_LINK
    u2f_set_debug_rx_callback(u2f_filtered_debug_usb_rx);
#endif
}

void fsm_sendSuccess(const char *text)
{
    if(reset_msg_stack)
    {
        fsm_msgInitialize((Initialize *)0);
        reset_msg_stack = false;
        return;
    }

    RESP_INIT(Success);

    if(text)
    {
        resp->has_message = true;
        strlcpy(resp->message, text, sizeof(resp->message));
    }

    msg_write(MessageType_MessageType_Success, resp);
}

#if DEBUG_LINK
void fsm_sendFailureDebug(FailureType code, const char *text, const char *source)
#else
void fsm_sendFailure(FailureType code, const char *text)
#endif
{
    if(reset_msg_stack)
    {
        fsm_msgInitialize((Initialize *)0);
        reset_msg_stack = false;
        return;
    }

    RESP_INIT(Failure);
    resp->has_code = true;
    resp->code = code;

#if DEBUG_LINK
    resp->has_message = true;
    strlcpy(resp->message, source, sizeof(resp->message));
    if (text) {
        strlcat(resp->message, text, sizeof(resp->message));
    }
#else
    if (text)
    {
        resp->has_message = true;
        strlcpy(resp->message, text, sizeof(resp->message));
    }
#endif
    msg_write(MessageType_MessageType_Failure, resp);
}


void fsm_msgClearSession(ClearSession *msg)
{
    (void)msg;
    session_clear(/*clear_pin=*/true);
    fsm_sendSuccess("Session cleared");
}

#include "fsm_msg_common.h"
#include "fsm_msg_coin.h"
#include "fsm_msg_ethereum.h"
#include "fsm_msg_crypto.h"
#include "fsm_msg_debug.h"
#include "fsm_msg_eos.h"
