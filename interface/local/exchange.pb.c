/* Automatically generated nanopb constant definitions */
/* Generated by nanopb-0.2.9.2 at Thu Jun 16 23:39:53 2016. */

#include "exchange.pb.h"



const pb_field_t ExchangeAddress_fields[4] = {
    PB_FIELD2(  1, STRING  , OPTIONAL, STATIC  , FIRST, ExchangeAddress, address, address, 0),
    PB_FIELD2(  2, STRING  , OPTIONAL, STATIC  , OTHER, ExchangeAddress, dest_tag, address, 0),
    PB_FIELD2(  3, STRING  , OPTIONAL, STATIC  , OTHER, ExchangeAddress, rs_address, dest_tag, 0),
    PB_LAST_FIELD
};

const pb_field_t SendAmountRequest_fields[7] = {
    PB_FIELD2(  1, UINT64  , OPTIONAL, STATIC  , FIRST, SendAmountRequest, withdrawal_amount, withdrawal_amount, 0),
    PB_FIELD2(  2, MESSAGE , OPTIONAL, STATIC  , OTHER, SendAmountRequest, withdrawal_address, withdrawal_amount, &ExchangeAddress_fields),
    PB_FIELD2(  3, STRING  , OPTIONAL, STATIC  , OTHER, SendAmountRequest, withdrawal_coin_type, withdrawal_address, 0),
    PB_FIELD2(  4, STRING  , OPTIONAL, STATIC  , OTHER, SendAmountRequest, deposit_coin_type, withdrawal_coin_type, 0),
    PB_FIELD2(  5, MESSAGE , OPTIONAL, STATIC  , OTHER, SendAmountRequest, return_address, deposit_coin_type, &ExchangeAddress_fields),
    PB_FIELD2(  6, BYTES   , OPTIONAL, STATIC  , OTHER, SendAmountRequest, api_key, return_address, 0),
    PB_LAST_FIELD
};

const pb_field_t SendAmountResponse_fields[8] = {
    PB_FIELD2(  1, MESSAGE , OPTIONAL, STATIC  , FIRST, SendAmountResponse, request, request, &SendAmountRequest_fields),
    PB_FIELD2(  2, MESSAGE , OPTIONAL, STATIC  , OTHER, SendAmountResponse, deposit_address, request, &ExchangeAddress_fields),
    PB_FIELD2(  3, UINT64  , OPTIONAL, STATIC  , OTHER, SendAmountResponse, deposit_amount, deposit_address, 0),
    PB_FIELD2(  4, UINT32  , OPTIONAL, STATIC  , OTHER, SendAmountResponse, expiration, deposit_amount, 0),
    PB_FIELD2(  5, UINT32  , OPTIONAL, STATIC  , OTHER, SendAmountResponse, quoted_rate, expiration, 0),
    PB_FIELD2(  6, BYTES   , OPTIONAL, STATIC  , OTHER, SendAmountResponse, signature, quoted_rate, 0),
    PB_FIELD2(  7, BYTES   , OPTIONAL, STATIC  , OTHER, SendAmountResponse, approval, signature, 0),
    PB_LAST_FIELD
};


/* Check that field information fits in pb_field_t */
#if !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_32BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in 8 or 16 bit
 * field descriptors.
 */
STATIC_ASSERT((pb_membersize(SendAmountRequest, withdrawal_address) < 65536 && pb_membersize(SendAmountRequest, return_address) < 65536 && pb_membersize(SendAmountResponse, request) < 65536 && pb_membersize(SendAmountResponse, deposit_address) < 65536), YOU_MUST_DEFINE_PB_FIELD_32BIT_FOR_MESSAGES_ExchangeAddress_SendAmountRequest_SendAmountResponse)
#endif

#if !defined(PB_FIELD_16BIT) && !defined(PB_FIELD_32BIT)
/* If you get an error here, it means that you need to define PB_FIELD_16BIT
 * compile-time option. You can do that in pb.h or on compiler command line.
 * 
 * The reason you need to do this is that some of your messages contain tag
 * numbers or field sizes that are larger than what can fit in the default
 * 8 bit descriptors.
 */
STATIC_ASSERT((pb_membersize(SendAmountRequest, withdrawal_address) < 256 && pb_membersize(SendAmountRequest, return_address) < 256 && pb_membersize(SendAmountResponse, request) < 256 && pb_membersize(SendAmountResponse, deposit_address) < 256), YOU_MUST_DEFINE_PB_FIELD_16BIT_FOR_MESSAGES_ExchangeAddress_SendAmountRequest_SendAmountResponse)
#endif


