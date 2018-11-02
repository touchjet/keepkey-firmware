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
    (void)msg;

    // TODO: check initialized

    // TODO: check PIN

    if (msg->address_n_count != 3 ||
        msg->address_n[0] != (0x80000000 |  44) ||
        msg->address_n[1] != (0x80000000 | 194) ||
        (msg->address_n[2] & 0x80000000) == 0x80000000) {
        // TODO: warn unusual BIP32 path
        // TODO TODO: make this common betwee BTC clones / ETH / this
    }

    RESP_INIT(EosPublicKey);

    msg_write(MessageType_MessageType_EosPublicKey, resp);
}

void fsm_msgEosSignTx(const EosSignTx *msg) {
    CHECK_PARAM(msg->chain_id.size == 32, "Wrong chain_id size");
    CHECK_PARAM(msg->has_header, "Must have transaction header");
    CHECK_PARAM(msg->has_num_actions && 0 < msg->num_actions,
                "Eos transaction must have actions");

    // TODO: check initialized

    // TODO: check pin

    eos_signingInit(msg->num_actions, &msg->header);

    RESP_INIT(EosTxActionRequest);
    msg_write(MessageType_MessageType_EosTxActionRequest, resp);
}

static bool eos_transfer(const EosActionCommon *common,
                         const EosActionTransfer *transfer) {
    (void)common;
    (void)transfer;

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
