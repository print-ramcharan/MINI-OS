#include "env.h"
#include "vga.h"

static env_var_t env_table[MAX_ENV_VARS];

static int env_strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static void env_strcpy(char *dest, const char *src, size_t max_len) {
  size_t i = 0;
  while (src[i] && i < max_len - 1) {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}

int env_set(const char *key, const char *val) {
  if (!key || !val) return -1;
  for (int i = 0; i < MAX_ENV_VARS; i++) {
    if (env_table[i].used && env_strcmp(env_table[i].key, key) == 0) {
      env_strcpy(env_table[i].val, val, MAX_ENV_VAL_LEN);
      return 0;
    }
  }
  for (int i = 0; i < MAX_ENV_VARS; i++) {
    if (!env_table[i].used) {
      env_strcpy(env_table[i].key, key, MAX_ENV_KEY_LEN);
      env_strcpy(env_table[i].val, val, MAX_ENV_VAL_LEN);
      env_table[i].used = 1;
      return 0;
    }
  }
  return -1;
}

const char *env_get(const char *key) {
  if (!key) return NULL;
  for (int i = 0; i < MAX_ENV_VARS; i++) {
    if (env_table[i].used && env_strcmp(env_table[i].key, key) == 0) {
      return env_table[i].val;
    }
  }
  return NULL;
}

void env_list(void) {
  int count = 0;
  for (int i = 0; i < MAX_ENV_VARS; i++) {
    if (env_table[i].used) {
      print("  ");
      print(env_table[i].key);
      print("=");
      print(env_table[i].val);
      print("\n");
      count++;
    }
  }
  if (count == 0) {
    print("  (no environment variables set)\n");
  }
}

void env_init(void) {
  for (int i = 0; i < MAX_ENV_VARS; i++) {
    env_table[i].used = 0;
    env_table[i].key[0] = '\0';
    env_table[i].val[0] = '\0';
  }

  env_set("OSNAME", "MINI-OS");
  env_set("VER", "1.2");
  env_set("USER", "root");
  env_set("SHELL", "/bin/sh");
}
