#include <tinyvm/artifact_v2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void id(char out[64],const char *value){snprintf(out,64,"%s",value);}
static bool files_equal(const char *a,const char *b){
    FILE *left=fopen(a,"rb"),*right=fopen(b,"rb");if(!left||!right){if(left)fclose(left);if(right)fclose(right);return false;}
    bool equal=true;for(;;){unsigned char x[4096],y[4096];size_t nx=fread(x,1,sizeof x,left),ny=fread(y,1,sizeof y,right);if(nx!=ny||memcmp(x,y,nx)){equal=false;break;}if(nx<sizeof x)break;}fclose(left);fclose(right);return equal;
}
int main(int argc,char **argv){
    bool keep=false,no_import=false;
    if(argc<2||argc>4){fprintf(stderr,"usage: %s OUTPUT [--keep] [--no-import]\n",argv[0]);return 2;}
    for(int i=2;i<argc;++i){if(strcmp(argv[i],"--keep")==0)keep=true;else if(strcmp(argv[i],"--no-import")==0)no_import=true;else{fprintf(stderr,"unknown option: %s\n",argv[i]);return 2;}}
    char second[1024],diagnostic[200];snprintf(second,sizeof second,"%s.roundtrip",argv[1]);
    InstrWord code[]={{OP_ADD,0,1,0},{OP_HALT,0,0,0}};
    TinyvmConstant constants[]={{1,TINYVM_CARRIER_I1,1},{2,TINYVM_CARRIER_I32,42},{3,TINYVM_CARRIER_I64,99},{4,TINYVM_CARRIER_OPAQUE_HANDLE,0}};
    uint8_t hello[]={'h','e','l','l','o'};TinyvmString strings[]={{1,hello,sizeof hello}};
    TinyvmStorage storage[]={{1,4096,16,2}};
    TinyvmImport imports[1]={0};imports[0].id=1;id(imports[0].contract,"libc-abs-v1");id(imports[0].library,"libc.so.6");id(imports[0].convention,"c");id(imports[0].symbol,"abs");id(imports[0].effect,"pure");id(imports[0].parameters,"c_int");id(imports[0].result,"c_int");id(imports[0].evidence,"binding-evidence-1");
    TinyvmProvenance provenance[2]={0};for(size_t i=0;i<2;++i){provenance[i].instruction=i;provenance[i].operation=i+1;provenance[i].block=1;provenance[i].line=(uint32_t)(10+i);provenance[i].column=3;id(provenance[i].source,"source-identity-1");id(provenance[i].derivation,"lowering-derivation-1");}
    TinyvmArtifactV2 a;tinyvm_artifact_v2_init(&a);id(a.artifact_id,"artifact-identity-1");id(a.source_id,"source-identity-1");id(a.target_policy_id,"tinyvm-portable");id(a.lowering_plan_id,"lowering-plan-1");id(a.optimization_id,"optimization-1");a.code=code;a.code_count=2;a.constants=constants;a.constant_count=4;a.strings=strings;a.string_count=1;a.storage=storage;a.storage_count=1;a.imports=no_import?NULL:imports;a.import_count=no_import?0:1;a.provenance=provenance;a.provenance_count=2;
    if(!tinyvm_artifact_v2_write(argv[1],&a,diagnostic,sizeof diagnostic)){fprintf(stderr,"write: %s\n",diagnostic);return 1;}
    TinyvmArtifactV2 read;tinyvm_artifact_v2_init(&read);if(!tinyvm_artifact_v2_read(argv[1],&read,diagnostic,sizeof diagnostic)){fprintf(stderr,"read: %s\n",diagnostic);return 1;}
    if(!tinyvm_artifact_v2_write(second,&read,diagnostic,sizeof diagnostic)){fprintf(stderr,"rewrite: %s\n",diagnostic);tinyvm_artifact_v2_destroy(&read);return 1;}
    bool equal=files_equal(argv[1],second);tinyvm_artifact_v2_destroy(&read);if(!keep)remove(argv[1]);remove(second);if(!equal){fputs("v2 round-trip was not byte deterministic\n",stderr);return 1;}puts("TinyVM artifact v2 deterministic round-trip passed");return 0;
}
