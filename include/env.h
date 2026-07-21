#ifndef ENV_H
#define ENV_H

#include <stdint.h>
#include <stddef.h>

#define MAX_ENV_VARS 16
#define MAX_ENV_KEY_LEN 32
#define MAX_ENV_VAL_LEN 64

typedef struct {
  char key[MAX_ENV_KEY_LEN];
  char val[MAX_ENV_VAL_LEN];
  uint8_t used;
} env_var_t;

void env_init(void);
int env_set(const char *key, const char *val);
const char *env_get(const char *key);
void env_list(void);

#endif
