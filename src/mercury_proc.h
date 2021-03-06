/*
 * Copyright (C) 2013-2016 Argonne National Laboratory, Department of Energy,
 *                    UChicago Argonne, LLC and The HDF Group.
 * All rights reserved.
 *
 * The full copyright notice, including terms governing use, modification,
 * and redistribution, is contained in the COPYING file that can be
 * found at the root of the source code distribution tree.
 */

#ifndef MERCURY_PROC_H
#define MERCURY_PROC_H

#include "mercury_types.h"
#include "mercury_error.h"
#include "mercury_bulk.h"

#include <stdlib.h>
#include <string.h>
#ifdef HG_HAS_XDR
#include <rpc/types.h>
#include <rpc/xdr.h>
#    ifdef __APPLE__
#        define xdr_int8_t   xdr_char
#        define xdr_uint8_t  xdr_u_char
#        define xdr_uint16_t xdr_u_int16_t
#        define xdr_uint32_t xdr_u_int32_t
#        define xdr_uint64_t xdr_u_int64_t
#    endif
#endif

/*****************/
/* Public Macros */
/*****************/

#ifndef HG_PROC_INLINE
  #if defined(__GNUC__) && !defined(__GNUC_STDC_INLINE__)
    #define HG_PROC_INLINE extern HG_INLINE
  #else
    #define HG_PROC_INLINE HG_INLINE
  #endif
#endif

/*********************/
/* Public Prototypes */
/*********************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Can be used to allocate a buffer that will be used by the generic processor.
 * Buffer should be freed using hg_proc_buf_free().
 *
 * \param size [IN]             request buffer size
 *
 * \return Pointer to memory address or NULL if allocation failed
 */
HG_EXPORT void *
hg_proc_buf_alloc(
        hg_size_t size
        );

/**
 * Free memory which has been previously allocated using hg_proc_buf_alloc().
 *
 * \param mem_ptr [IN]          pointer to memory address
 */
HG_EXPORT void
hg_proc_buf_free(
        void *mem_ptr
        );

/**
 * Create a new encoding/decoding processor.
 *
 * \param hg_class [IN]         HG class
 * \param hash [IN]             hash method used for computing checksum
 *                              (if NULL, checksum is not computed)
 *                              hash method: HG_CRC16, HG_CRC64, HG_NOHASH
 * \param proc [OUT]            pointer to abstract processor object
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
hg_proc_create(
        hg_class_t *hg_class,
        hg_proc_hash_t hash,
        hg_proc_t *proc
        );

/**
 * Create a new encoding/decoding processor.
 *
 * \param hg_class [IN]         HG class
 * \param buf [IN]              pointer to buffer that will be used for
 *                              serialization/deserialization
 * \param buf_size [IN]         buffer size
 * \param op [IN]               operation type: HG_ENCODE / HG_DECODE / HG_FREE
 * \param hash [IN]             hash method used for computing checksum
 *                              (if NULL, checksum is not computed)
 *                              hash method: HG_CRC16, HG_CRC64, HG_NOHASH
 * \param proc [OUT]            pointer to abstract processor object
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
hg_proc_create_set(
        hg_class_t *hg_class,
        void *buf,
        hg_size_t buf_size,
        hg_proc_op_t op,
        hg_proc_hash_t hash,
        hg_proc_t *proc
        );

/**
 * Free the processor.
 *
 * \param proc [IN/OUT]         abstract processor object
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
hg_proc_free(
        hg_proc_t proc
        );

/**
 * Reset the processor.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param buf [IN]              pointer to buffer that will be used for
 *                              serialization/deserialization
 * \param buf_size [IN]         buffer size
 * \param op [IN]               operation type: HG_ENCODE / HG_DECODE / HG_FREE
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
hg_proc_reset(
        hg_proc_t proc,
        void *buf,
        hg_size_t buf_size,
        hg_proc_op_t op
        );

/**
 * Get the HG class associated to the processor.
 *
 * \param proc [IN]             abstract processor object
 *
 * \return HG class
 */
HG_EXPORT hg_class_t *
hg_proc_get_class(
        hg_proc_t proc
        );

/**
 * Get the operation type associated to the processor.
 *
 * \param proc [IN]             abstract processor object
 *
 * \return Operation type
 */
HG_EXPORT hg_proc_op_t
hg_proc_get_op(
        hg_proc_t proc
        );

/**
 * Get buffer size available for processing.
 *
 * \param proc [IN]             abstract processor object
 *
 * \return Non-negative size value
 */
HG_EXPORT hg_size_t
hg_proc_get_size(
        hg_proc_t proc
        );

/**
 * Get amount of buffer space that has actually been consumed
 *
 * \param proc [IN]             abstract processor object
 *
 * \return Non-negative size value
 */
