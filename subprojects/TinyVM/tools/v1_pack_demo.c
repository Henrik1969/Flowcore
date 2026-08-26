#include <tinyvm/isa_v1.h>

#include <stdio.h>

static void id(char out[64],const char *value){snprintf(out,64,"%s",value);}
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s OUTPUT\n",argv[0]);return 2;}
    TinyvmConstant constants[]={{1,TINYVM_CARRIER_I32,19},{2,TINYVM_CARRIER_I32,23}};
    InstrWord code[]={{TV1_CONST,0,1,0},{TV1_CONST,1,2,0},{TV1_ADD,2,0,1},{TV1_CONVERT,3,2,TINYVM_CARRIER_I64},{TV1_RETURN,3,0,0}};
    TinyvmProvenance provenance[5]={0};for(size_t i=0;i<5;++i){provenance[i].instruction=i;provenance[i].operation=i+1;provenance[i].block=1;provenance[i].line=(uint32_t)(i+1);provenance[i].column=1;id(provenance[i].source,"flowtiny-v1-demo-source");id(provenance[i].derivation,"flowtiny-v1-demo-lowering");}
    TinyvmArtifactV2 a;tinyvm_artifact_v2_init(&a);a.isa_version=1;a.data_words=4;a.stack_words=16;id(a.artifact_id,"flowtiny-v1-demo-artifact");id(a.source_id,"flowtiny-v1-demo-source");id(a.target_policy_id,"tinyvm-portable");id(a.lowering_plan_id,"flowtiny-v1-demo-plan");id(a.optimization_id,"flowtiny-v1-demo-optimization");a.code=code;a.code_count=5;a.constants=constants;a.constant_count=2;a.provenance=provenance;a.provenance_count=5;char diagnostic[160];if(!tinyvm_artifact_v2_write(argv[1],&a,diagnostic,sizeof diagnostic)){fprintf(stderr,"flowtinyv1-pack-demo: %s\n",diagnostic);return 1;}return 0;
}
