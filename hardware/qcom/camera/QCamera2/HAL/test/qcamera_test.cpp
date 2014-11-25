/*=====================================================
 * FUNCTION:    enalePrintPreview
 *
 * DESCRIPTION: Enables printing the preview
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *=====================================================*/
void CameraContext::enalePrintPreview()
{
  mDoPrintMenu = true;
}

/*=====================================================
 * FUNCTION:    disalePrintPreview
 *
 * DESCRIPTION: Disables printing the preview
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *=====================================================*/
void CameraContext::disablePrintPreview()
{
  mDoPrintMenu = false;
}

/*=====================================================
 * FUNCTION:    Interpreter
 *
 * DESCRIPTION: Interpreter constructor
 *
 * PARAMETERS : 
 *  @file     : script file
 *
 * RETURN     : None
 *=====================================================*/
Interpreter::Interpreter(const char *file)
  :mCmdIndex(0)
  , mScript(NULL)
{
  if(!file){
    printf("no File Given\n");
    mUseScript = false;
    return;
  }

  FILE *fh = fopen(file, "r");
  if(!fh){
    printf("Could not open file %s\n", file);
    mUseScript = false;
    return;
  }
  
  fseek(fh, 0, SEEK_END);
  int len = ftell(fh);
  rewind(fh);
  
  if(!len){
    printf("Script file %s is empty! \n", file);
    fclose(fh);
    return;
  }
  
  mScript = new char[len + 1];
  if(!mScript){
    fclose(fh);
    return;
  }
  
  fread(mScript, sizeof(char), len, fh);
  mScript[len] = "\0"; // ensure null terminated
  fclose(fh);
  
  char *p1;
  char *p2;
  p1 = p2 = mScript;
  
  do{
    switch(*p1){
    case '\0':
    case '|':
      p1++;
      break;
    case SWITCH_CAMERA_CMD:
    case RESUME_PREVIEW_CMD:
    case START_PREVIEW_CMD:
    case STOP_PREVIEW_CMD:
    case CHANGE_PREVIEW_SIZE_CMD:
    case CHANGE_PICTURE_SIZE_CMD:
    case START_RECORD_CMD:
    case STOP_RECORD_CMD:
    case START_VIV_RECORD_CMD:
    case STOP_VIV_RECORD_CMD:
    case DUMP_CAPS_CMD:
    case AUTOFOCUS_CMD:
    case TAKEPICTURE_CMD:
    case TAKEPICTURE_IN_PICTURE_CMD:
    case ENABLE_PRV_CALLBACKS_CMD:
    case EXIT_CMD:
    case ZSL_CMD:
    case DELAY:
      p2 = p1;
      while((p2 != (mScript + len)) && (*p2 != '|')){
        p2++;
      }
      *p2 = '\0';
      if(p2 == (p1 + 1))
        mCommands.push_back(Command(static_cast<Interpreter::Commands_e>(*p1)));
      else
        mCommands.push_back(Command(static_cast<Interpreter::Commands_e>(*p1), (p1 + 1)));
      p1 = p2;
      break;
    default:
      printf("Invalid cmd %c \n", *p1);
      do{
        p1++;
      }while(p1 != (mScript + len)) && (*p1 != '|'))
    }
  }while(p1 != mScript + len);
  mUseScript = true;
}

/*=====================================================
 * FUNCTION:    ~Interpreter
 *
 * DESCRIPTION: Interpreter destructor
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *=====================================================*/
Interpreter::~Interpreter()
{
  if(mScript)
    delete[] mScript;
    
  mCommands.clear();
}

/*=====================================================
 * FUNCTION:    getCommand
 *
 * DESCRIPTION: get a command from interpreter
 *
 * PARAMETERS : 
 *  @currentCamera: Current camera context
 *
 * RETURN     : Interpreter::Command
 *=====================================================*/
