#include <tinyvm/artifact.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s ARTIFACT\n",argv[0]);return 2;}
    TinyvmArtifact a; char d[160]; if(!tinyvm_artifact_read(argv[1],&a,d,sizeof d)){fprintf(stderr,"flowtinyrun: %s\n",d);return 1;}
    vm_context ctx;vm_init(&ctx);ctx.pc=(size_t)a.entrypoint;ctx.regs[0]=19;ctx.regs[1]=23;
    const bool ok=vm_run_switch(&ctx,a.code,a.code_count);
    printf("{\"format\":\"flowtiny.execution_record\",\"version\":1,\"status\":\"%s\",\"artifact_id\":\"%s\",\"result\":%" PRId64 ",\"pc\":%zu}\n",ok?"completed":"faulted",a.artifact_id,ctx.regs[0],ctx.pc);
    tinyvm_artifact_destroy(&a);return ok?0:1;
}
