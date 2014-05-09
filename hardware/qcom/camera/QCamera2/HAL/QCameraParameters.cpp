/*==========================================
 * FUNCTION   : updateParameters
 * 
 * DESCRIPTION: update parameters from user setting
 *
 * PARAMETERS :
 * 		@params  : user setting parameters
 *		@needRestart : [output] if preview need restart upon setting changes
 * 
 * RETURN	  : int32_t type of status
 *				NO_ERROR  -- success
 *				none-zero failure code
 *==========================================*/
int32_t QCameraParameters::updateParameters(QCameraParameters& params,
											bool &needRestart)
{
	int32_t final_rc = NO_ERROR;
	int32_t rc;
	m_bNeedRestart = false;
	
	if(initBatchUpdate(m_pParamBuf) < 0){
		ALOGE("%s:Failed to initialize group update table", __func__);
		rc = BAD_TYPE;
		goto UPDATE_PARAM_DONE;
	}
	
	if ((rc = setPreviewSize(params)))					final_rc = rc;
	if ((rc = setVideoSize(params)))					final_rc = rc;
	if ((rc = setPictureSize(params)))					final_rc = rc;
	if ((rc = setPreviewFormat(params)))				final_rc = rc;
	if ((rc = setPictureFormat(params)))				final_rc = rc;
	if ((rc = setJpegThumbnailSize(params)))			final_rc = rc;
	if ((rc = setJpegQuality(params)))					final_rc = rc;
	if ((rc = setOrientation(params)))					final_rc = rc;
	if ((rc = setRotation(params)))						final_rc = rc;
	if ((rc = setVideoRotation(params)))				final_rc = rc;
	if ((rc = setNoDisplayMode(params)))				final_rc = rc;
	if ((rc = setZslMode(params))) 						final_rc = rc;
	if ((rc = setZslAttributes(params)))				final_rc = rc;
	if ((rc = setCameraMode(params)))					final_rc = rc;
	if ((rc = setRecordingHint(params)))				final_rc = rc;

	if ((rc = setPreviewFrameRate(params)))				final_rc = rc;
	....
	
	// update live snapshot size after all other parameters are set
	if ((rc = setLiveSnapshotSize(params)))				final_rc = rc;
	if ((rc = setStatsDebugMask()))						final_rc = rc;
	if ((rc = setMobicat(params)))						final_rc = rc;
	
UPDATE_PARAM_DONE:
	needRestart = m_bNeedRestart;
	return final_rc;
}

/*==========================================
 * FUNCTION   : setLiveSnapshotSize
 *
 * DESCRIPTION: set live snapshot size
 *
 * PARAMETERS :
 *	@params  : user setting parameters
 *  
 * RETURN     : int32_t type of status
 * 				NO_ERROR  -- success
 *				none-zero failure code
 *=========================================*/
int32_t QCameraParameters::setLiveSnapshotSize(const QCameraParameters& params)
{
	char value[PROPERTY_VALUE_MAX];
	property_get("persist.camera.opt.livepic", value, "1");
	bool useOptimal = atoi(value) > 0 ? true : false;
	
	// use picture size from user setting
	// czm call getPictureSize in CameraParameters.cpp(framework/av/camera)
	params.getPictureSize(&m_LiveSnapshotSize.width, &m_LiveSnapshotSize.height);
	
	// czm assignment procedure done in mct_pipeline.c(mm-camera2/media-controller/mct/pipeline/)
	uint8_t livesnapshot_sizes_tbl_cnt = m_pCapability->livesnapshot_sizes_tbl_cnt;
	cam_dimension_t *livesnapshot_sizes_tbl = &m_pCapability->livesnapshot_sizes_tbl[0];
	
	// check if HFR is enabled
	....
	
	if(useOptimal || hfrMode != CAM_HFR_MODE_OFF){
		bool found = false;
		// first check if picture size is within the list of supported sizes
		for(int i = 0; i < livesnapshot_sizes_tbl_cnt; ++i){
			if(m_LiveSnapshotSize.width == livesnapshot_sizes_tbl[i].width &&
			m_LiveSnapshotSize.height == livesnapshot_sizes_tbl[i].height){
				found = true;
				break;
			}
		}
	
		if(!found){
			// use optimal live snapshot size from supported list
			// that has same preview aspect ratio
			....
		}
	}//CAM_HFR_MODE_OFF
	ALOGI("%s: live snapshot size %d x %d", __func__,
		m_LiveSnapshotSize.width, m_LiveSnapshotSize.height);
		
	return NO_ERROR;
 }
 
