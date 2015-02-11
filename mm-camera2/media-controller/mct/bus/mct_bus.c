/*
 * mct_bus.c
 *
 * This file contains the bus implementation.
 *
 * Copyright (c) 2012-2014 Qualcomm Technologies, Inc. All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 *
 * */
#include "mct_bus.h"
#include "camera_dbg.h"
#include <sys/syscall.h>

#if 0
#undef CDBG
#define CDBG ALOGE
#endif
#define MCT_BUS_SOF_TIMEOUT 5000000000 /*in ns unit*/


/*
 * mct_bus_timeout_wait:
 *  cond:   POSIX conditional variable
 *  mutex:  POSIX mutex
 *  timeout:type of signed long long, specified
 *          timeout measured in nanoseconds;
 *          timeout = -1 means no timeout, it becomes
 *          to regular conditional timewait.
 *
 *  Commonly used for timeout waiting.
 * */
static int mct_bus_timeout_wait(pthread_cond_t *cond, pthread_mutex_t *mutex,
    signed long long timeout){
  signed long long end_time;
  struct timeval r;
  struct timespec ts;
  int ret;
  pthread_mutex_lock(mutex);
  if (timeout != -1) {
    gettimeofday(&r, NULL);
    end_time = (((((signed long long)r.tv_sec) * 1000000) + r.tv_usec) +
      (timeout / 1000));
    ts.tv_sec = (end_time / 1000000);
    ts.tv_nsec = ((end_time % 1000000) * 1000);
    ret = pthread_cond_timewait(cond, mutex, &ts);
  }else{
    ret = pthread_cond_wait(cond, mutex);
  }
  pthread_mutex_unlock(mutex);
  return ret;
}

static void* mct_bus_sof_thread_run(void *data)
{
  mct_bus_t *bus = (mct_bus_t *)data;
  int ret;
  CDBG_ERROR("%s thread id is %d\n", __func__, syscall(SYS_gettid));
  pthread_mutex_lock(&bus->bus_sof_init_lock);
  pthread_cond_signal(&bus->bus_sof_init_cond);
  pthread_mutex_unlock(&bus->bus_sof_init_lock);
  bus->thead_run = 1;
  while(bus->thread_run){
    ret = mct_bus_timeout_wait(&bus->bus_sof_msg_cond,
        &bus->bus_sof_msg_lock, MCT_BUS_SOF_TIMEOUT);
    if(ret == ETIMEOUT){
      CDBG_ERROR("%s: SOF freeze; Sending error message\n", __func__);
      break;
    }
  }
  if(bus->thread_run == 1) {
    /* Things went wrong */
    mct_bus_msg_t bus_msg;
    bus_msg.type = MCT_BUS_MSG_SEND_HW_ERROR;
    bus_msg.size = 0;
    bus_msg.sessionid = bus->session_id;
    bus->post_msg_to_bus(bus, &bus_msg);
  }
  return NULL;
}
static void start_sof_check_thread(mct_bus_t *bus)
{
  int rc = 0;
  if (bus->thread_run == 1)
    return;
  ALOGE("%s: Starting SOF timeout thread\n", __func__);
  pthread_mutex_init(&bus->bus_sof_msg_lock, NULL);
  pthread_cond_init(&bus->bus_sof_msg_cond, NULL);
  pthread_mutex_lock(&bus->bus_sof_init_lock);
  rc = pthread_create(&bus->bus_sof_tid, NULL, mct_bus_sof_thread_run, bus);
  if(!rc) {
    pthread_cond_wait(&bus->bus_sof_init_cond, &bus->bus_sof_init_lock);
  }
  pthread_mutex_unlock(&bus->bus_sof_init_lock);
}

static boolean msg_bus_post_msg(mct_bus_t *bus, mct_bus_msg_t *bus_msg)
{
  ...
  switch(bus_msg->type){
    case MCT_BUS_MSG_ISP_SOF:
      payload_size = sizeof(mct_bus_msg_isp_sof_t);
      if (bus->thread_run == 1) {
        pthread_mutex_lock(&bus->bus_sof_msg_lock);
        pthread_cond_signal(&bus->bus_sof_msg_cond);
        pthread_mutex_unlock(&bus->bus_sof_msg_lock);
      }
      break;
    case MCT_BUS_MSG_SENSOR_STARTING:
      start_sof_check_thread(bus);
      return TRUE;
      break;
    ...
    case MCT_BUS_MSG_SEND_HW_ERROR:
      payload_size = 0;
      post_msg = TRUE;      //flag to post message to Media Controller
      pthread_mutex_lock(&bus->bus_msg_q_lock);
      mct_bus_queue_flush(bus);
      pthread_mutex_unlock(&bus->bus_msg_q_lock);
      break;
    ...
    default:
      CDBG("%s: bus_msg type is not valid", __func__);
      goto error_2;
  }
  ...

  local_msg = malloc(sizeof(mct_bus_msg_t));

  if(!local_msg) {
    ALOGE("%s: %d Can't allocate memory", __func__, __LINE__);
    goto error_2;
  }

  local_msg->sessionid = bus_msg->sessionid;
  local_msg->type = bus_msg->size;
  local_msg->size = bus_msg->size;

  if(payload_size) {
    local_msg->msg = malloc(payload_size);
    if(!local_msg->msg){
      ALOGE("%s: %d Can't allocate memory", __func__, __LINE__);
      goto error_1;
    }
    memcpy(local_msg->msg, bus_msg->msg, payload_size);
  } else {
    local_msg->msg = NULL;
  }

  /*
   * Push message to Media Controller/Pipeline BUS Queue
   * and post signal to Media Controller
   * */
  pthread_mutex_lock(&bus->bus_msg_q_lock);
  mct_queue_push_tail(bus->bus_queue, local_msg);
  pthread_mutex_unlock(&bus->bus_msg_q_lock);

  if(post_msg){
    pthread_mutex_lock(bus->mct_mutex);
    bus->bus_cmd_q_flag = TRUE;
    pthread_cond_signal(bus->mct_cond);
    pthread_mutex_unlock(bus->mct_mutex);
  }

  return TRUE;

error_1:
  free(local_msg);
error_2:
  return FALSE;
}
