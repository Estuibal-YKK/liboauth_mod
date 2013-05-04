#include <stdio.h>
#include <string.h>

#include "oauth.h" // base64 encode fn's.
#include "xmalloc.h"
#ifndef PSP
	#include "sha1.h"
	typedef SHA1Context SHA1Context_t;
#else
	#include <psputils.h>
	typedef SceKernelUtilsSha1Context SHA1Context_t;
	#define SHA1Reset(ctx) 				sceKernelUtilsSha1BlockInit(ctx)
	#define SHA1Input(ctx, key, len)	sceKernelUtilsSha1BlockUpdate(ctx, (u8 *)key, (u32)len)
	#define SHA1Result(ctx, digest) 	sceKernelUtilsSha1BlockResult(ctx, (u8 *)digest)
#endif


static
void hmac_sha1(const unsigned char *key, size_t keylen, const unsigned char *in, size_t inlen, unsigned char *resbuf)
{
	const int IPAD = 0x36;
	const int OPAD = 0x5c;

	SHA1Context_t inner;
	SHA1Context_t outer;
	SHA1Context_t keyhash;
	unsigned char tmpkey[20];
	unsigned char digest[20];
	unsigned char block[64];
	size_t i;

	if( keylen > 64 )
	{
		SHA1Reset(&keyhash);
		SHA1Input(&keyhash, key, keylen);
		SHA1Result(&keyhash, tmpkey);
		key = tmpkey;
		keylen = 20;
	}

	for( i = 0; i < sizeof(block); i++ )
	{
		block[i] = IPAD ^ (i < keylen ? key[i] : 0);
	}

	SHA1Reset(&inner);
	SHA1Input(&inner, block, 64);
	SHA1Input(&inner, in, inlen);
	SHA1Result(&inner, digest);

	for( i = 0; i < sizeof(block); i++ )
	{
		block[i] = OPAD ^ (i < keylen ? key[i] : 0);
	}

	SHA1Reset(&outer);
	SHA1Input(&outer, block, 64);
	SHA1Input(&outer, digest, 20);
	SHA1Result(&outer, resbuf);
}

char *oauth_sign_hmac_sha1_raw(const char *m, size_t ml, const char *k, size_t kl)
{
	unsigned char result[20];
	hmac_sha1((unsigned char *)k, kl, (unsigned char *)m, ml, result);
	return oauth_encode_base64(20, result);
}

char *oauth_sign_hmac_sha1(const char *m, const char *k)
{
	return oauth_sign_hmac_sha1_raw(m, strlen(m), k, strlen(k));
}


char *oauth_body_hash_file(char *filename)
{
	size_t len = 0;
	char data[4096];
	SHA1Context_t ctx;
	FILE *fp = NULL;
	unsigned char *digest;

#ifdef WIN32
	fp = fopen(filename, "rb");
#else
	fp = fopen(filename, "r");
#endif
	if( !fp ) return NULL;

	SHA1Reset(&ctx); // init sha1

	// read file
	while( (len = fread(data, sizeof(unsigned char), sizeof data, fp)) > 0 ) {
		SHA1Input(&ctx, data, len);
	}

	fclose(fp);

	digest = (unsigned char *)xmalloc(20 * sizeof(unsigned char));
	SHA1Result(&ctx, digest);
	return oauth_body_hash_encode(20, digest);
}

char *oauth_body_hash_data(size_t length, const char *data)
{
	SHA1Context_t ctx;
	unsigned char *digest;

	SHA1Reset(&ctx);

/*	while( length-- > 0 ) {
		SHA1Input(&ctx, *data++, 1);
	}
*/
	SHA1Input(&ctx, data, length);

	digest = (unsigned char *)xmalloc(20 * sizeof(unsigned char));
	SHA1Result(&ctx, digest);
	return oauth_body_hash_encode(20, digest);
}

char *oauth_sign_rsa_sha1(const char *m, const char *k)
{
	/* NOT RSA/PK11 support */
	return xstrdup("---RSA/PK11-is-not-supported-by-this-version-of-liboauth---");
}

int oauth_verify_rsa_sha1(const char *m, const char *c, const char *sig)
{
	/* NOT RSA/PK11 support */
	return -1; // mismatch , error
}
