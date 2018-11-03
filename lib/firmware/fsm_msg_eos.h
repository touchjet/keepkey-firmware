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

void fsm_msgEosGetPublicKey(const EosGetPublicKey *msg) {
    CHECK_INITIALIZED

    CHECK_PIN

    const CoinType *coin = fsm_getCoin(true, "EOS");
    if (!coin) return;

    const curve_info *curve = get_curve_by_name(coin->curve_name);
    if (!curve) return;

    uint32_t fingerprint;
    HDNode *node = fsm_getDerivedNode(coin->curve_name, msg->address_n, msg->address_n_count, &fingerprint);
    if (!node) return;
    hdnode_fill_public_key(node);

    RESP_INIT(EosPublicKey);

    if (!eos_getPublicKey(node, curve, resp->public_key, sizeof(resp->public_key))) {
        fsm_sendFailure(FailureType_Failure_Other, "Could not derive EOS pubkey");
        return;
    }

    if (msg->has_show_display && msg->show_display) {
        char node_str[NODE_STRING_LENGTH];
        if (!bip32_node_to_string(node_str, sizeof(node_str), coin,
                                  msg->address_n,
                                  msg->address_n_count,
                                  /*whole_account=*/true) &&
            !bip32_path_to_string(node_str, sizeof(node_str),
                                  msg->address_n, msg->address_n_count)) {
            memset(node_str, 0, sizeof(node_str));
        }

        // Not really an xpub, but it'll do.
        if (!confirm_xpub(node_str, resp->public_key)) {
            fsm_sendFailure(FailureType_Failure_ActionCancelled,
                            "Show EOS public key cancelled.");
            layoutHome();
            return;
        }
    }

    msg_write(MessageType_MessageType_EosPublicKey, resp);
}

void fsm_msgEosSignTx(const EosSignTx *msg) {
    CHECK_PARAM(msg->chain_id.size == 32, "Wrong chain_id size");
    CHECK_PARAM(msg->has_header, "Must have transaction header");
    CHECK_PARAM(msg->has_num_actions && 0 < msg->num_actions,
                "Eos transaction must have actions");

    CHECK_INITIALIZED

    CHECK_PIN_TXSIGN

    uint32_t fingerprint;
    HDNode *node = fsm_getDerivedNode(SECP256K1_NAME, msg->address_n, msg->address_n_count, &fingerprint);
    if (!node) return;
    hdnode_fill_public_key(node);

    eos_signingInit(msg->num_actions, &msg->header, node);

    RESP_INIT(EosTxActionRequest);
    msg_write(MessageType_MessageType_EosTxActionRequest, resp);
}

static bool eos_transfer(const EosActionCommon *common,
                         const EosActionTransfer *transfer) {
    CHECK_PARAM_RET(common->name == EOS_Transfer, "Incorrect action name", false);

    char asset[EOS_ASSET_STR_SIZE];
    CHECK_PARAM_RET(eos_formatAsset(&transfer->quantity, asset),
                    "Invalid asset format", false);

    char from[EOS_NAME_STR_SIZE];
    CHECK_PARAM_RET(eos_formatName(transfer->from, from),
                    "Invalid name", false);

    char to[EOS_NAME_STR_SIZE];
    CHECK_PARAM_RET(eos_formatName(transfer->to, to),
                    "Invalid name", false);

    if (!confirm(ButtonRequestType_ButtonRequest_ConfirmOutput,
                 "Send", "Do you want to send %s from %s to %s?",
                 asset, from, to)) {
        fsm_sendFailure(FailureType_Failure_ActionCancelled, "Action Cancelled");
        eos_signingAbort();
        return false;
    }

    // TODO: confirm memo

    return eos_compileActionTransfer(common, transfer);
}

void fsm_msgEosTxActionAck(const EosTxActionAck *msg) {
    CHECK_PARAM(eos_signingIsInited(), "Must call EosSignTx to initiate signing");
    CHECK_PARAM(msg->has_common, "Must have common");

    int action_count =
        (int)msg->has_transfer +
        (int)msg->has_delegate +
        (int)msg->has_undelegate +
        (int)msg->has_buy_ram +
        (int)msg->has_buy_ram_bytes +
        (int)msg->has_sell_ram +
        (int)msg->has_vote_producer;
    CHECK_PARAM(action_count == 1, "Eos signing can only handle one action at a time");

    if (msg->has_transfer && !eos_transfer(&msg->common, &msg->transfer))
        return;
    else {
        fsm_sendFailure(FailureType_Failure_Other, "Unknown action");
        eos_signingAbort();
        return;
    }

    if (!eos_signingIsFinished()) {
        RESP_INIT(EosTxActionRequest);
        msg_write(MessageType_MessageType_EosTxActionRequest, resp);
        return;
    }

    RESP_INIT(EosSignedTx);

    if (!eos_signTx(resp))
        return;

    msg_write(MessageType_MessageType_EosSignedTx, resp);
}
