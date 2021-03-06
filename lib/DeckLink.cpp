
#include <string>

#include "libblackmagic/DeckLinkAPI.h"
#include <DeckLinkAPIVersion.h>

//#include "g3log/loglevels.hpp"
//#include "libg3logger/g3logger.h"
#include <g3log/logworker.hpp>

#include "libblackmagic/DeckLink.h"

namespace libblackmagic {

using std::string;

DeckLink::DeckLink(int cardNo)
    : _deckLink(CreateDeckLink(cardNo))
{
  CHECK(_deckLink != nullptr);
}

DeckLink::~DeckLink() {
  _deckLink->Release();
}

void DeckLink::ListCards() {
  IDeckLink *dl = nullptr;
  IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();

  int i = 0;
  while ((deckLinkIterator->Next(&dl)) == S_OK) {

    const char *modelName = NULL, *displayName = NULL;
    if (dl->GetModelName(&modelName) != S_OK) {
      LOG(WARNING) << "Unable to query model name.";
    }
    if (dl->GetDisplayName(&displayName) != S_OK) {
      LOG(WARNING) << "Unable to query display name.";
    }

    LOG(INFO) << "#" << i << " model name: " << modelName
              << "; display name: " << displayName;

    // free(modelName);
    // free(displayName);
    i++;
  }

  deckLinkIterator->Release();
}

void DeckLink::listInputModes() {

  IDeckLinkInput *input = nullptr;

  // Get the input (capture) interface of the DeckLink device
  auto result = _deckLink->QueryInterface(IID_IDeckLinkInput, (void **)&input);
  if (result != S_OK) {
    LOG(WARNING) << "Couldn't get input for Decklink";
    return;
  }

  IDeckLinkDisplayModeIterator *displayModeItr = NULL;
  IDeckLinkDisplayMode *displayMode = NULL;

  if (input->GetDisplayModeIterator(&displayModeItr) != S_OK) {
    LOG(WARNING) << "Unable to get DisplayModeIterator";
    return;
  }

  // Iterate through available modes
  while (displayModeItr->Next(&displayMode) == S_OK) {

    char *displayModeName = nullptr;
    if (displayMode->GetName((const char **)&displayModeName) != S_OK) {
      LOG(WARNING) << "Unable to get name of DisplayMode";
      return;
    }

    BMDTimeValue timeValue = 0;
    BMDTimeScale timeScale = 0;

    if (displayMode->GetFrameRate(&timeValue, &timeScale) != S_OK) {
      LOG(WARNING) << "Unable to get DisplayMode frame rate";
      return;
    }

    float frameRate =
        (timeScale != 0) ? float(timeValue) / timeScale : float(timeValue);

    LOG(INFO) << "Card supports display mode \"" << displayModeName << "\"    "
              << displayMode->GetWidth() << " x " << displayMode->GetHeight()
              << ", " << 1.0 / frameRate << " FPS"
              << ((displayMode->GetFlags() & bmdDisplayModeSupports3D) ? " (3D)"
                                                                       : "");
    ;
  }

  input->Release();
}


//=================================================================
// Configuration functions
IDeckLink *DeckLink::CreateDeckLink(int cardNo) {
  LOG(DEBUG) << "Using Decklink API  "
             << BLACKMAGIC_DECKLINK_API_VERSION_STRING;
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
  for (i = 0; i <= cardNo; i++) {
    if ((result = deckLinkIterator->Next(&deckLink)) != S_OK) {
      LOG(WARNING) << "Couldn't get information on DeckLink card " << i;
      return nullptr;
    }
  }

  free(deckLinkIterator);

  char *modelName, *displayName;
  if (deckLink->GetModelName((const char **)&modelName) != S_OK) {
    LOG(WARNING) << "Unable to query model name.";
  }

  if (deckLink->GetDisplayName((const char **)&displayName) != S_OK) {
    LOG(WARNING) << "Unable to query display name.";
  }

  // == Board-specific configuration ==

  {
    IDeckLinkProfileAttributes *profileAttributes = nullptr;

    // Get the input (capture) interface of the DeckLink device
    auto result = deckLink->QueryInterface(IID_IDeckLinkProfileAttributes,
                                           (void **)&profileAttributes);
    if (result == S_OK) {

      int64_t value = 0;
      if (profileAttributes->GetInt(BMDDeckLinkProfileID, &value) == S_OK) {

        string profileStr = "(unknown)";
        switch (value) {
        case bmdProfileOneSubDeviceFullDuplex:
          profileStr = "bmdProfileOneSubDeviceFullDuplex";
          break;
        case bmdProfileOneSubDeviceHalfDuplex:
          profileStr = "bmdProfileOneSubDeviceHalfDuplex";
          break;
        case bmdProfileTwoSubDevicesFullDuplex:
          profileStr = "bmdProfileTwoSubDevicesFullDuplex";
          break;
        case bmdProfileTwoSubDevicesHalfDuplex:
          profileStr = "bmdProfileTwoSubDevicesHalfDuplex";
          break;
        case bmdProfileFourSubDevicesHalfDuplex:
          profileStr = "bmdProfileFourSubDevicesHalfDuplex";
          break;
        }

        LOG(INFO) << "Has profile " << profileStr << " (0x" << std::hex << value
                  << ")";

      } else {
        LOG(WARNING) << "Unable to query profile ID";
      }

    } else {
      LOG(WARNING) << "Couldn't get profileAttributes for DeckLink";
    }

    profileAttributes->Release();
  }

  LOG(INFO) << "Using card " << cardNo << " model name: " << modelName;
  // display name: " << displayName;

  free(modelName);
  free(displayName);

  return deckLink;
}

} // namespace libblackmagic
