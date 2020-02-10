#include <xbee.h>

#include <stdio.h>
#include <string.h>

#include <main.h> // DEBUG

uint8_t xbee_log_buffer[XBEE_LOG_SIZE + 1];

#define BUFFER_LEN 1000
// Incoming radio buffer
static uint8_t xbee_rx[BUFFER_LEN];
// Size of data inside the incoming buffer
static uint16_t xbee_rx_len = 0;
// Calculated expected size of next frame
static uint16_t xbee_frame_len = 0;
// Expected ACK frame type
static uint8_t xbee_ack_frame_type = 0;

#define FRAME_LEN 100
typedef struct
{
    uint8_t rx[FRAME_LEN];
    uint16_t len;
} XBeeFrame;

#define XBEE_N_FRAMES_MAX 20
static XBeeFrame xbee_frames[XBEE_N_FRAMES_MAX];
static uint8_t xbee_n_frames = 0;

//#define XBEE_UART_IT_ENABLE()  HAL_NVIC_EnableIRQ( USART3_IRQn )
//#define XBEE_UART_IT_DISABLE() HAL_NVIC_DisableIRQ( USART3_IRQn )

#define XBEE_UART_IT_ENABLE() __enable_irq()
#define XBEE_UART_IT_DISABLE() __disable_irq()

extern UART_HandleTypeDef huart6;

static uint8_t xbee_mode_AT = 0;

event xbee_evt; // Received a message from xbee board
event xbee_evt_endtx;

// Duration of green led lighting when a frame is received
#define XBEE_RX_LED_Pin LD4_Pin
#define LED_RECV_DURATION_MS 80
static uint16_t led_ms = 0;

#define XBEE_RX_LED_START()                                   \
    do {                                                      \
        led_ms = LED_RECV_DURATION_MS;                        \
        HAL_GPIO_WritePin(LD3_GPIO_Port, XBEE_RX_LED_Pin, 1); \
    } while (0)

#define XBEE_RX_LED_STOP()                                    \
    do {                                                      \
        led_ms = 0;                                           \
        HAL_GPIO_WritePin(LD3_GPIO_Port, XBEE_RX_LED_Pin, 0); \
    } while (0)

#define XBEE_AT_NO_RESPONSE_NEEDED 0

xbee_function_error xbee_signal_error;

void xbee_notify_endtx() {
    event_trigger(&xbee_evt_endtx);
}

// ...........................................................................
#define xbee_frames_head xbee_frames[0]
#define xbee_frames_pop_head() xbee_frames_pop(0)

void xbee_frames_copy(int target_rank, int source_rank) {
    int i;
    for (i = 0; i < xbee_frames[source_rank].len; i++) {
        xbee_frames[target_rank].rx[i] = xbee_frames[source_rank].rx[i];
    }

    xbee_frames[target_rank].len = xbee_frames[source_rank].len;
}

void xbee_frames_pop(int rank) {
    if (xbee_n_frames > 0) {
        int i;
        for (i = rank; i < xbee_n_frames - 1; i++) {
            xbee_frames_copy(i, i + 1);
        }
        xbee_n_frames--;

        if (xbee_n_frames) {
            event_trigger(&xbee_evt);
        }
    } else {
        xbee_signal_error(__LINE__);
    }
}

void xbee_frames_push(uint8_t* data, uint16_t len) {
    if (xbee_n_frames >= XBEE_N_FRAMES_MAX) {
        xbee_signal_error(__LINE__);
        return;
    }

    if (len >= FRAME_LEN) {
        xbee_signal_error(__LINE__);
        return;
    }

    if (len < 4) {
        //Wrong frame ?
        xbee_signal_error(__LINE__);
        return;
    }

    int i;
    for (i = 0; i < len; i++) {
        xbee_frames[xbee_n_frames].rx[i] = data[i];
    }

    xbee_frames[xbee_n_frames].len = len;

    xbee_n_frames++;

    event_trigger(&xbee_evt);
}

int xbee_frames_find(uint8_t* rank, uint8_t type) {
    int k;
    for (k = 0; k < xbee_n_frames; k++) {
        if (xbee_frames[k].rx[3] == type) {
            *rank = k;
            return 1;
        }
    }

    return 0;
}

