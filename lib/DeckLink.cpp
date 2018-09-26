
#include <string>

#include <DeckLinkAPIVersion.h>

#include "g3log/loglevels.hpp"
#include "libg3logger/g3logger.h"

#include "libblackmagic/DeckLink.h"

namespace libblackmagic {

  using std::string;
  using namespace active_object;

  DeckLink::DeckLink( int cardNo )
  : _deckLink( createDeckLink( cardNo ) ),
    //_configuration( nullptr ),
    _inputHandler( nullptr  ),
    _outputHandler( nullptr ),
    _thread( Active::createActive() )
  {
    CHECK( _deckLink != nullptr );

    _inputHandler = new InputHandler( *this );
    CHECK( _inputHandler != nullptr );

    _outputHandler = new OutputHandler( *this );
    CHECK( _outputHandler  != nullptr );
  }

  DeckLink::~DeckLink()
  {
    // if( _deckLinkOutput ) {
    //   // Disable the video input interface
    //   _deckLinkOutput->DisableVideoOutput();
    //   _deckLinkOutput->Release();
    // }
    //
    // if( _deckLinkInput ) {
    //   _deckLinkInput->StopStreams();
    //   _deckLinkInput->Release();
    // }

    if( _inputHandler ) delete _inputHandler;
    if( _outputHandler) delete _outputHandler;

    _deckLink->Release();

  }


  void DeckLink::listCards() {
    IDeckLink *dl = nullptr;
    IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();

    int i = 0;
    while( (deckLinkIterator->Next(&dl)) == S_OK ) {
      char *modelName, *displayName;
      if( dl->GetModelName( (const char **)&modelName ) != S_OK ) {
        LOG(WARNING) << "Unable to query model name.";
      }
      if( dl->GetDisplayName( (const char **)&displayName ) != S_OK ) {
        LOG(WARNING) << "Unable to query display name.";
      }

      LOG(INFO) << "#" << i << " model name: " << modelName << "; display name: " << displayName;

      free(modelName);
      free(displayName);
      i++;
    }

    deckLinkIterator->Release();
  }

  void DeckLink::listInputModes() {

    IDeckLinkInput *input = nullptr;

    // Get the input (capture) interface of the DeckLink device
    auto result = _deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&input);
    if (result != S_OK) {
      LOG(WARNING) << "Couldn't get input for Decklink";
      return;
    }

    IDeckLinkDisplayModeIterator* displayModeItr = NULL;
    IDeckLinkDisplayMode *displayMode = NULL;

    if( input->GetDisplayModeIterator(&displayModeItr) != S_OK ) {
      LOG(WARNING) << "Unable to get DisplayModeIterator";
      return;
    }

    // Iterate through available modes
    while( displayModeItr->Next( &displayMode ) == S_OK ) {

      char *displayModeName = nullptr;
      if( displayMode->GetName( (const char **)&displayModeName) != S_OK ) {
        LOG(WARNING) << "Unable to get name of DisplayMode";
        return;
      }

      BMDTimeValue timeValue = 0;
      BMDTimeScale timeScale = 0;

      if( displayMode->GetFrameRate( &timeValue, &timeScale ) != S_OK ) {
        LOG(WARNING) << "Unable to get DisplayMode frame rate";
        return;
      }

      float frameRate = (timeScale != 0) ? float(timeValue)/timeScale : float(timeValue);

      LOG(INFO) << "Card supports display mode \"" << displayModeName << "\"    "
                    << displayMode->GetWidth() << " x " << displayMode->GetHeight()
                    << ", " << 1.0/frameRate << " FPS"
        << ( (displayMode->GetFlags() & bmdDisplayModeSupports3D) ? " (3D)" : "" );;

    }

    input->Release();

    }

  // IDeckLinkConfiguration *DeckLink::configuration() {
  //
  //   if( !_configuration ) {
  //     CHECK( S_OK == _deckLink->QueryInterface(IID_IDeckLinkConfiguration, (void**)&_configuration) )
  //                   << "Could not obtain the IDeckLinkConfiguration interface";
  //     CHECK(_configuration != nullptr );
  //   }
  //
  //   return _configuration;
  // }

  //=================================================================
  // Configuration functions
  IDeckLink *DeckLink::createDeckLink( int cardNo )
  {
    //LOG(INFO) << "Using Decklink API  " << BLACKMAGIC_DECKLINK_API_VERSION_STRING;
    //
    // if( _deckLink ) {
    //   _deckLink->Release();
    //   _deckLink = NULL;
    // }

    HRESULT result;

    IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();
    IDeckLink *deckLink = nullptr;

    // Index cards by number for now
    int i;
    for( i = 0; i <= cardNo; i++ ) {
      if( (result = deckLinkIterator->Next(&deckLink)) != S_OK) {
        LOG(WARNING) << "Couldn't get information on DeckLink card " << i;
        return nullptr;
      }
    }

    free( deckLinkIterator );

    char *modelName, *displayName;
    if( deckLink->GetModelName( (const char **)&modelName ) != S_OK ) {
      LOG(WARNING) << "Unable to query model name.";
    }

    if( deckLink->GetDisplayName( (const char **)&displayName ) != S_OK ) {
      LOG(WARNING) << "Unable to query display name.";
    }

    //LOG(INFO) << "Using card " << cardNo << " model name: " << modelName << "; display name: " << displayName;

    free(modelName);
    free(displayName);

   return deckLink;
  }

  //== API functions =================================================
  void DeckLink::inputFormatChanged( BMDDisplayMode newMode )
  {
    LOG(INFO) << "In inputFormatChanged with mode " << displayModeToString( newMode );

    // Change output mode
    _outputHandler->stopStreamsWait();

    _outputHandler->disable();
    _outputHandler->enable( newMode );

    LOG(INFO) << "Restarting streams";

    _outputHandler->startStreams();
  }

  //=================================================================

  bool DeckLink::startStreams( void )
  {

    // TODO.  Check responses
    if( !_outputHandler->startStreams() ) return false;
    if( !_inputHandler->startStreams() ) return false;;

    return true;
  }

  void DeckLink::stopStreams( void )
  {
    _inputHandler->stopStreams();
    _outputHandler->stopStreams();

  }

}
