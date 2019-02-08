//
// Created by cjh on 2/7/19.
//

#ifndef HTTPC_CONSUL_HTTP_H
#define HTTPC_CONSUL_HTTP_H

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

int consul_client_init(consul_client_t *cli, char *uri, char *dbname);

char* consul_get(consul_client_t *cli, char *key);
char* consul_put(consul_client_t *cli, char *key, char *val);
consul_kv_t* consul_get_all(consul_client_t *cli);
consul_kv_t* consul_get_recurse(consul_client_t *cli, char *key);
unsigned char* json_int_arr_to_native(char *val);
void consul_free_list(consul_kv_t* head);

#endif //HTTPC_CONSUL_HTTP_H
