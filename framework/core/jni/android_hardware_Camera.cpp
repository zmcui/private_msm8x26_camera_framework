
static jint android_hardware_Camera_getNumberOfCameras(JNIEnv *env, jobject thiz)
{
  return Camera::getNumberOfCameras();
}

//---------------------------------------------------------------
static JNINativeMethod camMethods[] = {
{ "getNumberOfCameras",
  "()I",
  (void *)android_hardware_Camera_getNumberOfCameras },

....
}

//Get all the required offsets in java class and register native functions
int register_android_hardware_Camera(JNIEnv *env)
{
....
  // Register native funcitions
  return AndroidRuntime::registerNativeMethods(env, "android/hardware/Camera", camMethods, NELEM(camMethods));
}
