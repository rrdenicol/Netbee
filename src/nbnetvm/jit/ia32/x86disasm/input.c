/* -----------------------------------------------------------------------------
 * input.c
 *
 * Copyright (c) 2004, 2005, 2006, Vivek Mohan <vivek@sig9.com>
 * All rights reserved. See LICENSE
 * -----------------------------------------------------------------------------
 */

#include <udis86.h>
#include "extern.h"
#include "dis_types.h"
#include "input.h"

/* -----------------------------------------------------------------------------
 * inp_buff_hook() - Hook for buffered inputs.
 * -----------------------------------------------------------------------------
 */
static int 
inp_buff_hook(struct ud* u)
{
  if (u->inp_buff < u->inp_buff_end)
	return *u->inp_buff++;
  else	return -1;
}

/* -----------------------------------------------------------------------------
 * inp_buff_hook() - Hook for FILE inputs.
 * -----------------------------------------------------------------------------
 */
static int 
inp_file_hook(struct ud* u)
{
  return fgetc(u->inp_file);
}

/* =============================================================================
 * ud_inp_set_hook() - Sets input hook.
 * =============================================================================
 */
extern void 
ud_set_input_hook(register struct ud* u, int (*hook)(struct ud*))
{
  u->inp_hook = hook;
  inp_init(u);
}

/* =============================================================================
 * ud_inp_set_buffer() - Set buffer as input.
 * =============================================================================
 */
extern void 
ud_set_input_buffer(register struct ud* u, uint8_t* buf, size_t len)
{
  u->inp_hook = inp_buff_hook;
  u->inp_buff = buf;
  u->inp_buff_end = buf + len;
  inp_init(u);
}

/* =============================================================================
 * ud_input_set_file() - Set buffer as input.
 * =============================================================================
 */
extern void 
ud_set_input_file(register struct ud* u, FILE* f)
{
  u->inp_hook = inp_file_hook;
  u->inp_file = f;
  inp_init(u);
}

/* =============================================================================
 * ud_input_skip() - Skip n input bytes.
 * =============================================================================
 */
extern void 
ud_input_skip(struct ud* u, size_t n)
{
  while (n--) {
	u->inp_hook(u);
  }
}

/* =============================================================================
 * ud_input_end() - Test for end of input.
 * =============================================================================
 */
extern int 
ud_input_end(struct ud* u)
{
  return (u->inp_curr == u->inp_fill) && u->inp_end;
}

/* -----------------------------------------------------------------------------
 * inp_next() - Loads and returns the next byte from input.
 * -----------------------------------------------------------------------------
 */
extern uint8_t
inp_next(struct ud* u) 
{
  int c;
	
  if (u->inp_curr < u->inp_fill) {
	++u->inp_curr;
	++u->inp_ctr;
	return *u->inp_curr;
  }

  if (u->inp_end) {
	u->error = 1;
	return 0;
  }

  if ( (c = u->inp_hook(u)) == -1) {
	u->error = 1;
	u->inp_end = 1;
	return 0;
  }

  if (u->inp_fill == NULL) {
	u->inp_curr = u->inp_cache;
	u->inp_fill = u->inp_cache;
  } else {
	++u->inp_curr;
	++u->inp_fill;
  }

  ++u->inp_ctr;
  *(u->inp_fill) = c;

  return (unsigned char)c;
}

/* -----------------------------------------------------------------------------
 * inp_cur() - Peek into the next byte in source. 
 * -----------------------------------------------------------------------------
 */
extern uint8_t
inp_peek(struct ud* u) 
{
	unsigned char r = inp_next(u);
	inp_back(u);
	return r;
}

/*------------------------------------------------------------------------------
 *  inp_uintN() - return uintN from source.
 *------------------------------------------------------------------------------
 */
extern uint8_t 
inp_uint8(struct ud* u)
{
	uint8_t *ref;
	return inp_next(u);
	ref = u->inp_curr;
	return *((uint8_t*)ref);
}

extern uint16_t 
inp_uint16(struct ud* u)
{
	uint8_t *ref;

	inp_next(u);
	ref = u->inp_curr;
	inp_move(u, sizeof(uint16_t) - 1);

	return *((uint8_t*)ref) | (*((uint8_t*)(ref+1)) << 8);
}

extern uint32_t 
inp_uint32(struct ud* u)
{
	uint8_t *ref;
	inp_next(u);
	ref = u->inp_curr;

	inp_move(u, sizeof(uint32_t) - 1);

	return *((uint32_t*)ref);

	return *((uint8_t*)ref) | 
		(*((uint8_t*)(ref+1)) << 8) |
		(*((uint8_t*)(ref+2)) << 16) |
		(*((uint8_t*)(ref+3)) << 24);
}

extern uint64_t 
inp_uint64(struct ud* u)
{
	uint8_t *ref;
	uint64_t ret, r;

	inp_next(u);
	ref = u->inp_curr;
	inp_move(u, sizeof(uint64_t) - 1);

	ret = *((uint8_t*)ref) | 
		(*((uint8_t*)(ref+1)) << 8) |
		(*((uint8_t*)(ref+2)) << 16) |
		(*((uint8_t*)(ref+3)) << 24);
	r = *((uint8_t*)(ref+4));
	ret = ret |  (r << 32);
	r = *((uint8_t*)(ref+4));
	ret = ret |  (r << 40);
	r = *((uint8_t*)(ref+4));
	ret = ret |  (r << 48);
	r = *((uint8_t*)(ref+4));
	ret = ret |  (r << 56);

	return ret;
}
