/*==========================================
 * FUNCTION		: alloc
 *
 * DESCRIPTION	: allocate requested number of buffers of certain size
 *
 * PARAMETERS	:
 *	@count		: number of buffers to be allocated
 *	@size		: length of the buffer to be allocated
 *  @heap_id	: heap id to indicate where the buffers will be allocated from
 *
 * RETURN		: int32_t type of status
 *				  NO_ERROR -- success
 *				  none-zero failure code
 *=========================================*/
int QCameraMemory::alloc(int count, int size, int heap_id)
{
	int rc = OK;
	
	if(mBufferCount < 0){
		mBufferCount = 0;
	}
	
	int new_bufCnt = mBufferCount + count;
	
	if(new_bufCnt > MM_CAMERA_MAX_NUM_FRAMES){
		ALOGE("%s: Buffer count %d out of bound. Max is %d",
			__func__, new_bufCnt, MM_CAMERA_MAX_NUM_FRAMES);
		return BAD_INDEX;
	}
	
	for(int i = mBufferCount; i < new_bufCnt; i++){
		if(NULL = mMemoryPool){
			ALOGE("%s : No memory pool available", __func__);
			rc = allocOneBuffer(mMemInfo[i], heap_id, size, m_bCached);
			if(rc < 0){
				ALOGE("%s: AllocateIonMemory failed", __func__);
				for(int j = i - 1; j >= 0; j--)
					deallocOneBuffer(mMemInfo[j]);
				break;
			}
		}else{
			rc = mMemoryPool->allocateBuffer(mMemInfo[i],
											 heap_id,
											 size,
											 m_bCached,
											 mStreamType);
			if(rc < 0){
				ALOGE("%s: Memory pool allocation failed", __func__);
				for(int j = i - 1; j >= 0; j--)
					mMemoryPool->releaseBuffer(mMemInfo[j],
											   mStreamType);
				break;
			}
		}
	}
	return rc;
}

/*==========================================
 * FUNCTION		: allocOneBuffer
 *
 * DESCRIPTION	: impl of allocating one buffers of certain size
 *
 * PARAMETERS	:
 *	@meminfo	: [output] reference to struct to store additional memory allocation info
 *	@heap		: [input] heap id to indicate where the buffers will be allocated from
 *	@size		: [input] length of the buffer to be allocated
 *  @cached		: [input] flag whether buffer needs to be cached
 *
 * RETURN		: int32_t type of status
 * 				  NO_ERROR -- success
 *				  none-zero failure code
 *=========================================*/
int QCameraMemory::allocOneBuffer(QCameraMemInfo &meminfo,
		int heap_id,
		int size,
		bool cached)
{
	int rc = OK;
	struct ion_handle_data handle_data;
	struct ion_allocation_data alloc;
	struct ion_fd_data ion_info_fd;
	int main_ion_fd = 0;
	
	main_ion_fd = open("/dev/ion", O_RDONLY);
	if(main_ion_fd < 0){
		ALOGE("Ion dev open failed: %s\n", strerror(errno));
		goto ION_OPEN_FAILED;
	}

	memset(&alloc, 0, sizeof(alloc));
	alloc.len = size;
	/* to make it page size aligned*/
	alloc.len = (alloc.len + 4095) & ~4095;
	alloc.align = 4096;
	if(cached){
		alloc.flags = ION_FLAG_CACHED;
	}
	alloc.heap_mask = heap_id;
	rc = ioctl(main_ion_fd, ION_IOC_ALLOC, &alloc);
	if(rc < 0){
		ALOGE("ION allocation failed:%s\n", strerror(errno));
		goto ION_ALLOC_FAILED;
	}
	
	memset(&ion_info_fd, 0, sizeof(ion_info_fd));
	ion_info_fd.handle = alloc.handle;
	rc = ioctl(main_ion_fd, ION_IOC_SHARE, &ion_info_fd);
	if(rc < 0){
		ALOGE("ION map failed %s\n", strerror(errno));
		goto ION_MAP_FAILED;
	}
	
	memInfo.main_ion_fd = main_ion_fd;
	memInfo.fd = ion_info_fd.fd;
	memInfo.handle = ion_info_fd.handle;
	memInfo.size = alloc.len;
	memInfo.cached = cached;
	memInfo.heap_id = heap_id;
	
	CDBG_HIGH("%s : ION buffer %lx with size %d allocated",
			__func__, (unsigned long)memInfo.handle, memInfo.size);
	return OK;
	
ION_MAP_FAILED:
	memset(&handle_data, 0, sizeof(handle_data));
	handle_data.handle = ion_info_fd.handle;
	ioctl(main_ion_fd, ION_IOC_FREE, &handle_data);
ION_ALLOC_FAILED:
	close(main_ion_fd);
ION_OPEN_FAILED:
	return NO_MEMORY;
}

