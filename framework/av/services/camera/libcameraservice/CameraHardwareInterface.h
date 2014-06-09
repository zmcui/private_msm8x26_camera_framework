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