HG_EXPORT hg_size_t
hg_proc_get_size_used(
        hg_proc_t proc
        );


/**
 * Request a new buffer size. This will modify the size of the buffer attached
 * to the processor or create an extra processing buffer.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param buf_size [IN]         buffer size
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
hg_proc_set_size(
        hg_proc_t proc,
        hg_size_t buf_size
        );

/**
 * Get size left for processing.
 *
 * \param proc [IN]             abstract processor object
 *
 * \return Non-negative size value
 */
HG_EXPORT hg_size_t
hg_proc_get_size_left(
        hg_proc_t proc
        );

/**
 * Get pointer to current buffer. Will reserve data_size for manual encoding.
 *
 * \param proc [IN]             abstract processor object
 * \param data_size [IN]        data size
 *
 * \return Buffer pointer
 */
HG_EXPORT void *
hg_proc_save_ptr(
        hg_proc_t proc,
        hg_size_t data_size
        );

#ifdef HG_HAS_XDR
/**
 * Get pointer to current XDR stream (for manual encoding).
 *
 * \param proc [IN]             abstract processor object
 *
 * \return XDR stream pointer
 */
HG_EXPORT XDR *
hg_proc_get_xdr_ptr(
        hg_proc_t proc
        );
#endif

/**
 * Restore pointer from current buffer.
 *
 * \param proc [IN]             abstract processor object
 * \param data [IN]             pointer to data
 * \param data_size [IN]        data size
 *
 * \return Buffer pointer
 */
HG_EXPORT hg_return_t
hg_proc_restore_ptr(
        hg_proc_t proc,
        void *data,
        hg_size_t data_size
        );

/**
 * Get eventual extra buffer used by processor.
 *
 * \param proc [IN]             abstract processor object
 *
 * \return Pointer to buffer or NULL if no extra buffer has been used
 */
HG_EXPORT void *
hg_proc_get_extra_buf(
        hg_proc_t proc
        );

/**
 * Get eventual size of the extra buffer used by processor.
 *
 * \param proc [IN]             abstract processor object
 *
 * \return Size of buffer or 0 if no extra buffer has been used
 */
HG_EXPORT hg_size_t
hg_proc_get_extra_size(
        hg_proc_t proc
        );

/**
 * Set extra buffer to mine (if other calls mine, buffer is no longer freed
 * after hg_proc_free())
 *
 * \param proc [IN]             abstract processor object
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
hg_proc_set_extra_buf_is_mine(
        hg_proc_t proc,
        hg_bool_t mine
        );

/**
 * Flush the proc after data has been encoded or decoded and verify data using
 * base checksum if available.
 *
 * \param proc [IN]             abstract processor object
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
hg_proc_flush(
        hg_proc_t proc
        );

/**
 * Base proc routine using memcpy().
 * \remark Only uses memcpy() / use hg_proc_raw() for encoding raw buffers.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param data [IN/OUT]         pointer to data
 * \param data_size [IN]        data size
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_EXPORT hg_return_t
hg_proc_memcpy(
        hg_proc_t proc,
        void *data,
        hg_size_t data_size
        );

/**
 * Inline prototypes (do not remove)
 */
HG_EXPORT HG_PROC_INLINE hg_return_t hg_proc_hg_int8_t(hg_proc_t proc,
        void *data);
HG_EXPORT HG_PROC_INLINE hg_return_t hg_proc_hg_uint8_t(hg_proc_t proc,
        void *data);
HG_EXPORT HG_PROC_INLINE hg_return_t hg_proc_hg_int16_t(hg_proc_t proc,
        void *data);
HG_EXPORT HG_PROC_INLINE hg_return_t hg_proc_hg_uint16_t(hg_proc_t proc,
        void *data);
HG_EXPORT HG_PROC_INLINE hg_return_t hg_proc_hg_int32_t(hg_proc_t proc,
        void *data);
HG_EXPORT HG_PROC_INLINE hg_return_t hg_proc_hg_uint32_t(hg_proc_t proc,
        void *data);
HG_EXPORT HG_PROC_INLINE hg_return_t hg_proc_hg_int64_t(hg_proc_t proc,
        void *data);
HG_EXPORT HG_PROC_INLINE hg_return_t hg_proc_hg_uint64_t(hg_proc_t proc,
        void *data);
HG_EXPORT HG_PROC_INLINE hg_return_t hg_proc_hg_bulk_t(hg_proc_t proc,
        void *handle);
HG_EXPORT HG_PROC_INLINE void *hg_proc_buf_memcpy(void *buf, void *data,
    hg_size_t data_size, hg_proc_op_t op);


/* Note: float types are not supported but can be built on top of the existing
 * proc routines; encoding floats using XDR could modify checksum */

/**
 * For convenience map stdint types to hg types
 */