void xbee_frames_extract(uint8_t* data, uint16_t* len, uint8_t rank) {
    int i;
    *len = xbee_frames[rank].len;
    for (i = 0; i < *len; i++) {
        data[i] = xbee_frames[rank].rx[i];
    }
}
// ...........................................................................

// ...........................................................................
#define xbee_recv_clear()   \
    {                       \
        xbee_rx_len = 0;    \
        xbee_frame_len = 0; \
    }
// ...........................................................................

// ...........................................................................
event* xbee_get_recv_event() { return &xbee_evt; }

int xbee_wait_recv(uint16_t timeout_ms) {
    uint32_t tickend = HAL_GetTick() + timeout_ms;

    while (!event_check(&xbee_evt)) {
        if (HAL_GetTick() >= tickend) {
            return 0;
        }
    }

    return 1;
}
// ...........................................................................

// ...........................................................................
void xbee_log_clear();

void xbee_init(int coordinator, xbee_function_error f) {
    xbee_signal_error = f;

    xbee_log_clear();

    event_init(&xbee_evt);
    event_init(&xbee_evt_endtx);

    //xbee_saved_init();

    HAL_Delay(1000);

    xbee_recv_clear();
    xbee_n_frames = 0;

#if 1
    //ID AAAA
    xbee_api_AT16((uint8_t*)"ID", 0xAAAA);

    //CH C
    xbee_api_AT8((uint8_t*)"CH", 0x11);
#endif

#if 1
    // CE 0 ou 1
    //xbee_api_AT8( ( uint8_t * )"CE", coordinator ? 1 : 0 );
    xbee_api_AT8((uint8_t*)"CE", 0);
#endif

#if 0
	// AC
	xbee_api_ATR( ( uint8_t * )"AC" );
#endif
}
// ...........................................................................

// ...........................................................................
uint8_t xbee_recv_get_frame_type() {
    if (xbee_n_frames)
        return xbee_frames_head.rx[3];

    return 0;
}
// ...........................................................................

// ...........................................................................
uint8_t xbee_checksum(uint8_t* data, uint16_t len) {
    uint8_t crc = 0;
    for (int i = 0; i < len; i++)
        crc += data[i];
    return 0xFF - crc;
}
// ...........................................................................

// ...........................................................................
// 0 1 2 3 4 5
// E E              xbee_frame_len = 2
// r r R R R        xbee_rx_len = 5
void xbee_pull_expected() {
    if (xbee_rx_len < xbee_frame_len) {
        xbee_signal_error(__LINE__);
        return;
    }

    int i, k;
    for (k = 0, i = xbee_frame_len; i < xbee_rx_len; i++, k++) {
        xbee_rx[k] = xbee_rx[i];
    }
    xbee_rx_len -= xbee_frame_len;
    xbee_frame_len = 0;
}
// ...........................................................................

// ...........................................................................
void xbee_analyse_resp() {
    int go;

    do {
        go = 0;

        if (xbee_frame_len == 0) {
            if (xbee_rx_len >= 1) {
                if (xbee_rx[0] != 0x7E) {
                    xbee_recv_clear();
                    xbee_signal_error(__LINE__);
                }
            }

            if (xbee_rx_len >= 3) {
                xbee_frame_len = xbee_rx[1];
                xbee_frame_len <<= 8;
                xbee_frame_len |= xbee_rx[2];
                xbee_frame_len += 4;

                if (xbee_frame_len > BUFFER_LEN) {
                    xbee_signal_error(__LINE__);
                }
            }
        }

        if (xbee_frame_len && (xbee_rx_len >= xbee_frame_len)) {
            uint8_t crc = xbee_checksum(xbee_rx + 3, xbee_frame_len - 4);

            if (crc != xbee_rx[xbee_frame_len - 1]) {
                xbee_recv_clear();
                xbee_signal_error(__LINE__);
            } else {
                // Display frame rx on the green led
                //XBEE_RX_LED_START();
                //sprintf( radio_log, "TRIGGER %d", xbee_rx_len );

                go = 1;

                //xbee_log( "<br/>" );

                // Save the frame and trigger the event
                xbee_frames_push(xbee_rx, xbee_frame_len);

                // Remove the frame
                xbee_pull_expected();
            }
        }
    } while (go);
}
// ...........................................................................

