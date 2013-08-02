/*
 * The high level API to be used by applications which use the dash7 stack
 *  Created on: Nov 22, 2012
 *  Authors:
 * 		maarten.weyn@artesis.be
 *  	glenn.ergeerts@artesis.be
 *
 */

#ifndef D7STACK_H_
#define D7STACK_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Configuration section
 *
 *  TODO get from configure step or similar
 */

#define DEBUG

#include "phy/phy.h"
#include "dll/dll.h"
#include "nwl/nwl.h"
#include "trans/trans.h"
#include "pres/pres.h"

void d7aoss_init(const filesystem_address_info *address_info);

#ifdef __cplusplus
extern "C" {
#endif

#endif /* D7STACK_H_ */
