#pragma once

#include <atomic>

#include "DeckLinkAPI.h"

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

    // Thin wrapper around refcounting functions
    ULONG AddRef(void)        { return _deckLink->AddRef(); }
    ULONG Release(void)       { return _deckLink->Release(); }


    // == Public API =========================

    static void ListCards();

    void listInputModes();

    IDeckLink *deckLink() { return _deckLink; }

  protected:

    static IDeckLink *CreateDeckLink( int cardNo );

  private:

    IDeckLink *_deckLink;
  };

}
