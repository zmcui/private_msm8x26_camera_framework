/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 */
#ifndef __QCAMERA_SEMAPHORE_H__
#define __QCAMERA_SEMAPHORE_H__

#ifdef __cplusplus
extern "C"{
#endif

/** Implement semaphore with mutex and conditional variable.
 *  Reason being, POSIX semaphore on Android are not used or 
 *  well tested.
 */
typedef struct{
	int val;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
}cam_semaphore_t;

static inline void cam_sem_init(cam_semaphore_t *s, int n)
{
	pthread_mutex_init(&(s->mutex), NULL);
	pthread_cond_init(&(s->cond), NULL);
	s->val = n;
}

static inline void cam_sem_post(cam_semaphore_t *s)
{
	pthread_mutex_lock(&(s->mutex));
	s->val++;
	pthread_cond_signal(&(s->mutex));
	pthread_mutex_unlock(&(s->mutex));
}

static inline void cam_sem_wait(cam_semaphore_t *s)
{
	int rc = 0;
	pthread_mutex_lock(&(s->mutex));
	while(s->val == 0)
		rc = pthread_cond_wait(&(s->cond), &(s->mutex));
	s->val--;
	pthread_mutex_unlock(&(s->mutex));
	return rc;
}

static inline void cam_sem_destroy(cam_semaphore_t *s)
{
	pthread_mutex_destroy(&(s->mutex));
	pthread_cond_destroy(&(s->cond));
	s->val = 0;
}

#ifdef __cplusplus
}
#endif

#endif /* __QCAMERA_SEMAPHORE_H__ */
