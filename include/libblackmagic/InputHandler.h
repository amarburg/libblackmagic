#pragma once

//#include <queue>
#include <condition_variable>
#include <mutex>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "active_object/active.h"
#include "active_object/shared_queue.h"
#include <DeckLinkAPI.h>
#include "ThreadSynchronizer.h"

#include "DataTypes.h"
#include "InputConfig.h"

#include "SDICameraControl.h"
#include "SDIMessageBuffer.h"

namespace libblackmagic {

  class DeckLink;

  class InputHandler : public IDeckLinkInputCallback
  {
  public:
    InputHandler( DeckLink &deckLink );
    virtual ~InputHandler();

    // Retrieve the current configuration
    InputConfig &config() { return _config; }
    void setConfig( const InputConfig &config ) { _config = config; }

    // Attempts to configure the input stream.   If not called explicitly,
    // will be called automatically by startStreams()
    bool enable( void );

    // Lazy initializers
    IDeckLinkInput *deckLinkInput();
    IDeckLinkOutput *deckLinkOutput();

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE  Release(void);

    // //== IDeckLinkInputCallback methods ==
    virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
    virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);

    active_object::shared_queue< cv::Mat > &queue( int i = 0 ) { return _queues[i]; }

    bool startStreams();
    bool stopStreams();

    int grab( void );
    int getRawImage( int i, cv::Mat &mat );


    // //== IDeckLinkOutputCallback methods ==
    // HRESULT	STDMETHODCALLTYPE ScheduledFrameCompleted(IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result);
    // HRESULT	STDMETHODCALLTYPE ScheduledPlaybackHasStopped(void);
    // std::condition_variable _scheduledPlaybackStoppedCond;
    // std::mutex _scheduledPlaybackStoppedMutex;
    //
    // const std::shared_ptr<SharedBMSDIBuffer> &sdiProtocolBuffer()
		// 	{ return _buffer; }


  protected:

    // Break into two functions to reduce complexity
    bool enableInput();
    //bool enableOutput();

    // bool startOutput();
    //
    // bool stopOutput();


    // Process input frames
    bool process( IDeckLinkVideoFrame *frame, int input = 0 );

    std::thread processInThread( IDeckLinkVideoFrame *frame, int input = 0 ) {
          return std::thread([=] { process(frame, input); });
      }


    // Sub-functions for video outputFlags
    //bool setOutputMode( BMDDisplayMode );


    // IDeckLinkMutableVideoFrame *blankFrame()
		// 	{		if( !_blankFrame ) _blankFrame = makeBlueFrame(deckLinkOutput(), true ); return _blankFrame; }
    //
		// void scheduleFrame( IDeckLinkVideoFrame *frame, uint8_t numRepeats = 1 );

  private:

    // bool _stop;
    // int32_t _refCount;

    unsigned long _frameCount;

    InputConfig _config;
    bool _enabled;

    DeckLink &_owner;
    IDeckLink *_deckLink;

    IDeckLinkInput *_deckLinkInput;
    IDeckLinkOutput *_deckLinkOutput;

    // == input member related variables ==
    std::array<cv::Mat,2> _grabbedImages;
    std::array<active_object::shared_queue< cv::Mat >,2> _queues;

    // //== output-related member variables ==
    //
		// // Cached values
		// BMDTimeValue _frameDuration;
		// BMDTimeScale _timeScale;
    //
		// //BMSDIBuffer *_bmsdiBuffer;
    //
		// unsigned int _totalFramesScheduled;
    //
		// std::shared_ptr<SharedBMSDIBuffer> _buffer;
		// IDeckLinkMutableVideoFrame *_blankFrame;
  };

}
