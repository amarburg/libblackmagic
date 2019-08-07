#pragma once

#include <atomic>

#include "DeckLinkAPI.h"

#include "active_object/active.h"

#include "InputHandler.h"
#include "OutputHandler.h"

namespace libblackmagic {

  // OO-ish abstraction around a Blackmagic "DeckLink"
  // e.g. one capture card / source.
  // It's really just a container for Blackmagic API IDeckLink,
  // IDeckLinkInput and IDeckLinkOutput instances, plus the callback handlers
  // InputHandler and OutputHandler
  //
  class DeckLink {
  public:

  	DeckLink( int cardno = 0 );
    ~DeckLink();

    // Delete the copy operators
    DeckLink( const DeckLink & ) = delete;
    DeckLink &operator=( const DeckLink & ) = delete;

    // == Public API =========================
    void sendInputFormatChanged( BMDDisplayMode newMode ) {
			_thread->send( std::bind(&DeckLink::inputFormatChanged, this, newMode) );
	   }

    static void ListCards();
    void listInputModes();

    IDeckLink *deckLink() { return _deckLink; }

    // These start and stop the input streams
    bool startStreams();
    void stopStreams();

    // Lazy constructors
    InputHandler &input()     { return *_inputHandler; }
    OutputHandler &output()   { return *_outputHandler; }

    // Set callbacks
    typedef std::function<void(BMDDisplayMode)> InputFormatChangedCallback;
    void setInputFormatChangedCallback( InputFormatChangedCallback cb ) { _onInputFormatChanged = cb; }

  protected:

    static IDeckLink *CreateDeckLink( int cardNo );

    // Callbacks to public API (runs in thread)
    void inputFormatChanged( BMDDisplayMode newMode );

  private:

    // int _cardNo;

    // For now assume an object uses just one Decklink board
    // COM model precludes use of auto ptrs
    IDeckLink *_deckLink;
    IDeckLinkConfiguration *_configuration;
    // IDeckLinkInput *_deckLinkInput;
    // IDeckLinkOutput *_deckLinkOutput;

    InputHandler  *_inputHandler;
    OutputHandler *_outputHandler;

    std::unique_ptr<active_object::Active> _thread;

    InputFormatChangedCallback _onInputFormatChanged;
    static void DefaultInputFormatChanged( BMDDisplayMode mode ) {;}

  };

}
