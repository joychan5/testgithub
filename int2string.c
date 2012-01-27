#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main() {
    //time_t t = time(NULL);
    time_t t = 199;
    char s[50] = "";
    sprintf(s,"%lX", t);
    printf("int %ld\nhex:%s\n", t, s);
    return 0;
}


//asdf
