/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
class CameraHardwareInterface : public virtual RefBase {
public:
	CameraHardwareInterface(const char *name)
	{
		mDevice = 0;
		mName = name;
	}
	
	~CameraHardwareInterface()
	{
		ALOGI("Destroying camera %s", mName.string());
		if(mDevice){
			int rc = mDevice->common.close(&mDevice->common);
			if(rc != OK)
				ALOGE("Could not close camera %s: %d", mName.string(), rc);
		}
	}
	
	status_t initialize(hw_module_t *module)
	{
		ALOGI("Opening camera %s", mName.string());
		/* czm: this will call extern "C" int  camera_device_open() in QCamera2/HAL/wrapper/QualcommCamera.cpp
		 * then mDevice could operate camera_device_ops_t QCamera2HardwareInterface::mCameraOps in QCamera2/HAL/QCamera2HWI.cpp
		 */
		int rc = module->methods->open(module, mName.string(),
										(hw_device_t **)&mDevice);
		if(rc != OK){
			ALOGE("Could not open camera %s: %d", mName.string(), rc);
			return rc;
		}
		initHalPreviewWindow();
		return rc;
	}
	
	/** Set the notification and data callbacks */
	void setCallbacks(notify_callback notify_cb,
					  data_callback data_cb,
					  data_callback_timestamp data_cb_timestamp,
					  void* user)
	{
		mNotifyCb = notify_cb;
		mDataCb = data_cb;
		mDataCbTimestamp = data_cb_timestamp;
		mCbUser = user;
		
		ALOGV("%s(%s)", __FUNCTION__, mName.string());
		
		if(mDevice->ops->set_callbacks){
			mDevice->ops->set_callbacks(mDevice,
									__notify_cb,
									__data_cb,
									__data_cb_timestamp,
									__get_memory,
									this);
		}
	}
	
	/**
	 * Set the camera parameters. This returns BAD_VALUE if any parameter is
	 * invalid or not supported. */
	status_t setParameters(const CameraParameters &params)
	{
		ALOGV("%s(%s)", __FUNCTION__, mName.string());
		if(mDevice->ops->set_parameters)
			/* czm: this will go into mm-camera*/
			return mDevice->ops->set_parameters(mDevice,
												params.flatten().string());
		return INVALID_OPERATION;
	}
}

//This is a utility class that combines a MemoryHeapbase and a MemoryBase
//in one. Since we tend to use them in a one-to-one relationship, this is
//handy.
class CameraHeapMemory : public RefBase {
public:
	CameraHeapMemory(int fd, size_t buf_size, uint_t num_buffers = 1) :
					 mBufSize(buf_size),
					 mNumBufs(num_buffers)
	{
		mHeap = new MemoryHeapBase(fd, buf_size * num_buffers);
		commonInitialization();
	}
	
	CameraHeapMemory(size_t buf_size, uint_t num_buffers = 1) :
					 mBuffer(buf_size),
					 mNumBufs(num_buffers)
	{
		mHeap = new MemoryHeapBase(buf_size * num_buffers);
		commonInitialization();	
	}
	
	void commonInitialization()
	{
		handle.data = mHeap->base();
		handle.size = mBufSize * mNumBufs;
		handle.handle = this;
		
		mBuffers = new sp<MemoryBase>[mNumBufs];
		for(uint_t i = 0; i < mNumBufs; i++)
			mBuffers[i] = new MemoryBase(mHeap,
										 i * mBufSize,
										 mBufSize);
		handle.release = __put_memory;
	}
	
	virtual ~CameraHeapMemory()
	{
		delete [] mBuffers;
	}
	
	size_t mBufSize;
	uint_t mNumBufs;
	sp<MemoryHeapBase> mHeap;
	sp<MemoryBase> *mBuffers;
	
	camera_memory_t handle;
};

static camera_memory_t* __get_memory(int fd, size_t buf_size, uint_t num_bufs,
									void *user __attribute__((unused)))
{
	CameraHeapMemory *mem;
	if(fd < 0)
		mem = new CameraHeapMemory(buf_size, num_bufs);
	else
		mem = new CameraHeapMemory(fd, buf_size, num_bufs);
	mem->incStrong(mem);	
	return &mem->handle;
}
