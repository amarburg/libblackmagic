
#include <iostream>
#include <thread>

#include "libg3logger/g3logger.h"

#include "libblackmagic/InputHandler.h"

#include "libblackmagic/DeckLink.h"

namespace libblackmagic {

  using std::shared_ptr;
  using std::vector;
  using std::deque;
  using std::thread;

  InputHandler::InputHandler( DeckLink &parent )
    :  _frameCount(0), _noInputCount(0),
    _pixelFormat( bmdFormat10BitYUV ),
    _currentConfig(),
    _enabled( false ),
    _parent( parent ),
    _deckLink( _parent.deckLink() ),
    _deckLinkInput(  nullptr ),
    _dlConfiguration( nullptr ),
    //_deckLinkOutput( nullptr ),
    _grabbedImages(),
    _queue()
  {
    _deckLink->AddRef();

    CHECK( _deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&_deckLinkInput) == S_OK );
    CHECK( _deckLinkInput != NULL ) << "Couldn't get DecklinkInput";

    CHECK( _deckLink->QueryInterface(IID_IDeckLinkConfiguration, (void**)&_dlConfiguration) == S_OK );

    CHECK( _deckLink != nullptr );

  }

  InputHandler::~InputHandler() {
    if( _deckLinkInput ) _deckLinkInput->Release();
    if( _dlConfiguration ) _dlConfiguration->Release();
    //if( _deckLinkOutput ) _deckLinkOutput->Release();
    if( _deckLink ) _deckLink->Release();
  }

  ULONG InputHandler::AddRef(void) {
    return 1;
  }

  ULONG InputHandler::Release(void) {
    return 1;
  }

  //== ===  ===

