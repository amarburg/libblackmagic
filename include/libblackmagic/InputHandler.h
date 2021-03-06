#pragma once

//#include <queue>
#include <condition_variable>
#include <mutex>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "active_object/bounded_shared_queue.h"
#include "DeckLinkAPI.h"

#include "ThreadSynchronizer.h"

#include "DataTypes.h"
#include "ModeConfig.h"

#include "SDICameraControl.h"
#include "SDIMessageBuffer.h"

#include "libblackmagic/DeckLink.h"

namespace libblackmagic {

  using std::vector;

  class InputHandler : public IDeckLinkInputCallback
  {
  public:

    typedef std::vector<cv::Mat> MatVector;
    typedef std::vector<IDeckLinkVideoFrame *> FrameVector;

    typedef active_object::bounded_shared_queue< MatVector, 10 > Queue;

    InputHandler( DeckLink &deckLink );
    virtual ~InputHandler();

    // Attempts to configure the input stream.   If not called explicitly,
    // will be called automatically by startStreams()
    bool enable( BMDDisplayMode mode = bmdModeHD1080p2997, bool doAuto = true, bool do3D = true );

    bool startStreams();
    bool stopStreams();

    //== IDeckLinkInterfaces callbacks ==
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE  Release(void);

    //== IDeckLinkInputCallback callbacks ==
    virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
    virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);


    typedef std::function< void( const MatVector & ) > NewImagesCallback;
    void setNewImagesCallback( NewImagesCallback callback );

    typedef std::function< void(BMDDisplayMode mode) > InputFormatChangedCallback;
    void setInputFormatChangedCallback( InputFormatChangedCallback );

    //
    ModeConfig currentConfig() const { return _currentConfig; }

  protected:

    // Process input frames
    void process( FrameVector frames );
    void frameToMat( IDeckLinkVideoFrame *videoFrame, cv::Mat &mat, int i );


  private:

    unsigned long _frameCount;
    unsigned long _noInputCount;

    BMDPixelFormat _pixelFormat;
    ModeConfig _currentConfig;
    bool _enabled;

    DeckLink &_deckLink;

    IDeckLinkInput *_deckLinkInput;
    IDeckLinkConfiguration *_dlConfiguration;

    // == input member related variables ==
    MatVector _grabbedImages;
    //Queue _queue;

    NewImagesCallback _newImagesCallback;
    InputFormatChangedCallback _inputFormatChangedCallback;

  };

}
