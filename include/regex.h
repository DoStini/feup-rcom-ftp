#ifndef INCLUDE_REGEX_H_
#define INCLUDE_REGEX_H_

#include <regex.h>

int regmatch_to_string(const char* original, const regmatch_t match,
                       char** result);

#endif /* INCLUDE_REGEX_H_ */
