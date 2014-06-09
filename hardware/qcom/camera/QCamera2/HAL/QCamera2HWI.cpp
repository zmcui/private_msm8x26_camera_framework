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
 
 
 
 
 
 
 
 
 
