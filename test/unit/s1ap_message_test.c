#include "core/abts.h"

#include "mme/s1ap_build.h"
#include "mme/s1ap_conv.h"

#include "testpacket.h"

static void s1ap_message_test1(abts_case *tc, void *data)
{
    /* S1SetupRequest */
    char *payload = 
        "0011002d000004003b00090000f11040"
        "54f64010003c400903004a4c542d3632"
        "3100400007000c0e4000f11000894001"
        "00";

    s1ap_message_t message;
    ogs_pkbuf_t *pkbuf;
    int result;
    char hexbuf[MAX_SDU_LEN];

    pkbuf = ogs_pkbuf_alloc(NULL, MAX_SDU_LEN);
    ogs_assert(pkbuf);
    ogs_pkbuf_put_data(pkbuf, 
            OGS_HEX(payload, strlen(payload), hexbuf), 49);

    result = s1ap_decode_pdu(&message, pkbuf);
    ABTS_INT_EQUAL(tc, 0, result);
    s1ap_free_pdu(&message);

    ogs_pkbuf_free(pkbuf);
}

static void s1ap_message_test2(abts_case *tc, void *data)
{
    /* InitialUE(Attach Request) */
    char *payload = 
        "000c406f000006000800020001001a00"
        "3c3b17df675aa8050741020bf600f110"
        "000201030003e605f070000010000502"
        "15d011d15200f11030395c0a003103e5"
        "e0349011035758a65d0100e0c1004300"
        "060000f1103039006440080000f1108c"
        "3378200086400130004b00070000f110"
        "000201";

    s1ap_message_t message;
    ogs_pkbuf_t *pkbuf;
    int result;
    char hexbuf[MAX_SDU_LEN];

    pkbuf = ogs_pkbuf_alloc(NULL, MAX_SDU_LEN);
    ogs_assert(pkbuf);
    ogs_pkbuf_put_data(pkbuf, 
            OGS_HEX(payload, strlen(payload), hexbuf), 115);

    result = s1ap_decode_pdu(&message, pkbuf);
    ABTS_INT_EQUAL(tc, 0, result);
    s1ap_free_pdu(&message);

    ogs_pkbuf_free(pkbuf);
}

static void s1ap_message_test3(abts_case *tc, void *data)
{
    /* initial context setup response */
    char *payload = 
        "2009002500000300004005c0020000bf"
        "0008400200010033400f000032400a0a"
        "1f0a0123c601000908";

    s1ap_message_t message;
    ogs_pkbuf_t *pkbuf;
    int result;
    char hexbuf[MAX_SDU_LEN];

    pkbuf = ogs_pkbuf_alloc(NULL, MAX_SDU_LEN);
    ogs_assert(pkbuf);
    ogs_pkbuf_put_data(pkbuf, 
            OGS_HEX(payload, strlen(payload), hexbuf), 41);

    result = s1ap_decode_pdu(&message, pkbuf);
    ABTS_INT_EQUAL(tc, 0, result);
    s1ap_free_pdu(&message);

    ogs_pkbuf_free(pkbuf);
}

static void s1ap_message_test4(abts_case *tc, void *data)
{
    s1ap_message_t message;
    int rv;
    ogs_pkbuf_t *pkbuf;
    int result;

    rv = s1ap_build_setup_rsp(&pkbuf);

    ABTS_INT_EQUAL(tc, OGS_OK, rv);
    ABTS_PTR_NOTNULL(tc, pkbuf);
    ABTS_PTR_NOTNULL(tc, pkbuf->data);
    ABTS_INT_EQUAL(tc, 27, pkbuf->len);

    result = s1ap_decode_pdu(&message, pkbuf);
    ABTS_INT_EQUAL(tc, 0, result);

    s1ap_free_pdu(&message);
    ogs_pkbuf_free(pkbuf);
}

