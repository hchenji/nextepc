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

int client_init(consul_client_t *cli, char *uri, char *dbname);

char* consul_get(consul_client_t *cli, char *key);
char* consul_put(consul_client_t *cli, char *key, char *val);
char* consul_get_all(consul_client_t *cli);

#endif //HTTPC_CONSUL_HTTP_H
