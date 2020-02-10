#ifndef __XBEE_H__
#define __XBEE_H__

#include "main.h"
#include "event.h"
//-----------------------------------------------------------------------------

#define XBEE_BROADCAST 0x000000000000FFFF

typedef void (*xbee_function_error)( int x );


void xbee_init( int coordinator, xbee_function_error f );

event *xbee_get_recv_event();

int  xbee_api_send_to( uint64_t to, uint8_t *data, uint16_t len );

// AT commands from API mode
int  xbee_api_AT16( uint8_t *cmd, uint16_t val );
int  xbee_api_AT8( uint8_t *cmd, uint8_t val );
int  xbee_api_ATR( uint8_t *cmd, uint8_t *resp_data, uint16_t *resp_len );

uint64_t xbee_api_read_unique_id();

// Receive functions
int     xbee_recv( uint64_t *from, uint8_t *data, uint16_t *len );
uint8_t xbee_recv_get_frame_type();
int     xbee_recv_full( uint8_t *data, uint16_t *len );
int     xbee_recv_discard();

void    xbee_notify_recv( uint8_t c );
void    xbee_notify_endtx();

// AT command from transparent mode
int  xbee_AT_check_OK();
int  xbee_AT_configure_API1();

void xbee_process( int ms );

// Debug
#define XBEE_LOG_SIZE 5000
void xbee_log( const char *msg );
const char *xbee_log_get();
int xbee_log_len();


#endif
