/*==========================================================
Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
===========================================================*/
#include "cpp_module.h"

/*
 * cpp_module_init:
 * Args:
 *  @name: module name
 * Return:
 *  - mct_module_t pointer corresponding to cpp on SUCCESS
 *  - NULL in case of FAILURE or if CPP hardware does not
 *    exit
 * */
mct_module_t *cpp_module_init(const char *name)
{
  mct_module_t *module;
  cpp_module_ctrl_t* ctrl;
  CDBG_HIGH("%s:%d name = %s", __func__, __LINE__, name);
  module = mct_module_create(name);
  if(!module) {
    CDBG_HIGH("%s:%d failed.", __func__, __LINE__);
    return NULL;
  }
  ctrl = cpp_module_create_cpp_ctrl();
  if(!ctrl) {
    CDBG_HIGH("%s:%d failed.", __func__, __LINE__);
    goto error_cleanup_module;
  }
  MCT_OBJECT_PRIVATE(module) = ctrl;
  ctrl->p_module = module;
  module->set_mod = cpp_module_set_mod;
  module->query_mod = cpp_module_query_mod;
  module->start_session = cpp_module_start_session;
  module->stop_session = cpp_module_stop_session;

  ...
}

boolean cpp_module_query_mod(mct_module_t *module, void *buf,
    unsigned int sessionid)
{
  int rc;
  if(!module || !buffer) {
    CDBG_ERROR("%s:%d: failed, module=%p, query_buf=%p",
        __func__, __LINE__, module, buf);
    return FALSE;
  }
  mct_pipeline_cap_t *query_buf = (mct_pipeline_cap_t *)buf;
  mct_pipeline_pp_cap_t *pp_cap = &(query_buf->pp_cap);

  ...
  /* TODO: Need a linking function to fill pp cap based on HW caps ? */
  if (query_buf->sensor_cap.senor_format != FORMAT_YCBCR){
    pp_cap->supported_effects[pp_cap->supported_effects_cnt++] =
      CAM_EFFECT_MODE_OFF;
    pp_cap->supported_effects[pp_cap->supported_effects_cnt++] =
      CAM_EFFECT_MODE_EMBOSS;
    pp_cap->supported_effects[pp_cap->supported_effects_cnt++] =
      CAM_EFFECT_MODE_SKETCH;
    pp_cap->supported_effects[pp_cap->supported_effects_cnt++] =
      CAM_EFFECT_MODE_NEON;
  }
  ...
}

int32_t cpp_module_process_downstream_event(mct_module_t* module,
    mct_event_t* event)
{
    boolean ret;
    int rc;
    if(!module || !event){
        CDBG_ERROR("%s:%d, failed, module=%p, event=%p", __func__, __LINE__, module, event);
        return -EINVAL
    }
    uint32_t identity = event->identity;
    cpp_module_ctrl_t *ctrl = (cpp_module_ctrl_t *)MCT_OBJECT_PRIVATE(module);
    /* handle events based on type, if not handled, forward it downstream */
    switch(event->type){
    case MCT_EVENT_MODULE_EVENT:{
        switch(event->u.module_event.type){
            ....
        }
    }
    case MCT_EVENT_CONTROL_CMD:{
        switch(event->u.ctrl_event.type){
        ....
        case MCT_EVENT_CONTROL_SET_PARM:{
            rc = cpp_module_handle_set_parm_event(module, event);
            if(rc < 0){
                CDBG_ERROR("%s:%d, failed\n", __func__, __LINE__);
                return rc;
            }
            break;
        }
        ....
        default:
            rc = cpp_module_send_event_downstream(module, event);
            if(rc < 0){
                CDBG_ERROR("%s:%d, failed, control_event_type = %d, identity = 0x%x",
                __func__, __LINE__, event->u.ctrl_event.type, identity);
                return -EFAULT;
            }
            break;
        }
        break;
    }
    default:
        CDBG_ERROR("%s:%d, failed, bad event type=%d, identity=0x%x",
        __func__, __LINE__, event->type, identity);
        return -EFAULT;
    }
    return 0;
}
