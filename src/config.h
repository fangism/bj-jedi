// a temporarily hard-coded config.h file

#ifndef	__CONFIG_H__
#define	__CONFIG_H__

#ifdef HAVE_CONFIG_H
#include "__config__.h"
#else
// use hard-coded copy
#define	HAVE_GNUREADLINE		1
#define	HAVE_READLINE_READLINE_H	1
#define	HAVE_READLINE_HISTORY_H		1
#define	READLINE_PROMPT_CONST		1
#define	HAVE_RL_COMPLETION_MATCHES	1
#define	__ATTRIBUTE_UNUSED__		__attribute__ ((unused))
#endif

#endif	// __CONFIG_H__
