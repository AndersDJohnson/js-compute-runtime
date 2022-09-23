#ifndef xqd_H
#define xqd_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WASM_IMPORT(module, name) __attribute__((import_module(module), import_name(name)))

#define XQD_ABI_VERSION 0x01ULL

// max header size to match vcl
#define HEADER_MAX_LEN 69000
#define METHOD_MAX_LEN 1024
#define URI_MAX_LEN 8192
#define CONFIG_STORE_ENTRY_MAX_LEN 8000
#define DICTIONARY_ENTRY_MAX_LEN CONFIG_STORE_ENTRY_MAX_LEN

// TODO ACF 2020-01-17: these aren't very C-friendly names
typedef struct {
  uint32_t handle;
} BodyHandle;

typedef struct {
  uint32_t handle;
} RequestHandle;

typedef struct {
  uint32_t handle;
} ResponseHandle;

typedef struct {
  uint32_t handle;
} PendingRequestHandle;

typedef struct {
  uint32_t handle;
} LogEndpointHandle;

typedef struct {
  uint32_t handle;
} DictionaryHandle;

typedef struct {
  uint32_t handle;
} ConfigStoreHandle;

typedef struct {
  uint32_t handle;
} ObjectStoreHandle;

#define INVALID_HANDLE (UINT32_MAX - 1)

typedef enum FastlyStatus {
  // Success value.
  // This indicates that a hostcall finished successfully.
  ok = 0,
  // Generic error value.
  // This means that some unexpected error occurred during a hostcall.
  error = 1,
  // Invalid argument.
  inval = 2,
  // Invalid handle.
  // Thrown when a handle is not valid. E.G. No dictionary exists with the given name.
  badf = 3,
  // Buffer length error.
  // Thrown when a buffer is too long.
  buflen = 4,
  // Unsupported operation error.
  // This error is thrown when some operation cannot be performed, because it is not supported.
  unsupported = 5,
  // Alignment error.
  // This is thrown when a pointer does not point to a properly aligned slice of memory.
  badalign = 6,
  // Invalid HTTP error.
  // This can be thrown when a method, URI, header, or status is not valid. This can also
  // be thrown if a message head is too large.
  httpinvalid = 7,
  // HTTP user error.
  // This is thrown in cases where user code caused an HTTP error. For example, attempt to send
  // a 1xx response code, or a request with a non-absolute URI. This can also be caused by
  // an unexpected header: both `content-length` and `transfer-encoding`, for example.
  httpuser = 8,
  // HTTP incomplete message error.
  // This can be thrown when a stream ended unexpectedly.
  httpincomplete = 9,
  // A `None` error.
  // This status code is used to indicate when an optional value did not exist, as opposed to
  // an empty value.
  none = 10,
  // Message head too large.
  httpheadtoolarge = 11,
  // Invalid HTTP status.
  httpinvalidstatus = 12,
} FastlyStatus;

typedef enum BodyWriteEnd {
  BodyWriteEndBack = 0,
  BodyWriteEndFront = 1,
} BodyWriteEnd;

#define CACHE_OVERRIDE_NONE (0u)
#define CACHE_OVERRIDE_PASS (1u << 0)
#define CACHE_OVERRIDE_TTL (1u << 1)
#define CACHE_OVERRIDE_STALE_WHILE_REVALIDATE (1u << 2)
#define CACHE_OVERRIDE_PCI (1u << 3)

// TODO ACF 2019-12-05: nicer type for the return value (XqdStatus)

// Module fastly_abi
WASM_IMPORT("fastly_abi", "init")
FastlyStatus xqd_init(uint64_t abi_version);

// Module fastly_uap
WASM_IMPORT("fastly_uap", "parse")
FastlyStatus xqd_uap_parse(const char *user_agent, size_t user_agent_len, char *family,
                  size_t family_max_len, size_t *family_nwritten, char *major, size_t major_max_len,
                  size_t *major_nwritten, char *minor, size_t minor_max_len, size_t *minor_nwritten,
                  char *patch, size_t patch_max_len, size_t *patch_nwritten);

// Module fastly_http_body
WASM_IMPORT("fastly_http_body", "append")
FastlyStatus xqd_body_append(BodyHandle dst_handle, BodyHandle src_handle);

WASM_IMPORT("fastly_http_body", "new")
FastlyStatus xqd_body_new(BodyHandle *handle_out);

