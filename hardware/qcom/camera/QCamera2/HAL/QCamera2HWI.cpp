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
