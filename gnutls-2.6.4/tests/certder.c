/*
 * Copyright (C) 2006, 2008 Free Software Foundation
 *
 * Author: Simon Josefsson
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNUTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNUTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>
#include "utils.h"

void
doit (void)
{
  int ret;
  unsigned char der[] = {
    0x30, 0x82, 0x04, 0x10, 0x30, 0x82, 0x03, 0x79,
    0xa0, 0x07, 0x02, 0x84, 0x90, 0x00, 0x00, 0x00,
    0x02, 0x02, 0x01, 0x00, 0x30, 0x0d, 0x06, 0x09,
    0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
    0x04, 0x05, 0x00, 0x30, 0x81, 0xbb, 0x31, 0x0b,
    0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13,
    0x02, 0x2d, 0x2d, 0x31, 0x12, 0x30, 0x10, 0x06,
    0x03, 0x55, 0x04, 0x08, 0x13, 0x09, 0x53, 0x6f,
    0x6d, 0x65, 0x53, 0x74, 0x61, 0x74, 0x65, 0x31,
    0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x07,
    0x13, 0x08, 0x53, 0x6f, 0x6d, 0x65, 0x43, 0x69,
    0x74, 0x79, 0x31, 0x19, 0x30, 0x17, 0x06, 0x03,
    0x55, 0x04, 0x0a, 0x13, 0x10, 0x53, 0x6f, 0x6d,
    0x65, 0x4f, 0x72, 0x67, 0x61, 0x6e, 0x69, 0x7a,
    0x61, 0x74, 0x69, 0x6f, 0x6e, 0x31, 0x1f, 0x30,
    0x1d, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x16,
    0x53, 0x6f, 0x6d, 0x65, 0x4f, 0x72, 0x67, 0x61,
    0x6e, 0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e,
    0x61, 0x6c, 0x55, 0x6e, 0x69, 0x74, 0x31, 0x1e,
    0x30, 0x1c, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
    0x15, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f,
    0x73, 0x74, 0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x6c,
    0x64, 0x6f, 0x6d, 0x61, 0x69, 0x6e, 0x31, 0x29,
    0x30, 0x27, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
    0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x1a, 0x72,
    0x6f, 0x6f, 0x74, 0x40, 0x6c, 0x6f, 0x63, 0x61,
    0x6c, 0x68, 0x6f, 0x73, 0x74, 0x2e, 0x6c, 0x6f,
    0x63, 0x61, 0x6c, 0x64, 0x6f, 0x6d, 0x61, 0x69,
    0x6e, 0x30, 0x1e, 0x17, 0x0d, 0x30, 0x34, 0x30,
    0x32, 0x31, 0x38, 0x32, 0x30, 0x30, 0x32, 0x33,
    0x34, 0x5a, 0x17, 0x0d, 0x30, 0x35, 0x31, 0x31,
    0x31, 0x37, 0x32, 0x30, 0x30, 0x32, 0x33, 0x34,
    0x5a, 0x30, 0x81, 0xbb, 0x31, 0x0b, 0x30, 0x09,
    0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x2d,
    0x2d, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03, 0x55,
    0x04, 0x08, 0x13, 0x09, 0x53, 0x6f, 0x6d, 0x65,
    0x53, 0x74, 0x61, 0x74, 0x65, 0x31, 0x11, 0x30,
    0x0f, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13, 0x08,
    0x53, 0x6f, 0x6d, 0x65, 0x43, 0x69, 0x74, 0x79,
    0x31, 0x19, 0x30, 0x17, 0x06, 0x03, 0x55, 0x04,
    0x0a, 0x13, 0x10, 0x53, 0x6f, 0x6d, 0x65, 0x4f,
    0x72, 0x67, 0x61, 0x6e, 0x69, 0x7a, 0x61, 0x74,
    0x69, 0x6f, 0x6e, 0x31, 0x1f, 0x30, 0x1d, 0x06,
    0x03, 0x55, 0x04, 0x0b, 0x13, 0x16, 0x53, 0x6f,
    0x6d, 0x65, 0x4f, 0x72, 0x67, 0x61, 0x6e, 0x69,
    0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x61, 0x6c,
    0x55, 0x6e, 0x69, 0x74, 0x31, 0x1e, 0x30, 0x1c,
    0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x15, 0x6c,
    0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74,
    0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x64, 0x6f,
    0x6d, 0x61, 0x69, 0x6e, 0x31, 0x29, 0x30, 0x27,
    0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
    0x01, 0x09, 0x01, 0x16, 0x1a, 0x72, 0x6f, 0x6f,
    0x74, 0x40, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68,
    0x6f, 0x73, 0x74, 0x2e, 0x6c, 0x6f, 0x63, 0x61,
    0x6c, 0x64, 0x6f, 0x6d, 0x61, 0x69, 0x6e, 0x30,
    0x81, 0x9f, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86,
    0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05,
    0x00, 0x03, 0x81, 0x8d, 0x00, 0x30, 0x81, 0x89,
    0x02, 0x81, 0x81, 0x00, 0xda, 0x3d, 0xb7, 0x66,
    0x9a, 0x41, 0x4f, 0xca, 0x1d, 0xd1, 0xc4, 0x1f,
    0xc9, 0x4c, 0xc6, 0x76, 0x45, 0xc5, 0x8e, 0x2f,
    0x3d, 0x45, 0xf5, 0x16, 0x9f, 0xb5, 0x22, 0x0b,
    0x61, 0x60, 0xa4, 0x42, 0x42, 0x98, 0xae, 0x45,
    0xe1, 0x4a, 0x17, 0x0b, 0x6e, 0xf7, 0x4e, 0xc0,
    0x1e, 0xe7, 0x78, 0xd0, 0x80, 0xfc, 0xde, 0x0a,
    0x96, 0x43, 0x13, 0xe4, 0xb5, 0xef, 0x47, 0xca,
    0x8f, 0xb3, 0x13, 0x92, 0x10, 0xc4, 0x02, 0x7b,
    0xbb, 0x6c, 0x9f, 0x2b, 0x63, 0x65, 0xfa, 0xac,
    0xcb, 0xc9, 0x14, 0x68, 0x53, 0xd9, 0xe2, 0x9c,
    0x57, 0x52, 0x23, 0xb9, 0x4f, 0x92, 0xc0, 0xa0,
    0xe3, 0xf5, 0x50, 0xb3, 0xc4, 0x5f, 0x4e, 0x73,
    0x9d, 0x0e, 0xfd, 0x9c, 0x57, 0x8e, 0x4c, 0x13,
    0xe0, 0x7a, 0x16, 0x6b, 0x27, 0xc9, 0xac, 0xb3,
    0x47, 0xb2, 0x3f, 0x8f, 0xe6, 0x1d, 0x00, 0xc8,
    0xaa, 0x6f, 0xdf, 0xcb, 0x02, 0x03, 0x01, 0x00,
    0x01, 0xa3, 0x82, 0x01, 0x1c, 0x30, 0x82, 0x01,
    0x18, 0x30, 0x1d, 0x06, 0x03, 0x55, 0x1d, 0x0e,
    0x04, 0x16, 0x04, 0x14, 0xe6, 0x30, 0x79, 0x2b,
    0xe2, 0xcf, 0x4f, 0xa7, 0x40, 0xa4, 0xb9, 0xa4,
    0x1e, 0x95, 0x56, 0xe8, 0x94, 0xda, 0xd9, 0x15,
    0x30, 0x81, 0xe8, 0x06, 0x03, 0x55, 0x1d, 0x23,
    0x04, 0x81, 0xe0, 0x30, 0x81, 0xdd, 0x80, 0x14,
    0xe6, 0x30, 0x79, 0x2b, 0xe2, 0xcf, 0x4f, 0xa7,
    0x40, 0xa4, 0xb9, 0xa4, 0x1e, 0x95, 0x56, 0xe8,
    0x94, 0xda, 0xd9, 0x15, 0xa1, 0x81, 0xc1, 0xa4,
    0x81, 0xbe, 0x30, 0x81, 0xbb, 0x31, 0x0b, 0x30,
    0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02,
    0x2d, 0x2d, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03,
    0x55, 0x04, 0x08, 0x13, 0x09, 0x53, 0x6f, 0x6d,
    0x65, 0x53, 0x74, 0x61, 0x74, 0x65, 0x31, 0x11,
    0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13,
    0x08, 0x53, 0x6f, 0x6d, 0x65, 0x43, 0x69, 0x74,
    0x79, 0x31, 0x19, 0x30, 0x17, 0x06, 0x03, 0x55,
    0x04, 0x0a, 0x13, 0x10, 0x53, 0x6f, 0x6d, 0x65,
    0x4f, 0x72, 0x67, 0x61, 0x6e, 0x69, 0x7a, 0x61,
    0x74, 0x69, 0x6f, 0x6e, 0x31, 0x1f, 0x30, 0x1d,
    0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x16, 0x53,
    0x6f, 0x6d, 0x65, 0x4f, 0x72, 0x67, 0x61, 0x6e,
    0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x61,
    0x6c, 0x55, 0x6e, 0x69, 0x74, 0x31, 0x1e, 0x30,
    0x1c, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x15,
    0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73,
    0x74, 0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x64,
    0x6f, 0x6d, 0x61, 0x69, 0x6e, 0x31, 0x29, 0x30,
    0x27, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
    0x0d, 0x01, 0x09, 0x01, 0x16, 0x1a, 0x72, 0x6f,
    0x6f, 0x74, 0x40, 0x6c, 0x6f, 0x63, 0x61, 0x6c,
    0x68, 0x6f, 0x73, 0x74, 0x2e, 0x6c, 0x6f, 0x63,
    0x61, 0x6c, 0x64, 0x6f, 0x6d, 0x61, 0x69, 0x6e,
    0x82, 0x01, 0x00, 0x30, 0x0c, 0x06, 0x03, 0x55,
    0x1d, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01,
    0xff, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48,
    0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04, 0x05, 0x00,
    0x03, 0x81, 0x81, 0x00, 0xcd, 0xc9, 0x30, 0x6d,
    0x02, 0x65, 0x41, 0xea, 0x0e, 0x46, 0x08, 0x6c,
    0x2f, 0xd5, 0xa7, 0xe4, 0x29, 0xd7, 0x3f, 0x18,
    0x16, 0xd7, 0x4b, 0x6f, 0x9d, 0xc0, 0x5b, 0xbf,
    0x68, 0x7b, 0x2e, 0x66, 0xa5, 0x1b, 0xfd, 0xff,
    0x09, 0x25, 0xa5, 0x56, 0x37, 0x41, 0xd8, 0xaf,
    0x07, 0xa6, 0x12, 0xa8, 0x58, 0xc4, 0x42, 0x9c,
    0xce, 0x90, 0x6a, 0x9e, 0x7e, 0x04, 0x27, 0xe3,
    0xfa, 0x8e, 0xe5, 0xdc, 0xa8, 0x5a, 0xf7, 0xc9,
    0x0d, 0x23, 0x56, 0x8e, 0x46, 0x84, 0xe8, 0x34,
    0x83, 0x86, 0xca, 0xc1, 0xcd, 0xfe, 0x68, 0x00,
    0x67, 0x3f, 0x24, 0x3b, 0x50, 0x63, 0x21, 0x7f,
    0xba, 0xc6, 0xdb, 0xff, 0xf4, 0x3a, 0x10, 0xb6,
    0xb5, 0x09, 0x4d, 0x41, 0xff, 0xef, 0xc0, 0x84,
    0x48, 0x1b, 0x51, 0x87, 0xe6, 0x85, 0xf0, 0x1e,
    0xbd, 0x99, 0x0d, 0xd3, 0x98, 0xd0, 0xab, 0xd8,
    0x30, 0x2a, 0xd5, 0x74
  };

  /* Triggers crash in _asn1_get_objectid_der. */
  unsigned char der2[] = {
    0x30, 0x82, 0x04, 0x10, 0x30, 0x82, 0x03, 0x79,
    0xa0, 0x3, 0x2, 0x1, 0x2, 0x2, 0x1, 0x0, 0x30,
    0x11, 0x6, 0x84, 0x10, 0x0, 0x0, 0x0, 0x2a,
    0x86, 0x48, 0x86, 0xf7, 0xd, 0x1, 0x1, 0x4,
    0x5, 0x0, 0x30, 0x81, 0xbb, 0x31, 0xb, 0x30,
    0x9, 0x6, 0x3, 0x55, 0x4, 0x6, 0x13, 0x2,
    0x2d, 0x2d, 0x31, 0x12, 0x30, 0x10, 0x6,
    0x3, 0x55, 0x4, 0x8, 0x13, 0x9, 0x53,
    0x6f, 0x6d, 0x65, 0x53, 0x74, 0x61, 0x74,
    0x65, 0x31, 0x11, 0x30, 0xf, 0x6, 0x3,
    0x55, 0x4, 0x7, 0x13, 0x8, 0x53, 0x6f,
    0x6d, 0x65, 0x43, 0x69, 0x74, 0x79,
    0x31, 0x19, 0x30, 0x17, 0x6, 0x3, 0x55,
    0x4, 0xa, 0x13, 0x10, 0x53, 0x6f, 0x6d,
    0x65, 0x4f, 0x72, 0x67, 0x61, 0x6e, 0x69,
    0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x31,
    0x1f, 0x30, 0x1d, 0x6, 0x3, 0x55, 0x4, 0xb,
    0x13, 0x16, 0x53, 0x6f, 0x6d, 0x65, 0x4f,
    0x72, 0x67, 0x61, 0x6e, 0x69, 0x7a, 0x61,
    0x74, 0x69, 0x6f, 0x6e, 0x61, 0x6c, 0x55,
    0x6e, 0x69, 0x74, 0x31, 0x1e, 0x30, 0x1c,
    0x6, 0x3, 0x55, 0x4, 0x3, 0x13, 0x15, 0x6c,
    0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73,
    0x74, 0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x6c,
    0x64, 0x6f, 0x6d, 0x61, 0x69, 0x6e, 0x31,
    0x29, 0x30, 0x27, 0x6, 0x9, 0x2a, 0x86,
    0x48, 0x86, 0xf7, 0xd, 0x1, 0x9, 0x1, 0x16,
    0x1a, 0x72
  };

  /* Triggers crash in asn1_der_decoding. */
  unsigned char der3[] = {
    0x30, 0x82, 0x4, 0x10, 0x30, 0x82, 0x3, 0x79,
    0xa0, 0x3, 0x2, 0x1, 0x2, 0x2, 0x1, 0x0,
    0x30, 0x11, 0x6, 0x9, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0xd,
    0x1, 0x1, 0x4, 0x5, 0x84, 0x10, 0x0, 0x0, 0x0, 0x30, 0x81, 0xbb, 0x31,
    0xb, 0x30, 0x9, 0x6, 0x3, 0x55, 0x4, 0x6, 0x13, 0x2, 0x2d, 0x2d, 0x31,
    0x12, 0x30, 0x10, 0x6, 0x3, 0x55, 0x4, 0x8, 0x13, 0x9, 0x53, 0x6f, 0x6d,
    0x65, 0x53, 0x74, 0x61, 0x74, 0x65, 0x31, 0x11, 0x30, 0xf, 0x6, 0x3,
    0x55, 0x4, 0x7, 0x13, 0x8, 0x53, 0x6f, 0x6d, 0x65, 0x43, 0x69, 0x74,
    0x79, 0x31, 0x19, 0x30, 0x17, 0x6, 0x3, 0x55, 0x4, 0xa, 0x13, 0x10,
    0x53, 0x6f, 0x6d, 0x65, 0x4f, 0x72, 0x67, 0x61, 0x6e, 0x69, 0x7a, 0x61,
    0x74, 0x69, 0x6f, 0x6e, 0x31, 0x1f, 0x30, 0x1d, 0x6, 0x3, 0x55, 0x4,
    0xb, 0x13, 0x16, 0x53, 0x6f, 0x6d, 0x65, 0x4f, 0x72, 0x67, 0x61, 0x6e,
    0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x61, 0x6c, 0x55, 0x6e, 0x69,
    0x74, 0x31, 0x1e, 0x30, 0x1c, 0x6, 0x3, 0x55, 0x4, 0x3, 0x13, 0x15,
    0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x2e, 0x6c, 0x6f,
    0x63, 0x61, 0x6c, 0x64, 0x6f, 0x6d, 0x61, 0x69, 0x6e, 0x31, 0x29, 0x30,
    0x27, 0x6, 0x9, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0xd, 0x1, 0x9, 0x1, 0x16,
    0x1a, 0x72, 0x6f, 0x6f, 0x74, 0x40, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68,
    0x6f, 0x73, 0x74, 0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x64, 0x6f, 0x6d,
    0x61, 0x69, 0x6e, 0x30, 0x1e, 0x17, 0xd, 0x30, 0x34, 0x30, 0x32, 0x31,
    0x38, 0x32, 0x30, 0x30, 0x32, 0x33, 0x34, 0x5a, 0x17, 0xd, 0x30, 0x35,
    0x31, 0x31, 0x31, 0x37, 0x32, 0x30, 0x30, 0x32, 0x33, 0x34, 0x5a, 0x30,
    0x81, 0xbb, 0x31, 0xb, 0x30, 0x9, 0x6, 0x3, 0x55, 0x4, 0x6, 0x13, 0x2,
    0x2d, 0x2d, 0x31, 0x12, 0x30, 0x10, 0x6, 0x3, 0x55, 0x4, 0x8, 0x13, 0x9,
    0x53, 0x6f, 0x6d, 0x65, 0x53, 0x74, 0x61, 0x74, 0x65, 0x31, 0x11, 0x30,
    0xf, 0x6, 0x3, 0x55, 0x4, 0x7, 0x13, 0x8, 0x53, 0x6f, 0x6d, 0x65, 0x43,
    0x69, 0x74, 0x79, 0x31, 0x19, 0x30, 0x17, 0x6, 0x3, 0x55, 0x4, 0xa,
    0x13, 0x10, 0x53, 0x6f, 0x6d, 0x65, 0x4f, 0x72, 0x67, 0x61, 0x6e, 0x69,
    0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x31, 0x1f, 0x30, 0x1d, 0x6, 0x3,
    0x55, 0x4, 0xb, 0x13, 0x16, 0x53, 0x6f, 0x6d, 0x65, 0x4f, 0x72, 0x67,
    0x61, 0x6e, 0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x61, 0x6c, 0x55,
    0x6e, 0x69, 0x74, 0x31, 0x1e, 0x30, 0x1c, 0x6, 0x3, 0x55, 0x4, 0x3,
    0x13, 0x15, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74, 0x2e,
    0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x64, 0x6f, 0x6d, 0x61, 0x69, 0x6e, 0x31,
    0x29, 0x30, 0x27, 0x6, 0x9, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0xd, 0x1, 0x9,
    0x1, 0x16, 0x1a, 0x72, 0x6f, 0x6f, 0x74, 0x40, 0x6c, 0x6f, 0x63, 0x61,
    0x6c, 0x68, 0x6f, 0x73, 0x74, 0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x64,
    0x6f, 0x6d, 0x61, 0x69, 0x6e, 0x30, 0x81, 0x9f, 0x30, 0xd, 0x6, 0x9,
    0x2a, 0x86, 0x48, 0x86, 0xf7, 0xd, 0x1, 0x1, 0x1, 0x5, 0x0, 0x3, 0x81,
    0x8d, 0x0, 0x30, 0x81, 0x89, 0x2, 0x81, 0x81, 0x0, 0xda, 0x3d, 0xb7,
    0x66, 0x9a, 0x41, 0x4f, 0xca, 0x1d, 0xd1, 0xc4, 0x1f, 0xc9, 0x4c, 0xc6,
    0x76, 0x45, 0xc5, 0x8e, 0x2f, 0x3d, 0x45, 0xf5, 0x16, 0x9f, 0xb5, 0x22,
    0xb, 0x61, 0x60, 0xa4, 0x42, 0x42, 0x98, 0xae, 0x45, 0xe1, 0x4a, 0x17,
    0xb, 0x6e, 0xf7, 0x4e, 0xc0, 0x1e, 0xe7, 0x78, 0xd0, 0x80, 0xfc, 0xde,
    0xa, 0x96, 0x43, 0x13, 0xe4, 0xb5, 0xef, 0x47, 0xca, 0x8f, 0xb3, 0x13,
    0x92, 0x10, 0xc4, 0x2, 0x7b, 0xbb, 0x6c, 0x9f, 0x2b, 0x63, 0x65, 0xfa,
    0xac, 0xcb, 0xc9, 0x14, 0x68, 0x53, 0xd9, 0xe2, 0x9c, 0x57, 0x52, 0x23,
    0xb9, 0x4f, 0x92, 0xc0, 0xa0, 0xe3, 0xf5, 0x50, 0xb3, 0xc4, 0x5f, 0x4e,
    0x73, 0x9d, 0xe, 0xfd, 0x9c, 0x57, 0x8e, 0x4c, 0x13, 0xe0, 0x7a, 0x16,
    0x6b, 0x27, 0xc9, 0xac, 0xb3, 0x47, 0xb2, 0x3f, 0x8f, 0xe6, 0x1d, 0x0,
    0xc8, 0xaa, 0x6f, 0xdf, 0xcb, 0x2, 0x3, 0x1, 0x0, 0x1, 0xa3, 0x82, 0x1,
    0x1c, 0x30, 0x82, 0x1, 0x18, 0x30, 0x1d, 0x6, 0x3, 0x55, 0x1d, 0xe, 0x4,
    0x16, 0x4, 0x14, 0xe6, 0x30, 0x79, 0x2b, 0xe2, 0xcf, 0x4f, 0xa7, 0x40,
    0xa4, 0xb9, 0xa4, 0x1e, 0x95, 0x56, 0xe8, 0x94, 0xda, 0xd9, 0x15, 0x30,
    0x81, 0xe8, 0x6, 0x3, 0x55, 0x1d, 0x23, 0x4, 0x81, 0xe0, 0x30, 0x81,
    0xdd, 0x80, 0x14, 0xe6, 0x30, 0x79, 0x2b, 0xe2, 0xcf, 0x4f, 0xa7, 0x40,
    0xa4, 0xb9, 0xa4, 0x1e, 0x95, 0x56, 0xe8, 0x94, 0xda, 0xd9, 0x15, 0xa1,
    0x81, 0xc1, 0xa4, 0x81, 0xbe, 0x30, 0x81, 0xbb, 0x31, 0xb, 0x30, 0x9,
    0x6, 0x3, 0x55, 0x4, 0x6, 0x13, 0x2, 0x2d, 0x2d, 0x31, 0x12, 0x30, 0x10,
    0x6, 0x3, 0x55, 0x4, 0x8, 0x13, 0x9, 0x53, 0x6f, 0x6d, 0x65, 0x53, 0x74,
    0x61, 0x74, 0x65, 0x31, 0x11, 0x30, 0xf, 0x6, 0x3, 0x55, 0x4, 0x7, 0x13,
    0x8, 0x53, 0x6f, 0x6d, 0x65, 0x43, 0x69, 0x74, 0x79, 0x31, 0x19, 0x30,
    0x17, 0x6, 0x3, 0x55, 0x4, 0xa, 0x13, 0x10, 0x53, 0x6f, 0x6d, 0x65,
    0x4f, 0x72, 0x67, 0x61, 0x6e, 0x69, 0x7a, 0x61, 0x74, 0x69, 0x6f, 0x6e,
    0x31, 0x1f, 0x30, 0x1d, 0x6, 0x3, 0x55, 0x4, 0xb, 0x13, 0x16, 0x53,
    0x6f, 0x6d, 0x65, 0x4f, 0x72, 0x67, 0x61, 0x6e, 0x69, 0x7a, 0x61, 0x74,
    0x69, 0x6f, 0x6e, 0x61, 0x6c, 0x55, 0x6e, 0x69, 0x74, 0x31, 0x1e, 0x30,
    0x1c, 0x6, 0x3, 0x55, 0x4, 0x3, 0x13, 0x15, 0x6c, 0x6f, 0x63, 0x61,
    0x6c, 0x68, 0x6f, 0x73, 0x74, 0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x64,
    0x6f, 0x6d, 0x61, 0x69, 0x6e, 0x31, 0x29, 0x30, 0x27, 0x6, 0x9, 0x2a,
    0x86, 0x48, 0x86, 0xf7, 0xd, 0x1, 0x9, 0x1, 0x16, 0x1a, 0x72, 0x6f,
    0x6f, 0x74, 0x40, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73, 0x74,
    0x2e, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x64, 0x6f, 0x6d, 0x61, 0x69, 0x6e,
    0x82, 0x1, 0x0, 0x30, 0xc, 0x6, 0x3, 0x55, 0x1d, 0x13, 0x4, 0x5, 0x30,
    0x3, 0x1, 0x1, 0xff, 0x30, 0xd, 0x6, 0x9, 0x2a, 0x86, 0x48, 0x86, 0xf7,
    0xd, 0x1, 0x1, 0x4, 0x5, 0x0, 0x3, 0x81, 0x81, 0x0, 0xcd, 0xc9, 0x30,
    0x6d, 0x2, 0x65, 0x41, 0xea, 0xe, 0x46, 0x8, 0x6c, 0x2f, 0xd5, 0xa7,
    0xe4, 0x29, 0xd7, 0x3f, 0x18, 0x16, 0xd7, 0x4b, 0x6f, 0x9d, 0xc0, 0x5b,
    0xbf, 0x68, 0x7b, 0x2e, 0x66, 0xa5, 0x1b, 0xfd, 0xff, 0x9, 0x25, 0xa5,
    0x56, 0x37, 0x41, 0xd8, 0xaf, 0x7, 0xa6, 0x12, 0xa8, 0x58, 0xc4, 0x42,
    0x9c, 0xce, 0x90, 0x6a, 0x9e, 0x7e, 0x4, 0x27, 0xe3, 0xfa, 0x8e, 0xe5,
    0xdc, 0xa8, 0x5a, 0xf7, 0xc9, 0xd, 0x23, 0x56, 0x8e, 0x46, 0x84, 0xe8,
    0x34, 0x83, 0x86, 0xca, 0xc1, 0xcd, 0xfe, 0x68, 0x0, 0x67, 0x3f, 0x24,
    0x3b, 0x50, 0x63, 0x21, 0x7f, 0xba, 0xc6, 0xdb, 0xff, 0xf4, 0x3a, 0x10,
    0xb6, 0xb5, 0x9, 0x4d, 0x41, 0xff, 0xef, 0xc0, 0x84, 0x48, 0x1b, 0x51,
    0x87, 0xe6, 0x85, 0xf0, 0x1e, 0xbd, 0x99, 0xd, 0xd3, 0x98, 0xd0, 0xab,
    0xd8, 0x30, 0x2a, 0xd5, 0x74
  };
  gnutls_datum_t derCert = { der, sizeof (der) };
  gnutls_datum_t der2Cert = { der2, sizeof (der2) };
  gnutls_datum_t der3Cert = { der3, sizeof (der3) };
  gnutls_x509_crt_t cert;

  ret = gnutls_global_init ();
  if (ret < 0)
    fail ("init %d\n", ret);

  ret = gnutls_x509_crt_init (&cert);
  if (ret < 0)
    fail ("crt_init %d\n", ret);

  ret = gnutls_x509_crt_import (cert, &derCert, GNUTLS_X509_FMT_DER);
  if (ret != GNUTLS_E_ASN1_DER_ERROR)
    fail ("crt_import %d\n", ret);

  gnutls_x509_crt_deinit (cert);

  ret = gnutls_x509_crt_init (&cert);
  if (ret < 0)
    fail ("crt_init %d\n", ret);

  ret = gnutls_x509_crt_import (cert, &der2Cert, GNUTLS_X509_FMT_DER);
  if (ret != GNUTLS_E_ASN1_DER_ERROR)
    fail ("crt2_import %d\n", ret);

  gnutls_x509_crt_deinit (cert);

  ret = gnutls_x509_crt_init (&cert);
  if (ret < 0)
    fail ("crt_init %d\n", ret);

  ret = gnutls_x509_crt_import (cert, &der3Cert, GNUTLS_X509_FMT_DER);
  if (ret != GNUTLS_E_ASN1_DER_ERROR)
    fail ("crt3_import %d\n", ret);

  success ("done\n");

  gnutls_x509_crt_deinit (cert);

  gnutls_global_deinit ();
}
