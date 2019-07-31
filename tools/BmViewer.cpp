
// TODO:   Reduce the DRY

#include <string>
#include <thread>
using namespace std;

#include <signal.h>

#include <CLI/CLI.hpp>

#include <libg3logger/g3logger.h>

#include "libblackmagic/DeckLink.h"
#include "libblackmagic/DataTypes.h"
using namespace libblackmagic;

#include "libbmsdi/helpers.h"

bool keepGoing = true;


void signal_handler( int sig )
{
	LOG(INFO) << "Signal handler: " << sig;

	switch( sig ) {
		case SIGINT:
				keepGoing = false;
				break;
		default:
				keepGoing = false;
				break;
	}
}

const int CamNum = 1;



static void processKbInput( char c, DeckLink &decklink ) {

	static uint8_t currentGain = 0x1;
	static uint32_t currentExposure = 0;  // Start at 1/60

	shared_ptr<SharedBMSDIBuffer> sdiBuffer( decklink.output().sdiProtocolBuffer() );

	SDIBufferGuard guard( sdiBuffer );

	switch(c) {
		case 'f':
					// Send absolute focus value
					LOG(INFO) << "Sending instantaneous autofocus to camera";
					guard( []( BMSDIBuffer *buffer ){ bmAddInstantaneousAutofocus( buffer, CamNum ); });
					break;
		 case '[':
					// Send positive focus increment
					LOG(INFO) << "Sending focus increment to camera";
					guard( []( BMSDIBuffer *buffer ){	bmAddFocusOffset( buffer, CamNum, 0.05 ); });
					break;
			case ']':
					// Send negative focus increment
					LOG(INFO) << "Sending focus decrement to camera";
					guard( []( BMSDIBuffer *buffer ){ bmAddFocusOffset( buffer, CamNum, -0.05 ); });
					break;

			//=== Aperture increment/decrement ===
			case ';':
 					// Send positive aperture increment
 					LOG(INFO) << "Sending aperture increment to camera";
 					guard( []( BMSDIBuffer *buffer ){	bmAddOrdinalApertureOffset( buffer, CamNum, 1 ); });
 					break;
 			case '\'':
 					// Send negative aperture decrement
 					LOG(INFO) << "Sending aperture decrement to camera";
					guard( []( BMSDIBuffer *buffer ){	bmAddOrdinalApertureOffset( buffer, CamNum, -1 ); });
 					break;

			//=== Shutter increment/decrement ===

			// case '.':
			// 		currentExposure -= 1000;
			// 		if( currentExposure < 2000 ) currentExposure = 2000;
			// 		LOG(INFO) << "Setting shutter to " << currentExposure << " us";
			// 		guard( []( BMSDIBuffer *buffer ){	bmAddExposureMicroseconds( buffer, CamNum, currentExposure ); });
 			// 		break;
 			// case '/':
			// 		currentExposure += 1000;
			// 		if( currentExposure > 42000 ) currentExposure = 42000;
			// 		LOG(INFO) << "Setting shutter to " << currentExposure << " us";
			// 		guard( []( BMSDIBuffer *buffer ){	bmAddExposureMicroseconds( buffer, CamNum, currentExposure ); });
			// 		break;

			case '.':
					if( currentExposure > 0 ) currentExposure--;
					LOG(INFO) << "Setting exposure to " << currentExposure << "";
					guard( []( BMSDIBuffer *buffer ){	bmAddOrdinalExposure( buffer, CamNum, currentExposure ); });
 					break;
 			case '/':
					currentExposure++;
					LOG(INFO) << "Setting exposure to " << currentExposure << " us";
					guard( []( BMSDIBuffer *buffer ){	bmAddOrdinalExposure( buffer, CamNum, currentExposure ); });
					break;

			//=== Gain increment/decrement ===
			case 'z':
					currentGain <<= 1;
					if( currentGain > 16 ) currentGain = 0x1;
					LOG(INFO) << "Sending gain ISO " << 100*currentGain << " to camera (" << std::hex << int(currentGain) << ")";
					guard( []( BMSDIBuffer *buffer ){	bmAddSensorGain( buffer, CamNum, currentGain ); });
 					break;
 			case 'x':
					currentGain = (currentGain > 1) ? currentGain >> 1 : 0x10;
 					LOG(INFO) << "Sending gain ISO " << 100*currentGain << " to camera (" << std::hex << int(currentGain) << ")";
					guard( []( BMSDIBuffer *buffer ){	bmAddSensorGain( buffer, CamNum, currentGain ); });
 					break;

			//== Increment/decrement white balance
			case 'w':
					LOG(INFO) << "Auto white balance";
					guard( []( BMSDIBuffer *buffer ){	bmAddAutoWhiteBalance( buffer, CamNum ); });
					break;

			case 'e':
					LOG(INFO) << "Restore white balance";
					guard( []( BMSDIBuffer *buffer ){	bmAddRestoreWhiteBalance( buffer, CamNum ); });
					break;

			case 'r':
					LOG(INFO) << "Sending decrement to white balance";
					guard( []( BMSDIBuffer *buffer ){	bmAddWhiteBalanceOffset( buffer, CamNum, -500, 0 ); });
					break;

			case 't':
					LOG(INFO) << "Sending increment to white balance";
					guard( []( BMSDIBuffer *buffer ){	bmAddWhiteBalanceOffset( buffer, CamNum, 500, 0 ); });
					break;

		case 'q':
				keepGoing = false;
				break;
	}

}


