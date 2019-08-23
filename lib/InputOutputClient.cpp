
#include <string>

#include "libblackmagic/DeckLinkAPI.h"
#include <DeckLinkAPIVersion.h>

//#include "g3log/loglevels.hpp"
//#include "libg3logger/g3logger.h"
#include <g3log/logworker.hpp>

#include "libblackmagic/InputOutputClient.h"

namespace libblackmagic {

using std::string;

InputOutputClient::InputOutputClient(int cardNo)
    : _deckLink(cardNo),
      //_configuration( nullptr ),
      _input( _deckLink ),
      _output( _deckLink )
 {
   _input.setInputFormatChangedCallback( std::bind( &OutputHandler::inputFormatChanged, &_output, std::placeholders::_1 ) );
 }

InputOutputClient::~InputOutputClient()
{;}

//=================================================================

bool InputOutputClient::startStreams(void) {

  // TODO.  Check responses
  if (!_output.startStreams())
    return false;
  if (!_input.startStreams())
    return false;
  ;

  return true;
}

void InputOutputClient::stopStreams(void) {
  _input.stopStreams();
  _output.stopStreams();
}

} // namespace libblackmagic