bool InputHandler::enable( BMDDisplayMode mode, bool doAuto, bool do3D ) {

    // Hardcode some parameters for now
    //BMDPixelFormat pixelFormat = _pix;
    BMDVideoInputFlags inputFlags = bmdVideoInputFlagDefault;
    BMDSupportedVideoModeFlags supportedFlags = bmdSupportedVideoModeDefault;
    IDeckLinkProfileAttributes *deckLinkAttributes = NULL;
    bool formatDetectionSupported;

    // Check the card supports format detection
    auto result = _deckLink->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&deckLinkAttributes);
    if (result != S_OK) {
      LOG(WARNING) << "Unable to query deckLinkAttributes";
      return false;
    }

    if( doAuto ) {
      LOG(INFO) << "Automatic mode detection requested";

      // Check for various desired features
      result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &formatDetectionSupported);
      if (result != S_OK || !formatDetectionSupported)
      {
        LOG(WARNING) << "      ... Format detection is not supported on this device";
        return false;
      } else {
        inputFlags |= bmdVideoInputEnableFormatDetection;
      }
    }

    if( do3D ) {
      LOG(INFO) << "Configured input for 3D";
      supportedFlags |= bmdSupportedVideoModeDualStream3D;
      inputFlags |= bmdVideoInputDualStream3D;

      //== Configure bmdDeckLinkConfigSDIInput3DPayloadOverride
      //  If set to true, the device will capture two genlocked SDI streams
      //  with matching video modes as a 3D stream.
      {
        // IDeckLinkConfiguration *dlConfiguration = NULL;
        // // Check the card supports format detection
        // auto result = _deckLink->QueryInterface(IID_IDeckLinkConfiguration, (void**)&dlConfiguration);
        // if (result == S_OK) {

          CHECK( _dlConfiguration != nullptr ) << "_dlConfiguration is nullptr";

          bool input3DPayloadOverride = false;
          if( _dlConfiguration->GetFlag(bmdDeckLinkConfigSDIInput3DPayloadOverride, &input3DPayloadOverride) != S_OK ) {
            LOG(WARNING) << "Unable to query bmdDeckLinkConfigSDIInput3DPayloadOverride";
          }
          LOG(INFO) << "  before setting, bmdDeckLinkConfigSDIInput3DPayloadOverride is " << (input3DPayloadOverride ? "true" : "false");

          if( _dlConfiguration->SetFlag( bmdDeckLinkConfigSDIInput3DPayloadOverride, true ) != S_OK ) {
            LOG(WARNING) << "Unable to set bmdDeckLinkConfigSDIInput3DPayloadOverride";
          }

          if( _dlConfiguration->GetFlag(bmdDeckLinkConfigSDIInput3DPayloadOverride, &input3DPayloadOverride) != S_OK ) {
            LOG(WARNING) << "Unable to query bmdDeckLinkConfigSDIInput3DPayloadOverride";
          }
          LOG(INFO) << "  after setting, bmdDeckLinkConfigSDIInput3DPayloadOverride is " << (input3DPayloadOverride ? "true" : "false");
        //
        // } else {
        //   LOG(WARNING) << "Unable to query dlConfiguration";
        //   return false;
        // }
        //
        // dlConfiguration->Release();
      }

    }

    // Why iterate?  Just ask!
    // BMDSupportedVideoModeFlags displayModeSupported;
    bool isSupported = false;
    result = _deckLinkInput->DoesSupportVideoMode(bmdVideoConnectionSDI,
                                                    mode,
                                                    _pixelFormat,
                                                    supportedFlags,
                                                    &isSupported );

    if( result != S_OK ) {
      LOG(WARNING) << "Error while checking if DeckLinkInput supports mode (result = " << result << ")";
      return false;
    }

    if( !isSupported ) {
      LOG(WARNING) << "Requested mode is not supported (result = " << std::hex << result << ")";
      return false;
    }

    IDeckLinkDisplayMode *displayMode = nullptr;
		if( _deckLinkInput->GetDisplayMode( mode, &displayMode ) != S_OK ) {
			LOG(WARNING) << "Unable to get display mode";
			return false;
		}

    // if( displayModeSupported == bmdDisplayModeNotSupported_v10_11 ) {
    //   LOG(WARNING) <<  "The display mode is not supported with the selected pixel format on this input";
    //   return false;
    // }

    CHECK( displayMode != nullptr ) << "Unable to find a video input mode with the desired properties";

    _deckLinkInput->SetCallback(this);
    _deckLinkInput->DisableAudioInput();

    const auto flags = displayMode->GetFlags();

    LOG(INFO) << "Enabling video input with mode " << displayModeToString(displayMode->GetDisplayMode())
              << ( (flags & bmdDisplayModeSupports3D) ? " 3D" : "")
              << " and pixel format " << pixelFormatToString( _pixelFormat );

    // Made it this far?  Great!
    if( S_OK != _deckLinkInput->EnableVideoInput(displayMode->GetDisplayMode(),
                                                  _pixelFormat,
                                                  inputFlags ) ) {
        LOG(WARNING) << "Failed to enable video input. Is another application using the card?";
        return false;
    }

    LOG(INFO) << "DeckLinkInput enabled!";

    // Update config with values
    _currentConfig.setMode( displayMode->GetDisplayMode() );
    _currentConfig.set3D( flags & bmdDisplayModeSupports3D );

    displayMode->Release();

    _enabled = true;
    return true;
  }


//-------
  bool InputHandler::startStreams() {
    if( !_enabled && !enable() ) return false;

    LOG(DEBUG) << "Starting DeckLinkInput streams ....";

    HRESULT result = _deckLinkInput->StartStreams();
    if (result != S_OK) {
      LOG(WARNING) << "Failed to start input streams";
      return false;
    }
    return true;
  }


//-------
bool InputHandler::stopStreams() {
  LOG(DEBUG) << " Stopping DeckLinkInput streams";
  if (_deckLinkInput->StopStreams() != S_OK) {
    LOG(WARNING) << "Failed to stop input streams";
    return false;
  }

  return true;
}


//-------

int InputHandler::grab( void ) {

  const int numImages = _currentConfig.do3D() ? 2 : 1;

  // If there was nothing in the queue, wait
  if( _queue.wait_for_pop(_grabbedImages, std::chrono::milliseconds(100) ) == false ) {
    LOG(WARNING) << "Timeout waiting for image queue ";
    return 0;
  }

  // Formerly checked for empty Mats here.  Still do that?
  return numImages;
}

int InputHandler::getRawImage( int i, cv::Mat &mat ) {

  if( i == 0 || i == 1 ) {
    mat = _grabbedImages[i];
    return i;
  }

 return -1;
}

// ImageSize InputHandler::imageSize( void ) const
// {
// //   return _inputHandler->imageSize();
// }


