/*
 * key.h
 *
 *  Created on: Mar 20, 2019
 */

#ifndef SRC_KEY_H_
#define SRC_KEY_H_


#ifdef __cplusplus
extern "C" {
#endif

//#define DEBUG_KEYS

void KeyInit(void);
void KeyScan(void);

#ifdef DEBUG_KEYS
const uint8_t* DebugGetKeys(void);
#endif

//definition included for determining if multiple keys are pressed
//prefer using GetNewKeys();
#define TOTAL_ROWS 5
extern uint8_t Keys[TOTAL_ROWS]; //only bottom nibbles get set
extern int8_t NewKeyPressed;


#ifdef __cplusplus
}
#endif

#endif /* SRC_KEY_H_ */
