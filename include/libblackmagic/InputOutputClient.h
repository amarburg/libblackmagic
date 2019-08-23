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
  class InputOutputClient {
  public:

  	InputOutputClient( int cardno = 0 );
    ~InputOutputClient();

    // Delete the copy operators
    InputOutputClient( const InputOutputClient & ) = delete;
    InputOutputClient &operator=( const InputOutputClient & ) = delete;

    // These start and stop the input streams
    bool startStreams();
    void stopStreams();

    InputHandler &input()     { return _input; }
    OutputHandler &output()   { return _output; }


  protected:

//    static IDeckLink *CreateDeckLink( int cardNo );

  private:

    // int _cardNo;

    DeckLink _deckLink;

    InputHandler  _input;
    OutputHandler _output;

  };

}
