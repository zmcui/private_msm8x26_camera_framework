camera_device_ops_t QCamera2HardwareInterface::mCameraOps = {
	set_preview_window:		QCamera2HardwareInterface::set_preview_window,
	set_callbacks:			QCamera2HarewareInterface::set_callbacks,
....
}

/*============================================
 * FUNCTION	: set_CallBacks
 * 
 * DESCRIPTION:	set callbacks for notify and data
 *
 * PARAMETERS:
 *	@device		: ptr to camera struct
 *	@notify_cb	: notify cb
 *	@data_cb	: data cb
 *	@data_cb_timestamp	: video data cb with timestamp
 *	@get_memory	: ops table for request gralloc memory
 *	@user		: user data ptr
 *
 *	RETURN	: none
 *===========================================*/
void QCamera2HardwareInterface::set_CallBacks(struct camera_device *device,
	camera_notify_callback notify_cb,
	camera_data_callback data_cb,
	camera_data_timestamp_callback data_cb_timestamp,
	camera_request_memory get_memory,
	void *user)
{
	QCamera2HardwareInterface *hw =
		reinterpret_cast<Qcamera2HardwareInterface *>(device->priv);
	if(!hw){
		ALOGE("NULL camera device");
		return;
	}
	
	qcamera_sm_evt_setcb_payload_t payload;
	payload.notify_cb = notify_cb;
	payload.data_cb = data_cb;
	payload.data_cb_timestamp = data_cb_timestamp;
	payload.get_memory = get_memory;
	payload.user = user;
	
	hw->lockAPI();
	qcamera_api_result_t apiResult;
	int32_t rc = hw->precessAPI(QCAMERA_SM_EVT_SET_CALLBACKS, (void *)&payload);
	if(rc == NO_ERROR){
		hw->waitAPIResult(QCAMERA_SM_EVT_SET_CALLBACKS, &apiResult);
	}
	hw->unlockAPI();
}


/*============================================
 * FUNCTION : allocateStreamBuf
 *
 * DESCRIPTION: allocate stream buffers
 *
 * PARAMETERS:
 *	@stream_type : type of stream
 *	@size		 : size of buffer
 *	@stride		 : stride of buffer
 *	@scanline	 : scanline of buffer
 *	@bufferCnt	 : [IN/OUT] minimum num of buffers to be allocated.
 *				   could be modified during allocation if more buffers needed
 *
 * RETURN		 : ptr to a memory obj that holds stream buffers.
 *				   NULL if failed
 *============================================*/
 QCameraMemory *QCamera2HardwareInterface::allocateStreamBuf(cam_stream_type_t stream_type,
 															 int size,
 															 int stride,
 															 int scanline,
 															 uint8_t &buffercnt)
{
	int rc = NO_ERROR;
	QCameraMemory *mem = NULL;
	bool bCacheMem = QCAMERA_ION_USE_CACHE;
	bool bPoolMem = false;
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.camera.mem.usepool", value, "1");
	if(atoi(value) == 1){
		bPoolMem = true;
	}
	
	// Allocate stream buffer memory object
	switch(stream_type){
	case CAM_STREAM_TYPE_PREVIEW:
		{
			if(isNoDisplayMode()){
				mem = new QCameraStreamMemory(mGetMemory,
						bCacheMem,
						(bPoolMem) ? &m_memoryPool : NULL,
						stream_type);
			}else{
				cam_dimension_t dim;
				QCameraGrallocMemory *grallocMemory =
					new QCameraGrallocMemory(mGetMemory);
					
				mParameters.getStreamDimension(stream_type, dim);
				if(grallocMemory)
					grallocMemory-setWindowInfo(mPreviewWindow, dim.width,
						dim.height, stride, scanline, mParameters.getPreviewHalPixelFormat());
				mem = grallocMemory;
			}
		}
		break;
	case CAM_STREAM_TYPE_POSTVIEW:
		{
			if(isPreviewRestartEnabled() || isNODisplayMode()){
				mem = new QCameraStreamMemory(mGetMemory, bCacheMem);
			}else{
				cam_dimemsion_t dim;
				QCameraGrallocMemory *grallocMemory =
					new QCameraGrallocMemory(mGetMemory);
					
				mParameters.getStreamDimension(stream_type, dim);
				if(grallocMemory)
					grallocMemory-setWindowInfo(mPreviewWindow, dim.width,
						dim.height, stride, scanline, mParameters.getPreviewHalPixelFormat());
				mem = grallocMemory;
			}
		}
		break;
	case CAM_STREAM_TYPE_SNAPSHOT:
	case CAM_STREAM_TYPE_RAW:
	case CAM_STREAM_TYPE_METADATA:
	case CAM_STREAM_TYPE_OFFLINE_PROC:
		mem = new QCameraStreamMemory(mGetMemory,
				bCachedMem,
				(bPoolMem) ? &m_memoryPool : NULL,
				stream_type);
		break;
	case CAM_STREAM_TYPE_VIDEO:
		{
			char value[PROPERTY_VALUE_MAX];
			property_get("persist.camera.mem.usecache", value, "1");
			if(atoi(value) == 0){
				bCacheMem = QCAMERA_ION_USE_NOCACHE;	
			}
			ALOGD("%s: video buf using cached memory = %d", __func__, bCachedMem);
			mem = new QCameraVideoMemory(mGetMemory, bCachedMem);
		}
		break;
	case CAM_STREAM_TYPE_DEFAULT:
	case CAM_STREAM_TYPE_MAX:
	default:
		break;
	}
	if(!mem){
		return NULL;
	}
	
	if(bufferCnt > 0){
		rc = mem->allocate(bufferCnt, size);
		if(rc < 0){
			delete mem;
			return NULL;
		}
		bufferCnt = mem->getCnt();
	}
	return mem;
}