#define hg_proc_int8_t   hg_proc_hg_int8_t
#define hg_proc_uint8_t  hg_proc_hg_uint8_t
#define hg_proc_int16_t  hg_proc_hg_int16_t
#define hg_proc_uint16_t hg_proc_hg_uint16_t
#define hg_proc_int32_t  hg_proc_hg_int32_t
#define hg_proc_uint32_t hg_proc_hg_uint32_t
#define hg_proc_int64_t  hg_proc_hg_int64_t
#define hg_proc_uint64_t hg_proc_hg_uint64_t

/* Map mercury common types */
#define hg_proc_hg_bool_t     hg_proc_hg_uint8_t
#define hg_proc_hg_ptr_t      hg_proc_hg_uint64_t
#define hg_proc_hg_size_t     hg_proc_hg_uint64_t
#define hg_proc_hg_id_t       hg_proc_hg_uint32_t

/* For now, map hg_proc_raw to hg_proc_memcpy */
#define hg_proc_raw     hg_proc_memcpy


/**
 * Generic processing routine.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param data [IN/OUT]         pointer to data
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PROC_INLINE hg_return_t
hg_proc_hg_int8_t(hg_proc_t proc, void *data)
{
    hg_return_t ret;
#ifdef HG_HAS_XDR
    ret = xdr_int8_t(hg_proc_get_xdr_ptr(proc), data) ? HG_SUCCESS : HG_PROTOCOL_ERROR;
#else
    ret = hg_proc_memcpy(proc, data, sizeof(hg_int8_t));
#endif
    return ret;
}

/**
 * Generic processing routine.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param data [IN/OUT]         pointer to data
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PROC_INLINE hg_return_t
hg_proc_hg_uint8_t(hg_proc_t proc, void *data)
{
    hg_return_t ret;
#ifdef HG_HAS_XDR
    ret = xdr_uint8_t(hg_proc_get_xdr_ptr(proc), data) ? HG_SUCCESS : HG_PROTOCOL_ERROR;
#else
    ret = hg_proc_memcpy(proc, data, sizeof(hg_uint8_t));
#endif
    return ret;
}

/**
 * Generic processing routine.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param data [IN/OUT]         pointer to data
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PROC_INLINE hg_return_t
hg_proc_hg_int16_t(hg_proc_t proc, void *data)
{
    hg_return_t ret;
#ifdef HG_HAS_XDR
    ret = xdr_int16_t(hg_proc_get_xdr_ptr(proc), data) ? HG_SUCCESS : HG_PROTOCOL_ERROR;
#else
    ret = hg_proc_memcpy(proc, data, sizeof(hg_int16_t));
#endif
    return ret;
}

/**
 * Generic processing routine.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param data [IN/OUT]         pointer to data
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PROC_INLINE hg_return_t
hg_proc_hg_uint16_t(hg_proc_t proc, void *data)
{
    hg_return_t ret;
#ifdef HG_HAS_XDR
    ret = xdr_uint16_t(hg_proc_get_xdr_ptr(proc), data) ? HG_SUCCESS : HG_PROTOCOL_ERROR;
#else
    ret = hg_proc_memcpy(proc, data, sizeof(hg_uint16_t));
#endif
    return ret;
}

/**
 * Generic processing routine.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param data [IN/OUT]         pointer to data
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PROC_INLINE hg_return_t
hg_proc_hg_int32_t(hg_proc_t proc, void *data)
{
    hg_return_t ret;
#ifdef HG_HAS_XDR
    ret = xdr_int32_t(hg_proc_get_xdr_ptr(proc), data) ? HG_SUCCESS : HG_PROTOCOL_ERROR;
#else
    ret = hg_proc_memcpy(proc, data, sizeof(hg_int32_t));
#endif
    return ret;
}

/**
 * Generic processing routine.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param data [IN/OUT]         pointer to data
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PROC_INLINE hg_return_t
hg_proc_hg_uint32_t(hg_proc_t proc, void *data)
{
    hg_return_t ret;
#ifdef HG_HAS_XDR
    ret = xdr_uint32_t(hg_proc_get_xdr_ptr(proc), data) ? HG_SUCCESS : HG_PROTOCOL_ERROR;
#else
    ret = hg_proc_memcpy(proc, data, sizeof(hg_uint32_t));
#endif
    return ret;
}

/**
 * Generic processing routine.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param data [IN/OUT]         pointer to data
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PROC_INLINE hg_return_t
hg_proc_hg_int64_t(hg_proc_t proc, void *data)
{
    hg_return_t ret;
#ifdef HG_HAS_XDR
    ret = xdr_int64_t(hg_proc_get_xdr_ptr(proc), data) ? HG_SUCCESS : HG_PROTOCOL_ERROR;
#else
    ret = hg_proc_memcpy(proc, data, sizeof(hg_int64_t));
#endif
    return ret;
}

/**
 * Generic processing routine.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param data [IN/OUT]         pointer to data
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PROC_INLINE hg_return_t
hg_proc_hg_uint64_t(hg_proc_t proc, void *data)
{
    hg_return_t ret;
#ifdef HG_HAS_XDR
    ret = xdr_uint64_t(hg_proc_get_xdr_ptr(proc), data) ? HG_SUCCESS : HG_PROTOCOL_ERROR;
#else
    ret = hg_proc_memcpy(proc, data, sizeof(hg_uint64_t));
#endif
    return ret;
}

/**
 * Generic processing routine.
 *
 * \param proc [IN/OUT]         abstract processor object
 * \param handle [IN/OUT]       pointer to bulk handle
 *
 * \return HG_SUCCESS or corresponding HG error code
 */
