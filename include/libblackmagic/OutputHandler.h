#pragma once

#include <memory>

#include <DeckLinkAPI.h>
#include <active_object/active.h>

#include "SDICameraControl.h"

#include "InputConfig.h"

#include "SDIMessageBuffer.h"

namespace libblackmagic {

	class DeckLink;

	class OutputHandler: public IDeckLinkVideoOutputCallback
	{
	public:
		OutputHandler( DeckLink &owner );
		virtual ~OutputHandler(void);

		// Retrieve the current configuration
    InputConfig &config() { return _config; }
		void setConfig( const InputConfig &config ) { _config = config; }

		// Lazy initializer
		IDeckLinkOutput *deckLinkOutput();

		bool enable( void );

		//void setBMSDIBuffer( const std::shared_ptr<SharedBMBuffer> &buffer );

		const std::shared_ptr<SharedBMSDIBuffer> &sdiProtocolBuffer()
			{ return _buffer; }

		void inputFormatChanged( BMDDisplayMode mode );

		HRESULT	STDMETHODCALLTYPE ScheduledFrameCompleted(IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result);
		HRESULT	STDMETHODCALLTYPE ScheduledPlaybackHasStopped(void);


		// Dummy implementations
		HRESULT	STDMETHODCALLTYPE QueryInterface (REFIID iid, LPVOID *ppv){ return E_NOINTERFACE; }
		ULONG STDMETHODCALLTYPE AddRef() { return 1; }
		ULONG STDMETHODCALLTYPE Release() { return 1; }

		bool startStreams( void );
		bool stopStreams( void );


		// Condition variables
		std::condition_variable _scheduledPlaybackStoppedCond;
		std::mutex _scheduledPlaybackStoppedMutex;

	protected:

		// Lazy initializer
		IDeckLinkMutableVideoFrame *blankFrame()
			{		if( !_blankFrame ) _blankFrame = makeBlueFrame(deckLinkOutput(), true ); return _blankFrame; }

		void scheduleFrame( IDeckLinkVideoFrame *frame, uint8_t numRepeats = 1 );

	private:

		InputConfig _config;
		bool _enabled;

		DeckLink &_owner;
		IDeckLink *_deckLink;
		IDeckLinkOutput *_deckLinkOutput;

		// Cached values
		BMDTimeValue _frameDuration;
		BMDTimeScale _timeScale;

		//BMSDIBuffer *_bmsdiBuffer;

		unsigned int _totalFramesScheduled;

		std::shared_ptr<SharedBMSDIBuffer> _buffer;
		IDeckLinkMutableVideoFrame *_blankFrame;

	};

}