//====== Input callbacks =====

// Callbacks are called in a private thread....
HRESULT InputHandler::VideoInputFrameArrived( IDeckLinkVideoInputFrame* videoFrame,
                                              IDeckLinkAudioInputPacket* audioFrame )
{
  // Drop audio first thing
  if( audioFrame ) audioFrame->Release();

  uint32_t availFrames;
  if( _deckLinkInput->GetAvailableVideoFrameCount( &availFrames ) == S_OK ) {
    LOG(DEBUG) << "videoInputFrameArrives; " << availFrames << " still available";
  } else {
    LOG(DEBUG) << "videoInputFrameArrives";
  }

  // Handle Video Frame
  if (!videoFrame) return E_FAIL;

  if (videoFrame->GetFlags() & bmdFrameHasNoInputSource) {
    LOG(WARNING) << "(thread " << std::this_thread::get_id()
                        << ") Frame received (" << _frameCount
                        << ") - No input signal detected";

    ++_noInputCount;
    return S_OK;
  }


  // const char *timecodeString = nullptr;
  // if (g_config.m_timecodeFormat != 0)
  // {
  //   IDeckLinkTimecode *timecode;
  //   if (videoFrame->GetTimecode(g_config.m_timecodeFormat, &timecode) == S_OK)
  //   {
  //     timecode->GetString(&timecodeString);
  //   }
  // }

  LOG(DEBUG) << "(thread " << std::hex << std::this_thread::get_id() << std::dec
              << ") Frame received (" << _frameCount
              << ") " << videoFrame->GetRowBytes() * videoFrame->GetHeight()
              << " bytes, " << videoFrame->GetWidth()
              << " x " << videoFrame->GetHeight();

  // The AddRef will ensure the frame is valid after the end of the callback.
  FrameVector frameVector;

  videoFrame->AddRef();
  frameVector.push_back( videoFrame );

  // If 3D mode is enabled we retreive the 3D extensions interface which gives.
  // us access to the right eye frame by calling GetFrameForRightEye() .
  IDeckLinkVideoFrame3DExtensions *threeDExtensions = nullptr;
  if( videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions, (void **) &threeDExtensions) == S_OK ) {

    IDeckLinkVideoFrame *rightEyeFrame = nullptr;

    if(threeDExtensions->GetFrameForRightEye(&rightEyeFrame) != S_OK) {
      LOG(INFO) << "Error getting right eye frame";
    }

    LOG(DEBUG) << "(" << std::hex << std::this_thread::get_id() << std::dec
                << ") Right frame received (" << _frameCount
                << ") " << rightEyeFrame->GetRowBytes() * rightEyeFrame->GetHeight()
                << " bytes, " << rightEyeFrame->GetWidth()
                << " x " << rightEyeFrame->GetHeight();

    //rightEyeFrame->AddRef();
    frameVector.push_back( rightEyeFrame );

    // The AddRef will ensure the frame is valid after the end of the callback.
    // std::thread t = processInThread( rightEyeFrame, 1 );
    // t.detach();
  }


  std::thread t = std::thread([=] { process(frameVector); });
  t.detach();

  if (threeDExtensions) threeDExtensions->Release();

  _frameCount++;

  return S_OK;
}



