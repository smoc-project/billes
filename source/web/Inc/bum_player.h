#ifndef __BUM_PLAYER_H__
#define __BUM_PLAYER_H__

#include "bum_common.h"
//-----------------------------------------------------------------------------

// ===========================================================================
// PLAYER PART

typedef struct
{
  // Callbacks: called by library on events from referee

  // Signals a game step
  int (*game_step)( uint8_t step, uint8_t param );

  // Signals a new player
  int (*game_new_player)( uint8_t id, const char *name, uint32_t color );

  // Signals a move from a player
  int (*game_player_move)( uint8_t id, uint16_t x, uint16_t y, uint16_t s );

  // Signals a message to be displayed
  int (*game_print)( const char *msg );

  // Signals an internal error
  void (*error)( int x );
  // Signals debug messages
  void (*debug)( int x );

  char name[ PLAYER_NAME_SIZE + 1 ];

} BumperProtocolPlayer;

// Initialize the player library
void bum_init_player( BumperProtocolPlayer *p );

// Call this function to register to a game
// name : only 5 first chars are taken into account
int bum_game_register( const char *name );

// Call this function to signal a new acceleration.
// Maximum rate: 5 Hz
// ax, ay, az: acceleration between -100 to 100
// ax: negative = toward left
// ay: negative = toward top
// az: negative = toward floor
int bum_game_acceleration( int8_t ax, int8_t ay, int8_t az );

//-----------------------------------------------------------------------------
// To be called in an infinite loop as fast as possible
void bum_process_player();

// ===========================================================================

#endif