/*==========================================
 * FUNCTION		: deallocOneBuffer
 *
 * DESCRIPTION	: impl of deallocating one buffers
 *
 * PARAMETERS	:
 *	@meminfo	: [output] reference to struct to store additional memory allocation info
 *
 * RETURN		: none
 *=========================================*/
int QCameraMemory::deallocOneBuffer(QCameraMemInfo &meminfo)
{
	struct ion_handle_data handle_data;
	
	if(memInfo.fd > 0){
		close(memInfo.fd);
		memInfo.fd = 0;
	}

	if(memInfo.main_ion_fd > 0){
		memset(&handle_data, 0, sizeof(handle_data));
		handle_data.handle = memInfo.handle;
		ioctl(memInfo.main_ion_fd, ION_IOC_FREE, &handle_data);
		close(memInfo.main_ion_fd);
		memInfo.main_ion_fd = 0;
	}
	memInfo.handle = 0;
	memInfo.size = 0
}

/*==========================================
 * FUNCTION		: QCameraStreamMemory
 *
 * DESCRIPTION	: constructor of QCameraStreamMemory
 *				  ION memory alllocated directly from /dev/ion and shared with framework
 *
 * PARAMETERS	:
 *	@getMemory	: camera memory request ops table
 *  @cached		: flag indicates if using cached memory
 *
 * RETURN		: none
 *=========================================*/
QCameraStreamMemory::QCameraStreamMemory(camera_request_memory getMemory,
		bool cached,
		QCameraMemoryPool *pool,
		cam_stream_type_t streamType)
	:QCameraMemory(cached, pool, streamType),
	mGetMemory(getMemory)
{
	for(int i = 0; i < MM_CAMERA_MAX_NUM_FRAME; i++)
		mCameraMemory[i] = NULL;
}

/*==========================================
 * FUNCTION		: ~QCameraStreamMemory
 *
 * DESCRIPTION	: deconstructor of QCameraStreamMemory
 *
 * PARAMETERS	: none
 *
 * RETURN		: none
 *=========================================*/
QCameraStreamMemory::~QCameraStreamMemory()
{
}
 
/*==========================================
 * FUNCTION		: allocate
 *
 * DESCRIPTION	: allocate requested number of buffers of certain size
 *
 * PARAMETERS	:
 *	@count		: number of buffers to be allocated
 *	@size		: length of the buffer to be allocated
 *
 * RETURN		: int32_t type of status
 *				  NO_ERROR -- success
 *				  none-zero failure code
 *=========================================*/
int QCameraStreamMemory::allocate(int count, int size)
{
	int heap_mask = 0x01 << ION_IOMMU_HEAP_ID;
	int rc = alloc(count, size, heap_mask);
	if(rc < 0)
		return rc;
		
	for(int i = 0; i < count; i++){
		mCameraMemory[i] = mGetmemory(mMemInfo[i].fd, mMemInfo[i].size, 1, this)
	}
	mBufferCount = count;
	return NO_ERROR;
}

/*==========================================
 * FUNCTION		: QCameraGrallocMemory
 *
 * DESCRIPTION	: constructor of QCameraGrallocMemory
 *				  preview stream buffers are allocated from gralloc native_window
 *
 * PARAMETERS	:
 *	@getMemory	: camera memory request ops table
 *
 * RETURN		: none
 *=========================================*/