HG_PROC_INLINE hg_return_t
hg_proc_hg_bulk_t(hg_proc_t proc, void *handle)
{
    hg_return_t ret = HG_SUCCESS;
    void *buf = NULL;
    hg_bulk_t *bulk = (hg_bulk_t*)handle;
    hg_uint64_t buf_size = 0;

    switch (hg_proc_get_op(proc)) {
        case HG_ENCODE: {
            hg_bool_t request_eager = HG_FALSE;

            if (*bulk == HG_BULK_NULL) {
                /* If HG_BULK_NULL set 0 to buf_size */
                buf_size = 0;
            } else {
#ifdef HG_HAS_EAGER_BULK
                request_eager = (hg_proc_get_size_left(proc)
                    > HG_Bulk_get_serialize_size(*bulk, HG_TRUE))
                    ? HG_TRUE : HG_FALSE;
#endif
                buf_size = HG_Bulk_get_serialize_size(*bulk, request_eager);
            }
            /* Encode size */
            ret = hg_proc_uint64_t(proc, &buf_size);
            if (ret != HG_SUCCESS) {
                HG_LOG_ERROR("Proc error");
                return ret;
            }
            if (buf_size) {
                buf = hg_proc_save_ptr(proc, buf_size);
                ret = HG_Bulk_serialize(buf, buf_size, request_eager, *bulk);
                if (ret != HG_SUCCESS) {
                    HG_LOG_ERROR("Could not serialize bulk handle");
                    return ret;
                }
                hg_proc_restore_ptr(proc, buf, buf_size);
            }
        }
        break;
        case HG_DECODE:
            /* Decode size */
            ret = hg_proc_uint64_t(proc, &buf_size);
            if (ret != HG_SUCCESS) {
                HG_LOG_ERROR("Proc error");
                return ret;
            }
            if (buf_size) {
                hg_class_t *hg_class = hg_proc_get_class(proc);

                buf = hg_proc_save_ptr(proc, buf_size);
                ret = HG_Bulk_deserialize(hg_class, bulk, buf, buf_size);
                if (ret != HG_SUCCESS) {
                    HG_LOG_ERROR("Could not deserialize bulk handle");
                    return ret;
                }
                hg_proc_restore_ptr(proc, buf, buf_size);
            } else {
                /* If buf_size is 0, define handle to HG_BULK_NULL */
                *bulk = HG_BULK_NULL;
            }
            break;
        case HG_FREE:
            if (*bulk != HG_BULK_NULL) {
                ret = HG_Bulk_free(*bulk);
                if (ret != HG_SUCCESS) {
                    HG_LOG_ERROR("Could not free bulk handle");
                    return ret;
                }
                *bulk = HG_BULK_NULL;
            } else {
                /* If *bulk is HG_BULK_NULL, just return success */
                ret = HG_SUCCESS;
            }
            break;
        default:
            break;
    }
    return ret;
}

/**
 * Copy data to buf if HG_ENCODE or buf to data if HG_DECODE and return
 * incremented pointer to buf.
 *
 * \param buf [IN/OUT]          abstract processor object
 * \param data [IN/OUT]         pointer to data
 * \param data_size [IN]        data size
 * \param op [IN]               operation type: HG_ENCODE / HG_DECODE
 *
 * \return incremented pointer to buf
 */
HG_PROC_INLINE void *
hg_proc_buf_memcpy(void *buf, void *data, hg_size_t data_size, hg_proc_op_t op)
{
    const void *src = NULL;
    void *dest = NULL;

    if ((op != HG_ENCODE) && (op != HG_DECODE)) return NULL;
    src = (op == HG_ENCODE) ? (const void *) data : (const void *) buf;
    dest = (op == HG_ENCODE) ? buf : data;
    memcpy(dest, src, data_size);

    return ((char *) buf + data_size);
}

#ifdef __cplusplus
}
#endif

#endif /* MERCURY_PROC_H */
