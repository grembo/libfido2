/*
 * Copyright (c) 2019 Yubico AB. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mutator_aux.h"
#include "fido.h"

#include "../openbsd-compat/openbsd-compat.h"

#define TAG_TYPE	0x01
#define TAG_CDH		0x02	/* client data hash */
#define TAG_RP_ID	0x03
#define TAG_RP_NAME	0x04
#define TAG_AUTHDATA	0x05
#define TAG_EXT		0x06
#define TAG_RK		0x07
#define TAG_UV		0x08
#define TAG_X509	0x09
#define TAG_SIG		0x0a
#define TAG_FMT		0x0b

struct param {
	bool		type;
	struct blob	cdh;
	char		rp_id[MAXSTR];
	char		rp_name[MAXSTR];
	struct blob	authdata;
	int		ext;
	bool		rk;
	bool		uv;
	struct blob	x509;
	struct blob	sig;
	bool		fmt;
};

static const uint8_t dummy_cdh[32] = {
	0xf9, 0x64, 0x57, 0xe7, 0x2d, 0x97, 0xf6, 0xbb,
	0xdd, 0xd7, 0xfb, 0x06, 0x37, 0x62, 0xea, 0x26,
	0x20, 0x44, 0x8e, 0x69, 0x7c, 0x03, 0xf2, 0x31,
	0x2f, 0x99, 0xdc, 0xaf, 0x3e, 0x8a, 0x91, 0x6b,
};

static const uint8_t dummy_authdata[198] = {
	0x58, 0xc4, 0x49, 0x96, 0x0d, 0xe5, 0x88, 0x0e,
	0x8c, 0x68, 0x74, 0x34, 0x17, 0x0f, 0x64, 0x76,
	0x60, 0x5b, 0x8f, 0xe4, 0xae, 0xb9, 0xa2, 0x86,
	0x32, 0xc7, 0x99, 0x5c, 0xf3, 0xba, 0x83, 0x1d,
	0x97, 0x63, 0x41, 0x00, 0x00, 0x00, 0x00, 0xf8,
	0xa0, 0x11, 0xf3, 0x8c, 0x0a, 0x4d, 0x15, 0x80,
	0x06, 0x17, 0x11, 0x1f, 0x9e, 0xdc, 0x7d, 0x00,
	0x40, 0x53, 0xfb, 0xdf, 0xaa, 0xce, 0x63, 0xde,
	0xc5, 0xfe, 0x47, 0xe6, 0x52, 0xeb, 0xf3, 0x5d,
	0x53, 0xa8, 0xbf, 0x9d, 0xd6, 0x09, 0x6b, 0x5e,
	0x7f, 0xe0, 0x0d, 0x51, 0x30, 0x85, 0x6a, 0xda,
	0x68, 0x70, 0x85, 0xb0, 0xdb, 0x08, 0x0b, 0x83,
	0x2c, 0xef, 0x44, 0xe2, 0x36, 0x88, 0xee, 0x76,
	0x90, 0x6e, 0x7b, 0x50, 0x3e, 0x9a, 0xa0, 0xd6,
	0x3c, 0x34, 0xe3, 0x83, 0xe7, 0xd1, 0xbd, 0x9f,
	0x25, 0xa5, 0x01, 0x02, 0x03, 0x26, 0x20, 0x01,
	0x21, 0x58, 0x20, 0x17, 0x5b, 0x27, 0xa6, 0x56,
	0xb2, 0x26, 0x0c, 0x26, 0x0c, 0x55, 0x42, 0x78,
	0x17, 0x5d, 0x4c, 0xf8, 0xa2, 0xfd, 0x1b, 0xb9,
	0x54, 0xdf, 0xd5, 0xeb, 0xbf, 0x22, 0x64, 0xf5,
	0x21, 0x9a, 0xc6, 0x22, 0x58, 0x20, 0x87, 0x5f,
	0x90, 0xe6, 0xfd, 0x71, 0x27, 0x9f, 0xeb, 0xe3,
	0x03, 0x44, 0xbc, 0x8d, 0x49, 0xc6, 0x1c, 0x31,
	0x3b, 0x72, 0xae, 0xd4, 0x53, 0xb1, 0xfe, 0x5d,
	0xe1, 0x30, 0xfc, 0x2b, 0x1e, 0xd2,
};

