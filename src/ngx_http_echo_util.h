#ifndef NGX_HTTP_ECHO_UTIL_H
#define NGX_HTTP_ECHO_UTIL_H

#include "ngx_http_echo_module.h"

#define ngx_http_echo_strcmp_const(a, b) \
    ngx_strncmp(a, b, sizeof(b) - 1)

ngx_int_t ngx_http_echo_init_ctx(ngx_http_request_t *r, ngx_http_echo_ctx_t **ctx_ptr);

ngx_int_t ngx_http_echo_eval_cmd_args(ngx_http_request_t *r,
        ngx_http_echo_cmd_t *cmd, ngx_array_t *computed_args,
        ngx_array_t *opts);

ngx_int_t ngx_http_echo_send_header_if_needed(ngx_http_request_t* r,
        ngx_http_echo_ctx_t *ctx);

ngx_int_t ngx_http_echo_send_chain_link(ngx_http_request_t* r,
        ngx_http_echo_ctx_t *ctx, ngx_chain_t *cl);

ssize_t ngx_http_echo_atosz(u_char *line, size_t n);

u_char * ngx_http_echo_strlstrn(u_char *s1, u_char *last, u_char *s2, size_t n);

ngx_int_t ngx_http_echo_post_request_at_head(ngx_http_request_t *r,
        ngx_http_posted_request_t *pr);


#endif /* NGX_HTTP_ECHO_UTIL_H */

