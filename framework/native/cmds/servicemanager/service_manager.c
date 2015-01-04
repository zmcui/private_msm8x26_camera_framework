/* Copyright 2008 The Android Open Source Project
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <private/android_filesystem_config.h>
#include "binder.h"

#if 0
#define ALOGI(x...) fprintf(stderr, "svcmgr: " x)
#define ALOGE(x...) fprintf(stderr, "svcmgr: " x)
#else
#define LOG_TAG "ServiceManager"
#include <cutils/log.h>
#endif

/* TODO:
 * These should come from a config file or perhaps be
 * based on some namespace rules of some sort(media
 * uid can register media.*, etc)
 */
static struct {
  unsigned uid;
  const char *name;
} allowed[] = {
  { AID_MEDIA, ""},
  { AID_MEDIA, ""},

}
