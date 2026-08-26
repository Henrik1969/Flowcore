#include <tinyvm/artifact.h>
#include <tinyvm/artifact_v2.h>
#include <tinyvm/isa_v1.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
static unsigned version(const char *path){unsigned char h[12];FILE *f=fopen(path,"rb");if(!f)return 0;size_t n=fread(h,1,sizeof h,f);fclose(f);if(n!=sizeof h)return 0;return (unsigned)h[8]|(unsigned)h[9]<<8|(unsigned)h[10]<<16|(unsigned)h[11]<<24;}
int main(int argc,char **argv){
    if(argc<2){fprintf(stderr,"usage: %s ARTIFACT [PROGRAM-ARGUMENT ...]\n",argv[0]);return 2;}
    if(version(argv[1])==2){TinyvmArtifactV2 a;char d[160];if(!tinyvm_artifact_v2_read(argv[1],&a,d,sizeof d)){fprintf(stderr,"flowtinyrun: %s\n",d);return 1;}if(a.import_count){fprintf(stderr,"flowtinyrun: artifact imports require an authorized runtime provider\n");tinyvm_artifact_v2_destroy(&a);return 2;}if(a.isa_version==1||a.isa_version==2){TinyvmIsaV1Context ctx;if(!tinyvm_isa_v1_context_init(&ctx,(size_t)a.data_words,UINT64_C(10000000))){tinyvm_artifact_v2_destroy(&a);return 1;}ctx.argument_count=(size_t)(argc-1);ctx.arguments=(const char *const *)&argv[1];const bool ok=tinyvm_isa_v1_run_switch(&a,&ctx);printf("{\"format\":\"flowtiny.execution_record\",\"version\":1,\"status\":\"%s\",\"artifact_format\":2,\"artifact_id\":\"%s\",\"target_policy_id\":\"%s\",\"carrier\":%u,\"result\":%" PRIu64 ",\"pc\":%" PRIu64 ",\"trap\":%u}\n",ok?"completed":"faulted",a.artifact_id,a.target_policy_id,ctx.result.carrier,ctx.result.bits,ctx.pc,ctx.trap);tinyvm_isa_v1_context_destroy(&ctx);tinyvm_artifact_v2_destroy(&a);return ok?0:1;}vm_context ctx;vm_init(&ctx);ctx.pc=(size_t)a.entrypoint;ctx.regs[0]=19;ctx.regs[1]=23;const bool ok=vm_run_switch(&ctx,a.code,a.code_count);printf("{\"format\":\"flowtiny.execution_record\",\"version\":1,\"status\":\"%s\",\"artifact_format\":2,\"artifact_id\":\"%s\",\"target_policy_id\":\"%s\",\"result\":%" PRId64 ",\"pc\":%zu}\n",ok?"completed":"faulted",a.artifact_id,a.target_policy_id,ctx.regs[0],ctx.pc);tinyvm_artifact_v2_destroy(&a);return ok?0:1;}
    TinyvmArtifact a; char d[160]; if(!tinyvm_artifact_read(argv[1],&a,d,sizeof d)){fprintf(stderr,"flowtinyrun: %s\n",d);return 1;}
    vm_context ctx;vm_init(&ctx);ctx.pc=(size_t)a.entrypoint;ctx.regs[0]=19;ctx.regs[1]=23;
    const bool ok=vm_run_switch(&ctx,a.code,a.code_count);
    printf("{\"format\":\"flowtiny.execution_record\",\"version\":1,\"status\":\"%s\",\"artifact_id\":\"%s\",\"result\":%" PRId64 ",\"pc\":%zu}\n",ok?"completed":"faulted",a.artifact_id,ctx.regs[0],ctx.pc);
    tinyvm_artifact_destroy(&a);return ok?0:1;
}
