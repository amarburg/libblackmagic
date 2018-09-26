#include <gtest/gtest.h>

#include "libblackmagic/DataTypes.h"

using namespace libblackmagic;

// Test the float to fixed16 conversion
TEST(TestModeParams, goodModes) {

  {
    auto p = modeParams( bmdModeHD1080p2997 );

    ASSERT_TRUE( p.valid() );
    ASSERT_EQ( p.width, 1920 );
    ASSERT_EQ( p.height, 1080 );
    ASSERT_FLOAT_EQ( p.frameRate, 29.97 );
  }

}

TEST(TestModeParams, invalidMode) {

  {
    // I can't imagine defining this mode...
    auto p = modeParams( bmdModeCintelRAW );

    ASSERT_FALSE( p.valid() );
  }

}
