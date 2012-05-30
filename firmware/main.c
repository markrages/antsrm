#include <msp430.h>
#include <legacymsp430.h>
#include <stdint.h>

#include "generated_messages.h"

#define SLOPE 25.2
#define OFFSET 500
#define DEVICE_ID 250

#define COOKED_SLOPE ((int16_t)(SLOPE * 10))
uint16_t slope=COOKED_SLOPE;
uint16_t offset=OFFSET;

// all these on port 1
#define SDI_BIT   (1<<7) // host out, ANT in
#define SDO_BIT   (1<<6) // host in, ANT out
#define SCK_BIT   (1<<5) // read on rising edge
#define TAT_BIT   (1<<4) // uncommitted debug
#define SEN_BIT   (1<<3) // from ANT to host
#define SRM_BIT   (1<<2)
#define MRDY_BIT  (1<<0) // from host to ANT
#define SRDY_BIT  (1<<1) // from host to ANT

uint16_t ant_cycles_since_last_torque_pulse=0;
volatile uint16_t ant_cycles_since_last_cadence_pulse=0;

static inline void init_p1(void) {
  P1OUT= (MRDY_BIT | SRDY_BIT);
  P1DIR= (SDO_BIT | TAT_BIT | MRDY_BIT | SRDY_BIT);
  P1SEL= (SDI_BIT | SDO_BIT | SCK_BIT );
  P1IE = (SRM_BIT | SEN_BIT);
}

volatile uint16_t status_bits=0;
#define STATUS_BIT_ANT_MESSAGE_RECEIVED  (1<<0)
#define STATUS_BIT_CALCULATE_REED_TIME   (1<<1)
#define STATUS_BIT_ANT_AWAKE             (1<<2)

uint8_t rx_buffer[18];
uint8_t tx_buffer[14];

static inline void ant_phy_reset(void) {
  // follow the sequence in the doc, 3.4.2
  int i=0;

  P1OUT |= MRDY_BIT | SRDY_BIT;

  i=100; while (i--);

  P1OUT &= ~SRDY_BIT; // drop srdy,
  i=4000; while (i--); // wait,
  P1OUT &= ~MRDY_BIT; // drop mrdy

  while (!(P1IN & SEN_BIT)) // wait for SEN to go high
    ;

  while (P1IN & SEN_BIT) // wait for SEN to go low
    ;

  P1OUT |= SRDY_BIT; // set srdy again

  // CKPH=1 CKPL=0;
  // USIMST=0; USCI2C=0; USCIOE=1;
  // USI16B=0;

  // init serial port
  USICTL0 = USIPE7 | USIPE6 | USIPE5 | USILSB /*| USIGE*/ | USIOE | USISWRST;
  USICTL1 = USICKPH;
  USICKCTL = USIDIV0 | USISSEL_4; // | USICKPL;
  USICTL0 &= ~USISWRST;

}

static inline uint8_t ant_tx_rx_1(uint8_t in) {

  USISRL=in;
  USICNT = 8; // resets interrupt flag too

  P1OUT &= ~SRDY_BIT; // pulse srdy so ANT will talk to us
  P1OUT |= SRDY_BIT;

  USICTL1 |= USIIE;

  // snooze, await USI interrupt  
  while (!(USICTL1 & USIIFG))
    LPM3;

  return USISRL;
}

static inline void ant_tx_rx(void) {

  uint8_t c;
  // get the first character in to see if ANT wants to talk or listen
  c=ant_tx_rx_1(0xff);

  if (c==0xa5) { // host-to-ant
    int i=1;

    P1OUT |= MRDY_BIT;

    while (!(P1IN & SEN_BIT)) {
      ant_tx_rx_1(tx_buffer[i++]);
    }
  } else if (c==0xa4) { // ant-to-host
    int i=1;

    while (!(P1IN & SEN_BIT)) {
      rx_buffer[i++]=ant_tx_rx_1(0xff);
    }
    status_bits |= STATUS_BIT_ANT_MESSAGE_RECEIVED;
  }
}

static void check_tx_rx( void ) {
  if (!(P1IN & SEN_BIT)) {
    ant_tx_rx();
  }
}

uint8_t checksum=1; // checksum figured on 0xa5, which is 0xa4+1

#define TRANSMIT(offset, byte) do {		\
    tx_buffer[offset]=byte;			\
    checksum ^= byte;				\
  } while (0)

#define SEND_CHECK(offset) do {				\
    TRANSMIT(offset,checksum);				\
    P1OUT &= ~MRDY_BIT;  /* request ANT attention */	\
    checksum=1;						\
  } while (0)

