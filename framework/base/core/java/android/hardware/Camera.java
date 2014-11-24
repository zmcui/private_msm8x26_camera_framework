/**
 * Returns the number of physical cameras available on this device.
 */
public native static int getNumberOfCameras();

/**
 * Creates a new Camera object to access the first back-facing camera on the
 * device. If the device does not have a back-facing camera, this returns
 * Null.
 * @see #open(int)
 */
public static Camera open(){
  int numberOfCameras = getNumberOfCameras();
  CameraInfo cameraInfo = new CameraInfo();
  for(int i = 0; i < numberOfCameras; i++){
    getCameraInfo(i, cameraInfo);
    if(cameraInfo.facing == CameraInfo.CAMERA_FACING_BACK){
      return new Camera(i);
    }
  }
  return NULL;
}