/*=============================================
 * FUNCTION:	set_parameters
 *
 * DESCRIPTION: set camera parameters
 *
 * PARAMETERS:	
 *	@device :	ptr to camera device struct
 *	@parms	:	string of packed parameters
 *
 * RETURN	:	int32_t type of status
 *				NO_ERROR -- success
 *				none-zero failure code
 *=============================================*/
 int Qcamera2HardwareInterface::set_parameters(struct camera_device *device
 												const char *parms)
 {
 	int ret = NO_ERROR;
 	QCamera2HardwareInterface *hw = 
 		reinterpret_cast<QCamera2HardwareInterface *>(device->priv);
 	if(!hw){
 		ALOGE("NULL camera device");
 		return BAD_VALUE;
 	}
 	hw->lockAPI();
 	/* czm it will call QCameraStateMachine.cpp then QCameraParameters::updateParameters() in QCameraParameters.cpp*/
 	ret = hw->processAPI(QCAMERA_SM_EVT_SET_PARAMS, (void *)parms);
 	if(ret == NO_ERROR){
 		hw->waitAPIResult(QCAMERA_SM_EVT_SET_PARAMS);
 		ret = hw->m_apiResult.status;
 	}
 	hw->unlockAPI();

 	return ret;
 }


/*=======================================================
 * FUNCTION   : commitParameterChanges
 *
 * DESCRIPTION: commit parameter changes to the backend to take effect
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 * NOTE       : This function must be called after updateParameters.
 *              Otherwise, no change will be passed to backend to take effect.
 *=======================================================*/
 int QCamera2HardwareInterface::commitParameterChanges()
 {
 	int rc = NO_ERROR;
 	pthread_mutex_lock(&m_parm_lock);
 	rc = mParameters.commitParameters();
 	if(rc == NO_ERROR){
 		// update number of snapshot based on committed parameters setting
 		rc = mParameters.setNumofSnapshot();
 	}
 	pthread_mutex_unlock(&m_parm_lock);
 	return rc;
 }
 
/*==========================================
 * FUNCTION	: setCallBacks(class QCamera2HardwareInterface)
 *
 * DESCRIPTION: set callbacks impl
 *
 * PARAMETERS:
 *	@notify_cb	: notify cb
 *	@data_cb	: data cb
 *	@data_cb_timestamp : data cb with time stamp
 *	@get_memory	: request memory ops table
 *	@user		: user data ptr
 *
 * RETURN		: int32_t type of status
 *				  NO_ERROR -- success
 *				  none-zero failure code
 *=========================================*/
int QCamera2HardwareInterface::setCallBacks(camera_notify_callback notify cb,
											camera_data_callback data_cb,
											camera_data_timestamp_callback data_cb_timestamp,
											camera_request_memory get_memory,
											void *user)
{
	mNotifyCb = notify_cb;
	mDataCb	  = data_cb;
	mDataCbTimestamp = data_cb_timestamp;
	mGetmemory = get_memory;
	mCallbackCookie = user;
	m_cbNotifier.setCallbacks(notify_cb, data_cb, data_cb_timestamp, user);
	return NO_ERROR;
}
 
/*==========================================
 * FUNCTION	: setCallBacks(class QCameraCbNotifier)
 *
 * DESCRIPTION: Initialinzes the callback functions, which would be used for
 *				communication with the upper layers and lanches the callback
 *				context in which the callbacks will occur.
 *
 * PARAMETERS:
 *	@notifyCb	: notification callback
 *	@dataCb		: data callback
 *	@dataCbTimestamp : data with time stamp
 *	@callbackCookie	 : callback context data
 *
 * RETURN		: none
 *=========================================*/
void QCameraCbNotifier::setCallbacks(camera_notify_callback notify notifyCb,
									 camera_data_callback dataCb,
									 camera_data_timestamp_callback dataCbTimestamp,
									 void *callbackCookie)
{
	if((NULL == mNotifyCb)&&
	   (NULL == mDataCb)&&
	   (NULL == mDataCbTimestamp)&&
	   (NULL == mCallbackCookie)){
	 mNotifyCb = notifyCb;
	 mDataCb = dataCb;
	 mDataCbTimestamp = dataCbTimestamp;
	 mCallbackCookie = callbackCookie;
	 mActive = true;
	 mProcTh.launch(cbNotifyRoutine, this);
	}else{
		ALOGE("%s : Camera callback notifier already initialized!",
				__func__);
	}
}