Interpreter::Command Interpreter::getCommand(
  sp<CameraContext> currentCamera)
{
  if(mUseScript){
    return mCommands[mCmdIndex++];
  }else{
    currentCamera->printMenu(currentCamera);
    return Interpreter::Command(
      static_cast<Interpreter::Commands_e>(getchar()));
  }
}

/*=====================================================
 * FUNCTION:    TestContext
 *
 * DESCRIPTION: TestContext constructor
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *=====================================================*/
TestContext::TestContext()
{
  int i = 0;
  mTestRunning = false;
  mInterperter = NULL;
  mViVVid.ViVIdx = 0;
  mViVVid.buf_cnt = 9;
  mViVVid.graphBuf = 0;
  mViVVid.mappedBuff = NULL;
  mViVVid.isBuffValic = false;
  mViVVid.sourceCameraID = -1;
  mViVVid.destinationCameraID = -1;
  mPiPinUse = false;
  mViViUse = false;
  mIsZSLOn = false;
  memset(&mViVBuff, 0, sizeof(ViVBuff_t));
  
  ProcessState::self()->startThreadPool();
  
  do{
    camera[i] = new CameraContext(i);
    if(NULL == camera[i].get()){
      break;
    }
    camera[i]->setTestCtxInstance(this);
    
    status_t stat = camera[i]->openCamera();
    if(NO_ERROR != stat){
      printf("Error encountered Opening camera id: %d\n", i);
      break;
    }
    mAvailableCameras.add(camera[i]);
    i++;
  }while(i < camera[0]->getNumberOfCaemras());

  if(i < camera[0]->getNumberOfCaemras()){
    for(size_t j = 0; j < mAvailableCameras.size(); j++){
      camera[j] = mAvailableCameras.itemAt(j);
      camera[j]->closeCamera();
      camera[j].clear();
    }
    
    mAvailableCameras.clear();
  }
}

/*=====================================================
 * FUNCTION:    ~TestContext
 *
 * DESCRIPTION: TestContext destructor
 *
 * PARAMETERS : None
 *
 * RETURN     : None
 *=====================================================*/
TestContext::~TestContext()
{
  delete mInterpreter;
  
  for(size_t j = 0; j < mAvailableCameras.size(); j++){
      camera[j] = mAvailableCameras.itemAt(j);
      camera[j]->closeCamera();
      camera[j].clear();
  }
  
  mAvailableCameras.clear();
}

/*=====================================================
 * FUNCTION:    AddScriptFromFile
 *
 * DESCRIPTION: add script from file
 *
 * PARAMETERS :
 *  @scriptFile:Script file
 *
 * RETURN     : status_t type of status
                NO_ERROR -- success
                none-zero failure code
 *=====================================================*/
status_t TestContext::AddScriptFromFile(const char *scriptFile)
{
  mInterpreter = new Interpreter(scriptFile);
  mInterpreter->setTestCtxInst(this);
  
  return NO_ERROR;
}

/*=====================================================
 * FUNCTION:    FunctionalTest
 *
 * DESCRIPTION: queries and executes client supplied commands for testing a particular camera.
 *
 * PARAMETERS :
 *  @availableCameras : List with all cameras supported
 *
 * RETURN     : status_t type of status
 *              NO_ERROR -- continue testing
 *              none-zero -- quit test
 *=====================================================*/