WASM_IMPORT("fastly_http_body", "read")
FastlyStatus xqd_body_read(BodyHandle body_handle, char *buf, size_t buf_len, size_t *nread);

WASM_IMPORT("fastly_http_body", "write")
FastlyStatus xqd_body_write(BodyHandle body_handle, const char *buf, size_t buf_len, BodyWriteEnd end,
                   size_t *nwritten);

WASM_IMPORT("fastly_http_body", "close")
FastlyStatus xqd_body_close(BodyHandle body_handle);

// Module fastly_log
WASM_IMPORT("fastly_log", "endpoint_get")
FastlyStatus xqd_log_endpoint_get(const char *name, size_t name_len, LogEndpointHandle *endpoint_handle);

WASM_IMPORT("fastly_log", "write")
FastlyStatus xqd_log_write(LogEndpointHandle endpoint_handle, const char *msg, size_t msg_len,
                  size_t *nwritten);

// Module fastly_http_req
WASM_IMPORT("fastly_http_req", "body_downstream_get")
FastlyStatus xqd_req_body_downstream_get(RequestHandle *req_handle_out, BodyHandle *body_handle_out);

/**
 * Set the cache override behavior for this request.
 *
 * The default behavior, equivalent to `CACHE_OVERRIDE_NONE`, respects the cache control headers
 * from the origin's response.
 *
 * Calling this function with `CACHE_OVERRIDE_PASS` will ignore the subsequent arguments and Pass
 * unconditionally.
 *
 * To override, TTL, stale-while-revalidate, or stale-with-error, set the appropriate bits in the
 * tag using the corresponding constants, and pass the override values in the appropriate arguments.
 *
 * xqd_req_cache_override_v2_set also includes an optional Surrogate-Key which will be set or added
 * to any received from the origin.
 */
WASM_IMPORT("fastly_http_req", "cache_override_set")
FastlyStatus xqd_req_cache_override_set(RequestHandle req_handle, int tag, uint32_t ttl,
                               uint32_t stale_while_revalidate);

WASM_IMPORT("fastly_http_req", "cache_override_v2_set")
FastlyStatus xqd_req_cache_override_v2_set(RequestHandle req_handle, int tag, uint32_t ttl,
                                  uint32_t stale_while_revalidate, const char *surrogate_key,
                                  size_t surrogate_key_len);

/**
 * `octets` must be a 16-byte array.
 * If, after a successful call, `nwritten` == 4, the value in `octets` is an IPv4 address.
 * Otherwise, if `nwritten` will is `16`, the value in `octets` is an IPv6 address.
 * Otherwise, `nwritten` will be `0`, and no address is available.
 */
WASM_IMPORT("fastly_http_req", "downstream_client_ip_addr")
FastlyStatus xqd_req_downstream_client_ip_addr_get(char *octets, size_t *nwritten);

// TODO:

// (@interface func (export "downstream_tls_cipher_openssl_name")
//    (param $cipher_out (@witx pointer char8))
//    (param $cipher_max_len (@witx usize))
//    (param $nwritten_out (@witx pointer (@witx usize)))
//    (result $err $fastly_status)
// )

// (@interface func (export "downstream_tls_protocol")
//    (param $protocol_out (@witx pointer char8))
//    (param $protocol_max_len (@witx usize))
//    (param $nwritten_out (@witx pointer (@witx usize)))
//    (result $err $fastly_status)
// )

// (@interface func (export "downstream_tls_client_hello")
//    (param $chello_out (@witx pointer char8))
//    (param $chello_max_len (@witx usize))
//    (param $nwritten_out (@witx pointer (@witx usize)))
//    (result $err $fastly_status)
// )

WASM_IMPORT("fastly_http_req", "new")
FastlyStatus xqd_req_new(RequestHandle *req_handle_out);

WASM_IMPORT("fastly_http_req", "header_names_get")
FastlyStatus xqd_req_header_names_get(RequestHandle req_handle, char *buf, size_t buf_len, uint32_t cursor,
                             int64_t *ending_cursor, size_t *nwritten);

WASM_IMPORT("fastly_http_req", "original_header_names_get")
FastlyStatus xqd_req_original_header_names_get(char *buf, size_t buf_len, uint32_t cursor,
                                      int64_t *ending_cursor, size_t *nwritten);