// ...........................................................................
int xbee_recv_full_rank(uint8_t* data, uint16_t* len, uint8_t rank) {
    uint16_t recvlen = xbee_frames[rank].len - 4;

    //sprintf( radio_log, "RECVFULL %d", xbee_rx_len );

#if 0
  // Check if data has enough space to hold the frame
  if ( *len < recvlen )
  {
	  *len = 0;
	  xbee_signal_error( __LINE__ );
	  return 0;
  }
#endif
    if (recvlen >= FRAME_LEN) {
        *len = 0;
        xbee_signal_error(__LINE__);
        return 0;
    }

    //sprintf( radio_log, "RECVFULL-- %d", xbee_rx_len );

    *len = recvlen;

    for (int i = 0; i < recvlen; i++)
        data[i] = xbee_frames[rank].rx[i + 3];

    XBEE_UART_IT_DISABLE();
    xbee_frames_pop(rank);
    XBEE_UART_IT_ENABLE();
    return 1;
}

int xbee_recv_full(uint8_t* data, uint16_t* len) {
    return xbee_recv_full_rank(data, len, 0);
}
// ...........................................................................

// ...........................................................................
int xbee_recv_discard_rank(uint8_t rank) {
    XBEE_UART_IT_DISABLE();
    xbee_frames_pop(rank);
    XBEE_UART_IT_ENABLE();
    return 1;
}

int xbee_recv_discard() {
    return xbee_recv_discard_rank(0);
}
// ...........................................................................

// ...........................................................................
int xbee_recv(uint64_t* from, uint8_t* data, uint16_t* len) {
    int i;
    uint16_t recvlen = xbee_frames_head.len - 16;

    //sprintf( radio_log, "RECV %d (%d - %d)", (int)xbee_rx_len, (int)(*len), (int)recvlen );

    // Check if data has enough space to hold the frame
    if (*len < recvlen) {
        *len = 0;
        xbee_signal_error(__LINE__);
        return 0;
    }

    //sprintf( radio_log, "RECV-- %d", xbee_rx_len );

    *len = recvlen;

    *from = 0;
    for (i = 0; i < 8; i++) {
        *from <<= 8;
        *from |= xbee_frames_head.rx[i + 4];
    }

    for (i = 0; i < recvlen; i++)
        data[i] = xbee_frames_head.rx[i + 15];

    XBEE_UART_IT_DISABLE();
    xbee_frames_pop_head();
    XBEE_UART_IT_ENABLE();
    return 1;
}
// ...........................................................................

// ...........................................................................
void xbee_notify_recv(uint8_t c) {
    XBEE_RX_LED_START();

    if (xbee_rx_len < BUFFER_LEN) {
        xbee_rx[xbee_rx_len++] = c;

        //char buff[4];
        //sprintf(buff, "%02x ", c );
        //xbee_log( buff );
    } else {
        // Discard all data
        xbee_recv_clear();
        xbee_signal_error(__LINE__);
        return;
    }

    //sprintf( radio_log, "%d", xbee_rx_len );

    if (xbee_mode_AT)
        return;

    xbee_analyse_resp();
}

// ...........................................................................
int xbee_AT_check_OK() {
    int r = 0;

    if (xbee_rx_len == 3) {
        xbee_rx[3] = 0;
        if (strcmp((char*)xbee_rx, "OK\r") == 0) {
            r = 1;
        }
        xbee_rx_len = 0;
    }

    return r;
}
// ...........................................................................