static const uint8_t dummy_x509[742] = {
	0x30, 0x82, 0x02, 0xe2, 0x30, 0x81, 0xcb, 0x02,
	0x01, 0x01, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86,
	0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05,
	0x00, 0x30, 0x1d, 0x31, 0x1b, 0x30, 0x19, 0x06,
	0x03, 0x55, 0x04, 0x03, 0x13, 0x12, 0x59, 0x75,
	0x62, 0x69, 0x63, 0x6f, 0x20, 0x55, 0x32, 0x46,
	0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x43, 0x41,
	0x30, 0x1e, 0x17, 0x0d, 0x31, 0x34, 0x30, 0x35,
	0x31, 0x35, 0x31, 0x32, 0x35, 0x38, 0x35, 0x34,
	0x5a, 0x17, 0x0d, 0x31, 0x34, 0x30, 0x36, 0x31,
	0x34, 0x31, 0x32, 0x35, 0x38, 0x35, 0x34, 0x5a,
	0x30, 0x1d, 0x31, 0x1b, 0x30, 0x19, 0x06, 0x03,
	0x55, 0x04, 0x03, 0x13, 0x12, 0x59, 0x75, 0x62,
	0x69, 0x63, 0x6f, 0x20, 0x55, 0x32, 0x46, 0x20,
	0x54, 0x65, 0x73, 0x74, 0x20, 0x45, 0x45, 0x30,
	0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48,
	0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86,
	0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42,
	0x00, 0x04, 0xdb, 0x0a, 0xdb, 0xf5, 0x21, 0xc7,
	0x5c, 0xce, 0x63, 0xdc, 0xa6, 0xe1, 0xe8, 0x25,
	0x06, 0x0d, 0x94, 0xe6, 0x27, 0x54, 0x19, 0x4f,
	0x9d, 0x24, 0xaf, 0x26, 0x1a, 0xbe, 0xad, 0x99,
	0x44, 0x1f, 0x95, 0xa3, 0x71, 0x91, 0x0a, 0x3a,
	0x20, 0xe7, 0x3e, 0x91, 0x5e, 0x13, 0xe8, 0xbe,
	0x38, 0x05, 0x7a, 0xd5, 0x7a, 0xa3, 0x7e, 0x76,
	0x90, 0x8f, 0xaf, 0xe2, 0x8a, 0x94, 0xb6, 0x30,
	0xeb, 0x9d, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86,
	0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b, 0x05,
	0x00, 0x03, 0x82, 0x02, 0x01, 0x00, 0x95, 0x40,
	0x6b, 0x50, 0x61, 0x7d, 0xad, 0x84, 0xa3, 0xb4,
	0xeb, 0x88, 0x0f, 0xe3, 0x30, 0x0f, 0x2d, 0xa2,
	0x0a, 0x00, 0xd9, 0x25, 0x04, 0xee, 0x72, 0xfa,
	0x67, 0xdf, 0x58, 0x51, 0x0f, 0x0b, 0x47, 0x02,
	0x9c, 0x3e, 0x41, 0x29, 0x4a, 0x93, 0xac, 0x29,
	0x85, 0x89, 0x2d, 0xa4, 0x7a, 0x81, 0x32, 0x28,
	0x57, 0x71, 0x01, 0xef, 0xa8, 0x42, 0x88, 0x16,
	0x96, 0x37, 0x91, 0xd5, 0xdf, 0xe0, 0x8f, 0xc9,
	0x3c, 0x8d, 0xb0, 0xcd, 0x89, 0x70, 0x82, 0xec,
	0x79, 0xd3, 0xc6, 0x78, 0x73, 0x29, 0x32, 0xe5,
	0xab, 0x6c, 0xbd, 0x56, 0x9f, 0xd5, 0x45, 0x91,
	0xce, 0xc1, 0xdd, 0x8d, 0x64, 0xdc, 0xe9, 0x9c,
	0x1f, 0x5e, 0x3c, 0xd2, 0xaf, 0x51, 0xa5, 0x82,
	0x18, 0xaf, 0xe0, 0x37, 0xe7, 0x32, 0x9e, 0x76,
	0x05, 0x77, 0x02, 0x7b, 0xe6, 0x24, 0xa0, 0x31,
	0x56, 0x1b, 0xfd, 0x19, 0xc5, 0x71, 0xd3, 0xf0,
	0x9e, 0xc0, 0x73, 0x05, 0x4e, 0xbc, 0x85, 0xb8,
	0x53, 0x9e, 0xef, 0xc5, 0xbc, 0x9c, 0x56, 0xa3,
	0xba, 0xd9, 0x27, 0x6a, 0xbb, 0xa9, 0x7a, 0x40,
	0xd7, 0x47, 0x8b, 0x55, 0x72, 0x6b, 0xe3, 0xfe,
	0x28, 0x49, 0x71, 0x24, 0xf4, 0x8f, 0xf4, 0x20,
	0x81, 0xea, 0x38, 0xff, 0x7c, 0x0a, 0x4f, 0xdf,
	0x02, 0x82, 0x39, 0x81, 0x82, 0x3b, 0xca, 0x09,
	0xdd, 0xca, 0xaa, 0x0f, 0x27, 0xf5, 0xa4, 0x83,
	0x55, 0x6c, 0x9a, 0x39, 0x9b, 0x15, 0x3a, 0x16,
	0x63, 0xdc, 0x5b, 0xf9, 0xac, 0x5b, 0xbc, 0xf7,
	0x9f, 0xbe, 0x0f, 0x8a, 0xa2, 0x3c, 0x31, 0x13,
	0xa3, 0x32, 0x48, 0xca, 0x58, 0x87, 0xf8, 0x7b,
	0xa0, 0xa1, 0x0a, 0x6a, 0x60, 0x96, 0x93, 0x5f,
	0x5d, 0x26, 0x9e, 0x63, 0x1d, 0x09, 0xae, 0x9a,
	0x41, 0xe5, 0xbd, 0x08, 0x47, 0xfe, 0xe5, 0x09,
	0x9b, 0x20, 0xfd, 0x12, 0xe2, 0xe6, 0x40, 0x7f,
	0xba, 0x4a, 0x61, 0x33, 0x66, 0x0d, 0x0e, 0x73,
	0xdb, 0xb0, 0xd5, 0xa2, 0x9a, 0x9a, 0x17, 0x0d,
	0x34, 0x30, 0x85, 0x6a, 0x42, 0x46, 0x9e, 0xff,
	0x34, 0x8f, 0x5f, 0x87, 0x6c, 0x35, 0xe7, 0xa8,
	0x4d, 0x35, 0xeb, 0xc1, 0x41, 0xaa, 0x8a, 0xd2,
	0xda, 0x19, 0xaa, 0x79, 0xa2, 0x5f, 0x35, 0x2c,
	0xa0, 0xfd, 0x25, 0xd3, 0xf7, 0x9d, 0x25, 0x18,
	0x2d, 0xfa, 0xb4, 0xbc, 0xbb, 0x07, 0x34, 0x3c,
	0x8d, 0x81, 0xbd, 0xf4, 0xe9, 0x37, 0xdb, 0x39,
	0xe9, 0xd1, 0x45, 0x5b, 0x20, 0x41, 0x2f, 0x2d,
	0x27, 0x22, 0xdc, 0x92, 0x74, 0x8a, 0x92, 0xd5,
	0x83, 0xfd, 0x09, 0xfb, 0x13, 0x9b, 0xe3, 0x39,
	0x7a, 0x6b, 0x5c, 0xfa, 0xe6, 0x76, 0x9e, 0xe0,
	0xe4, 0xe3, 0xef, 0xad, 0xbc, 0xfd, 0x42, 0x45,
	0x9a, 0xd4, 0x94, 0xd1, 0x7e, 0x8d, 0xa7, 0xd8,
	0x05, 0xd5, 0xd3, 0x62, 0xcf, 0x15, 0xcf, 0x94,
	0x7d, 0x1f, 0x5b, 0x58, 0x20, 0x44, 0x20, 0x90,
	0x71, 0xbe, 0x66, 0xe9, 0x9a, 0xab, 0x74, 0x32,
	0x70, 0x53, 0x1d, 0x69, 0xed, 0x87, 0x66, 0xf4,
	0x09, 0x4f, 0xca, 0x25, 0x30, 0xc2, 0x63, 0x79,
	0x00, 0x3c, 0xb1, 0x9b, 0x39, 0x3f, 0x00, 0xe0,
	0xa8, 0x88, 0xef, 0x7a, 0x51, 0x5b, 0xe7, 0xbd,
	0x49, 0x64, 0xda, 0x41, 0x7b, 0x24, 0xc3, 0x71,
	0x22, 0xfd, 0xd1, 0xd1, 0x20, 0xb3, 0x3f, 0x97,
	0xd3, 0x97, 0xb2, 0xaa, 0x18, 0x1c, 0x9e, 0x03,
	0x77, 0x7b, 0x5b, 0x7e, 0xf9, 0xa3, 0xa0, 0xd6,
	0x20, 0x81, 0x2c, 0x38, 0x8f, 0x9d, 0x25, 0xde,
	0xe9, 0xc8, 0xf5, 0xdd, 0x6a, 0x47, 0x9c, 0x65,
	0x04, 0x5a, 0x56, 0xe6, 0xc2, 0xeb, 0xf2, 0x02,
	0x97, 0xe1, 0xb9, 0xd8, 0xe1, 0x24, 0x76, 0x9f,
	0x23, 0x62, 0x39, 0x03, 0x4b, 0xc8, 0xf7, 0x34,
	0x07, 0x49, 0xd6, 0xe7, 0x4d, 0x9a,
};

