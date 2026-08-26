#include <tinyvm/artifact.h>
#include <stdio.h>
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s ARTIFACT\n",argv[0]);return 2;}
    TinyvmArtifact a; char d[160]; if(!tinyvm_artifact_read(argv[1],&a,d,sizeof d)){printf("{\"format\":\"flowtiny.validation_report\",\"version\":1,\"status\":\"invalid\",\"reason\":\"%s\"}\n",d);return 1;}
    printf("{\"format\":\"flowtiny.validation_report\",\"version\":1,\"status\":\"valid\",\"artifact_id\":\"%s\",\"source_id\":\"%s\",\"target\":\"%s\",\"isa_version\":%u,\"instructions\":%zu}\n",a.artifact_id,a.source_id,a.target,a.isa_version,a.code_count); tinyvm_artifact_destroy(&a); return 0;
}
