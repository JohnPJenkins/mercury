/*
 * Copyright (C) 2013-2016 Argonne National Laboratory, Department of Energy,
 *                    UChicago Argonne, LLC and The HDF Group.
 * All rights reserved.
 *
 * The full copyright notice, including terms governing use, modification,
 * and redistribution, is contained in the COPYING file that can be
 * found at the root of the source code distribution tree.
 */

#include "na_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NA_TEST_BULK_SIZE 1024 * 1024
#define NA_TEST_SEND_TAG 100
#define NA_TEST_BULK_TAG 102
#define NA_TEST_BULK_ACK_TAG 103

static int test_msg_done_g = 0;
static int test_msg_recv_done_g = 0;
static int test_msg_send_done_g = 0;
static int test_bulk_done_g = 0;

/* Test parameters */
struct na_test_params {
    na_class_t *na_class;
    na_context_t *context;
    na_addr_t server_addr;
    char *send_buf;
    char *recv_buf;
    unsigned int *bulk_buf;
    na_size_t send_buf_len;
    na_size_t recv_buf_len;
    na_size_t bulk_size;
    na_mem_handle_t local_mem_handle;
    int ret;
};

/* NA test routines */
static int test_msg_forward(struct na_test_params *params);
static int test_bulk(struct na_test_params *params);

/* NA test user-defined callbacks */
static na_return_t
lookup_cb(const struct na_cb_info *callback_info)
{
    struct na_test_params *params = (struct na_test_params *) callback_info->arg;
    na_return_t ret = NA_SUCCESS;

    if (callback_info->ret != NA_SUCCESS) {
        return ret;
    }

    params->server_addr = callback_info->info.lookup.addr;

    params->ret = test_msg_forward(params);

    return ret;
}

static na_return_t
msg_expected_recv_cb(const struct na_cb_info *callback_info)
{
    struct na_test_params *params = (struct na_test_params *) callback_info->arg;
    na_return_t ret = NA_SUCCESS;

    if (callback_info->ret != NA_SUCCESS) {
        return ret;
    }

    printf("Received msg (%s)\n", params->recv_buf);

    test_msg_recv_done_g = 1;
    if (test_msg_recv_done_g && test_msg_send_done_g) {
        test_msg_done_g = 1;
        params->ret = test_bulk(params);
    }

    return ret;
}

static na_return_t
msg_unexpected_send_cb(const struct na_cb_info *callback_info)
{
    struct na_test_params *params = (struct na_test_params *) callback_info->arg;
    na_return_t ret = NA_SUCCESS;

    if (callback_info->ret != NA_SUCCESS) {
        return ret;
    }

    printf("Sent msg (%s)\n", params->send_buf);

    test_msg_send_done_g = 1;
    if (test_msg_recv_done_g && test_msg_send_done_g) {
        test_msg_done_g = 1;
        params->ret = test_bulk(params);
    }

    return ret;
}

static na_return_t
ack_expected_recv_cb(const struct na_cb_info *callback_info)
{
    struct na_test_params *params = (struct na_test_params *) callback_info->arg;
    na_return_t ret = NA_SUCCESS;
    unsigned int i;
    na_bool_t error = 0;

    if (callback_info->ret != NA_SUCCESS) {
        return ret;
    }

    printf("Bulk transfer complete\n");

    /* Check bulk buf */
    for (i = 0; i < params->bulk_size; i++) {
        if (params->bulk_buf[i] != 0) {
            printf("Error detected in bulk transfer, bulk_buf[%u] = %u,\t"
                    " was expecting %d!\n", i, params->bulk_buf[i], 0);
            error = 1;
            break;
        }
    }
    if (!error) printf("Successfully reset %zu bytes!\n",
        (size_t) params->bulk_size * sizeof(int));

    ret = NA_Mem_deregister(params->na_class, params->local_mem_handle);
    if (ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not deregister memory");
        goto done;
    }

    ret = NA_Mem_handle_free(params->na_class, params->local_mem_handle);
    if (ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not free memory handle");
        goto done;
    }

    test_bulk_done_g = 1;

done:
    return ret;
}

/* NA test routines */
static int
test_msg_forward(struct na_test_params *params)
{
    na_return_t na_ret;
    int ret = EXIT_SUCCESS;

    /* Send a message to addr */
    sprintf(params->send_buf, "Hello Server!");

    /* Preposting response */
    na_ret = NA_Msg_recv_expected(params->na_class, params->context,
        msg_expected_recv_cb, params, params->recv_buf, params->recv_buf_len,
        params->server_addr, NA_TEST_SEND_TAG + 1, NA_OP_ID_IGNORE);
    if (na_ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not prepost recv of expected message");
        ret = EXIT_FAILURE;
        goto done;
    }

    na_ret = NA_Msg_send_unexpected(params->na_class, params->context,
        msg_unexpected_send_cb, params, params->send_buf, params->send_buf_len,
        params->server_addr, NA_TEST_SEND_TAG, NA_OP_ID_IGNORE);
    if (na_ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not start send of unexpected message");
        ret = EXIT_FAILURE;
        goto done;
    }

done:
    return ret;
}

