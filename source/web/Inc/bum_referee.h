#ifndef __BUM_REFEREE_H__
#define __BUM_REFEREE_H__

#include "bum_common.h"
//-----------------------------------------------------------------------------

// ===========================================================================
// REFEREE PART
typedef struct
{
#if 1
  int (*game_register)( uint8_t id, const char *name );
  int (*game_acceleration)( uint8_t id, int8_t ax, int8_t ay, int8_t az );

  int (*game_over)();

  int (*utility_random)( uint32_t *value, uint32_t max );
#endif

  void (*error)( int x );
  void (*debug)( int x );

} BumperProtocolReferee;

void bum_init_referee( BumperProtocolReferee *p );

#define BUM_TO_BROADCAST 0x000000000000FFFF
#define BUM_TO_ALL       0xFFFFFFFFFFFFFFFF

int bum_game_step( uint64_t to, uint8_t step, uint8_t param );

int bum_game_new_player( uint64_t to, uint8_t id, const char *name, uint32_t color );

int bum_game_player_move( uint64_t to, uint8_t id, uint16_t x, uint16_t y, uint16_t s );

int bum_game_print( uint64_t to, const char *msg );

// ---------------------------------------------------------------------------
// REFEREE Utilities

// Calculate new position of playing players, and notify this to all players
// dt: time between 2 calls
void bum_referee_calculate( uint16_t dt );

void bum_referee_start_game();

int  bum_referee_get_n_players();

//-----------------------------------------------------------------------------
// To be called in an infinite loop as fast as possible
void bum_process_referee();


// ===========================================================================

#endif
