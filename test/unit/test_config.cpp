#include <gtest/gtest.h>

#include "libblackmagic/ModeConfig.h"

using namespace libblackmagic;

// Test the float to fixed16 conversion
TEST(TestConfig, goodModes) {

  {
    const BMDDisplayMode mode = bmdModeHD1080p2997;
    ModeConfig c( mode );

    ASSERT_EQ( c.mode(), mode );

    ASSERT_EQ( c.width(), 1920 );
    ASSERT_EQ( c.height(), 1080 );
    ASSERT_FLOAT_EQ( c.frameRate(), 29.97 );

    auto params = c.params();
    ASSERT_TRUE( params.valid() );

  }

}
