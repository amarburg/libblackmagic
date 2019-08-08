# libblackmagic

This library provides an OO-ish interface to the [Blackmagic DeckLink](https://www.blackmagicdesign.com/products/decklink) SDI interface cards.  I will admit it is closely tuned to my particular needs (capture of mono and stereo video over SDI), and isn't very "generic."

The build is coordinated with [fips](https://github.com/floooh/fips).

This library has a number of dependencies.  Most of them are handled by fips, but both the [Blackmagic Desktop Video and Desktop Video API](https://www.blackmagicdesign.com/developer/product/capture-and-playback) must be downloaded and installed separately.  The file [cmake/FindBlackmagicSDK.cmake](cmake/FindBlackmagicSDK.cmake) searches for the API ... if needed, the environment variable `BLACKMAGIC_DIR` can be provided as hint.

__We are currently building against Blackmagic API version 11.2__

To build:

    ./fips fetch
    ./fips gen
    ./fips build

In general, this can be shortened to:

    ./fips build

This package builds a single binary, `bm_viewer` which can display video and send remote control commands to the camera.

# License

This code is released under the [MIT License](LICENSE).
