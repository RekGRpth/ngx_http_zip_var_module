#include <ndk.h>
#include <ngx_http.h>
#include <zlib.h>

ngx_module_t ngx_http_zip_var_module;

static ngx_int_t ngx_http_zip_var_set_zip_func(ngx_http_request_t *r, ngx_str_t *val, ngx_http_variable_value_t *v, void *data) {
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "%s", __func__);
    if (!v->len) return NGX_DECLINED;
    val->len = compressBound(v->len);
    if (!(val->data = ngx_pnalloc(r->pool, val->len + sizeof(size_t)))) { ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "!ngx_pnalloc"); return NGX_ERROR; }
    ngx_int_t *level = data;
    int level_ = level ? *level : 1;
    switch (compress2((Bytef *)val->data + sizeof(size_t), &val->len, (const Bytef *)v->data, v->len, level_)) {
        case Z_OK: break;
        case Z_MEM_ERROR: ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "compress2 == Z_MEM_ERROR"); return NGX_ERROR;
        case Z_BUF_ERROR: ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "compress2 == Z_BUF_ERROR"); return NGX_ERROR;
        case Z_STREAM_ERROR: ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "compress2 == Z_STREAM_ERROR"); return NGX_ERROR;
    }
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "%ul ~ %ul", v->len, val->len);
    *(size_t *)val->data = v->len;
    val->len += sizeof(size_t);
//    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "*(size_t *)val->data = %ul", *(size_t *)val->data);
    return NGX_OK;
}

static ngx_int_t ngx_http_zip_var_set_unzip_func(ngx_http_request_t *r, ngx_str_t *val, ngx_http_variable_value_t *v) {
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "%s", __func__);
    if (!v->len) return NGX_DECLINED;
    val->len = ((*(size_t *)v->data) << 48) >> 48;
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "%ul ~ %ul", v->len, val->len);
    if (!(val->data = ngx_pnalloc(r->pool, val->len))) { ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "!ngx_pnalloc"); return NGX_ERROR; }
    switch (uncompress((Bytef *)val->data, &val->len, (const Bytef *)v->data + sizeof(size_t), v->len - sizeof(size_t))) {
        case Z_OK: break;
        case Z_MEM_ERROR: ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "uncompress == Z_MEM_ERROR"); return NGX_ERROR;
        case Z_BUF_ERROR: ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "uncompress == Z_BUF_ERROR"); return NGX_ERROR;
        case Z_DATA_ERROR: ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "uncompress == Z_DATA_ERROR"); return NGX_ERROR;
    }
    return NGX_OK;
}

static char *ngx_http_zip_var_set_zip_conf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_int_t *level = NULL;
    ngx_str_t *elts = cf->args->elts;
    if (cf->args->nelts == 4) {
        if (!(level = ngx_palloc(cf->pool, sizeof(int)))) return "!ngx_palloc";
        if ((*level = ngx_atoi(elts[3].data, elts[3].len)) == NGX_ERROR) return "ngx_atoi == NGX_ERROR";
        if (*level < 1) return "*level < 1";
        if (*level > 9) return "*level > 9";
    }
    ndk_set_var_t filter = { NDK_SET_VAR_VALUE_DATA, ngx_http_zip_var_set_zip_func, 1, level };
    return ndk_set_var_value_core(cf, &elts[1], &elts[2], &filter);
}

static ngx_command_t ngx_http_zip_var_commands[] = {
  { .name = ngx_string("set_zip"),
    .type = NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE2|NGX_CONF_TAKE3,
    .set = ngx_http_zip_var_set_zip_conf,
    .conf = NGX_HTTP_LOC_CONF_OFFSET,
    .offset = 0,
    .post = NULL },
  { .name = ngx_string("set_unzip"),
    .type = NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_TAKE2,
    .set = ndk_set_var_value,
    .conf = NGX_HTTP_LOC_CONF_OFFSET,
    .offset = 0,
    .post = &(ndk_set_var_t){ NDK_SET_VAR_VALUE, ngx_http_zip_var_set_unzip_func, 1, NULL } },
    ngx_null_command
};

static ngx_http_module_t ngx_http_zip_var_ctx = {
    .preconfiguration = NULL,
    .postconfiguration = NULL,
    .create_main_conf = NULL,
    .init_main_conf = NULL,
    .create_srv_conf = NULL,
    .merge_srv_conf = NULL,
    .create_loc_conf = NULL,
    .merge_loc_conf = NULL
};

ngx_module_t ngx_http_zip_var_module = {
    NGX_MODULE_V1,
    .ctx = &ngx_http_zip_var_ctx,
    .commands = ngx_http_zip_var_commands,
    .type = NGX_HTTP_MODULE,
    .init_master = NULL,
    .init_module = NULL,
    .init_process = NULL,
    .init_thread = NULL,
    .exit_thread = NULL,
    .exit_process = NULL,
    .exit_master = NULL,
    NGX_MODULE_V1_PADDING
};
