

#include <algorithm>
#include <iostream>
using namespace std;

#include <libg3logger/g3logger.h>

#include "libblackmagic/DataTypes.h"


namespace libblackmagic {

  struct StringToDisplayTableMember {
    uint32_t    mode;
    const char *name;
  } StringToDisplayTable[] = {
    // Incoming strings are toupper()'ed' before lookup in the table
    {bmdModeHD1080p25,     "1080P25"},
    {bmdModeHD1080p25,     "HD1080P25"},
    {bmdModeHD1080p2997,   "1080P2997"},
    {bmdModeHD1080p2997,   "HD1080P2997"},
    {bmdModeHD1080p30,     "1080P30"},
    {bmdModeHD1080p30,     "HD1080P30"},
    {bmdModeHD1080p6000,   "1080P60"},
    {bmdModeHD1080p6000,   "HD1080P60"},
    {bmdMode4K2160p25,     "4K25"},
    {bmdMode4K2160p2997,   "4K2997"},
    {bmdModeDetect,        "DETECT"}
  };


  BMDDisplayMode stringToDisplayMode( const std::string &str )
  {
    std::string upcase;
    std::transform(str.begin(), str.end(), std::back_inserter(upcase), ::toupper);

    for( unsigned int i = 0; i < sizeof(StringToDisplayTable)/sizeof(StringToDisplayTableMember); ++i ) {
      if( upcase == StringToDisplayTable[i].name ) return StringToDisplayTable[i].mode;
    }

    return bmdModeUnknown;

  }

  const std::string displayModeToString( BMDDisplayMode mode )
  {
    for( unsigned int i = 0; i < sizeof(StringToDisplayTable)/sizeof(StringToDisplayTableMember); ++i ) {
      if( mode == StringToDisplayTable[i].mode ) return StringToDisplayTable[i].name;
    }

    return "(unknown)";
  }

  //=== Pixel format to string ===========================

  const std::string pixelFormatToString( BMDPixelFormat pix )
  {
    if( pix == bmdFormat8BitYUV )
      return "bmdFormat8BitYUV";
    else if ( pix == bmdFormat8BitARGB )
      return "bmdFormat8BitARGB";
    else if ( pix == bmdFormat8BitBGRA )
      return "bmdFormat8BitBGRA";
    else if ( pix == bmdFormat10BitYUV )
      return "bmdFormat10BitYUV";
    return "(unknown)";
  }




  //=== Mode parameters table ============================

  ModeParams ModeParamsTable[] = {
    { bmdModeHD1080p2997, 1920, 1080, 29.97 },
    { bmdModeHD1080p30,   1920, 1080, 30.0 },
    { bmdModeHD1080p6000, 1920, 1080, 60.0 }
  };

  ModeParams modeParams( BMDDisplayMode mode )
  {
    for( unsigned int i = 0; i < sizeof(ModeParamsTable)/sizeof(ModeParams); ++i ) {
      if( mode == ModeParamsTable[i].mode ) return ModeParamsTable[i];
    }

    cerr << "Unable to find mode " << mode << endl;
    return ModeParams( bmdModeUnknown, 0, 0, 0.0 );
  }



};