// ...........................................................................
int xbee_api_transmit(uint8_t* data, uint16_t len, uint8_t ack_frame_type, uint8_t* resp_data, uint16_t* resp_len) {
    static uint8_t buffer[FRAME_LEN];

    xbee_ack_frame_type = ack_frame_type;

    buffer[0] = 0x7E;
    buffer[1] = (len >> 8) & 0xFF;
    buffer[2] = len & 0xFF;
    for (int i = 0; i < len; i++)
        buffer[3 + i] = data[i];
    buffer[len + 3] = xbee_checksum(data, len);

    HAL_StatusTypeDef r;
    while ((r = HAL_UART_Transmit_IT(&huart6, buffer, len + 4)) == HAL_BUSY) {
    }

    if (r != HAL_OK)
        return 0;

    while (!event_check(&xbee_evt_endtx)) {
    }

    //return 0;

    // Wait for ack from the XBee board
    uint8_t rank;

    uint32_t tickend = HAL_GetTick() + 5000;

    while (HAL_GetTick() < tickend) {
        // Check if there is an ack
        if (xbee_frames_find(&rank, ack_frame_type)) {
            if (resp_data) {
                xbee_recv_full_rank(resp_data, resp_len, rank);
            } else {
                xbee_recv_discard_rank(rank);
            }
            return 1;
        }
    }

    xbee_signal_error(__LINE__);
    return 0;
}
// ...........................................................................

static uint8_t frame_id = 1;
#define INC_FRAME_ID()     \
    {                      \
        frame_id++;        \
        if (frame_id == 0) \
            frame_id = 1;  \
    }

// ...........................................................................
// Note : len = 73 max
int xbee_api_send_to(uint64_t to, uint8_t* data, uint16_t len) {
    int i;
    uint8_t xreq[FRAME_LEN];

    // Type
    xreq[0] = 0x10;

    // Frame ID
    xreq[1] = frame_id;
    INC_FRAME_ID();

    // Options
    //   - Point-multipoint (0x40)
    //   - Directed Broadcast (0x80)
    //   - DigiMesh (0xC0)
    // Note: done here before we destroy the value of 'to'
    //xreq[ 13 ] = 0x80;
    xreq[13] = (to == XBEE_BROADCAST ? 0x80 : 0x40);

    // Dest address
    for (i = 7; i >= 0; i--) {
        xreq[i + 2] = to & 0xFF;
        to >>= 8;
    }

    // Reserved
    xreq[10] = 0xFF;
    xreq[11] = 0xFE;

    // Radius
    xreq[12] = 0;

    for (i = 0; i < len; i++)
        xreq[14 + i] = data[i];

    if (xbee_api_transmit(xreq, 14 + len, 0x8B, 0, 0)) {
        return 1;
    }

    return 0;
}
// ...........................................................................

// ...........................................................................
int xbee_api_AT16(uint8_t* cmd, uint16_t val) {
    int i;
    uint8_t xreq[FRAME_LEN];

    // Type
    xreq[0] = 0x08;

    // Frame ID
#if XBEE_AT_NO_RESPONSE_NEEDED
    xreq[1] = 0;
#else
    xreq[1] = frame_id;
    INC_FRAME_ID();
#endif

    // Command and parameters
    for (i = 0; i < 2; i++)
        xreq[2 + i] = cmd[i]; // Works even with 1 char cmd (0 ended)

    xreq[4] = (val >> 8) & 0xFF;
    xreq[5] = val & 0xFF;

    return xbee_api_transmit(xreq, 6, 0x88, 0, 0);
}
// ...........................................................................

// ...........................................................................
int xbee_api_AT8(uint8_t* cmd, uint8_t val) {
    int i;
    uint8_t xreq[FRAME_LEN];

    // Type
    xreq[0] = 0x08;

    // Frame ID
#if XBEE_AT_NO_RESPONSE_NEEDED
    xreq[1] = 0;
#else
    xreq[1] = frame_id;
    INC_FRAME_ID();
#endif

    // Command and parameters
    for (i = 0; i < 2; i++)
        xreq[2 + i] = cmd[i]; // Works even with 1 char cmd (0 ended)

    xreq[4] = val;

    return xbee_api_transmit(xreq, 5, 0x88, 0, 0);
}

// ...........................................................................
int xbee_api_ATR(uint8_t* cmd, uint8_t* resp_data, uint16_t* resp_len) {
    int i;
    uint8_t xreq[FRAME_LEN];

    // Type
    xreq[0] = 0x08;

    // Frame ID
#if XBEE_AT_NO_RESPONSE_NEEDED
    xreq[1] = 0;
#else
    xreq[1] = frame_id;
    INC_FRAME_ID();
#endif

    // Command
    for (i = 0; i < 2; i++)
        xreq[2 + i] = cmd[i]; // Works even with 1 char cmd (0 ended)

    return xbee_api_transmit(xreq, 4, 0x88, resp_data, resp_len);
}
// ...........................................................................