WASM_IMPORT("fastly_http_req", "original_header_count")
FastlyStatus xqd_req_original_header_count(uint32_t *count);

WASM_IMPORT("fastly_http_req", "header_value_get")
FastlyStatus xqd_req_header_value_get(RequestHandle req_handle, const char *name, size_t name_len,
                             char *value, size_t value_max_len, size_t *nwritten);

WASM_IMPORT("fastly_http_req", "header_values_get")
FastlyStatus xqd_req_header_values_get(RequestHandle req_handle, const char *name, size_t name_len,
                              char *buf, size_t buf_len, uint32_t cursor, int64_t *ending_cursor,
                              size_t *nwritten);

WASM_IMPORT("fastly_http_req", "header_values_set")
FastlyStatus xqd_req_header_values_set(RequestHandle req_handle, const char *name, size_t name_len,
                              const char *values, size_t values_len);

WASM_IMPORT("fastly_http_req", "header_insert")
FastlyStatus xqd_req_header_insert(RequestHandle req_handle, const char *name, size_t name_len,
                          const char *value, size_t value_len);

WASM_IMPORT("fastly_http_req", "header_append")
FastlyStatus xqd_req_header_append(RequestHandle req_handle, const char *name, size_t name_len,
                          const char *value, size_t value_len);

WASM_IMPORT("fastly_http_req", "header_remove")
FastlyStatus xqd_req_header_remove(RequestHandle req_handle, const char *name, size_t name_len);

WASM_IMPORT("fastly_http_req", "method_get")
FastlyStatus xqd_req_method_get(RequestHandle req_handle, char *method, size_t method_max_len,
                       size_t *nwritten);

WASM_IMPORT("fastly_http_req", "method_set")
FastlyStatus xqd_req_method_set(RequestHandle req_handle, const char *method, size_t method_len);

WASM_IMPORT("fastly_http_req", "uri_get")
FastlyStatus xqd_req_uri_get(RequestHandle req_handle, char *uri, size_t uri_max_len, size_t *nwritten);

WASM_IMPORT("fastly_http_req", "uri_set")
FastlyStatus xqd_req_uri_set(RequestHandle req_handle, const char *uri, size_t uri_len);

WASM_IMPORT("fastly_http_req", "version_get")
FastlyStatus xqd_req_version_get(RequestHandle req_handle, uint32_t *version);

WASM_IMPORT("fastly_http_req", "version_set")
FastlyStatus xqd_req_version_set(RequestHandle req_handle, uint32_t version);

WASM_IMPORT("fastly_http_req", "send")
FastlyStatus xqd_req_send(RequestHandle req_handle, BodyHandle body_handle, const char *backend,
                 size_t backend_len, ResponseHandle *resp_handle_out,
                 BodyHandle *resp_body_handle_out);

WASM_IMPORT("fastly_http_req", "send_async")
FastlyStatus xqd_req_send_async(RequestHandle req_handle, BodyHandle body_handle, const char *backend,
                       size_t backend_len, PendingRequestHandle *pending_req_out);

WASM_IMPORT("fastly_http_req", "send_async_streaming")
FastlyStatus xqd_req_send_async_streaming(RequestHandle req_handle, BodyHandle body_handle,
                                 const char *backend, size_t backend_len,
                                 PendingRequestHandle *pending_req_out);

WASM_IMPORT("fastly_http_req", "pending_req_poll")
FastlyStatus xqd_req_pending_req_poll(PendingRequestHandle req_handle, uint32_t *is_done_out,
                             ResponseHandle *resp_handle_out, BodyHandle *resp_body_handle_out);

WASM_IMPORT("fastly_http_req", "pending_req_wait")
FastlyStatus xqd_req_pending_req_wait(PendingRequestHandle req_handle, ResponseHandle *resp_handle_out,
                             BodyHandle *resp_body_handle_out);

WASM_IMPORT("fastly_http_req", "pending_req_select")
FastlyStatus xqd_req_pending_req_select(PendingRequestHandle req_handles[], size_t req_handles_len,
                               uint32_t *done_idx_out, ResponseHandle *resp_handle_out,
                               BodyHandle *resp_body_handle_out);

// Module fastly_http_resp
WASM_IMPORT("fastly_http_resp", "new")
FastlyStatus xqd_resp_new(ResponseHandle *resp_handle_out);

