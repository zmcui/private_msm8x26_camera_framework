/*
 * module_isp_start_session
 *  @module: mct module instance
 *  @sessionid: session id to be started
 *
 * starts specified session
 *
 * this function executes in Imaging Server context
 *
 * Returns TRUE in case of success
 * */
static boolean module_isp_start_session(mct_module_t *module,
    unsigned int sessionid)
{
  boolean rc = FALSE;
  int ret = 0;;
  isp_t *isp = module->module_private;

  ISP_DBG(ISP_MOD_COM, "%s: E, module->module_private = %p, sessionid %d \n", __func__,
      module->module_private, sessionid);
  pthread_mutex_lock(&isp_mutex);
  ret = isp_start_session(isp, sessionid);
  pthread_mutex_unlock(&isp_mutex);
  rc = (ret == 0)? TRUE : FALSE;
  ISP_DBG(ISP_MOD_COM, "%s: X, rc = %d \n", __func__, rc);
  return rc;
}

/*
 * module_isp_query_mod_func
 *  @module: mct module instance
 *  @query_buf: query capabilities data
 *  @sessionid: current session id
 *
 * This method populates isp capabilities such as max zoom and
 * supported color effects
 *
 * This function excutes in Imaging Server context
 *
 * Returns TRUE in case of success
 * */
static boolean module_isp_query_mod_func(mct_module_t *module,
    void *query_buf, unsigned int sessionid)
{
  int32_t rc = 0;
  mct_pipeline_isp_cap_t *isp_cap = NULL;
  mct_pipeline_cap_t *cap_buf = (mct_pipeline_cap_t *)query_buf;
  isp_t *isp;
  int num;

  ISP_DBG(ISP_MOD_COM, "%s: E, sessionid %d\n", __func__, sessionid);
  if (!query_buf || !module) {
    CDBG_ERROR("%s:%d failed query_buf %p s_module %p\n", __func__, __LINE__,
        query_buf, module);
    return FALSE;
  }

  isp_cap = $cap_buf->isp_cap;
  ...

  /* populate supported color effects */
  isp_cap->supported_effects_cnt = 9;
  isp_cap->supported_effects[0] = CAM_EFFECT_MODE_OFF;
  isp_cap->supported_effects[1] = CAM_EFFECT_MODE_MONO;
  isp_cap->supported_effects[2] = CAM_EFFECT_MODE_NEGATIVE;
  isp_cap->supported_effects[3] = CAM_EFFECT_MODE_SOLARIZE;
  isp_cap->supported_effects[4] = CAM_EFFECT_MODE_SEPIA;
  isp_cap->supported_effects[5] = CAM_EFFECT_MODE_POSTERIZE;
  isp_cap->supported_effects[6] = CAM_EFFECT_MODE_WHITEBOARD;
  isp_cap->supported_effects[7] = CAM_EFFECT_MODE_BLACKBOARD;
  isp_cap->supported_effects[8] = CAM_EFFECT_MODE_AQUA;


  ISP_DBG(ISP_MOD_COM, "%s: X, rc = %d\n", __func__, rc);
  return rc;
}

/*
 * module_isp_init:
 *  @name: name of this ISP interface module("ISP")
 *
 * Initializes new instance of ISP module
 *
 * This function execues in Imaging Server context
 *
 * Returns new instance on success or NULL on faii
 * */
mct_module_t *module_isp_init(const char *name)
{
  int rc = 0;
  mct_module_t *module_isp = NULL;
  isp_t        *isp = NULL;

  ...

  /* overload function ptrs and save mct_module ptr in isp base */
  module_isp->process_event = module_isp_process_event_func;
  module_isp->set_mod = module_isp_set_mod_func;
  module_isp->query_mod = module_isp_query_mod_func;
  module_isp->start_session = module_isp_start_session;
  module_isp->stop_session = module_isp_stop_session;
  isp->module->module_private = isp;

  ISP_DBG(ISP_MOD_COM, "%s: X, isp->module %x\n", __func__, (unsigned int)isp->module);
  return isp->module;

error_ports:
  isp_destroy(isp);
error:
  mct_module_destroy(module_isp);
  return NULL;
}
