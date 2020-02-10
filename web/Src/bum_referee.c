#include <bum_referee.h>
#include <xbee.h>

#include <bum_private.h>

#include <stdio.h>
#include <string.h>

// **********************************************************************************************
// **********************************************************************************************
// **********************************************************************************************
// **********************************************************************************************
// ----------------------------------------------------------------------------------------------
// REFEREE

typedef struct
{
  // 64 bit factory assigned unique id
  uint64_t               unit_addr;

  BumperProtocolReferee *referee;
} BumperStateReferee;

static BumperStateReferee bumper_state;

typedef struct
{
  uint64_t addr;
  uint8_t  id;

  char name[ PLAYER_NAME_SIZE + 1 ];

  // Game data
  // Current position
  double Px, Py;
  // Current speed
  double Vx, Vy;

  // Current scaling, in 1/1000
  uint16_t s;

  // Current ball size
  uint16_t r;

  // Current acceleration (-100 to 100)
  int8_t ax, ay, az;

  // Color
  uint32_t color;

  // 1 if the player is living in the game
  uint8_t playing;

} PlayerData;

#define MAX_PLAYERS 20
// Number of start locations
#define N_STARTING_BOXES 25


typedef struct
{
  PlayerData player_data[ MAX_PLAYERS ];
  int n_players;

  uint8_t start_location[ N_STARTING_BOXES ];

  int game_started;
} RefereeState;

RefereeState referee_state;

// Find a random starting location
void bumper_start_location_calc( double *x, double *y )
{
  uint32_t random;
  bumper_state.referee->utility_random( &random, N_STARTING_BOXES - referee_state.n_players );

  int i = random;
  //i = 0; // TODEL
  while ( referee_state.start_location[ i ] )
  {
    i++;
    if ( i == N_STARTING_BOXES ) i = 0;
  }


  referee_state.start_location[ i ] = 1;

  int X = i / 5;
  int Y = i % 5;

  *x = ( X * ( BOARD_W / 5 ) ) + ( BOARD_W / 10 );
  *y = ( Y * ( BOARD_H / 5 ) ) + ( BOARD_H / 10 );
}

static uint32_t bum_colors[ MAX_PLAYERS ] =
{
  0x00FF0000,
  0x0033FF00,
  0x003333CC,
  0x00FFFF00,
  0x0099CC00,

  0x00CCFF33,
  0x00CCFFCC,
  0x00FFCC00,
  0x00DEB887,
  0x009932CC,

  0x00FFFFF0,
  0x00FF00FF,
  0x0000FA9A,
  0x00808000,
  0x009ACD32,

  0x004682B4,
  0x00FFC0CB,
  0x0048D1CC,
  0x00E6E6FA,
  0x00FFF8DC,

};

static void bum_api_send_to( uint64_t to, uint8_t *data, uint16_t len )
{
	if ( to == BUM_TO_ALL )
	{
		for ( int i = 0 ; i < referee_state.n_players ; i++ )
		{
			xbee_api_send_to( referee_state.player_data[ i ].addr, data, len );
		}
	}
	else if ( to == BUM_TO_BROADCAST )
	{
		xbee_api_send_to( XBEE_BROADCAST, data, len );
	}
	else
	{
		xbee_api_send_to( to, data, len );
	}
}

PlayerData *find_player_id( uint8_t id )
{
  for ( int i = 0 ; i < referee_state.n_players ; i++ )
  {
	  if ( referee_state.player_data[ i ].id == id ) return referee_state.player_data + i;
  }
  return 0;
}
PlayerData *find_player_addr( uint64_t addr )
{
  for ( int i = 0 ; i < referee_state.n_players ; i++ )
  {
	  if ( referee_state.player_data[ i ].addr == addr ) return referee_state.player_data + i;
  }
  return 0;
}

PlayerData *add_player( uint64_t addr, const char *name )
{
  PlayerData *d = referee_state.player_data + referee_state.n_players;

  d->addr = addr;
  d->id = referee_state.n_players;

  strncpy( d->name, name, PLAYER_NAME_SIZE );
  d->name[ PLAYER_NAME_SIZE ] = 0;

  bumper_start_location_calc( &d->Px, &d->Py );
  d->r = BUM_DEFAULT_BALL_SIZE;

  d->s = 1000; // No scaling

  d->color = bum_colors[ referee_state.n_players ];

  // Current speed
  d->Vx = 0;
  d->Vy = 0;

  // Current acceleration (-100 to 100)
  d->ax = 0;
  d->ay = 0;
  d->az = 0;

  // 1 if the player is living in the game
  d->playing = 1;


  referee_state.n_players++;

  return d;
}

void bums_referee_reset()
{
  referee_state.game_started = 0;
  referee_state.n_players = 0;

  for ( int i = 0 ; i < N_STARTING_BOXES ; i++ )
  {
	referee_state.start_location[ i ] = 0;
  }
}

