#ifndef __BUM_PRIVATE_H__
#define __BUM_PRIVATE_H__

// Size W, H of the drawing board
#define BOARD_W 1000
#define BOARD_H 1000

#define BUM_DEFAULT_BALL_SIZE 40


// Frame definition
#define BUM_GAME_REGISTER_TYPE  0x01
#define BUM_GAME_REGISTER_LEN   ( 1 + PLAYER_NAME_SIZE + 1 )
// 0   : BUM type = 0x01
// 1-PLAYER_NAME_SIZE+1 : name

// Frame definition
#define BUM_GAME_ACCELERATION_TYPE  0x02
#define BUM_GAME_ACCELERATION_LEN   4
// 0   : BUM type = 0x02
// 1   : ax (from 0 to 200)
// 2   : ay (from 0 to 200)
// 3   : az (from 0 to 200)


// Frame definition
#define BUM_GAME_STEP_TYPE  0x81
#define BUM_GAME_STEP_LEN   3
// 0   : BUM type
// 1   : step
// 2   : param

// Frame definition
#define BUM_GAME_NEWPLAYER_TYPE  0x82
#define BUM_GAME_NEWPLAYER_LEN   ( 6 + PLAYER_NAME_SIZE + 1 )
// 0    : BUM type
// 1    : id
// 2-5  : color
// 6-   : name - 0 terminated


// Frame definition
#define BUM_GAME_PLAYERMOVE_TYPE  0x83
#define BUM_GAME_PLAYERMOVE_LEN   8
// 0    : BUM type
// 1    : id
// 2-3  : x
// 4-5  : y
// 6-7  : r

#define BUM_MSG_SIZE 25

// Frame definition
#define BUM_GAME_PRINT_TYPE  0x84
#define BUM_GAME_PRINT_LEN   ( 1 + BUM_MSG_SIZE + 1 )
// 0    : BUM type
// 1-21 : msg - 0 terminated


#endif
