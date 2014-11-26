#ifndef QCAMERA_TEST_H
#define QCAMERA_TEST_H

#include <SkData.h>
#include <SkBitmap.h>
#include <SkStream.h>

namespace qcamera {

using namespace android;

#define MAX_CAM_INSTANCES 3

class TestContext;

class CameraContext : public CameraListener,
  public ICameraRecordingProxyListener{
public:
  typedef enum {
    READ_METADATA = 1;
    READ_IMAGE = 2;
    READ_ALL = 3;
  } ReadMode_t;
  
  //This structure is used to store jpeg file sections in memory
  typedef struct {
    unsigned char * Data;
    int Type;
    usigned Size;
  } Sections_t;
  
public:
  static const char KEY_ZSL[];
    
  CameraContext(int cameraIndex);
  virtual ~CameraContext();
    
  status_t openCamera();
  status_t closeCamera();
    
  ....
private:
  ....
}

class Interpreter
{
public:
  


}

class TestContext
{
  friend class CameraContext;
  friend class Interpreter;
public:
  TestContext();
  ~TestContext();
  
  int32_t GetCamerasNum();
  status_t FuntionalTest();
  status_t AddScriptFromFile(const char *scriptFile);
  void setVivSize(Size VideoSize, int camIndex);
  void PiPLock();
  void PiPUnlock();
  void ViVLock();
  void ViVUnlock();
  
private:
  ....
}

}

#endif
