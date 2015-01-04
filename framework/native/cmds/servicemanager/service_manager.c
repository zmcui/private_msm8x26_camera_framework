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
  { AID_MEDIA, "media.audio_flinger"},
  { AID_MEDIA, "meida.log"},
  { AID_MEDIA, "media.player"},
  { AID_MEDIA, "media.camera"},
  { AID_MEDIA, "media.audio_policy"},  
  { AID_DRM, "drm.drmManager"},
  { AID_NFC, "nfc"},
  { AID_BLUETOOTH, "bluetooth"},
  { AID_RADIO, "radio.phone"},
  { AID_RADIO, "radio.sms"},
  { AID_RADIO, "radio.phonesubinfo"},
  { AID_RADIO, "radio.simphonebook"},
  /*TODO: remove after phone services are updated: */
  { AID_RADIO, "phone"},
  { AID_RADIO, "sip"},
  { AID_RADIO, "isms"},
  { AID_RADIO, "iphonesubinfo"},
  { AID_RADIO, "simphonebook"},
  { AID_RADIO, "phone_msim"},
  { AID_RADIO, "isms_msim"},
  { AID_RADIO, "iphonesubinfo_msim"},
  { AID_RADIO, "simphonebook_msim"},
  { AID_MEDIA, "common_time.clock"},
  { AID_MEDIA, "common_time.config"},
  { AID_KEYSTORE, "android.security.keystore"},
  { AID_MEDIA, "listen.service"},
};

....
int main(int argc, char **argv)
{
  struct binder_state *bs;
  void *svcmgr = BINDER_SERVICE_MANAGER;
  
  bs = binder_open(128*1024);
  
  if(binder_become_context_manager(bs)){
    ALOGE("cannot become context manager (%s) \n", strerror(errno));
    return -1;
  }
  
  svcmgr_handle = svcmgr;
  binder_loop(bs, svcmgr_handler);
  return 0;
}
