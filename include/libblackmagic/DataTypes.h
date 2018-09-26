#pragma once

#include <string>

#include <DeckLinkAPI.h>

enum {
  bmdModeDetect = /* 'auto' */ 0x6175746F,
};

namespace libblackmagic {

  BMDDisplayMode stringToDisplayMode( const std::string &str );
  const std::string displayModeToString( BMDDisplayMode mode );


  struct ModeParams {
    BMDDisplayMode mode;
    unsigned int width, height;
    float frameRate;

    ModeParams( BMDDisplayMode m, unsigned int w, unsigned int h, float f )
      : mode(m), width(w), height(h), frameRate(f) {;}

    bool valid() const { return (mode != bmdModeUnknown); }
  };

  ModeParams modeParams( BMDDisplayMode mode );

};