static void s1ap_message_test5(abts_case *tc, void *data)
{
    s1ap_message_t message;
    int rv;
    ogs_pkbuf_t *pkbuf;
    int result;

    rv = tests1ap_build_setup_req(&pkbuf, S1AP_ENB_ID_PR_macroENB_ID,
            0x54f64, 12345, 1, 1, 2);

    ABTS_INT_EQUAL(tc, OGS_OK, rv);
    ABTS_PTR_NOTNULL(tc, pkbuf);
    ABTS_PTR_NOTNULL(tc, pkbuf->data);
    ABTS_INT_EQUAL(tc, 35, pkbuf->len);

    result = s1ap_decode_pdu(&message, pkbuf);
    ABTS_INT_EQUAL(tc, 0, result);

    s1ap_free_pdu(&message);
    ogs_pkbuf_free(pkbuf);
}

static void s1ap_message_test6(abts_case *tc, void *data)
{
    int rv;
    ogs_pkbuf_t *s1apbuf = NULL;
    ogs_pkbuf_t *emmbuf = NULL;
    enb_ue_t enb_ue;
    int i;
    s1ap_message_t message;
    char buffer[1024];
    char *_result = 
    "000b4080 8c000003 00000002 00010008 00020001 001a0079 78efefef efefefef"
    "efefefef efefefef efefefef efefefef efefefef efefefef efefefef efefefef"
    "efefefef efefefef efefefef efefefef efefefef efefefef efefefef efefefef"
    "efefefef efefefef efefefef efefefef efefefef efefefef efefefef efefefef"
    "efefefef efefefef efefefef efefefef ef";

    enb_ue.mme_ue_s1ap_id = 1;
    enb_ue.enb_ue_s1ap_id = 1;

    emmbuf = ogs_pkbuf_alloc(NULL, 120);
    ABTS_PTR_NOTNULL(tc, emmbuf);
    ogs_pkbuf_put(emmbuf, 120);
    for (i = 0; i < emmbuf->len; i++)
        ((char *)emmbuf->data)[i] = 0xef;

    rv = s1ap_build_downlink_nas_transport(&s1apbuf, &enb_ue, emmbuf);
    ABTS_INT_EQUAL(tc, OGS_OK, rv);
    ABTS_TRUE(tc, memcmp(OGS_HEX(_result, strlen(_result), buffer),
            s1apbuf->data, s1apbuf->len) == 0);
    ogs_pkbuf_free(s1apbuf);
}

static void s1ap_message_test7(abts_case *tc, void *data)
{
    /* InitialUE(Service Request) */
    char *payload = 
        "000c402d000005000800020071001a00 0504c706b410004300060013f1890001"
        "006440080013f189400bb75000864001 40006440080013f189400bb750004340"
        "060013f18900014300060013f1890001 006440080013f189400db09000864001"
        "30000000000000000000000000000000 00000000000000000000000000000000";

    s1ap_message_t message;
    ogs_pkbuf_t *pkbuf;
    int result;
    char hexbuf[MAX_SDU_LEN];

    pkbuf = ogs_pkbuf_alloc(NULL, MAX_SDU_LEN);
    ogs_assert(pkbuf);
    ogs_pkbuf_put_data(pkbuf, 
            OGS_HEX(payload, strlen(payload), hexbuf), 128);

    result = s1ap_decode_pdu(&message, pkbuf);
    ABTS_INT_EQUAL(tc, 0, result);
    s1ap_free_pdu(&message);

    ogs_pkbuf_free(pkbuf);
}

abts_suite *test_s1ap_message(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, s1ap_message_test1, NULL);
    abts_run_test(suite, s1ap_message_test2, NULL);
    abts_run_test(suite, s1ap_message_test3, NULL);
#if 0
    abts_run_test(suite, s1ap_message_test4, NULL);
#endif
    abts_run_test(suite, s1ap_message_test5, NULL);
    abts_run_test(suite, s1ap_message_test6, NULL);
    abts_run_test(suite, s1ap_message_test7, NULL);

    return suite;
}