WASM_IMPORT("fastly_http_resp", "header_names_get")
FastlyStatus xqd_resp_header_names_get(ResponseHandle resp_handle, char *buf, size_t buf_len,
                              uint32_t cursor, int64_t *ending_cursor, size_t *nwritten);

WASM_IMPORT("fastly_http_resp", "header_value_get")
FastlyStatus xqd_resp_header_value_get(ResponseHandle resp_handle, const char *name, size_t name_len,
                              char *value, size_t value_max_len, size_t *nwritten);

WASM_IMPORT("fastly_http_resp", "header_values_get")
FastlyStatus xqd_resp_header_values_get(ResponseHandle resp_handle, const char *name, size_t name_len,
                               char *buf, size_t buf_len, uint32_t cursor, int64_t *ending_cursor,
                               size_t *nwritten);

WASM_IMPORT("fastly_http_resp", "header_values_set")
FastlyStatus xqd_resp_header_values_set(ResponseHandle resp_handle, const char *name, size_t name_len,
                               const char *buf, size_t buf_len);

WASM_IMPORT("fastly_http_resp", "header_insert")
FastlyStatus xqd_resp_header_insert(ResponseHandle resp_handle, const char *name, size_t name_len,
                           const char *value, size_t value_len);

WASM_IMPORT("fastly_http_resp", "header_append")
FastlyStatus xqd_resp_header_append(ResponseHandle resp_handle, const char *name, size_t name_len,
                           const char *value, size_t value_len);

WASM_IMPORT("fastly_http_resp", "header_remove")
FastlyStatus xqd_resp_header_remove(ResponseHandle resp_handle, const char *name, size_t name_len);

WASM_IMPORT("fastly_http_resp", "version_get")
FastlyStatus xqd_resp_version_get(ResponseHandle resp_handle, uint32_t *version_out);

WASM_IMPORT("fastly_http_resp", "version_set")
FastlyStatus xqd_resp_version_set(ResponseHandle resp_handle, uint32_t version);

WASM_IMPORT("fastly_http_resp", "send_downstream")
FastlyStatus xqd_resp_send_downstream(ResponseHandle resp_handle, BodyHandle body_handle,
                             uint32_t streaming);

WASM_IMPORT("fastly_http_resp", "status_get")
FastlyStatus xqd_resp_status_get(ResponseHandle resp_handle, uint16_t *status_out);

WASM_IMPORT("fastly_http_resp", "status_set")
FastlyStatus xqd_resp_status_set(ResponseHandle resp_handle, uint16_t status);

// Module fastly_dictionary
WASM_IMPORT("fastly_dictionary", "open")
FastlyStatus xqd_dictionary_open(const char *name, size_t name_len, DictionaryHandle *dict_handle_out);

WASM_IMPORT("fastly_dictionary", "get")
FastlyStatus xqd_dictionary_get(DictionaryHandle dict_handle, const char *key, size_t key_len, char *value,
                       size_t value_max_len, size_t *nwritten);

WASM_IMPORT("fastly_dictionary", "open")
FastlyStatus xqd_config_store_open(const char *name, size_t name_len, ConfigStoreHandle *dict_handle_out);

WASM_IMPORT("fastly_dictionary", "get")
FastlyStatus xqd_config_store_get(ConfigStoreHandle dict_handle, const char *key, size_t key_len,
                         char *value, size_t value_max_len, size_t *nwritten);

// Module fastly_object_store
WASM_IMPORT("fastly_object_store", "open")
int fastly_object_store_open(const char *name, size_t name_len,
                             ObjectStoreHandle *object_store_handle_out);
WASM_IMPORT("fastly_object_store", "lookup")
int fastly_object_store_get(ObjectStoreHandle object_store_handle, const char *key, size_t key_len,
                            BodyHandle *opt_body_handle_out);
WASM_IMPORT("fastly_object_store", "insert")
int fastly_object_store_insert(ObjectStoreHandle object_store_handle, const char *key,
                               size_t key_len, BodyHandle body_handle);

WASM_IMPORT("fastly_geo", "lookup")
FastlyStatus xqd_geo_lookup(const char *addr_octets, size_t addr_len, char *buf, size_t buf_len,
                   size_t *nwritten);

WASM_IMPORT("wasi_snapshot_preview1", "random_get")
int32_t random_get(int32_t arg0, int32_t arg1);

#ifdef __cplusplus
}
#endif
#endif
