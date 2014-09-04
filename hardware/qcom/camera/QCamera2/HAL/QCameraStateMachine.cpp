/* Copyright (c) 2012-2013, The Linux Foundataion.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/*======================================
 * FUNCTION	: procAPI
 * DESCRIPTION : process incoming API request from framework layer.
 *
 * PARMETERS   :
 *	@evt		: event to be processed
 *	@api_payload: API payload. Can be NULL if not need.
 *
 * RETUTN	: int32_t type of status
 *			  NO_ERROR -- success
 *			  none-zero failure code
 *====================================*/			  
int32_t QCameraStateMachine::procAPI(qcamera_sm_evt_enum_t evt,
									void *api_payload)
{
	qcamera_sm_cmd_t *node =
		(qcamera_sm_cmd_t *)malloc(sizeof(qcamera_sm_cmd_t));
	if(NULL == node){
		ALOGE("%s: NO memory for qcamera_sm_cmd_t", __func__);
		return NO_MEMORY;
	}
	
	memset(nnode, 0, sizeof(qcamera_sm_cmd_t));
	node->cmd = QCAMERA_SM_CMD_TYPE_API;
	node->evt = evt;
	node->evt_payload = api_payload;
	if(api_queue.enqueue((void *)node)){
		cam_sem_post(&cmd_sem);
		return NO_ERROR;
	}else{
		free(node);
		return UNKNOWN_ERROR;
	}
}

/*======================================
 * FUNCTION	: procEvtPreviewStoppedState
 *
 * DESCRIPTION: finite state machine function to handle event in state of
 *				QCAMERA_SM_STATE_PREVIEW_STOPPED
 * PARAMETERS :
 *	@evt	: event to be processed
 *	@payload: event payload. Can be NULL if not needed
 *
 * RETURN	: int32_t tpye of status
 *			  NO_ERROR -- success
 *			  none-zero failure code
 *=====================================*/
int32_t QCameraStateMachine::procEvtPreviewStoppedState(qcamera_sm_evt_enum_t evt,
														void *payload)
{
	....
	case QCAMERA_SM_EVT_SET_CALLBACKS:
		{
			qcamera_sm_evt_setcb_payload_t *setcbs =
				(qcamera_sm_evt_setcb_payload *)payload;
			rc = m_parent->setCallBacks(setcbs->notify_cb,
										setcbs->data_cb,
										setcbs->data_cb_timestamp,
										setcbs->get_memory,
										setcbs->user);
			result.status = rc;
			result.request_api = evt;
			result.request_type = QCAMEA_API_RESULT_TYPE_DEF;
			m_parent->signalAPIResult(&result);
		}
		break;
	....
	case QCAMERA_SM_EVT_SET_PARAMS:
		{
			bool needRestart = false;
			rc = m_parent->updateParameters((char*)payload, needRestart);
			if (rc == NO_ERROR){
				if (needRestart){
					// need restart preview for parameters to take effect
					m_parent->unpreparePreview();
					// commit parameter changes to server
					m_parent->commitParameterChanges();
					// prepare preview again
					rc = m_parent->preparePreview();
					if (rc != NO_ERROR) {
						m_state = QCAMERA_SM_STATE_PREVIEW_STOPPED;
					}
				}else{
					rc = m_parent->commitParameterChanges();
				}
			}
			
			result.status = rc;
			result.request_api = evt;
			result.result_type = QCAMERA_API_RESULT_TYPE_DEF;
			m_parent->signalAPIResult(&result);
		}
		break;
	....
}
 
 
 
 