static inline void ant_init( void ) {

  //ANT+Sport Power Settings (default)

#define ANT_SPORT_CHANNEL 1
#define ANT_SPORT_NETWORK_NUMBER 1
#define ANT_SPORT_CHANNEL_TYPE   0x10
#define ANT_SPORT_DEVICE_TYPE    11 // ANT+ power
#define ANT_SPORT_TRANS_TYPE     5 // ANT+ trans type

#define ANT_SPORT_CHANNEL_FREQ   (57)
#define ANT_SPORT_PERIOD         (8182)

  uint8_t *msg_buf=2+rx_buffer; // skip ID and length

  SEND_MESSAGE_ASSIGN_CHANNEL(ANT_SPORT_CHANNEL,
			      ANT_SPORT_CHANNEL_TYPE,
			      ANT_SPORT_NETWORK_NUMBER);

  while (1) {

    while (0==(status_bits & STATUS_BIT_ANT_MESSAGE_RECEIVED))
      check_tx_rx();
      
    
    status_bits &= ~STATUS_BIT_ANT_MESSAGE_RECEIVED;

    if (MESSAGE_IS_RESPONSE_ASSIGN_CHANNEL_OK(msg_buf))
      SEND_MESSAGE_SET_NETWORK(ANT_SPORT_NETWORK_NUMBER,
			       0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45);

    else if (MESSAGE_IS_RESPONSE_SET_NETWORK_OK(msg_buf))
      SEND_MESSAGE_CHANNEL_ID(ANT_SPORT_CHANNEL,
			      DEVICE_ID, // device number
			      ANT_SPORT_DEVICE_TYPE,
			      ANT_SPORT_TRANS_TYPE );

    else if (MESSAGE_IS_RESPONSE_CHANNEL_ID_OK(msg_buf))
      SEND_MESSAGE_CHANNEL_PERIOD(ANT_SPORT_CHANNEL, ANT_SPORT_PERIOD);

    else if (MESSAGE_IS_RESPONSE_CHANNEL_PERIOD_OK(msg_buf))
      SEND_MESSAGE_CHANNEL_FREQUENCY(ANT_SPORT_CHANNEL, ANT_SPORT_CHANNEL_FREQ);

    else if (MESSAGE_IS_RESPONSE_CHANNEL_FREQUENCY_OK(msg_buf))
      SEND_MESSAGE_TRANSMIT_POWER(3);

    else if (MESSAGE_IS_RESPONSE_TRANSMIT_POWER_OK(msg_buf))
      SEND_MESSAGE_OPEN_CHANNEL(ANT_SPORT_CHANNEL);

    else if (MESSAGE_IS_RESPONSE_OPEN_CHANNEL_OK(msg_buf))
      return;
  }
}


static inline void ant_uninit( void ) {

  uint8_t *msg_buf=2+rx_buffer; // skip ID and length

  SEND_MESSAGE_CLOSE_CHANNEL(ANT_SPORT_CHANNEL);

  while (1) {

    while (0==(status_bits & STATUS_BIT_ANT_MESSAGE_RECEIVED))
      check_tx_rx();

    status_bits &= ~STATUS_BIT_ANT_MESSAGE_RECEIVED;

    if (MESSAGE_IS_EVENT_CHANNEL_CLOSED(msg_buf))         
      SEND_MESSAGE_UNASSIGN_CHANNEL(ANT_SPORT_CHANNEL);

    else if (MESSAGE_IS_RESPONSE_CHANNEL_UNASSIGN_OK(msg_buf)) 
      return;
  }
}

int reed=0;
volatile int torque_counter=0;
uint16_t crank_time_counter;

uint16_t clock_time=0;

static inline void ant_rx_interpret( void ) {
  uint8_t *msg_buf=2+rx_buffer; // skip ID and length

  if (MESSAGE_IS_EVENT_TX(msg_buf)) {
    static uint16_t last_TAR=0;
    uint16_t this_TAR;

    //P1OUT |= TAT_BIT;
    do {
      this_TAR = TAR;
    } while ( this_TAR != TAR);

    clock_time = this_TAR - last_TAR; /* now clock time is the number
					 of clock cycles between EVENT_TX's. 
					 
					 This time is (within a few
					 PPM of) 8182/32768 seconds. */    
    last_TAR = this_TAR;

    ant_cycles_since_last_torque_pulse++;    
    ant_cycles_since_last_cadence_pulse++;

  } else if (MESSAGE_IS_CALIBRATION_REQUEST(msg_buf)) {
    ant_cycles_since_last_torque_pulse=23;

  } else if (MESSAGE_IS_SRM_SLOPE_SAVE(msg_buf)) {
    slope=SRM_SLOPE_SAVE_SLOPE(msg_buf);
    SEND_MESSAGE_SRM_SLOPE_ACK(ANT_SPORT_CHANNEL);
    ant_cycles_since_last_torque_pulse=24;

  } else if (MESSAGE_IS_SRM_OFFSET_SAVE(msg_buf)) {
    offset=SRM_OFFSET_SAVE_OFFSET(msg_buf);
    SEND_MESSAGE_SRM_OFFSET_ACK(ANT_SPORT_CHANNEL);
    ant_cycles_since_last_torque_pulse=24;
  }
}

