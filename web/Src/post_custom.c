#include "lwip/apps/httpd.h"

err_t httpd_post_begin(void *connection, const char *uri, const char *http_request,
                       u16_t http_request_len, int content_len, char *response_uri,
                       u16_t response_uri_len, u8_t *post_auto_wnd)
{
	return ERR_OK;
}

err_t httpd_post_receive_data(void *connection, struct pbuf *p)
{
	return ERR_OK;
}

void httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len)
{

}