status_t TestContext::FunctionalTest()
{
  status_t stat = NO_ERROR;
  const char *ZSLStr = NULL;
  
  assert(mAvailableCameras.size());
  
  if(!mInterpreter){
    mInterpreter = new Interpreter();
    mInterpreter->setTestCtxInst(this);
  }
  
  mTestRunning = true;
  
  while(mTestRunning){
    sp<CameraContext> currentCamera =
      mAvailableCameras.itemAt(mCurrentCameraIndex);
    Interpreter::Command command =
      mInterpreter->getCommand(currentCamera);
    currentCamera->enablePrintPreview();
    
    switch(command.cmd){
    case Interpreter::SWITCH_CAMERA_CMD:
    {
      mCurrentCameraIndex++;
      mCurrentCameraIndex %= mAvailableCameras.size();
      currentCamera = mAvailableCameras.itemAt(mCurrentCameraIndex);
    }
      break;
      
    case Interpreter::RESUME_PREVIEW_CMD:
    {
      stat = currentCamerea->resumePreview();
    }
      break;
    
    case Interpreter::START_PREVIEW_CMD:
    {
      stat = currentCamerea->startPreview();
    }
      break;
     
    case Interpreter::STOP_PREVIEW_CMD:
    {
      stat = currentCamerea->stopPreview();
    }
      break;
    
    case Interpreter::CHANGE_VIDEO_SIZE_CMD:
    {
      if(command.arg)
        stat = currentCamerea->setVideoSize(command.arg);
      else
        stat = currentCamerea->nextVideoSize();
    }
      break;

    case Interpreter::CHANGE_PREVIEW_SIZE_CMD:
    {
      if(command.arg)
        stat = currentCamerea->setPreviewSize(command.arg);
      else
        stat = currentCamerea->nextPreviewSize();
    }
      break;
        
    case Interpreter::CHANGE_PICTURE_SIZE_CMD:
    {
      if(command.arg)
        stat = currentCamerea->setPictureSize(command.arg);
      else
        stat = currentCamerea->nextPictureSize();
    }
      break;

    case Interpreter::DUMP_CAPS_CMD:
    {
      stat = currentCamerea->printSupportedParams();
    }
      break;

    case Interpreter::AUTOFOCUS_CMD:
    {
      stat = currentCamerea->autoFocus();
    }
      break;

    case Interpreter::TAKEPICTURE_CMD:
    {
      stat = currentCamerea->takePicture();
    }
      break;

    case Interpreter::TAKEPICTURE_IN_PICTURE_CMD:
    {
    ....
    
    }
    break;
    
    case Interpreter::ENABLE_PRV_CALLBACKS_CMD:
    {
      stat = currentCamerea->enablePreviewCallbacks();
    }
      break;

    case Interpreter::START_RECORD_CMD:
    {
      stat = currentCamerea->stopPreview();
      stat = currentCamerea->configureRecorder();
      stat = currentCamerea->startPreview();
      stat = currentCamerea->startRecording();
    }
      break;

    case Interpreter::STOP_RECORD_CMD:
    {
      stat = currentCamerea->stopRecording();
      stat = currentCamerea->stopPreview();
      stat = currentCamerea->unconfigureRecorder();
      stat = currentCamerea->startPreview();
    }
      break;

    case Interpreter::START_VIV_RECORD_CMD:
    {
    ....
    
    }
    break;

    case Interpreter::STOP_VIV_RECORD_CMD:
    {
    ....
    
    }
    break;

    case Interpreter::EXIT_CMD:
    {
      currentCamerea->stopPreview();
      mTestRunning = false;
    }
      break;

    case Interpreter::DELAY:
    {
      if(command.arg)
        usleep(1000 * atoi(command.arg));
    }
      break;

    case Interpreter::ZSL_CMD:
    {
      ....
    }
      break;
      
    default:
    {
      currentCamera->disablePrintPreview();
    }
      break;
    }
    printf("Command status 0x%x \n", stat);
  }

  return NO_ERROR;
}

/*=====================================================
 * FUNCTION:    main
 *
 * DESCRIPTION: main function
 *
 * PARAMETERS :
 *  @argc     : argc
 *  @argv     : argv
 *
 * RETURN     : int status
 *=====================================================*/
int main(int argc, char *argv[])
{
  TestContext ctx;
  
  if(argc > 1){
    if(ctx.AddScriptFromFile((const char *)argv[1])){
      printf("Could not add script file..."
        "continuing in normal menu mode! \n");
    }
  }

  ctx.FunctionalTest();
}
