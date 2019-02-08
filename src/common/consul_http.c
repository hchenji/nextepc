//
// Created by cjh on 2/7/19.
//

#include "consul_http.h"
#include <stdbool.h>
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "jsmn.h"
#include "base64.h"

size_t write_data(char *buffer, size_t size, size_t nmemb, void *userp) {

	//	userp is the actual buf. copy buffer into userp
	char* buf = (char *) userp;

	size_t total = size * nmemb;
//	printf("total %zu\n", total);
//	printf("last char %x\n", buffer[total]);
	memcpy(buf, buffer, total);
	buf[total] = '\0';
//	puts(buf);

	return total;

}

CURLcode curl_get(char *url, char *buf) {
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();

	if (!curl)
		return CURLE_FAILED_INIT;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		puts("http error occurred");
	}

	/* always cleanup */
	curl_easy_cleanup(curl);

	return res;
}

CURLcode curl_put(char *url, char *data, char *response) {
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();

	if (!curl)
		return CURLE_FAILED_INIT;

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		puts("http error occurred");
	}

	/* always cleanup */
	curl_easy_cleanup(curl);

	return res;
}

void print_substring(const char *str, int skip, int size)
{
	assert(skip >= 0);
	assert(size >= 0);
	printf("%.*s", size, str + skip);
}


char* consul_get(consul_client_t* cli, char* key)
{

	char buf[1024];
	char url[1024];

	strcpy(url, cli->url);
	strcat(url, key);
	curl_get(url, buf);

//	puts(buf);

	jsmn_parser parser;
	jsmntok_t tokens[256];
//	const char *js;
	int r;

	jsmn_init(&parser);
	r = jsmn_parse(&parser, buf, strlen(buf), tokens, 256);

//	printf("%d tokens parsed \n", r);

//	for (int i = 0; i<r; ++i) {
//		jsmntok_t *t = &tokens[i];

//		printf("tok %d type %d size %d start %d end %d value...", i, t->type, t->size, t->start, t->end);
//		printf("%.*s\n", (t->end - t->start), buf + t->start);
//	}

	jsmntok_t *valtok = NULL;

	//	TODO: iterate within the object in the json array until you hit token with key "Key"
	for (int i = 0; i < r; ++i) {
		jsmntok_t *t = &tokens[i];
		if (t->type != JSMN_STRING)
			continue;
//		printf("%d\n", i);
		if (!memcmp(buf + t->start, "Value", 5)) {
			valtok = &tokens[i+1];
//			printf("%d\n", i+1);
			break;
		}
	}

	char* decd = malloc(sizeof(char) * 64);
	int len = Base64decode(decd, buf + valtok->start);
	decd[len] = 0;
//	printf("decoded key value is %.*s : %s\n", (keytok->end - keytok->start), buf + keytok->start, decd);

	return decd;
}

char* consul_put(consul_client_t* cli, char* key, char* val)
{
	char *response = malloc (1024);

	char url[1024];
	strcpy(url, cli->url);
	strcat(url, key);

	curl_put(url, val, response);

	return response;
}
char* consul_get_all(consul_client_t* cli)
{
	char buf[1024];
	char url[1024];

	strcpy(url, cli->url);
	strcat(url, "?recurse=");
	curl_get(url, buf);

	return NULL;
}
int client_init(consul_client_t* cli, char* uri, char* dbname)
{
	cli->uri = uri;
	cli->dbname = dbname;
	strcpy(cli->url, cli->uri);
	strcat(cli->url, "/v1/kv/");
	strcat(cli->url, cli->dbname);
	strcat(cli->url, "/");

	return 0;
}
