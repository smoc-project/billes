#include <bum_player.h>
#include <xbee.h>

#include <bum_private.h>

#include <stdio.h>
#include <string.h>

// **********************************************************************************************
// **********************************************************************************************
// **********************************************************************************************
// **********************************************************************************************
// ----------------------------------------------------------------------------------------------
// PLAYER
typedef struct
{
  // 64 bit factory assigned unique id
  uint64_t               unit_addr;

  BumperProtocolPlayer  *player;

  uint64_t               referee_addr;
} BumperStatePlayer;

static BumperStatePlayer bumper_state;


void bum_init_player( BumperProtocolPlayer *p )
{
  xbee_init( 0, p->error );

  // Not used
  //bumper_state.unit_addr = xbee_api_read_unique_id();

  // At startup, we don't know the referee address, so let's broadcast by default
  bumper_state.referee_addr = XBEE_BROADCAST;

  bumper_state.player = p;

  //strcat( ( char * )radio_log, "Starting Player<br/>");
}


// name : only 5 first chars are taken into account
int bum_game_register( const char *name )
{
	int i;
	// We send a broadcast to try to find a referee

	uint8_t frame[ BUM_GAME_REGISTER_LEN ];
	frame[ 0 ] = BUM_GAME_REGISTER_TYPE;
	for ( i = 0 ; ( i < PLAYER_NAME_SIZE ) && name[ i ] ; i++ ) frame[ i + 1 ] = name[ i ];
	for ( ; ( i < PLAYER_NAME_SIZE + 1 ) ; i++ ) frame[ i + 1 ] = 0;

	return xbee_api_send_to( bumper_state.referee_addr, frame, BUM_GAME_REGISTER_LEN );
}


// ax, ay, az: acceleration between -100 to 100
// ax: negative = toward left
// ay: negative = toward top
// az: negative = toward floor
int bum_game_acceleration( int8_t ax, int8_t ay, int8_t az )
{
	if ( ax < -100 ) ax = -100;
	else if ( ax > 100 ) ax = 100;

	if ( ay < -100 ) ay = -100;
	else if ( ay > 100 ) ay = 100;

	if ( az < -100 ) az = -100;
	else if ( az > 100 ) az = 100;

	uint8_t frame[ BUM_GAME_ACCELERATION_LEN ];
	frame[ 0 ] = BUM_GAME_ACCELERATION_TYPE;
	frame[ 1 ] = ax + 100;
	frame[ 2 ] = ay + 100;
	frame[ 3 ] = az + 100;

	return xbee_api_send_to( bumper_state.referee_addr, frame, BUM_GAME_ACCELERATION_LEN );
}


// **********************************************************************************************
// **********************************************************************************************
// **********************************************************************************************
// **********************************************************************************************
// ----------------------------------------------------------------------------------------------
// COMMON
#define DATA_SIZE 100

#define BUM_SAVE_REFEREE_ADDR() \
	if ( bumper_state.referee_addr == XBEE_BROADCAST ) \
	{ \
		bumper_state.referee_addr = from; \
	}

void bum_process_player()
{
  static uint8_t data[ DATA_SIZE ];
  uint16_t len;

  if ( !event_check( xbee_get_recv_event() ) ) return;

  len = DATA_SIZE;
  switch ( xbee_recv_get_frame_type() )
  {
      case 0: break;

	  case 0x90:
	  {
		uint64_t from;

		if ( xbee_recv( &from, data, &len ) && ( len > 0 ) )
		{
			switch ( data[ 0 ] )
			{
			  case BUM_GAME_STEP_TYPE:
				BUM_SAVE_REFEREE_ADDR();

				if ( data[ 1 ] == BUM_STEP_REGISTERED )
				{ bumper_state.player->debug( BUM_GAME_STEP_TYPE ); }

				if ( len != BUM_GAME_STEP_LEN ) return;
				bumper_state.player->game_step( data[ 1 ], data[ 2 ] );
				break;

			  case BUM_GAME_NEWPLAYER_TYPE:
				BUM_SAVE_REFEREE_ADDR();
				if ( len != BUM_GAME_NEWPLAYER_LEN ) return;
				uint32_t color = ( data[ 2 ] << 24 ) | ( data[ 3 ] << 16 ) | ( data[ 4 ] << 8 ) | data[ 5 ];
				bumper_state.player->game_new_player( data[ 1 ], ( const char * )data + 6, color );
				break;

			  case BUM_GAME_PLAYERMOVE_TYPE:
				BUM_SAVE_REFEREE_ADDR();

				// Debug : toggle blue LED
				bumper_state.player->debug( BUM_GAME_PLAYERMOVE_TYPE );

				if ( len != BUM_GAME_PLAYERMOVE_LEN ) return;
				uint16_t x = ( ((uint16_t)data[ 2 ]) << 8 ) | data[ 3 ];
				uint16_t y = ( ((uint16_t)data[ 4 ]) << 8 ) | data[ 5 ];
				uint16_t s = ( ((uint16_t)data[ 6 ]) << 8 ) | data[ 7 ];
				bumper_state.player->game_player_move( data[ 1 ], x, y, s );
				break;

			  case BUM_GAME_PRINT_TYPE:
				BUM_SAVE_REFEREE_ADDR();
				if ( len != BUM_GAME_PRINT_LEN ) return;
				bumper_state.player->game_print( ( const char * )data + 1 );
				break;
			}
		}
	    break;
	  }

	  case 0x88:
	  case 0x8B:
	  {
		break;
	  }
	  default:
	  {
		xbee_recv_full( data, &len );
		break;
	  }
  }
}