QCameraGrallocMemory::QCameraGrallocMemory(camera_request_memory getMemory)
		: QCameraMemory(true)
{
	mMinUndequeueBuffers = 0;
	mWindow = NULL;
	mWidth = mHeight = mStride = mScanline = 0;
	mFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
	mGetMemory = getMemory;
	for(int i = 0; i < MM_CAMERA_MAX_NUM_FRAMES; i ++){
		mBufferHandle[i] = NULL;
		mLocalFlag[i] = BUFFER_NOT_OWNED;
		mPrivateHandle[i] = NULL;
	}
}
 
/*==========================================
 * FUNCTION		: ~QCameraGrallocMemory
 *
 * DESCRIPTION	: deconstructor of QCameraGrallocMemory
 *				  preview stream buffers are allocated from gralloc native_window
 *
 * PARAMETERS	: none
 *
 * RETURN		: none
 *=========================================*/
 QCameraGrallocMemory::QCameraGrallocMemory()
 {
 }
 
/*==========================================
 * FUNCTION		: allocate
 *
 * DESCRIPTION	: allocate requested number of buffers of certain size
 *
 * PARAMETERS	:
 *	@count		: number of buffers to be allocated
 *	@size 		: length of the buffer to be allocated
 *
 * RETURN		: int32_t type of status
 *				  NO_ERROR -- success
 *				  none - zero failure code
 *=========================================*/
int QCameraGrallocMemory::allocate(int count, int /*size*/)
{
	int err = 0;
	status_t ret = NO_ERROR;
	int gralloc_uage = 0;
	struct ion_fd_data ion_info_fd;
	memset(&ion_info_fd, 0, sizeof(ion_info_fd));
	
	CDBG_HIGH("%s : E", __func__);
	
	if(!mWindow){
		ALOGE("Invalid native window");
		return INVALID_OPERATION;
	}
	
	// Increment buffer count by min undequeued buffer.
	err = mWindow->get_min_undequeue_buffer_count(mWindow, &mMinUndequeuedBuffers);   //CameraHardwareInterface.h
	if(err != 0){
		ALOGE("get_min_undequeu_buffer_count failed: %s (%d)"), strerror(-err), -err;
		ret = UNKNOW_ERROR;
		goto end;
	}
	
	err = mWindow->set_buffer_count(mWindow, count);	//CameraHardwareInterface.h
	if(err != 0){
		ALOGE("set_buffer_count failed: %s (%d)", strerror(-err), -err);
		ret = UNKNOWN_ERROR;
		goto end;
	}
	
	err = mWindow->set_buffers_geometry(mWindow, mStride, mScanline, mFormat);	//CameraHardwareInterface.h
	if(err != 0){
		ALOGE("%s: set_buffers_geometry failed: %s (%d)", __func__, strerror(-err), -err);
		ret = UNKNOWN_ERROR;
		goto end;
	}
	
	err = mWindow->set_crop(mWindow, 0, 0, mWidth, mHeight);	//CameraHardwareInterface.h
	if(err != 0){
		ALOGE("%s: set_crop failed: %s (%d)", __func__, strerror(-err), -err);
		ret = UNKNOWN_ERROR;
		goto end;
	}
	
	gralloc_usage = GRALLOC_USAGE_HW_CAMERA_WRITE | GRALLOC_USAGE_PRIVATE_IOMMU_HEAP;
	err = mWindow->set_usage(mWindow, gralloc_usage);
	if(err != 0){
		/*set_uage error out*/
		ALOGE("%s: set usage rc = %d", __func__, err);
		ret = UNKNOWN_ERROR;
		goto end;
	}
	CDBG_HIGH("%s: usage = %d, geometry: %p, %d, %d, %d, %d, %d",
			__func__, gralloc_usage, mWindow, mWidth, mHeight, mStride,
			mScanline, mFormat);
	
	//Allocate cnt number of buffers from native window
	for(int cnt = 0; cnt < count; cnt++){
		int stride;
		err = mWindow->dequeue_buffer(mWindow, &mBufferHandle[cnt], &stride);
		if(!err){
			CDBG("dequeue buf hdl = %p", mBufferHandle[cnt]);
			mLocalFlag[cnt] = BUFFER_OWNED;
		}else{
			mLocalFlag[cnt] = BUFFER_NOT_OWNED;
			ALOGE("%s:dequeue_buffer idx = %d err = %d", __func__, cnt, err);
		}
	
	}
	
}
