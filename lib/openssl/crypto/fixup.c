#include <debug.h>
#include <ossl_typ.h>

int BN_rand(BIGNUM *rnd, int bits, int top, int bottom)
{
	PANIC_UNIMPLEMENTED;
}

int	BN_pseudo_rand_range(BIGNUM *rnd, const BIGNUM *range) {
	PANIC_UNIMPLEMENTED;
}

int PKCS1_MGF1(unsigned char *mask, long len,
	const unsigned char *seed, long seedlen, const EVP_MD *dgst) {
	PANIC_UNIMPLEMENTED;
}

int RAND_bytes(unsigned char *buf, int num) {
	PANIC_UNIMPLEMENTED;
}

int hmac_pkey_meth = 0;