static const uint8_t dummy_sig[70] = {
	0x30, 0x44, 0x02, 0x20, 0x54, 0x92, 0x28, 0x3b,
	0x83, 0x33, 0x47, 0x56, 0x68, 0x79, 0xb2, 0x0c,
	0x84, 0x80, 0xcc, 0x67, 0x27, 0x8b, 0xfa, 0x48,
	0x43, 0x0d, 0x3c, 0xb4, 0x02, 0x36, 0x87, 0x97,
	0x3e, 0xdf, 0x2f, 0x65, 0x02, 0x20, 0x1b, 0x56,
	0x17, 0x06, 0xe2, 0x26, 0x0f, 0x6a, 0xe9, 0xa9,
	0x70, 0x99, 0x62, 0xeb, 0x3a, 0x04, 0x1a, 0xc4,
	0xa7, 0x03, 0x28, 0x56, 0x7c, 0xed, 0x47, 0x08,
	0x68, 0x73, 0x6a, 0xb6, 0x89, 0x0d,
};

static const char dummy_rp_id[] = "localhost";
static const char dummy_rp_name[] = "sweet home localhost";

int    LLVMFuzzerTestOneInput(const uint8_t *, size_t);
size_t LLVMFuzzerCustomMutator(uint8_t *, size_t, size_t, unsigned int);
size_t LLVMFuzzerMutate(uint8_t *, size_t, size_t);

