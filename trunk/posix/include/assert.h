/*
 * assert.h
 *
 * verify program assertion. Based on the Single UNIX(r) Specification,
 * Version 2
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __ASSERT_H_INCLUDED__
#define __ASSERT_H_INCLUDED__

/* types */

/* constants */

/* prototypes */

/* macros */
#ifdef NDEBUG
#define assert(ignore) ((void) 0)
#else /* !NDEBUG */

#define assert(expression) \
#ifdef __PSXDLL__

/* headers for internal usage by psxdll.dll and ReactOS */
#include <psxdll/stdio.h>
#include <psxdll/stdlib.h>

#else /* ! __PSXDLL__ */

/* standard POSIX headers */
#include <stdio.h>
#include <stdlib.h>

#endif

 if(!(expression)) \
 { \
  fputs("__FILE__, line __LINE__: assertion \"expression\" failed\n", stderr); \
  abort(); \
 }

#endif /* NDEBUG */

#endif /* __ASSERT_H_INCLUDED__ */

/* EOF */

