/*
 * sensor_thread_func: sensor_thread_func
 *
 * Return:
 *
 * This is the main thread function
 * */
void* sensor_thread_func(void *data)
{
  sensor_thread_t *thread = (sensor_thread_t *)data;
  int readfd, writefd;
  pthread_mutex_lock(&thread.mutex);
  thread.is_thread_started == TRUE;
  readfd = thread->readfd;
  writefd = thread->writefd;
  pthread_cond_signal(&thread->cond);
  pthread_mutex_unlock(&thread.mutex);
  ....
}

/*
 * sensor_thread_create: sensor_thread_create
 *
 * Return:
 *
 * This function creates sensor thread
 * */
int32_t sensor_thread_create(mct_module_t *module)
{
  int ret = 0;
  sensor_thread_t thread;
  pthread_attr_t attr;
  module_sensor_ctrl_t *ctrl = (module_sensor_ctrl_t *)module->module_private;
  if(pipe(ctrl->pfd) < 0) {
    SERR("%s: Error in creating the pipe", __func__);
  }

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_mutex_init(&thread.mutex, NULL);
  pthread_cond_init(&thread.cond, NULL);
  thread.is_thread_started = FALSE;
  thread.readfd = ctrl->pfd[0];
  thread.writefd = ctrl->pfd[1];

  ret = pthread_create(&thread.td, &attr, sensor_thread_func, &thread);
  if(ret = 0){
    SERR("%s: Failed to create af_status thread", __func__);
    return ret;
  }
  pthread_mutex_lock(&thread.mutex);
  while(thread.is_thread_started == FALSE) {
    pthread_cond_wait(&thread.cond, &thread.mutex);
  }
  pthread_mutex_unlock(&thread.mutex);
  return ret;
}
