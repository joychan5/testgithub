#include <stdlib.h>
#include <stdio.h>

#include <openssl/hmac.h>

int main( int argc, char **argv ){
	const EVP_MD *md;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	OpenSSL_add_all_digests();
	if( argc != 4 ) {
		printf("Usage: hmac digestname value key\n");
		exit(1);
	}
	const char *value = argv[2];
	const char *key   = argv[3];
	md = EVP_get_digestbyname(argv[1]);
	int md_len;
	HMAC( md, key, strlen(key), value, strlen(value), md_value, &md_len );
	int i = 0;
	printf("Digest is: ");
    for(i = 0; i < md_len; i++) printf("%02x", md_value[i]);
	printf("\n");
	return 0;
/*
	unsigned char *HMAC(const EVP_MD *evp_md, const void *key,
                      int key_len, const unsigned char *d, int n,
                      unsigned char *md, unsigned int *md_len);
*/
	
}
