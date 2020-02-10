#include <bum_common.h>
#include <xbee.h>

#include <bum_private.h>

void bum_notify_recv( uint8_t c )
{
  xbee_notify_recv( c );
}

void bum_process( int ms )
{
	xbee_process( ms );
}

void bum_notify_endtx()
{
	xbee_notify_endtx();
}

void web_interface_init( WebInterface *wi )
{
  event_init( &wi->evt );
  wi->button_register_player = 0;
  wi->button_acc = 0;
  wi->acc_x = 0;
  wi->acc_y = 0;
}

void bum_log( const char *msg )
{
	xbee_log( msg );
}
