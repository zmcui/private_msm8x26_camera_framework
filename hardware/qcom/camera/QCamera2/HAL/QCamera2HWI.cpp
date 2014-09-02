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
 
 
 
 
 
 
 
 
 
