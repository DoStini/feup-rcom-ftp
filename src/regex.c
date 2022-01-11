#include "include/regex.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "include/constants.h"

int regmatch_to_string(const char* original, const regmatch_t match,
                       char** result) {
    size_t size = match.rm_eo - match.rm_so;
    *result = malloc((size + 1) * sizeof(char));
    if ((*result) == NULL) {
        return MEMORY_ERR;
    }

    memcpy(*result, &original[match.rm_so], size);
    (*result)[size] = '\0';

    return OK;
}


