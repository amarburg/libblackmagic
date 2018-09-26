#pragma once

#include <DeckLinkAPI.h>

#include "DataTypes.h"

namespace libblackmagic {

class ModeConfig {
public:
  ModeConfig( BMDDisplayMode defaultMode = bmdModeDetect )
    : _do3D( false ),
      _mode( defaultMode )
  {;}

  ModeConfig &set3D( bool do3D = true )
    { _do3D = do3D;  return *this; }

  ModeConfig &setMode( BMDDisplayMode m )
    { _mode = m; return *this; }


  bool           do3D() const     { return _do3D; }
  BMDDisplayMode mode() const     { return _mode; }

  ModeParams     params() const     { return modeParams(_mode); }
  unsigned int   height() const     { return params().height; }
  unsigned int   width() const      { return params().width; }
  float          frameRate() const  { return params().frameRate; }

private:
  bool _do3D;
  BMDDisplayMode _mode;
};

}
