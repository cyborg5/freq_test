#include "wiring_private.h"
#define IR_SEND_PWM_PIN 9
#define FREQUENCY 40

#define IR_SEND_PWM_START {IR_TCCx->CTRLA.bit.ENABLE = 1;\
  while (IR_TCCx->SYNCBUSY.bit.ENABLE);}
#define IR_SEND_PWM_STOP {IR_TCCx->CTRLA.bit.ENABLE = 0;\
  while (IR_TCCx->SYNCBUSY.bit.ENABLE);}

Tcc* IR_TCCx;

void setup() {
  Serial.begin(9600);
  delay(2000); while (!Serial); 
  Serial.println(F("Serial output ready"));
  #if defined(__SAMD21G18A__)
    Serial.println ("SAMD detected");
  #endif
  uint8_t port=g_APinDescription[IR_SEND_PWM_PIN].ulPort;
  uint8_t hw_pin=g_APinDescription[IR_SEND_PWM_PIN].ulPin;
  Serial.print("Port:"); Serial.print(port,DEC);
  Serial.print(" hwPin:"); Serial.println(hw_pin,DEC);

  PinDescription pinDesc = g_APinDescription[IR_SEND_PWM_PIN];
  uint32_t attr = pinDesc.ulPinAttribute;
  //If PWM unsupported then do nothing and exit
  Serial.print("attr:"); Serial.println(attr,HEX);
  Serial.print("PIN_ATTR_PWM:"); Serial.println(PIN_ATTR_PWM,HEX);

  if ( !((attr & PIN_ATTR_PWM) == PIN_ATTR_PWM) ) {
    return;
  };
  
  uint32_t tcNum = GetTCNumber(pinDesc.ulPWMChannel);
  uint8_t tcChannel = GetTCChannelNumber(pinDesc.ulPWMChannel);
  Serial.print("tcNum:"); Serial.println(tcNum,DEC);
  Serial.print("tcChannel:"); Serial.println(tcChannel,DEC);

  if (attr & PIN_ATTR_TIMER) {
    Serial.println("Using Primary timer");
    pinPeripheral(IR_SEND_PWM_PIN, PIO_TCC_PDEC);
  } else {
    Serial.println("Using Alternate timer");
    // We suppose that attr has PIN_ATTR_TIMER_ALT bit set...
    pinPeripheral(IR_SEND_PWM_PIN, PIO_TIMER_ALT);
  }
  
  uint32_t GCLK_CLKCTRL_IDs[] = {
    TCC0_GCLK_ID,
    TCC1_GCLK_ID,
    TCC2_GCLK_ID,
  #if defined(TCC3)
    TCC3_GCLK_ID,
    TCC4_GCLK_ID,
    TC5_GCLK_ID,
  #endif
  };
  GCLK->PCHCTRL[GCLK_CLKCTRL_IDs[tcNum]].reg = GCLK_PCHCTRL_GEN_GCLK0_Val | (1 << GCLK_PCHCTRL_CHEN_Pos); //use clock generator 0
  
  // Normal (single slope) PWM operation: timers countinuously count up to PER 
  // register value and then is reset to 0
  IR_TCCx = (Tcc*) GetTC(pinDesc.ulPWMChannel);
  IR_TCCx->CTRLA.bit.SWRST = 1;
  while (IR_TCCx->SYNCBUSY.bit.SWRST);
  
  // Disable TCCx
  IR_TCCx->CTRLA.bit.ENABLE = 0;
  while (IR_TCCx->SYNCBUSY.bit.ENABLE);
  // Sent pre-scaler to 1
  IR_TCCx->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 | TCC_CTRLA_PRESCSYNC_GCLK;
  
  //Set TCCx as normal PWM
  IR_TCCx->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
  while (IR_TCCx->SYNCBUSY.bit.WAVE);

  while (IR_TCCx->SYNCBUSY.bit.CC0 || IR_TCCx->SYNCBUSY.bit.CC1);

  // Each timer counts up to a maximum or TOP value set by the PER register,
  // this determines the frequency of the PWM operation.
  uint32_t cc = 120000000UL/(FREQUENCY*1000) - 1;
  // The CCx register value corresponds to the pulsewidth in microseconds (us) 
  // Set the duty cycle of the PWM on TCCx to 33%
  IR_TCCx->CC[tcChannel].reg = (uint32_t) cc/3;      
  while (IR_TCCx->SYNCBUSY.bit.CC0 || IR_TCCx->SYNCBUSY.bit.CC1);

  IR_TCCx->PER.reg = cc;      // Set the frequency of the PWM on IR_TCCx
  while(IR_TCCx->SYNCBUSY.bit.PER);

  IR_TCCx->CTRLA.bit.ENABLE = 0;            //initially off will turn on later
  while (IR_TCCx->SYNCBUSY.bit.ENABLE);

  Serial.println("Sending");
  uint8_t i;
  for(i=0;i<50;i++) {
    IR_SEND_PWM_START;
    delay(100);
    IR_SEND_PWM_STOP 
    delay(100);
  }
  Serial.println("Finished");

}

void loop() {
  }

