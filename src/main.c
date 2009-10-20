#define DDEBUG 0

#include "ddebug.h"
#include "handler.h"
#include "filter.h"

#include <nginx.h>
#include <ngx_config.h>
#include <ngx_log.h>

/* config init handler */
static void* ngx_http_echo_create_conf(ngx_conf_t *cf);

/* config directive handlers */
static char* ngx_http_echo_echo(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static char* ngx_http_echo_echo_client_request_headers(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


static char* ngx_http_echo_echo_sleep(ngx_conf_t *cf, ngx_command_t *cmd,
        void *conf);

static char* ngx_http_echo_echo_flush(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static char* ngx_http_echo_echo_blocking_sleep(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf);

static char* ngx_http_echo_echo_reset_timer(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf);

static char* ngx_http_echo_echo_before_body(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf);

static char* ngx_http_echo_echo_after_body(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf);

static char* ngx_http_echo_echo_location_async(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf);

/*
 * TODO
static char* ngx_http_echo_echo_location(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf);
*/

static char* ngx_http_echo_echo_duplicate(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf);

static char* ngx_http_echo_helper(ngx_http_echo_opcode_t opcode,
        ngx_http_echo_cmd_category_t cat,
        ngx_conf_t *cf, ngx_command_t *cmd, void* conf);

static ngx_http_module_t ngx_http_echo_module_ctx = {
    /* TODO we could add our own variables here... */
    ngx_http_echo_handler_init,                 /* preconfiguration */
    ngx_http_echo_filter_init,                  /* postconfiguration */

    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */

    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */

    ngx_http_echo_create_conf, /* create location configuration */
    NULL                           /* merge location configuration */
};

static ngx_command_t  ngx_http_echo_commands[] = {

    { ngx_string("echo"),
      NGX_HTTP_LOC_CONF|NGX_CONF_ANY,
      ngx_http_echo_echo,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_echo_loc_conf_t, handler_cmds),
      NULL },
    /* TODO echo_client_request_headers should take an
     * optional argument to change output format to
     * "html" or other things */
    { ngx_string("echo_client_request_headers"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_echo_echo_client_request_headers,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_echo_loc_conf_t, handler_cmds),
      NULL },
    { ngx_string("echo_sleep"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_echo_echo_sleep,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_echo_loc_conf_t, handler_cmds),
      NULL },
    { ngx_string("echo_flush"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_echo_echo_flush,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_echo_loc_conf_t, handler_cmds),
      NULL },
    { ngx_string("echo_blocking_sleep"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_echo_echo_blocking_sleep,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_echo_loc_conf_t, handler_cmds),
      NULL },
    { ngx_string("echo_reset_timer"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_echo_echo_reset_timer,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_echo_loc_conf_t, handler_cmds),
      NULL },
    { ngx_string("echo_before_body"),
      NGX_HTTP_LOC_CONF|NGX_CONF_ANY,
      ngx_http_echo_echo_before_body,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_echo_loc_conf_t, before_body_cmds),
      NULL },
    { ngx_string("echo_after_body"),
      NGX_HTTP_LOC_CONF|NGX_CONF_ANY,
      ngx_http_echo_echo_after_body,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_echo_loc_conf_t, after_body_cmds),
      NULL },
    { ngx_string("echo_location_async"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
      ngx_http_echo_echo_location_async,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    /* TODO
    { ngx_string("echo_location"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_echo_echo_location,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },
    */

    { ngx_string("echo_duplicate"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_http_echo_echo_duplicate,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};

ngx_module_t ngx_http_echo_module = {
    NGX_MODULE_V1,
    &ngx_http_echo_module_ctx, /* module context */
    ngx_http_echo_commands,   /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};

static void*
ngx_http_echo_create_conf(ngx_conf_t *cf) {
    ngx_http_echo_loc_conf_t *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_echo_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    return conf;
}

static char*
ngx_http_echo_helper(ngx_http_echo_opcode_t opcode,
        ngx_http_echo_cmd_category_t cat,
        ngx_conf_t *cf, ngx_command_t *cmd, void* conf) {
    ngx_http_core_loc_conf_t        *clcf;
    /* ngx_http_echo_loc_conf_t        *ulcf = conf; */
    ngx_array_t                     **args_ptr;
    ngx_http_script_compile_t       sc;
    ngx_str_t                       *raw_args;
    ngx_http_echo_arg_template_t    *arg;
    ngx_array_t                     **cmds_ptr;
    ngx_http_echo_cmd_t             *echo_cmd;
    ngx_uint_t                       i, n;

    /* cmds_ptr points to ngx_http_echo_loc_conf_t's
     * handler_cmds, before_body_cmds, or after_body_cmds
     * array, depending on the actual offset */
    cmds_ptr = (ngx_array_t**)(((u_char*)conf) + cmd->offset);
    if (*cmds_ptr == NULL) {
        *cmds_ptr = ngx_array_create(cf->pool, 1,
                sizeof(ngx_http_echo_cmd_t));
        if (*cmds_ptr == NULL) {
            return NGX_CONF_ERROR;
        }
        if (cat == echo_handler_cmd) {
            DD("registering the content handler");
            /* register the content handler */
            clcf = ngx_http_conf_get_module_loc_conf(cf,
                    ngx_http_core_module);
            if (clcf == NULL) {
                return NGX_CONF_ERROR;
            }
            DD("registering the content handler (2)");
            clcf->handler = ngx_http_echo_handler;
        } else {
            DD("filter used = 1");
            ngx_http_echo_filter_used = 1;
        }
    }
    echo_cmd = ngx_array_push(*cmds_ptr);
    if (echo_cmd == NULL) {
        return NGX_CONF_ERROR;
    }
    echo_cmd->opcode = opcode;
    args_ptr = &echo_cmd->args;
    *args_ptr = ngx_array_create(cf->pool, 1,
            sizeof(ngx_http_echo_arg_template_t));
    if (*args_ptr == NULL) {
        return NGX_CONF_ERROR;
    }
    raw_args = cf->args->elts;
    /* we skip the first arg and start from the second */
    for (i = 1 ; i < cf->args->nelts; i++) {
        arg = ngx_array_push(*args_ptr);
        if (arg == NULL) {
            return NGX_CONF_ERROR;
        }
        arg->raw_value = raw_args[i];
        DD("found raw arg %s", raw_args[i].data);
        arg->lengths = NULL;
        arg->values  = NULL;
        n = ngx_http_script_variables_count(&arg->raw_value);
        if (n > 0) {
            ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));
            sc.cf = cf;
            sc.source = &arg->raw_value;
            sc.lengths = &arg->lengths;
            sc.values = &arg->values;
            sc.variables = n;
            sc.complete_lengths = 1;
            sc.complete_values = 1;
            if (ngx_http_script_compile(&sc) != NGX_OK) {
                return NGX_CONF_ERROR;
            }
        }
    } /* end for */
    return NGX_CONF_OK;
}

static char*
ngx_http_echo_echo(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    DD("in echo_echo...");
    return ngx_http_echo_helper(echo_opcode_echo,
            echo_handler_cmd,
            cf, cmd, conf);
}

static char*
ngx_http_echo_echo_client_request_headers(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf) {
    return ngx_http_echo_helper(
            echo_opcode_echo_client_request_headers,
            echo_handler_cmd,
            cf, cmd, conf);
}

static char*
ngx_http_echo_echo_sleep(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    DD("in echo_sleep...");
    return ngx_http_echo_helper(echo_opcode_echo_sleep,
            echo_handler_cmd,
            cf, cmd, conf);
}

static char*
ngx_http_echo_echo_flush(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    DD("in echo_flush...");
    return ngx_http_echo_helper(echo_opcode_echo_flush,
            echo_handler_cmd,
            cf, cmd, conf);
}

static char*
ngx_http_echo_echo_blocking_sleep(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    DD("in echo_blocking_sleep...");
    return ngx_http_echo_helper(echo_opcode_echo_blocking_sleep,
            echo_handler_cmd,
            cf, cmd, conf);
}

static char*
ngx_http_echo_echo_reset_timer(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf) {
    return ngx_http_echo_helper(echo_opcode_echo_reset_timer,
            echo_handler_cmd,
            cf, cmd, conf);
}

static char*
ngx_http_echo_echo_before_body(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf) {
    DD("processing echo_before_body directive...");
    return ngx_http_echo_helper(echo_opcode_echo_before_body,
            echo_filter_cmd,
            cf, cmd, conf);
}

static char*
ngx_http_echo_echo_after_body(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf) {
    return ngx_http_echo_helper(echo_opcode_echo_after_body,
            echo_filter_cmd,
            cf, cmd, conf);
}

static char*
 ngx_http_echo_echo_location_async(ngx_conf_t *cf, ngx_command_t *cmd,
         void *conf) {

#if ! defined(nginx_version)

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
            "Directive echo_location_async does not work with nginx "
            "versions ealier than 0.7.46.");

    return NGX_CONF_ERROR;

#else

    return ngx_http_echo_helper(echo_opcode_echo_location_async,
            echo_handler_cmd,
            cf, cmd, conf);

#endif

}

static char*
ngx_http_echo_echo_duplicate(ngx_conf_t *cf,
        ngx_command_t *cmd, void *conf) {
    return ngx_http_echo_helper(echo_opcode_echo_duplicate,
            echo_handler_cmd,
            cf, cmd, conf);
}