static int
deserialize(const uint8_t *data, size_t size, struct param *p) NO_MSAN
{
	fido_cred_t *cred = NULL;

	if (deserialize_bool(TAG_TYPE, (void *)&data, &size, &p->type) < 0 ||
	    deserialize_blob(TAG_CDH, (void *)&data, &size, &p->cdh) < 0 ||
	    deserialize_string(TAG_RP_ID, (void *)&data, &size, p->rp_id) < 0 ||
	    deserialize_string(TAG_RP_NAME, (void *)&data, &size, p->rp_name) < 0 ||
	    deserialize_blob(TAG_AUTHDATA, (void *)&data, &size, &p->authdata) < 0 ||
	    deserialize_int(TAG_EXT, (void *)&data, &size, &p->ext) < 0 ||
	    deserialize_bool(TAG_RK, (void *)&data, &size, &p->rk) < 0 ||
	    deserialize_bool(TAG_UV, (void *)&data, &size, &p->uv) < 0 ||
	    deserialize_blob(TAG_X509, (void *)&data, &size, &p->x509) < 0 ||
	    deserialize_blob(TAG_SIG, (void *)&data, &size, &p->sig) < 0 ||
	    deserialize_bool(TAG_FMT, (void *)&data, &size, &p->fmt) < 0)
		return (-1);

#ifndef WITH_MSAN
	if ((cred = fido_cred_new()) == NULL)
		return (-1);

	if (p->type)
		fido_cred_set_type(cred, COSE_ES256);
	else
		fido_cred_set_type(cred, COSE_RS256);

	if (fido_cred_set_authdata(cred, p->authdata.body,
	    p->authdata.len) != FIDO_OK) {
		fido_cred_free(&cred);
		return (-1);
	}
#endif

	fido_cred_free(&cred);

	return (0);
}

