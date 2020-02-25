#ifndef __BUMPER_PROTOCOL_H__
#define __BUMPER_PROTOCOL_H__

#include "main.h"
#include "event.h"

// ===========================================================================
// COMMON

// Player name size
#define PLAYER_NAME_SIZE 5

//-----------------------------------------------------------------------------
// Steps :
#define BUM_STEP_REGISTERED 0x00  // param: 0 = internal error, 1 = OK, 2 = OK but already registered, 3 = NO, too many players, 4 = NO, game already started
#define BUM_STEP_START      0x01  // param: always 0
#define BUM_STEP_RESULT     0x02  // param: id of winner
#define BUM_STEP_END        0x03  // param: always 0


// To be called when a char is received on the radio UART
void bum_notify_recv( uint8_t c );

// To be called each 10 ms
void bum_process( int ms );

void bum_notify_endtx();

void bum_log( const char *msg );


// Utility
typedef struct
{
  event evt;
  uint8_t button_register_player;
  uint8_t button_acc;
  int8_t acc_x;
  int8_t acc_y;

} WebInterface;

void web_interface_init( WebInterface *wi );

void bum_log( const char *msg );


#endif
