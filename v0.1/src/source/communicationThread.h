/*
 * communicationThread.h
 *
 *  Created on: Feb 17, 2019
 *      Author: Max Shvetsov
 */

#ifndef SOURCE_COMMUNICATIONTHREAD_H_
#define SOURCE_COMMUNICATIONTHREAD_H_

#include "ch.h"
#include "hal.h"


THD_FUNCTION(communicationThrFunction, arg);
THD_FUNCTION(usbThreadFunction, arg);
THD_FUNCTION(steeringSafetyFunction, arg);

#endif /* SOURCE_COMMUNICATIONTHREAD_H_ */