static size_t
serialize(uint8_t *ptr, size_t len, const struct param *p)
{
	const size_t max = len;

#ifndef WITH_MSAN
	fido_cred_t *cred = NULL;

	if ((cred = fido_cred_new()) == NULL)
		return (0);

	if (p->type)
		fido_cred_set_type(cred, COSE_ES256);
	else
		fido_cred_set_type(cred, COSE_RS256);

	if (fido_cred_set_authdata(cred, p->authdata.body,
	    p->authdata.len) != FIDO_OK) {
		fido_cred_free(&cred);
		return (0);
	}

	fido_cred_free(&cred);
#endif

	if (serialize_bool(TAG_TYPE, &ptr, &len, p->type) < 0 ||
	    serialize_blob(TAG_CDH, &ptr, &len, &p->cdh) < 0 ||
	    serialize_string(TAG_RP_ID, &ptr, &len, p->rp_id) < 0 ||
	    serialize_string(TAG_RP_NAME, &ptr, &len, p->rp_name) < 0 ||
	    serialize_blob(TAG_AUTHDATA, &ptr, &len, &p->authdata) < 0 ||
	    serialize_int(TAG_EXT, &ptr, &len, p->ext) < 0 ||
	    serialize_bool(TAG_RK, &ptr, &len, p->rk) < 0 ||
	    serialize_bool(TAG_UV, &ptr, &len, p->uv) < 0 ||
	    serialize_blob(TAG_X509, &ptr, &len, &p->x509) < 0 ||
	    serialize_blob(TAG_SIG, &ptr, &len, &p->sig) < 0 ||
	    serialize_bool(TAG_FMT, &ptr, &len, p->fmt) < 0)
		return (0);

	return (max - len);
}

static void
consume(const uint8_t *ptr, size_t len)
{
	volatile uint8_t x = 0;

	while (len--)
		x ^= *ptr++;
}
 
int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	struct param	 p;
	fido_cred_t	*cred = NULL;

	memset(&p, 0, sizeof(p));

	if (deserialize(data, size, &p) < 0)
		return (0);

	fido_init(0);

	if ((cred = fido_cred_new()) == NULL)
		return (0);

	if (p.type)
		fido_cred_set_type(cred, COSE_ES256);
	else
		fido_cred_set_type(cred, COSE_RS256);

	if (p.fmt)
		fido_cred_set_fmt(cred, "packed");
	else
		fido_cred_set_fmt(cred, "fido-u2f");

	fido_cred_set_clientdata_hash(cred, p.cdh.body, p.cdh.len);
	fido_cred_set_rp(cred, p.rp_id, p.rp_name);
	fido_cred_set_authdata(cred, p.authdata.body, p.authdata.len);
	fido_cred_set_extensions(cred, p.ext);
	fido_cred_set_options(cred, p.rk, p.uv);
	fido_cred_set_x509(cred, p.x509.body, p.x509.len);
	fido_cred_set_sig(cred, p.sig.body, p.sig.len);

	fido_cred_verify(cred);

	consume(fido_cred_pubkey_ptr(cred), fido_cred_pubkey_len(cred));
	consume(fido_cred_id_ptr(cred), fido_cred_id_len(cred));

	fido_cred_free(&cred);

	return (0);
}

