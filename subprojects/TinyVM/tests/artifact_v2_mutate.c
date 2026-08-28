#include <openssl/sha.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t g32(const uint8_t *p){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=(uint32_t)p[i]<<(8*i);return v;}
static uint64_t g64(const uint8_t *p){uint64_t v=0;for(unsigned i=0;i<8;++i)v|=(uint64_t)p[i]<<(8*i);return v;}
static void p32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8*i));}
static void p64(uint8_t *p,uint64_t v){for(unsigned i=0;i<8;++i)p[i]=(uint8_t)(v>>(8*i));}
static uint8_t *entry(uint8_t *b,uint32_t type){uint32_t n=g32(b+424);for(uint32_t i=0;i<n;++i){uint8_t *e=b+512+i*32;if(g32(e)==type)return e;}return NULL;}
int main(int argc,char **argv){
    if(argc!=4){fprintf(stderr,"usage: %s INPUT OUTPUT MUTATION\n",argv[0]);return 2;}
    FILE *f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long end=ftell(f);rewind(f);if(end<704){fclose(f);return 2;}size_t size=(size_t)end;uint8_t *b=malloc(size);if(!b||fread(b,1,size,f)!=size){fclose(f);free(b);return 2;}fclose(f);
    if(strcmp(argv[3],"identity-padding")==0)b[84]='X';
    else if(strcmp(argv[3],"duplicate-section")==0)p32(b+544,1);
    else if(strcmp(argv[3],"constant-bool")==0){uint8_t *e=entry(b,2);if(!e)return 2;p64(b+g64(e+8)+16,2);}
    else if(strcmp(argv[3],"import-reserved")==0){uint8_t *e=entry(b,5);if(!e)return 2;b[g64(e+8)+520]=1;}
    else if(strcmp(argv[3],"provenance-source")==0){uint8_t *e=entry(b,6);if(!e)return 2;b[g64(e+8)+40]='X';}
    else if(strcmp(argv[3],"padding")==0){uint8_t *left=entry(b,3),*right=entry(b,4);if(!left||!right||g64(left+8)+g64(left+16)>=g64(right+8))return 2;b[g64(left+8)+g64(left+16)]=1;}
    else{fprintf(stderr,"unknown mutation\n");free(b);return 2;}
    memset(b+384,0,32);SHA256(b,size,b+384);f=fopen(argv[2],"wb");bool ok=f&&fwrite(b,1,size,f)==size;if(f&&fclose(f))ok=false;free(b);return ok?0:2;
}