void bum_init_referee( BumperProtocolReferee *p )
{
  xbee_init( 1, p->error );

  // Not used
  //bumper_state.unit_addr = xbee_api_read_unique_id();

  bumper_state.referee = p;

  xbee_log( "Starting Referee<br/>");

  bums_referee_reset();
}

int bum_game_step( uint64_t to, uint8_t step, uint8_t param )
{
	uint8_t frame[ BUM_GAME_STEP_LEN ];
	frame[ 0 ] = BUM_GAME_STEP_TYPE;
	frame[ 1 ] = step;
	frame[ 2 ] = param;

	bum_api_send_to( to, frame, BUM_GAME_STEP_LEN );
	return 1;
}

int bum_game_new_player( uint64_t to, uint8_t id, const char *name, uint32_t color )
{
	int i;
	uint8_t frame[ BUM_GAME_NEWPLAYER_LEN ];
	frame[ 0 ] = BUM_GAME_NEWPLAYER_TYPE;
	frame[ 1 ] = id;

	frame[ 2 ] = ( color >> 24 ) & 0xFF;
	frame[ 3 ] = ( color >> 16 ) & 0xFF;
	frame[ 4 ] = ( color >> 8 ) & 0xFF;
	frame[ 5 ] = ( color ) & 0xFF;

	for ( i = 0 ; ( i < PLAYER_NAME_SIZE ) && name[ i ] ; i++ ) frame[ i + 6 ] = name[ i ];
	for ( ; ( i < PLAYER_NAME_SIZE + 1 ) ; i++ ) frame[ i + 6 ] = 0;

	bum_api_send_to( to, frame, BUM_GAME_NEWPLAYER_LEN );
	return 1;
}

int bum_game_player_move( uint64_t to, uint8_t id, uint16_t x, uint16_t y, uint16_t s )
{
	uint8_t frame[ BUM_GAME_PLAYERMOVE_LEN ];
	frame[ 0 ] = BUM_GAME_PLAYERMOVE_TYPE;
	frame[ 1 ] = id;
	frame[ 2 ] = ( x >> 8 ) & 0xFF;
	frame[ 3 ] = ( x ) & 0xFF;
	frame[ 4 ] = ( y >> 8 ) & 0xFF;
	frame[ 5 ] = ( y ) & 0xFF;
	frame[ 6 ] = ( s >> 8 ) & 0xFF;
	frame[ 7 ] = ( s ) & 0xFF;



	bum_api_send_to( to, frame, BUM_GAME_PLAYERMOVE_LEN );
	return 1;
}

int bum_game_print( uint64_t to, const char *msg )
{
	int i;
	uint8_t frame[ BUM_GAME_PRINT_LEN ];
	frame[ 0 ] = BUM_GAME_PRINT_TYPE;
	for ( i = 0 ; ( i < BUM_MSG_SIZE ) && msg[ i ] ; i++ ) frame[ i + 1 ] = msg[ i ];
	for ( ; ( i < BUM_MSG_SIZE + 1 ) ; i++ ) frame[ i + 1 ] = 0;
	bum_api_send_to( to, frame, BUM_GAME_PRINT_LEN );
	return 1;
}


#define DATA_SIZE 100


void bum_process_referee()
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
			  case BUM_GAME_REGISTER_TYPE:
			  {
				if ( len != BUM_GAME_REGISTER_LEN ) return;

				uint8_t result = 0;
				PlayerData *d = 0;

				if ( referee_state.game_started )
				{ result = 4; }
				else if ( referee_state.n_players == MAX_PLAYERS )
				{ result = 3; }
				else
				{
				  // We update internal list of players
				  d = find_player_addr( from );
				  if ( d )
				  { result = 2; }
				  else
				  {
					d = add_player( from, ( const char * )data + 1 );
					if ( d ) { result = 1; }
				  }
				}
				// We ack the registering
				bum_game_step( from, BUM_STEP_REGISTERED, result );

				if ( result == 1 )
				{
					int i;
					bum_game_print( from, "WAIT..." );

#if 1
					for ( i = 0 ; i < referee_state.n_players ; i++ )
					{
						// We send to the new player the list of players
						bum_game_new_player( from, referee_state.player_data[ i ].id, referee_state.player_data[ i ].name, referee_state.player_data[ i ].color );
						bum_game_player_move( from, referee_state.player_data[ i ].id, ( uint16_t )referee_state.player_data[ i ].Px, ( uint16_t )referee_state.player_data[ i ].Py, referee_state.player_data[ i ].s );
					}

					for ( i = 0 ; i < referee_state.n_players - 1 ; i++ )
					{
						// We send to all other players the new player
						bum_game_new_player( referee_state.player_data[ i ].addr, d->id, d->name, d->color );
						bum_game_player_move( referee_state.player_data[ i ].addr, d->id, ( uint16_t )d->Px, ( uint16_t )d->Py, d->s );
					}
#endif

					// Signal upper layer
					bumper_state.referee->game_register( d->id, d->name );
				}

				break;
			  }

			  case BUM_GAME_ACCELERATION_TYPE:
			  {
				  if ( len != BUM_GAME_ACCELERATION_LEN ) return;

				  PlayerData *d = find_player_addr( from );
				  if ( d )
				  {
					// Save these new acceleration
					d->ax = data[ 1 ] - 100;
					d->ay = data[ 2 ] - 100;
					d->az = data[ 3 ] - 100;

					// Signal upper layer
					bumper_state.referee->game_acceleration( d->id, d->ax, d->ay, d->az );
				  }

				  break;
			  }

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

