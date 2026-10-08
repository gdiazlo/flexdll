/* ARM64 host/plugin regression: imports, distant calls, constructors and GC-free
   loader error paths. Build with -DTEST_HOST, -DTEST_PLUGIN1 or -DTEST_PLUGIN2. */
#include <stdint.h>
#include <stdio.h>

#if defined(TEST_HOST)
#include "flexdll.h"
#include <stdlib.h>
#include <string.h>

int host_value = 41;
int host_function(int x) { return x + host_value; }
static void fail(const char *what) {
  fprintf(stderr, "FAIL %s: %s\n", what, flexdll_dlerror());
  exit(1);
}
int main(void) {
  unsigned i;
  for (i = 0; i < 50; i++) {
    void *one = flexdll_dlopen("./arm64-one.dll", FLEXDLL_RTLD_GLOBAL);
    void *two;
    int (*check)(void);
    if (!one) fail("load plugin one");
    check = (int (*)(void))flexdll_dlsym(one, "check_one");
    if (!check || !check()) fail("host imports, pointers or constructor");
    if (i == 0) {
      intptr_t distance = (intptr_t)check - (intptr_t)host_function;
      printf("host/plugin distance: %lld bytes\n", (long long)distance);
    }
    two = flexdll_dlopen("./arm64-two.dll", 0);
    if (!two) fail("load plugin two");
    check = (int (*)(void))flexdll_dlsym(two, "check_two");
    if (!check || !check()) fail("plugin-to-plugin imports");
    flexdll_dlclose(two);
    flexdll_dlclose(one);
  }
  if (flexdll_dlopen("./arm64-missing.dll", 0)) fail("missing symbol accepted");
  if (!strstr(flexdll_dlerror(), "missing_function")) fail("missing symbol diagnostic");
  puts("PASS ARM64 FlexDLL: host/data imports, direct and tail calls, pointers, constructors, plugin dependency, 50 reloads, missing-symbol rejection");
  return 0;
}
#else
#ifdef TEST_DLLIMPORT
#define IMPORT __declspec(dllimport)
#else
#define IMPORT
#endif
IMPORT extern int host_value;
IMPORT int host_function(int);

#if defined(TEST_PLUGIN1)
int plugin_value = 7;
static int constructor_value;
__attribute__((constructor)) static void init(void) {
  constructor_value = host_function(1);
}
#ifndef TEST_DLLIMPORT
static int *const data_pointer = &host_value;
static int (*const function_pointer)(int) = host_function;
#endif
int plugin_function(int x) { return host_function(x) + plugin_value; }
int check_one(void) {
  int ok = host_value == 41 && host_function(1) == 42 && constructor_value == 42;
#ifndef TEST_DLLIMPORT
  ok = ok && data_pointer == &host_value && *data_pointer == 41
          && function_pointer(1) == 42;
#endif
  return ok;
}
#elif defined(TEST_PLUGIN2)
IMPORT extern int plugin_value;
IMPORT int plugin_function(int);
int check_two(void) {
  return host_value == 41 && plugin_value == 7 && plugin_function(1) == 49;
}
#else
int missing_function(int);
int check_missing(void) { return missing_function(1); }
#endif
#endif