using cv::Mat;

int main( int argc, char** argv )
{
	libg3logger::G3Logger logger("bmRecorder");

	signal( SIGINT, signal_handler );

	CLI::App app{"Simple BlackMagic camera viewer."};

	const int cardNum = 0;

	int verbosity = 0;
	app.add_flag("-v", verbosity, "Additional output (use -vv for even more!)");

	bool do3D = false;
	app.add_flag("-d,--do-3d",do3D, "Enable 3D modes");

	bool noDisplay = false;
	app.add_flag("--no-display,-x", noDisplay, "Disable display");

	string desiredModeString = "1080p2997";
	app.add_option("--mode,-m", desiredModeString, "Desired mode");

	bool skipAutoConfig = false;
	app.add_flag("--no-auto-mode", skipAutoConfig, "Don't do auto mode detection");

	bool skipConfigCamera = false;
	app.add_flag("--no-config-camera", skipConfigCamera, "Skip sending config info to cameras");

	bool doListCards = false;
	app.add_flag("--list-cards", doListCards, "List Decklink cards in the system then exit");

	bool doListInputModes = false;
	app.add_flag("--list-input-modes", doListInputModes, "List Input modes then exit");

	int stopAfter = -1;
	app.add_option("--stop-after", stopAfter, "Stop after N frames");

	float scale = 1.0;
	app.add_option("--scale", scale, "Scale");

	CLI11_PARSE(app, argc, argv);

	// Must be showing INFO to the console for either of these modes to show
	// anything
	if( doListCards || doListInputModes ) verbosity = std::max(1,verbosity);

	switch(verbosity) {
		case 1:
			logger.setLevel( INFO );
			break;
		case 2:
			logger.setLevel( DEBUG );
			break;
	}

	// Handle the one-off commands
	if( doListCards ) {
			if(doListCards) DeckLink::ListCards();
			return 0;
	}


	// Help string
	cout << "Commands" << endl;
	cout << "    q       quit" << endl;
	cout << "   [ ]     Adjust focus" << endl;
	cout << "    f      Set autofocus" << endl;
	cout << "   ; '     Adjust aperture" << endl;
	cout << "   . /     Adjust shutter speed" << endl;
	cout << "   z x     Adjust sensor gain" << endl;
	cout << "    s      Cycle through reference sources" << endl;

	DeckLink deckLink( cardNum );

	if( doListInputModes ) {
		if(doListInputModes) deckLink.listInputModes();
		return 0;
	}

	BMDDisplayMode mode = stringToDisplayMode( desiredModeString );
	if( mode == bmdModeUnknown ) {
		LOG(WARNING) << "Didn't understand mode \"" << desiredModeString << "\"";
		return -1;
	} else if ( mode == bmdModeDetect ) {
		LOG(WARNING) << "Card will attempt automatic detection, starting in HD1080p2997 mode";
		mode = bmdModeHD1080p30;
	} else {
		LOG(WARNING) << "Setting initial mode " << desiredModeString;
	}

	const bool doAutoConfig = !skipAutoConfig;
	if( !deckLink.input().enable( mode, doAutoConfig, do3D ) ) {
		LOG(WARNING) << "Failed to enable input";
		return -1;
	}

	if( !deckLink.output().enable( mode, false ) ) {
		LOG(WARNING) << "Failed to enable output";
		return -1;
	}

	// Need to wait for initialization
//	if( decklink.initializedSync.wait_for( std::chrono::seconds(1) ) == false || !decklink.initialized() ) {
	// if( !decklink.initialize() ) {
	// 	LOG(WARNING) << "Unable to initialize DeckLink";
	// 	exit(-1);
	// }

	// std::chrono::steady_clock::time_point start( std::chrono::steady_clock::now() );
	// std::chrono::steady_clock::time_point end( start + std::chrono::seconds( duration ) );

	int count = 0, miss = 0, displayed = 0;

	if( !deckLink.startStreams() ) {
			LOG(WARNING) << "Unable to start streams";
			exit(-1);
	}

	LOG(INFO) << "Streams started!";


	if ( !skipConfigCamera ) {
		LOG(INFO) << "Sending configuration to cameras";

		// Be careful not to exceed 255 byte buffer length
		SDIBufferGuard guard( deckLink.output().sdiProtocolBuffer() );
		guard( [mode]( BMSDIBuffer *buffer ) {

			bmAddAutoExposureMode( buffer, CamNum, BM_AUTOEXPOSURE_SHUTTER );
			//bmAddOrdinalAperture( buffer, CamNum, 0 );
			//bmAddSensorGain( buffer, CamNum, 0 );
			bmAddReferenceSource( buffer, CamNum, BM_REF_SOURCE_PROGRAM );

			//bmAddAutoWhiteBalance( buffer, CamNum );

			if(mode != bmdModeDetect) {
				LOG(INFO) << "Sending video mode " << displayModeToString( mode ) << " to camera";
				bmAddVideoMode( buffer, CamNum, mode );
			}

		});
	} else {
		LOG(DEBUG) << " .. _NOT_ sending config info to cameras";
	}

	while( keepGoing ) {

		std::chrono::steady_clock::time_point loopStart( std::chrono::steady_clock::now() );
		//if( (duration > 0) && (loopStart > end) ) { keepGoing = false;  break; }

		InputHandler::MatVector rawImages, images;
		if( !deckLink.input().queue().wait_for_pop( rawImages, std::chrono::milliseconds(100) ) ) {
			//
			++miss;
			continue;
		}

		if( scale == 1.0 ) {
			images = rawImages;
		} else {
			std::transform( rawImages.begin(), rawImages.end(), back_inserter(images),
											[scale]( const cv::Mat &input ) -> cv::Mat {
														cv::Mat output;
														cv::resize( input, output, cv::Size(), scale, scale);
														return output; }
										);
		}

		if( !noDisplay ) {

			const int numImages = images.size();

			if( numImages == 1 && !images[0].empty()  ) {
				cv::imshow("Image", images[0]);
			} else if ( numImages == 2 ) {

				cv::Mat composite( cv::Size( images[0].size().width + images[0].size().width,
														std::max(images[0].size().height, images[1].size().height )), images[0].type() );

				if( !images[0].empty() ) {
					cv::Mat leftROI( composite, cv::Rect(0,0,images[0].size().width,images[0].size().height) );
					images[0].copyTo( leftROI );
				}

				if( !images[1].empty() ) {
					cv::Mat rightROI( composite, cv::Rect(images[0].size().width, 0, images[1].size().width, images[1].size().height) );
					images[1].copyTo( rightROI );
				}

				cv::imshow("Composite", composite );

			}

			LOG_IF(INFO, (displayed % 50) == 0) << "Frame #" << displayed;
			char c = cv::waitKey(1);

			++displayed;

			// Take action on character
			processKbInput( c, deckLink );
		}

		++count;

		if((stopAfter > 0) && (count >= stopAfter)) keepGoing = false;

	}

	 //std::chrono::duration<float> dur( std::chrono::steady_clock::now()  - start );

	LOG(INFO) << "End of main loop, stopping streams...";

	deckLink.stopStreams();


	// LOG(INFO) << "Recorded " << count << " frames in " <<   dur.count();
	// LOG(INFO) << " Average of " << (float)count / dur.count() << " FPS";
	// LOG(INFO) << "   " << miss << " / " << (miss+count) << " misses";
	// LOG_IF( INFO, displayed > 0 ) << "   Displayed " << displayed << " frames";



		return 0;
	}