size_t
LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t maxsize,
    unsigned int seed)
{
	struct param	dummy;
	struct param	p;
	uint8_t		blob[4096];
	size_t		blob_len;
	size_t		n;

	(void)seed;

	memset(&p, 0, sizeof(p));

	if (deserialize(data, size, &p) < 0) {
		memset(&dummy, 0, sizeof(dummy));
		dummy.type = COSE_ES256;
		dummy.cdh.len = sizeof(dummy_cdh);
		memcpy(&dummy.cdh.body, &dummy_cdh, dummy.cdh.len);
		strlcpy(dummy.rp_id, dummy_rp_id, sizeof(dummy.rp_id));
		strlcpy(dummy.rp_name, dummy_rp_name, sizeof(dummy.rp_name));
		dummy.authdata.len = sizeof(dummy_authdata);
		memcpy(&dummy.authdata.body, &dummy_authdata, dummy.authdata.len);
		dummy.ext = FIDO_EXT_HMAC_SECRET;
		dummy.rk = false;
		dummy.uv = false;
		dummy.x509.len = sizeof(dummy_x509);
		memcpy(&dummy.x509.body, &dummy_x509, dummy.x509.len);
		dummy.sig.len = sizeof(dummy_sig);
		memcpy(&dummy.sig.body, &dummy_sig, dummy.sig.len);
		dummy.fmt = true;

		blob_len = serialize(blob, sizeof(blob), &dummy);
		assert(blob_len != 0);

		if (blob_len > maxsize) {
			memcpy(data, blob, maxsize);
			return (maxsize);
		}

		memcpy(data, blob, blob_len);
		return (blob_len);
	}

	LLVMFuzzerMutate((uint8_t *)&p.type, sizeof(p.type), sizeof(p.type));
	LLVMFuzzerMutate((uint8_t *)&p.ext, sizeof(p.ext), sizeof(p.ext));
	LLVMFuzzerMutate((uint8_t *)&p.rk, sizeof(p.rk), sizeof(p.rk));
	LLVMFuzzerMutate((uint8_t *)&p.uv, sizeof(p.uv), sizeof(p.uv));
	LLVMFuzzerMutate((uint8_t *)&p.fmt, sizeof(p.fmt), sizeof(p.fmt));

	p.authdata.len = LLVMFuzzerMutate((uint8_t *)p.authdata.body,
	    p.authdata.len, sizeof(p.authdata.body));
	p.cdh.len = LLVMFuzzerMutate((uint8_t *)p.cdh.body, p.cdh.len,
	    sizeof(p.cdh.body));
	p.x509.len = LLVMFuzzerMutate((uint8_t *)p.x509.body, p.x509.len,
	    sizeof(p.x509.body));
	p.sig.len = LLVMFuzzerMutate((uint8_t *)p.sig.body, p.sig.len,
	    sizeof(p.sig.body));

	n = LLVMFuzzerMutate((uint8_t *)p.rp_id, strlen(p.rp_id), MAXSTR - 1);
	p.rp_id[n] = '\0';
	n = LLVMFuzzerMutate((uint8_t *)p.rp_name, strlen(p.rp_name), MAXSTR - 1);
	p.rp_name[n] = '\0';

	if ((blob_len = serialize(blob, sizeof(blob), &p)) == 0 ||
	    blob_len > maxsize)
		return (0);

	memcpy(data, blob, blob_len);

	return (blob_len);
}