void bum_referee_calculate( uint16_t dt )
{
  if ( !referee_state.game_started ) return;

  int i, j;
  PlayerData *d;

  int n_playing = 0;
  for ( i = 0 ; i < referee_state.n_players ; i++ )
  {
	d = referee_state.player_data + i;

	if ( d->playing ) n_playing++;
  }

  for ( i = 0 ; i < referee_state.n_players ; i++ )
  {
	d = referee_state.player_data + i;

	if ( !d->playing ) continue;

	const double Vfactor = 0.00020;
	const double Pfactor = 0.00075;

	double sdt = ( ( double )dt );

	// First re-calculate the new position
	d->Vx += ( ( double )d->ax ) * sdt * Vfactor;
	d->Vy += ( ( double )d->ay ) * sdt * Vfactor;

	d->Px += d->Vx * sdt * Pfactor;
	d->Py += d->Vy * sdt * Pfactor;

	// Check if the new position is outside the gaming board
	if ( ( d->Px < 0 ) || ( d->Px > BOARD_W ) || ( d->Py < 0 ) || ( d->Py > BOARD_H ) )
	{
		// This player lost
		d->playing = 0;
		d->Px = 0; // - d->r / 2;
		d->Py = 50 + ( n_playing * d->r );
		d->r /= 2;
		d->s = 400;
        bum_game_player_move( BUM_TO_ALL, d->id, ( uint16_t )d->Px, ( uint16_t )d->Py, d->s );
	}
	else
	{
		// Calculate if this player hit another player
		  for ( j = 0 ; j < referee_state.n_players ; j++ )
		  {
			  if ( i == j ) continue;
			  PlayerData *e = referee_state.player_data + j;
  			  if ( !e->playing ) continue;

  			  // Calculate distance between the 2 players
  			  int16_t dx = d->Px - e->Px;
  			  int16_t dy = d->Py - e->Py;
  			  int32_t D = dx * dx + dy * dy;
  			  // Radius
  			  int32_t DR = d->r + e->r;
  			  DR = DR * DR;
  			  if ( D <= DR )
  			  {
  				  if ( D == 0 ) D = 1;
  				  // Hit !
  				  // Now we need to calculate new speeds for both players
#define a1 dx
#define b1 dy
#define xi1 d->Vx
#define yi1 d->Vy
#define xi2 e->Vx
#define yi2 e->Vy

  				double x1 = (yi2*b1*a1 - yi1*b1*a1 + b1*b1*xi1 + a1*a1*xi2)/(D);
  				double y1 = (yi2*b1*b1 + b1*xi2*a1 - a1*xi1*b1 + a1*a1*yi1)/(D);
  				double x2 = (a1*a1*xi1 - yi2*b1*a1 + yi1*b1*a1 + b1*b1*xi2)/(D);
  				double y2 = (a1*xi1*b1 + yi1*b1*b1 - b1*xi2*a1 + a1*a1*yi2)/(D);

				d->Vx = x1;
				d->Vy = y1;
				e->Vx = x2;
				e->Vy = y2;
  			  }
		  }

	      bum_game_player_move( BUM_TO_ALL, d->id, ( uint16_t )d->Px, ( uint16_t )d->Py, d->s );
	}

  }


  if ( n_playing == 0 )
  {
	  // No winner
	  bum_game_print( BUM_TO_ALL, "No winner..." );
	  bum_game_step( BUM_TO_ALL, BUM_STEP_END, 0 );
	  bumper_state.referee->game_over();

      bums_referee_reset();
  }
  else  if ( n_playing == 1 )
  {
#if 1
	  d = 0;
	  // We have a winner
	  for ( i = 0 ; i < referee_state.n_players ; i++ )
	  {
		if ( referee_state.player_data[ i ].playing )
		{
		  d = referee_state.player_data + i;
		  break;
		}
	  }

	if ( d )
	{ bum_game_step( BUM_TO_ALL, BUM_STEP_RESULT, d->id ); }

	char buffer[ BUM_MSG_SIZE ];
	sprintf( buffer, "The winner is %s", d->name );
	bum_game_print( BUM_TO_ALL, buffer );

	bum_game_step( BUM_TO_ALL, BUM_STEP_END, 0 );
	bumper_state.referee->game_over();

    bums_referee_reset();
#endif
  }

}

void bum_referee_start_game()
{
  if ( referee_state.game_started ) return;

  referee_state.game_started = 1;

  bum_game_print( BUM_TO_ALL, "START !" );

  bum_game_step( BUM_TO_ALL, BUM_STEP_START, 0 );
}

int bum_referee_get_n_players()
{
	return referee_state.n_players;
}

