/* -----------------------------------------------------------------------------
 * input.h
 *
 * Copyright (c) 2006, Vivek Mohan <vivek@sig9.com>
 * All rights reserved. See LICENSE
 * -----------------------------------------------------------------------------
 */

#pragma once


#include <stdint.h>
#include "dis_types.h"



uint8_t inp_next(struct ud*);
uint8_t inp_peek(struct ud*);
uint8_t inp_uint8(struct ud*);
uint16_t inp_uint16(struct ud*);
uint32_t inp_uint32(struct ud*);
uint64_t inp_uint64(struct ud*);

/* inp_init() - Initializes the input system. */
static INLINE void inp_init(struct ud* u)
{
  u->inp_curr = NULL;
  u->inp_fill = NULL;
  u->inp_sess = NULL;
  u->inp_ctr  = 0;
}

/* inp_start() - Should be called before each de-code operation. */
static INLINE void inp_start(struct ud* u)
{
  u->inp_ctr = 0;
 
  if (u->inp_curr == u->inp_fill) {
	u->inp_curr = NULL;
	u->inp_fill = NULL;
	u->inp_sess = u->inp_cache;
  } else 
	u->inp_sess = u->inp_curr + 1;
}

/* inp_back() - Move back a byte. */
static INLINE void inp_back(struct ud* u)
{
  if (u->inp_ctr > 0) {
	--u->inp_curr;
	--u->inp_ctr; 
  }
}

/* inp_back() - Resets the current pointer to its position before the current
 * instruction disassembly was started.
 */
static INLINE void inp_reset(struct ud* u)
{
  u->inp_curr -= u->inp_ctr;
  u->inp_ctr = 0;
}

/* inp_sess() - Returns the pointer to current session. */
static INLINE uint8_t* inp_sess(struct ud* u)
{
  return u->inp_sess;
}

/* inp_cur() - Returns the current input byte. */
static INLINE uint8_t inp_curr(struct ud* u)
{
  if (u->inp_curr == NULL) 
	return(0);
  return *(u->inp_curr);
}

/* inp_move() - move ahead n input bytes. */
static INLINE void inp_move(struct ud* u, size_t n)
{
  while (n--)
	inp_next(u);
}

#ifdef __cplusplus
}
#endif

