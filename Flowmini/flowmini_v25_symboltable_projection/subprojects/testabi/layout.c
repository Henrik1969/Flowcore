#include "flowmini_testabi.h"

#include <stddef.h>
#include <stdio.h>

int main(void) {
    printf("{\"format\":\"flowcore.abi_manifest\",\"version\":1,"
           "\"provider\":\"flowmini_testabi\",\"types\":["
           "{\"name\":\"Point\",\"size\":%zu,\"alignment\":%zu,"
           "\"fields\":[{\"name\":\"x\",\"type\":\"c_int\",\"offset\":%zu},"
           "{\"name\":\"y\",\"type\":\"c_int\",\"offset\":%zu}]}]}\n",
           sizeof(FlowminiTestAbiPoint), _Alignof(FlowminiTestAbiPoint),
           offsetof(FlowminiTestAbiPoint, x), offsetof(FlowminiTestAbiPoint, y));
    return 0;
}
