#include <iostream>
#include "sha.h"
typedef  char byte;

int main(){
	byte pbOutputBuffer[SHA256::DIGESTSIZE] = 0;
	byte pbData[] = "test";
	SHA256 hash;
	hash.Update(pbData, strlen(pbData));
	hash.Final(pbOutputBuffer);
	//SHA256().CalculateDigest(pbOutputBuffer, pbData, strlen(pbData));
	int i = 0;
	while( i < SHA256::DIGESTSIZE ){
		printf("%c",pbOutputBuffer[i]);
		i++;
	}
	printf("\n");
	return 0;
}
