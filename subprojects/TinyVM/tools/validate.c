#include <tinyvm/artifact.h>
#include <tinyvm/artifact_v2.h>
#include <stdio.h>
static unsigned version(const char *path){unsigned char h[12];FILE *f=fopen(path,"rb");if(!f)return 0;size_t n=fread(h,1,sizeof h,f);fclose(f);if(n!=sizeof h)return 0;return (unsigned)h[8]|(unsigned)h[9]<<8|(unsigned)h[10]<<16|(unsigned)h[11]<<24;}
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s ARTIFACT\n",argv[0]);return 2;}
    if(version(argv[1])==2){TinyvmArtifactV2 a;char d[160];if(!tinyvm_artifact_v2_read(argv[1],&a,d,sizeof d)){printf("{\"format\":\"flowtiny.validation_report\",\"version\":1,\"status\":\"invalid\",\"artifact_format\":2,\"reason\":\"%s\"}\n",d);return 1;}printf("{\"format\":\"flowtiny.validation_report\",\"version\":1,\"status\":\"valid\",\"artifact_format\":2,\"artifact_id\":\"%s\",\"source_id\":\"%s\",\"target_policy_id\":\"%s\",\"isa_version\":%u,\"instructions\":%zu,\"imports\":%zu}\n",a.artifact_id,a.source_id,a.target_policy_id,a.isa_version,a.code_count,a.import_count);tinyvm_artifact_v2_destroy(&a);return 0;}
    TinyvmArtifact a; char d[160]; if(!tinyvm_artifact_read(argv[1],&a,d,sizeof d)){printf("{\"format\":\"flowtiny.validation_report\",\"version\":1,\"status\":\"invalid\",\"reason\":\"%s\"}\n",d);return 1;}
    printf("{\"format\":\"flowtiny.validation_report\",\"version\":1,\"status\":\"valid\",\"artifact_id\":\"%s\",\"source_id\":\"%s\",\"target\":\"%s\",\"isa_version\":%u,\"instructions\":%zu}\n",a.artifact_id,a.source_id,a.target,a.isa_version,a.code_count); tinyvm_artifact_destroy(&a); return 0;
}