interrupt (USI_VECTOR) wakeup USI1SR (void) {
  // we're just here for the wakeup
  USICTL1 &= ~USIIE;
}

volatile uint16_t reed_switch_TAR;

interrupt (PORT1_VECTOR) wakeup PORT1SR (void) {
 
  static volatile uint16_t long_count;
  static volatile uint16_t short_count;

  if (P1IFG & SRM_BIT) {
    BCSCTL1 = CALBC1_1MHZ;
    torque_counter++;
  } else {
    P1IFG=0;
    return;
  }

  // This delay is to wait for the end of the short pulse
  asm("nop"); asm("nop"); asm("nop");
  
  /* The short pulse should have died away by now.  Clear the
     interrupt flag and, if it's set again, the pulses are still
     ongoing, so we're in a long pulse. */

  P1IFG=0;

  P1OUT |= TAT_BIT; 

  if (P1IFG & SRM_BIT) {
    long_count++;
    if (long_count==12)
      short_count=0;
  } else {
    short_count++;
    if (short_count==16)
      long_count=0;
  }

  if ((long_count >= 14) && (short_count < 6)) {
    // this routine takes 120 us at 8MHz clock
    short_count=6; // prevent retriggering for a little while
    
    do {  
      reed_switch_TAR=TAR; 
    } while (reed_switch_TAR != TAR);

    status_bits |= STATUS_BIT_CALCULATE_REED_TIME;
  }
   
  P1IFG=0;

  ant_cycles_since_last_torque_pulse=0;
  BCSCTL1 = 12;

  P1OUT &= ~TAT_BIT;  
}

void update_crank_time( void ) {
    uint16_t this_TAR;
    uint16_t crank_TAR_diff;
       
    static uint16_t last_reed_TAR;

    reed++;
    
    do {
      this_TAR = reed_switch_TAR;
    } while (reed_switch_TAR != this_TAR);
    
    crank_TAR_diff = (this_TAR-last_reed_TAR);
    last_reed_TAR = this_TAR;
     
    uint32_t crank_diff = crank_TAR_diff;
    crank_diff *= 999; //(4091L * 125L)/1024L;
    crank_diff /= 2;   
    crank_diff /= clock_time;
    
    crank_time_counter += crank_diff;

    SEND_MESSAGE_CRANK_SRM(ANT_SPORT_CHANNEL,
			   reed,
			   slope,
			   crank_time_counter,
			   torque_counter);

    ant_cycles_since_last_cadence_pulse=0;
}


int main(void) {

  WDTCTL=WDTPW | WDTHOLD;

  init_p1();

  DCOCTL = 0;
  BCSCTL1 = 12; 
  DCOCTL = 3;  
  BCSCTL2 = 0;

  ant_phy_reset();

  __bis_SR_register(GIE);

  SEND_MESSAGE_RESET_SYSTEM();

  while (1) {

    check_tx_rx();

    if (status_bits & STATUS_BIT_ANT_MESSAGE_RECEIVED) {
      ant_rx_interpret();
      status_bits &= ~STATUS_BIT_ANT_MESSAGE_RECEIVED;
    }

    if (ant_cycles_since_last_torque_pulse > 10) {
      if (status_bits & STATUS_BIT_ANT_AWAKE) {
	ant_uninit();
	//ant_phy_reset();
	BCSCTL3 = 0;
	TACTL = 0;
	status_bits &= ~STATUS_BIT_ANT_AWAKE;
      }

    } else if (!(status_bits & STATUS_BIT_ANT_AWAKE)) {
	ant_init();
	BCSCTL3 = LFXT1S_2;
	TACTL = TASSEL_1 | MC_2 | TACLR; 
	status_bits |= STATUS_BIT_ANT_AWAKE;
    }
    /*    
    if (ant_cycles_since_last_cadence_pulse==21) {
      SEND_MESSAGE_TORQUE_SUPPORT(ANT_SPORT_CHANNEL,
				  1 | 2, // autozero status
				  0xffff,
				  offset);
      ant_cycles_since_last_cadence_pulse++; // hack, avoid retriggering
    } 
    */   

    if (ant_cycles_since_last_cadence_pulse==23) {
      SEND_MESSAGE_SRM_ZERO_RESPONSE(ANT_SPORT_CHANNEL,
				     offset);
      ant_cycles_since_last_cadence_pulse++; // hack, avoid retriggering
    }
    
    if (status_bits & STATUS_BIT_CALCULATE_REED_TIME) {
      status_bits &= ~STATUS_BIT_CALCULATE_REED_TIME;
      
       update_crank_time();
    }

    LPM3;

  }
}
