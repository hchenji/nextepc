//
// Created by cjh on 2/7/19.
//

#ifndef HTTPC_CONSUL_HTTP_H
#define HTTPC_CONSUL_HTTP_H

#include <sys/types.h>
#include <curl/curl.h>

typedef struct {
	char *uri;
	char *dbname;
	char url[1024];

} consul_client_t;

struct _consul_kv_t {
  char key[1024];
  char val[1024];
  struct _consul_kv_t *next;
};

typedef struct _consul_kv_t consul_kv_t;

struct _consul_buf_t {
  char buf[20480];
  size_t curr_ptr;
};

typedef struct _consul_buf_t consul_buf_t;


//	prototypes

size_t write_data(char *buffer, size_t size, size_t nmemb, void *userp);

CURLcode curl_get(char *url, consul_buf_t *buf);

CURLcode curl_put(char *url, char *data, consul_buf_t *response);

void print_substring(const char *str, int skip, int size);

char* consul_get(consul_client_t* cli, char* key);

void consul_put(consul_client_t* cli, char* key, char* val);

consul_kv_t* consul_get_all(consul_client_t* cli);

consul_client_t* consul_client_init(char* uri, char* dbname);

consul_kv_t* consul_get_recurse(consul_client_t* cli, const char* key);

void consul_free_list(consul_kv_t* head);

unsigned char* json_int_arr_to_native(char *val);

#endif //HTTPC_CONSUL_HTTP_H