// Callback if bmdVideoInputEnableFormatDetection was set when
// enabling video input
  HRESULT InputHandler::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events,
                                              IDeckLinkDisplayMode *mode,
                                              BMDDetectedVideoInputFormatFlags formatFlags)
  {
    LOG(INFO) << "(" << std::this_thread::get_id() << ") Received Video Input Format Changed";

    char*   displayModeName = nullptr;
    // BMDPixelFormat  pixelFormat = _pixelFormat; //bmdFormat10BitYUV;
    // if (formatFlags & bmdDetectedVideoInputRGB444) pixelFormat = bmdFormat10BitRGB;

    mode->GetName((const char**)&displayModeName);
    LOG(INFO) << "Input video format changed to " << displayModeName
              << ((formatFlags & bmdDetectedVideoInputDualStream3D) ? " with 3D" : " not 3D");

    if (displayModeName) free(displayModeName);

    _deckLinkInput->PauseStreams();

    LOG(INFO) << "Enabling input at new resolution";
    enable( mode->GetDisplayMode(), true, _currentConfig.do3D() );

    //formatFlags & bmdDetectedVideoInputDualStream3D );

    _queue.flush();

    _currentConfig.setMode( mode->GetDisplayMode() );
    //_currentConfig.set3D( formatFlags & bmdDetectedVideoInputDualStream3D );

    LOG(INFO) << "Enabling output at new mode";

    _deckLinkInput->FlushStreams();
    _deckLinkInput->StartStreams();

    _parent.sendInputFormatChanged( mode->GetDisplayMode() );

    // And reconfigure output
    LOG(INFO) << "Restarted streams at new mode " << displayModeToString( mode->GetDisplayMode() );

    return S_OK;
  }


  //
  //
  //
  void InputHandler::process( FrameVector frameVector )
  {
    MatVector out( frameVector.size() );

    deque< shared_ptr<thread> > workers;

    for( unsigned int i = 1; i < frameVector.size(); ++i ) {
      workers.push_back( shared_ptr<thread>(new std::thread( &InputHandler::frameToMat, this, frameVector[i], std::ref(out[i]), i )) );
    }

    frameToMat( frameVector[0], out[0], 0 );

    for( auto worker : workers ) worker->join();

    _queue.push( out );
  }


  void InputHandler::frameToMat( IDeckLinkVideoFrame *videoFrame, cv::Mat &out, int i )
  {
    CHECK( videoFrame != nullptr ) << "Input VideoFrame in frameToMat";
    //CHECK( out ) << "Output Mat undefined in frameToMat";

    std::string frameName( _currentConfig.do3D() ? ((i==1) ? "[RIGHT]" : "[LEFT]") : "" );

    //LOG(DEBUG) << frameName << " Processing frame with format " << pixelFormatToString( videoFrame->GetPixelFormat() );

    void *data = nullptr;
    if ( videoFrame->GetBytes(&data) == S_OK ) {

      auto pixFmt = videoFrame->GetPixelFormat();

      if( pixFmt == bmdFormat8BitYUV ) {
        // YUV is stored as 2 pixels in 4 bytes
        cv::Mat mat(videoFrame->GetHeight(), videoFrame->GetWidth(), CV_8UC2, data, videoFrame->GetRowBytes());
        mat.copyTo( out );
      } else if ( (pixFmt == bmdFormat8BitBGRA) || (pixFmt == bmdFormat8BitARGB) ) {
        cv::Mat mat(videoFrame->GetHeight(), videoFrame->GetWidth(), CV_8UC4, data, videoFrame->GetRowBytes());
        mat.copyTo( out );
      } else {

        IDeckLinkOutput *deckLinkOutput = NULL;
        CHECK( _deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&deckLinkOutput) == S_OK );

        IDeckLinkMutableVideoFrame*     dstFrame = NULL;
        HRESULT result = deckLinkOutput->CreateVideoFrame(videoFrame->GetWidth(), videoFrame->GetHeight(),  4*videoFrame->GetWidth(),
                                                              bmdFormat8BitBGRA, bmdFrameFlagDefault, &dstFrame);
        CHECK(result == S_OK) << "Failed to create destination video frame";

        IDeckLinkVideoConversion *converter =  CreateVideoConversionInstance();

        LOG(DEBUG) << "Converting " << std::hex << pixelFormatToString(videoFrame->GetPixelFormat()) << " to " << pixelFormatToString(dstFrame->GetPixelFormat());
        result =  converter->ConvertFrame(videoFrame, dstFrame);

        CHECK(result == S_OK ) << frameName << " Failed to do conversion " << std::hex << result;

        void *buffer = nullptr;
        CHECK( dstFrame->GetBytes( &buffer ) == S_OK ) << frameName << " Unable to get bytes from dstFrame";

         cv::Mat dst( cv::Size(dstFrame->GetWidth(), dstFrame->GetHeight()), CV_8UC4, buffer, dstFrame->GetRowBytes() );
         dst.copyTo( out );

         dstFrame->Release();
         deckLinkOutput->Release();
      }

    }

    // Regardless of what happens, release the frames
    LOG(DEBUG) << frameName << " Release; " << videoFrame->Release() << " references remain";

  }

}