static int
test_bulk(struct na_test_params *params)
{
    na_return_t na_ret;
    int ret = EXIT_SUCCESS;

    /* Register memory */
    printf("Registering local memory...\n");
    na_ret = NA_Mem_handle_create(params->na_class, params->bulk_buf,
        sizeof(int) * params->bulk_size, NA_MEM_READWRITE,
        &params->local_mem_handle);
    if (na_ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not create NA memory handle");
        ret = EXIT_FAILURE;
        goto done;
    }

    na_ret = NA_Mem_register(params->na_class, params->local_mem_handle);
    if (na_ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not register NA memory handle");
        ret = EXIT_FAILURE;
        goto done;
    }

    /* Serialize mem handle */
    printf("Serializing bulk memory handle...\n");
    na_ret = NA_Mem_handle_serialize(params->na_class, params->send_buf,
        params->send_buf_len, params->local_mem_handle);
    if (na_ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not serialize memory handle");
        ret = EXIT_FAILURE;
        goto done;
    }

    /* Recv completion ack */
    printf("Preposting recv of transfer ack...\n");
    na_ret = NA_Msg_recv_expected(params->na_class, params->context,
        &ack_expected_recv_cb, params, params->recv_buf, params->recv_buf_len,
        params->server_addr, NA_TEST_BULK_ACK_TAG, NA_OP_ID_IGNORE);
    if (na_ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not start receive of ack");
        ret = EXIT_FAILURE;
        goto done;
    }

    /* Send mem handle */
    printf("Sending local memory handle...\n");
    na_ret = NA_Msg_send_expected(params->na_class, params->context, NULL,
        NULL, params->send_buf, params->send_buf_len, params->server_addr,
        NA_TEST_BULK_TAG, NA_OP_ID_IGNORE);
    if (na_ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not start send of memory handle");
        ret = EXIT_FAILURE;
        goto done;
    }

done:
    return ret;
}

int
main(int argc, char *argv[])
{
    char server_name[NA_TEST_MAX_ADDR_NAME];
    struct na_test_params params;
    na_return_t na_ret;
    unsigned int i;
    int ret = EXIT_SUCCESS;

    /* Initialize the interface */
    params.na_class = NA_Test_client_init(argc, argv, server_name,
    NA_TEST_MAX_ADDR_NAME, NULL);

    params.context = NA_Context_create(params.na_class);

    /* Allocate send and recv bufs */
    params.send_buf_len = NA_Msg_get_max_unexpected_size(params.na_class);
    params.recv_buf_len = params.send_buf_len;
    params.send_buf = (char *) calloc(params.send_buf_len, sizeof(char));
    params.recv_buf = (char *) calloc(params.recv_buf_len, sizeof(char));

    /* Prepare bulk_buf */
    params.bulk_size = NA_TEST_BULK_SIZE;
    params.bulk_buf = (unsigned int *) malloc(
        params.bulk_size * sizeof(unsigned int));
    for (i = 0; i < params.bulk_size; i++) {
        params.bulk_buf[i] = i;
    }

    /* Perform an address lookup on the target */
    na_ret = NA_Addr_lookup(params.na_class, params.context, lookup_cb, &params,
        server_name, NA_OP_ID_IGNORE);
    if (na_ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not start lookup of addr %s", server_name);
        ret = EXIT_FAILURE;
        goto done;
    }

    while (1) {
        na_return_t trigger_ret;
        unsigned int actual_count = 0;

        do {
            trigger_ret = NA_Trigger(params.context, 0, 1, &actual_count);
        } while ((trigger_ret == NA_SUCCESS) && actual_count);

        if (test_msg_done_g && test_bulk_done_g)
            break;

        na_ret = NA_Progress(params.na_class, params.context, NA_MAX_IDLE_TIME);
        if (na_ret != NA_SUCCESS) {
            ret = EXIT_FAILURE;
            goto done;
        }
    }

    ret = params.ret;
    printf("Finalizing...\n");

    /* Free memory and addresses */
    na_ret = NA_Addr_free(params.na_class, params.server_addr);
    if (na_ret != NA_SUCCESS) {
        NA_LOG_ERROR("Could not free addr");
        ret = EXIT_FAILURE;
        goto done;
    }

    free(params.recv_buf);
    free(params.send_buf);
    free(params.bulk_buf);

    NA_Context_destroy(params.na_class, params.context);

    NA_Test_finalize(params.na_class);

done:
    return ret;
}
