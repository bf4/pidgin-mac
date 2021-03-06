/*
 * Copyright (C) 2004, 2005, 2008 Free Software Foundation
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNUTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#include "../lib/gnutls_int.h"
#include "../lib/gnutls_hash_int.h"
#include "../lib/x509/pbkdf2-sha1.h"
#include "../lib/debug.h"

int
is_secure_mem (const void *ign)
{
  return 0;
}

void
doit (void)
{
  char digest[20];
  int err;

  /* XXX: We need this to fix secure memory. */
  gnutls_global_init ();

  err =
    _gnutls_hmac_fast (GNUTLS_MAC_MD5, "keykeykey", 9, "abcdefgh", 8, digest);
  if (err < 0)
    fail ("_gnutls_hmac_fast(MD5) failed: %d\n", err);
  else
    {
      if (memcmp (digest, "\x3c\xb0\x9d\x83\x28\x01\xef\xc0"
		  "\x7b\xb3\xaf\x42\x69\xe5\x93\x9a", 16) == 0)
	success ("_gnutls_hmac_fast(MD5) OK\n");
      else
	{
	  hexprint (digest, 16);
	  fail ("_gnutls_hmac_fast(MD5) failure\n");
	}
    }

  err =
    _gnutls_hmac_fast (GNUTLS_MAC_SHA1, "keykeykey", 9, "abcdefgh", 8,
		       digest);
  if (err < 0)
    fail ("_gnutls_hmac_fast(SHA1) failed: %d\n", err);
  else
    {
      if (memcmp (digest, "\x58\x93\x7a\x58\xfe\xea\x82\xf8"
		  "\x0e\x64\x62\x01\x40\x2b\x2c\xed\x5d\x54\xc1\xfa",
		  20) == 0)
	success ("_gnutls_hmac_fast(SHA1) OK\n");
      else
	{
	  hexprint (digest, 20);
	  fail ("_gnutls_hmac_fast(SHA1) failure\n");
	}
    }

  err = _gnutls_pbkdf2_sha1 ("password", 8, "salt", 4, 4711, digest, 16);
  if (err < 0)
    fail ("_gnutls_pkcs5_pbkdf2_sha1() failed: %d\n", err);
  else
    {
      if (memcmp (digest, "\x09\xb7\x85\x57\xdd\xf6\x07\x15"
		  "\x1c\x52\x34\xde\xba\x5c\xdc\x59", 16) == 0)
	success ("_gnutls_pkcs5_pbkdf2_sha1() OK\n");
      else
	{
	  hexprint (digest, 16);
	  fail ("_gnutls_pkcs5_pbkdf2_sha1() failure\n");
	}
    }

  gnutls_global_deinit ();
}
