/*==========================================================
Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential.
===========================================================*/
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
