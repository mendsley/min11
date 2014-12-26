/*
 * Copyright 2014 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef MIN11__CONFIG_H
#define MIN11__CONFIG_H

/**
 * MIN11_HAS_EXCEPTIONS
 *
 * Set to 0 to disable exception support. If disabled, min11 will call
 * MIN11_THROW_EXCEPTION(message) if the library would have thrown an
 * exception. By default this is defined to call std::abort
 *
 */
#if !defined(MIN11_HAS_EXCEPTIONS)
// check for msvc _HAS_EXCEPTIONS
#	if defined(_MSC_VER) && defined(_HAS_EXCEPTIONS) && !(_HAS_EXCEPTIONS)
#		define MIN11_HAS_EXCEPTIONS 0
#	else
#		define MIN11_HAS_EXCEPTIONS 1
#	endif
#endif

/**
 * MIN11_THROW_EXCEPTION(message)
 *
 * Overrides the default behavior of aborting the program when an exception
 * would have been thrown. Overridden implementations must not return control
 * to the library, as it is in an invalid state.
 *
 * message is of type `const char*'
 *
 * NOTE: This is only called when MIN11_HAS_EXCEPTIONS = 0
 */
#if !defined(MIN11_THROW_EXCEPTION) && !MIN11_HAS_EXCEPTIONS
#	include <cstdlib>
#	define MIN11_THROW_EXCEPTION(message) std::abort()
#endif

#endif // MIN11__CONFIG_H
