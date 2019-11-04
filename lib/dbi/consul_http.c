//
// Created by cjh on 2/7/19.
//

#include "consul_http.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "jsmn.h"
#include "base64.h"
#include "ogs-core.h"

size_t write_data(char *buffer, size_t size, size_t nmemb, void *userp) {

	//	userp is my buf. copy buffer into userp
	consul_buf_t *cbuf = (consul_buf_t *) userp;
	size_t total = size * nmemb;
//	ogs_warn("total is %zu last char is %x", total, buffer[total]);
	memcpy(cbuf->buf + cbuf->curr_ptr, buffer, total);
	cbuf->curr_ptr += total;
	cbuf->buf[cbuf->curr_ptr] = '\0';
//	ogs_warn("current buf is %s", cbuf->buf);

	return total;

}

CURLcode curl_get(char *url, consul_buf_t *cbuf) {
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();

	if (!curl)
		return CURLE_FAILED_INIT;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, cbuf);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
	curl_easy_setopt(curl, CURLOPT_TCP_FASTOPEN, 1L);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		ogs_error("http error occurred for url %s buf %s\n", url, cbuf->buf);
	}

	/* always cleanup */
	curl_easy_cleanup(curl);

	return res;
}

CURLcode curl_put(char *url, char *data, consul_buf_t *response) {
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
	curl_easy_setopt(curl, CURLOPT_TCP_FASTOPEN, 1L);

	res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		ogs_error("http error occurred for url %s data %s\n", url, data);
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

	consul_buf_t *cbuf = ogs_calloc(1, sizeof(consul_buf_t));

	char url[1024];
	memset(url, 0, 1024*sizeof(char));

	strcpy(url, cli->url);
	strcat(url, key);
	curl_get(url, cbuf);

	jsmn_parser parser;
	jsmntok_t tokens[256];
	int r;

	jsmn_init(&parser);
	r = jsmn_parse(&parser, cbuf->buf, strlen(cbuf->buf), tokens, 256);

//	printf("%d tokens parsed \n", r);

//	for (int i = 0; i<r; ++i) {
//		jsmntok_t *t = &tokens[i];

//		printf("tok %d type %d size %d start %d end %d value...", i, t->type, t->size, t->start, t->end);
//		printf("%.*s\n", (t->end - t->start), buf + t->start);
//	}

	jsmntok_t *valtok = NULL;

	int i = 0;
	//	TODO: iterate within the object in the json array until you hit token with key "Key"
	for (i = 0; i < r; ++i) {
		jsmntok_t *t = &tokens[i];
		if (t->type != JSMN_STRING)
			continue;
//		printf("%d\n", i);
		if (!memcmp(cbuf->buf + t->start, "Value", 5)) {
			valtok = &tokens[i+1];
//			printf("%d\n", i+1);
			break;
		}
	}

	char* decd = malloc(sizeof(char) * 64);
	int len = Base64decode(decd, cbuf->buf + valtok->start);
	decd[len] = 0;
//	printf("decoded key value is %.*s : %s\n", (keytok->end - keytok->start), buf + keytok->start, decd);

	ogs_free(cbuf);
	return decd;
}

void consul_put(consul_client_t* cli, char* key, char* val)
{
	consul_buf_t *cbuf = ogs_calloc(1, sizeof(consul_buf_t));

	char url[1024];
	strcpy(url, cli->url);
	strcat(url, key);

	curl_put(url, val, cbuf);

	//TODO: return err code
	ogs_free(cbuf);

//	return cbuf->buf;
}

consul_kv_t* consul_get_all(consul_client_t* cli)
{
	return consul_get_recurse(cli, "");
}

consul_client_t* consul_client_init(char* uri, char* dbname)
{
	consul_client_t *cli = malloc(sizeof(consul_client_t));

	cli->uri = uri;
	cli->dbname = dbname;
	strcpy(cli->url, cli->uri);
	strcat(cli->url, "/v1/kv/");
	strcat(cli->url, cli->dbname);
	strcat(cli->url, "/");

	return cli;
}


consul_kv_t* consul_get_recurse(consul_client_t* cli, const char* key)
{
	consul_buf_t *cbuf = ogs_calloc(1, sizeof(consul_buf_t));

	char url[1024];
	memset(url, 0, 1024*sizeof(char));

	strcpy(url, cli->url);
	strcat(url, key);
	strcat(url, "?recurse=");
//	ogs_warn("url is %s\n", url);

	curl_get(url, cbuf);

//	ogs_warn("buf in get recurse answer is %s\n", cbuf->buf);

	jsmn_parser parser;
	jsmntok_t tokens[512];
	int r;

	jsmn_init(&parser);
	r = jsmn_parse(&parser, cbuf->buf, strlen(cbuf->buf), tokens, 512);

	consul_kv_t *head = NULL;
	consul_kv_t *ll = head;

	int i = 0;
	//	create and return a key-val linked list
	for (i = 0; i < r; ++i) {
		jsmntok_t *t = &tokens[i];

		//	scan until you get an object {}
		if (t->type != JSMN_OBJECT)
			continue;

		//	found a new object
		if (ll == NULL) {
			ll = malloc(sizeof(consul_kv_t));
			memset(ll, 0, sizeof(*ll));
			ll->next = NULL;
			head = ll;
		} else {
			consul_kv_t *tmp = malloc(sizeof(consul_kv_t));
			memset(tmp, 0, sizeof(*tmp));
			tmp->next = NULL;
			ll->next = tmp;
			ll = ll->next;
		}

		//	scan until you get the "key" field
		while (memcmp(cbuf->buf + t++->start, "Key", 3));

//		ogs_warn("end-start is %.*s\n", (t->end - t->start), buf + t->start);

		//	current token must be the actual key
		memcpy(ll->key, cbuf->buf + t->start, (t->end - t->start));
		ll->key[(t->end - t->start)] = 0;
//		ogs_warn("key is %s\n", ll->key);

		//	scan until you get the "value" field
		while (memcmp(cbuf->buf + t++->start, "Value", 5));

//		ogs_warn("end-start value is %.*s\n", (t->end - t->start), buf + t->start);

		//	current token must be the actual value
		int len = Base64decode(ll->val, cbuf->buf + t->start);
		(ll->val)[len] = 0;
//		ogs_warn("decoded val is %s\n", ll->val);
	}

	ogs_free(cbuf);
	return head;
}

void consul_free_list(consul_kv_t* head)
{
	consul_kv_t* tmp;

	while (head != NULL)
	{
		tmp = head;
		head = head->next;
		free(tmp);
	}

}

unsigned char* json_int_arr_to_native(char *val) {
	jsmn_parser parser;
	jsmntok_t tokens[256];
	int r;

	//	first one is size of array!
	unsigned char *ret = NULL;
	size_t j = 0;

	jsmn_init(&parser);
	r = jsmn_parse(&parser, val, strlen(val), tokens, 256);

	int i = 0;
	//	create and return a key-val linked list
	for (i = 0; i < r; ++i) {
		jsmntok_t* t = &tokens[i];
		if (t->type == JSMN_ARRAY) {
			ret = malloc(sizeof(unsigned char) * (t->size + 1));
			ret[j++] = (unsigned char) t->size;
			continue;
		}

		if (t->type == JSMN_PRIMITIVE) {
			char tmp[16];
			memcpy(tmp, val + t->start, (size_t) (t->end - t->start));
			tmp[(t->end - t->start)] = 0;
			ret[j++] = (unsigned char) strtol(tmp, NULL, 10);
//			printf("json intarr %ld\n", strtol(tmp, NULL, 10));
		}
	}

	return ret;
}