// ...........................................................................
uint64_t xbee_api_read_unique_id() {
    uint8_t buffer[FRAME_LEN];
    uint16_t len;
    int i;

    uint64_t id = 0;

    // SH
    if (!xbee_api_ATR((uint8_t*)"SH", buffer, &len)) {
        xbee_signal_error(__LINE__);
        return 0;
    }

    // Check result
    if ((buffer[0] == 0x88) && (buffer[4] == 0)) {
        for (i = 0; i < len - 5; i++) {
            id <<= 8;
            id |= buffer[i + 5];
        }
    } else {
        xbee_signal_error(__LINE__);
        return 0;
    }

    // SL
    if (!xbee_api_ATR((uint8_t*)"SL", buffer, &len)) {
        xbee_signal_error(__LINE__);
        return 0;
    }

    // Check result
    if ((buffer[0] == 0x88) && (buffer[4] == 0)) {
        for (i = 0; i < len - 5; i++) {
            id <<= 8;
            id |= buffer[i + 5];
        }
    } else {
        xbee_signal_error(__LINE__);
        return 0;
    }
    return id;
}
// ...........................................................................

// ...........................................................................
void xbee_process(int ms) {
    if (led_ms) {
        if (led_ms <= ms) {
            XBEE_RX_LED_STOP();
        } else {
            led_ms -= ms;
        }
    }
}
// ...........................................................................

// ...........................................................................
int xbee_AT_configure_API1() {
    xbee_mode_AT = 1;

    HAL_Delay(1100);
    HAL_UART_Transmit(&huart6, (uint8_t*)"+++", 3, 1000);
    HAL_Delay(1100);

    if (!xbee_AT_check_OK()) {
        xbee_mode_AT = 0;
        return 1;
    }

    HAL_UART_Transmit(&huart6, (uint8_t*)"ATRE\r", 5, 1000);
    HAL_Delay(100);
    if (!xbee_AT_check_OK()) {
        xbee_mode_AT = 0;
        return 2;
    }

    HAL_UART_Transmit(&huart6, (uint8_t*)"ATAP1\r", 6, 1000);
    HAL_Delay(100);
    if (!xbee_AT_check_OK()) {
        xbee_mode_AT = 0;
        return 2;
    }

    HAL_UART_Transmit(&huart6, (uint8_t*)"ATBD7\r", 6, 1000); // 115200
    HAL_Delay(100);
    if (!xbee_AT_check_OK()) {
        xbee_mode_AT = 0;
        return 3;
    }

    HAL_UART_Transmit(&huart6, (uint8_t*)"ATWR\r", 5, 1000);
    HAL_Delay(100);
    if (!xbee_AT_check_OK()) {
        xbee_mode_AT = 0;
        return 4;
    }

    HAL_UART_Transmit(&huart6, (uint8_t*)"ATCN\r", 5, 1000);
    HAL_Delay(100);

    xbee_mode_AT = 0;
    return 0;
}
// ...........................................................................

void xbee_log_clear() {
    xbee_log_buffer[0] = 0;
}

void xbee_log(const char* msg) {
    int len_rl = strlen((const char*)xbee_log_buffer);
    int len_msg = strlen(msg);

    if (len_rl + len_msg > XBEE_LOG_SIZE) {
        if (len_msg < XBEE_LOG_SIZE / 3)
            len_msg = XBEE_LOG_SIZE / 3;

        // Free some space
        int i, k;
        len_rl++;
        for (k = 0, i = len_msg; i < len_rl; i++, k++)
            xbee_log_buffer[k] = xbee_log_buffer[i];
    }

    strcat((char*)xbee_log_buffer, msg);
}

const char* xbee_log_get() {
    return (const char*)xbee_log_buffer;
}

int xbee_log_len() {
    return strlen((const char*)xbee_log_buffer);
}
