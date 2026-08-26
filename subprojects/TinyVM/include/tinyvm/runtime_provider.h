#pragma once

#include <tinyvm/isa_v1.h>

typedef struct {
    const char *policy_path;
    size_t argument_count;
    const char *const *arguments;
} TinyvmRuntimeProvider;

bool tinyvm_runtime_provider_preflight(const TinyvmRuntimeProvider *provider,
                                       const TinyvmArtifactV2 *artifact,
                                       const char **fault);

bool tinyvm_runtime_provider_resolve(void *user,
                                     const TinyvmArtifactV2 *artifact,
                                     const TinyvmImport *import,
                                     const TinyvmValue *arguments,
                                     size_t argument_count,
                                     TinyvmValue *result,
                                     const char **fault);
