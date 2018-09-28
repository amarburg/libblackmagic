
#include <iostream>
#include <thread>

#include "libg3logger/g3logger.h"

#include "libblackmagic/InputHandler.h"

#include "libblackmagic/DeckLink.h"

namespace libblackmagic {

  InputHandler::InputHandler( DeckLink &parent )
    :  _frameCount(0), _noInputCount(0),
    _pixelFormat( bmdFormat8BitBGRA ),
    _currentConfig(),
    _enabled( false ),
    _parent( parent ),
    _deckLink( _parent.deckLink() ),
    _deckLinkInput(  nullptr ),
    //_deckLinkOutput( nullptr ),
    _grabbedImages(),
    _queue()
  {
    _deckLink->AddRef();

    CHECK( _deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&_deckLinkInput) == S_OK );
    CHECK( _deckLinkInput != NULL ) << "Couldn't get DecklinkInput";

    // CHECK( _deckLink->QueryInterface(IID_IDeckLinkOutput, (void**)&_deckLinkOutput) == S_OK );
    // CHECK( _deckLinkOutput != nullptr ) << "Couldn't get DecklinkOutput";

  }

  InputHandler::~InputHandler() {
    if( _deckLinkInput ) _deckLinkInput->Release();
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
    BMDVideoInputFlags inputFlags = bmdVideoInputFlagDefault;
    //BMDPixelFormat pixelFormat = _pix;

    IDeckLinkAttributes* deckLinkAttributes = NULL;
    bool formatDetectionSupported;

    // Check the card supports format detection
    auto result = _deckLink->QueryInterface(IID_IDeckLinkAttributes, (void**)&deckLinkAttributes);
    if (result != S_OK) {
      LOG(WARNING) << "Unable to query deckLinkAttributes";
      return false;
    }

    if( doAuto ) {
      LOG(INFO) << "Automatic mode detection requested, checking for support";

      // Check for various desired features
      result = deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &formatDetectionSupported);
      if (result != S_OK || !formatDetectionSupported)
      {
        LOG(WARNING) << "* Format detection is not supported on this device";
        return false;
      } else {
        LOG(INFO) << "* Enabling automatic format detection on input card.";
        inputFlags |= bmdVideoInputEnableFormatDetection;
      }
    }

    if( do3D ) {
      inputFlags |= bmdVideoInputDualStream3D;
    }

    //  IDeckLinkDisplayModeIterator* displayModeIterator = NULL;
    IDeckLinkDisplayMode *displayMode = nullptr;

    // WHy iterate?  Just ask!
    BMDDisplayModeSupport displayModeSupported;
    result = _deckLinkInput->DoesSupportVideoMode(mode,
                                                    _pixelFormat,
                                                    inputFlags,
                                                    &displayModeSupported,
                                                    &displayMode);


    if (result != S_OK) {
      LOG(WARNING) << "Error while checking if DeckLinkInput supports mode";
      return false;
    }

    if (displayModeSupported == bmdDisplayModeNotSupported) {
      LOG(WARNING) <<  "The display mode is not supported with the selected pixel format on this input";
      return false;
    }

    CHECK( displayMode != nullptr ) << "Unable to find a video input mode with the desired properties";

    _deckLinkInput->SetCallback(this);

    _deckLinkInput->DisableAudioInput();

    // Made it this far?  Great!
    if( S_OK != _deckLinkInput->EnableVideoInput(displayMode->GetDisplayMode(),
                                                  _pixelFormat,
                                                  inputFlags) ) {
        LOG(WARNING) << "Failed to enable video input. Is another application using the card?";
        return false;
    }

    LOG(INFO) << "DeckLinkInput complete!";

    // Update config with values
    _currentConfig.setMode( displayMode->GetDisplayMode() );
    _currentConfig.set3D( displayMode->GetFlags() & bmdDetectedVideoInputDualStream3D );

    displayMode->Release();

      _enabled = true;
      return true;
    }


//-------
  bool InputHandler::startStreams() {
    if( !_enabled && !enable() ) return false;

    LOG(INFO) << "Starting DeckLink input streams ....";

    HRESULT result = _deckLinkInput->StartStreams();
    if (result != S_OK) {
      LOG(WARNING) << "Failed to start input streams";
      return false;
    }
    return true;
  }


