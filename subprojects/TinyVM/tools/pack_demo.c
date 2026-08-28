#include <tinyvm/artifact.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s OUTPUT\n",argv[0]);return 2;}
    InstrWord code[]={{OP_ADD,0,1,0},{OP_HALT,0,0,0}}; TinyvmArtifact a; tinyvm_artifact_init(&a);
    snprintf(a.artifact_id,64,"demo-artifact-v1"); snprintf(a.source_id,64,"demo-source-v1"); snprintf(a.target,64,"tinyvm-portable");
    snprintf(a.lowering_plan_id,64,"demo-lowering-v1"); snprintf(a.optimization_id,64,"demo-optimization-v1"); a.code=code;a.code_count=2;
    char d[160]; if(!tinyvm_artifact_write(argv[1],&a,d,sizeof d)){fprintf(stderr,"flowtinypack-demo: %s\n",d);return 1;} return 0;
}