//-------
bool InputHandler::stopStreams() {
  LOG(INFO) << " Stopping DeckLinkInput streams";
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
HRESULT InputHandler::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame)
{

  IDeckLinkVideoFrame3DExtensions *threeDExtensions = nullptr;

  // Drop audio first thing
  if( audioFrame ) audioFrame->Release();

  uint32_t availFrames;
  if( _deckLinkInput->GetAvailableVideoFrameCount( &availFrames ) == S_OK ) {
    LOG(DEBUG) << "videoInputFrameArrives; " << availFrames << " still available";
  } else {
    LOG(DEBUG) << "videoInputFrameArrives";
  }

  // Handle Video Frame
  if (!videoFrame) {
    return E_FAIL;
  }

  if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
  {
    LOG(WARNING) << "(" << std::this_thread::get_id()
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

    LOG(DEBUG) << "(" << std::this_thread::get_id()
                << ") Frame received (" << _frameCount
                << ") " << videoFrame->GetRowBytes() * videoFrame->GetHeight()
                << " bytes, " << videoFrame->GetWidth()
                << " x " << videoFrame->GetHeight();

    // The AddRef will ensure the frame is valid after the end of the callback.
    FramePair framePair;

    videoFrame->AddRef();
    framePair[0] = videoFrame;

    // If 3D mode is enabled we retreive the 3D extensions interface which gives.
    // us access to the right eye frame by calling GetFrameForRightEye() .
    if( videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions, (void **) &threeDExtensions) == S_OK ) {

      IDeckLinkVideoFrame *rightEyeFrame = nullptr;

      if(threeDExtensions->GetFrameForRightEye(&rightEyeFrame) != S_OK) {
        LOG(INFO) << "Error getting right eye frame";
      }

      LOG(DEBUG) << "(" << std::this_thread::get_id()
                  << ") Right frame received (" << _frameCount
                  << ") " << rightEyeFrame->GetRowBytes() * rightEyeFrame->GetHeight()
                  << " bytes, " << rightEyeFrame->GetWidth()
                  << " x " << rightEyeFrame->GetHeight();

      //rightEyeFrame->AddRef();
      framePair[1] = rightEyeFrame;

      // The AddRef will ensure the frame is valid after the end of the callback.
      // std::thread t = processInThread( rightEyeFrame, 1 );
      // t.detach();
    }


  std::thread t = std::thread([=] { process(framePair); });
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
    enable( mode->GetDisplayMode(), true, formatFlags & bmdDetectedVideoInputDualStream3D );

    _queue.flush();

    _currentConfig.setMode( mode->GetDisplayMode() );
    _currentConfig.set3D( formatFlags & bmdDetectedVideoInputDualStream3D );

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
  void InputHandler::process( FramePair framePair )
  {
    std::array<cv::Mat,2> out;

    for( unsigned int i = 0; i < 2; i++ ) {

      if( framePair[i] == nullptr ) continue;

      IDeckLinkVideoFrame *videoFrame = framePair[i];

      std::string frameName( _currentConfig.do3D() ? ((i==1) ? "[RIGHT]" : "[LEFT]") : "" );

      LOG(DEBUG) << frameName << " Processing frame...";

      void *data = nullptr;
      if ( videoFrame->GetBytes(&data) == S_OK ) {

        auto pixFmt = videoFrame->GetPixelFormat();
        if( pixFmt == bmdFormat8BitYUV ) {
          // YUV is stored as 2 pixels in 4 bytes
          cv::Mat mat(videoFrame->GetHeight(), videoFrame->GetWidth(), CV_8UC2, data, videoFrame->GetRowBytes());
          mat.copyTo( out[i] );
        } else if ( pixFmt == bmdFormat8BitBGRA ) {
          cv::Mat mat(videoFrame->GetHeight(), videoFrame->GetWidth(), CV_8UC4, data, videoFrame->GetRowBytes());
          mat.copyTo( out[i] );
        } else {
          LOG(WARNING) << "Sorry, don't understand pixel format " << videoFrame->GetPixelFormat();
        }

      }

      // Regardless of what happens, release the frames
      LOG(DEBUG) << frameName << " Release; " << videoFrame->Release() << " references remain";

    }



//    dstFrame->Release();

    // while( _queue.size() >= maxDequeDepth && _queue.pop_and_drop() ) {;}
    _queue.push( out );

    //LOG(DEBUG) << " Push!  queue now " << _queue.size();

  }


  // == Old conversion code for posterity
  // default:
  // {
  //   //CvMatDeckLinkVideoFrame cvMatWrapper(videoFrame->GetHeight(), videoFrame->GetWidth());
  // //  LOG(DEBUG) << frameName << "Converting through Blackmagic VideoConversionInstance to " << videoFrame->GetWidth() << " x " << videoFrame->GetHeight();
  //
  //   IDeckLinkVideoConversion *converter =  CreateVideoConversionInstance();
  //
  //   //LOG(WARNING) << "Converting " << std::hex << videoFrame->GetPixelFormat() << " to " << dstFrame->GetPixelFormat();
  //   result =  converter->ConvertFrame(videoFrame, dstFrame);
  //
  //   if (result != S_OK ) {
  //     LOG(WARNING) << frameName << " Failed to do conversion " << std::hex << result;
  //     goto bail;
  //   }
  //
  //   void *buffer = nullptr;
  //   if( dstFrame->GetBytes( &buffer ) != S_OK ) {
  //     LOG(WARNING) << frameName << " Unable to get bytes from dstFrame";
  //     goto bail;
  //   }
  //
  //    cv::Mat dst( cv::Size(dstFrame->GetWidth(), dstFrame->GetHeight()), CV_8UC4, buffer, dstFrame->GetRowBytes() );
  //    dst.copyTo( out[i] );
  //    //cv::cvtColor(srcMat, out[i], cv::COLOR_BGRA2BGR);
  // }

}
