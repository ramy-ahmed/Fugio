#include "fftnode.h"

#include <fugio/core/uuid.h>
#include <fugio/audio/uuid.h>

#include <fugio/context_interface.h>
#include <fugio/core/variant_interface.h>

#include <fugio/context_signals.h>
#include <fugio/performance.h>
#include <fugio/pin_signals.h>

#include <QFile>
#include <QLabel>
#include <QImage>
#include <QPainter>

QMap<QString,FFTNode::WindowType>		 FFTNode::mWindowTypes;

FFTNode::FFTNode( QSharedPointer<fugio::NodeInterface> pNode )
	: NodeControlBase( pNode ), mWindowType( HANNING ), mSampleCount( 2048 ), mSamplePosition( 0 ),
	  mProducerInstance( nullptr )
	#if defined( FFTW_PLUGIN_SUPPORTED )
	, mPlan( 0 ), mBufSrc( 0 ), mBufDst( 0 )
	#endif
{
	static const QUuid	PID_WINDOW   = QUuid( "{f6830fed-0b9d-4ecf-94e5-156cd704a102}" );

	if( mWindowTypes.isEmpty() )
	{
		mWindowTypes.insert( "None",		WindowType::NONE );
		mWindowTypes.insert( "Hanning",		WindowType::HANNING );
		mWindowTypes.insert( "Hamming",		WindowType::HAMMING );
		mWindowTypes.insert( "Blackman",	WindowType::BLACKMAN );
		mWindowTypes.insert( "Parzen",		WindowType::PARZEN );
		mWindowTypes.insert( "Welch",		WindowType::WELCH );
		mWindowTypes.insert( "Steeper",		WindowType::STEEPER );
	}

	mPinInputAudio = pinInput( "Audio" );

	mValOutputFFT = pinOutput<fugio::FftInterface *>( "FFT", mPinOutputFFT, PID_FFT );

	mValOutputFFT->setSource( this );

	mPinInputSamples = pinInput( tr( "Samples" ) );

	mPinInputSamples->setValue( mSampleCount );


	mPinInputWindowChoice = pinInput<fugio::ChoiceInterface *>( tr( "Window" ), mPinInputWindow, PID_CHOICE, PID_WINDOW );

	mPinInputWindowChoice->setChoices( mWindowTypes.keys() );

	mPinInputWindow->setValue( mWindowTypes.key( mWindowType ) );


	mPinInputAudio->setDescription( tr( "An audio input" ) );

	mPinInputSamples->setDescription( tr( "The number of samples to calculate each FFT from" ) );

	mPinOutputFFT->setDescription( tr( "The output Fast Fourier Transform" ) );

	mPinInputWindow->setDescription( tr( "The window function that will be applied to the audio before FFT processing" ) );
}

FFTNode::~FFTNode( void )
{	
	fftwFree();
}

bool FFTNode::initialise()
{
	if( !fugio::NodeControlBase::initialise() )
	{
		return( false );
	}

	if( mPinInputAudio->isConnected() && !mPinInputAudio->connectedNode()->isInitialised() )
	{
		return( false );
	}

	connect( mPinInputAudio->qobject(), SIGNAL(linked(QSharedPointer<fugio::PinInterface>)), this, SLOT(audioPinLinked(QSharedPointer<fugio::PinInterface>)) );
	connect( mPinInputAudio->qobject(), SIGNAL(unlinked(QSharedPointer<fugio::PinInterface>)), this, SLOT(audioPinUnlinked(QSharedPointer<fugio::PinInterface>)) );

	if( mPinInputAudio->isConnected() )
	{
		audioPinLinked( mPinInputAudio->connectedPin() );
	}

	return( true );
}

bool FFTNode::deinitialise()
{
	return( NodeControlBase::deinitialise() );
}

void FFTNode::onContextFrame( qint64 pTimeStamp )
{
	if( mProducerInstance && !mProducerInstance->isValid() )
	{
		delete mProducerInstance;

		mProducerInstance = nullptr;

		fftwFree();
	}

	fugio::AudioProducerInterface	*API = input<fugio::AudioProducerInterface *>( mPinInputAudio );

	if( !API )
	{
		if( mProducerInstance )
		{
			delete mProducerInstance;

			mProducerInstance = nullptr;

			fftwFree();

			mSamplePosition = 0;
		}

		return;
	}

	if( !mProducerInstance )
	{
		mProducerInstance = API->audioAllocInstance( 48000, fugio::AudioSampleFormat::Format32FS, 1 );

		calculateWindow();
	}

	if( !mProducerInstance )
	{
		return;
	}

	const qint64	CurPos = ( mNode->context()->global()->timestamp() * 48000 ) / 1000;

	if( CurPos - mSamplePosition > 48000 )
	{
		mSamplePosition = CurPos - samples() - API->audioLatency();
	}

	if( CurPos - mSamplePosition < samples() )
	{
		return;
	}

	fugio::Performance	P( mNode, "onContextFrame", pTimeStamp );

#if defined( FFTW_PLUGIN_SUPPORTED )
	if( !mBufSrc )
	{
		mBufSrc = fftwf_alloc_real( samples() );
	}

	if( !mBufDst )
	{
		mBufDst = fftwf_alloc_complex( count() );
	}

	if( !mPlan )
	{
		mPlan = fftwf_plan_dft_r2c_1d( samples(), mBufSrc, mBufDst, 0 );
	}

	float		*AudPtr = mBufSrc;

	memset( mBufSrc, 0, sizeof( float ) * samples() );

	mProducerInstance->audio( mSamplePosition, samples(), 0, 1, (void **)&AudPtr );

	//			if( true )
	//			{
	//				QFile		TEST_FILE( "/Users/bigfug/Desktop/TEST_FILE.raw" );

	//				if( TEST_FILE.open( QIODevice::Append ) || TEST_FILE.open( QIODevice::WriteOnly ) )
	//				{
	//					TEST_FILE.write( (const char *)mBufSrc, sizeof( float ) * samples() );

	//					TEST_FILE.close();
	//				}
	//			}

	if( mWindowType != NONE )
	{
		for( int i = 0 ; i < samples() ; i++ )
		{
			mBufSrc[ i ] *= mWindow[ i ];
		}
	}

	fftwf_execute( mPlan );

	pinUpdated( mPinOutputFFT );
#endif

	mSamplePosition += samples();
}

void FFTNode::audioPinLinked( QSharedPointer<fugio::PinInterface> P )
{
	Q_UNUSED( P )

	connect( mNode->context()->qobject(), SIGNAL(frameProcess(qint64)), this, SLOT(onContextFrame(qint64)) );
}

void FFTNode::audioPinUnlinked( QSharedPointer<fugio::PinInterface> P )
{
	Q_UNUSED( P )

	disconnect( mNode->context()->qobject(), SIGNAL(frameProcess(qint64)), this, SLOT(onContextFrame(qint64)) );
}

bool is_power_of_2(int i)
{
	if ( i <= 0 ) {
		return 0;
	}
	return ! (i & (i-1));
}

void FFTNode::inputsUpdated( qint64 pTimeStamp )
{
	fugio::VariantInterface	*V;

	if( !pTimeStamp )
	{
		mSamplePosition = 0;
	}

	WindowType	WT = mWindowTypes.value( variant( mPinInputWindow ).toString() );

	if( WT != mWindowType )
	{
		mWindowType = WT;

		calculateWindow();
	}

	int			SampleCount = mSampleCount;

	if( ( V = input<fugio::VariantInterface *>( mPinInputSamples ) ) != 0 )
	{
		SampleCount = V->variant().toInt();
	}
	else
	{
		SampleCount = mPinInputSamples->value().toInt();
	}

	if( SampleCount <= 0 || SampleCount == mSampleCount )
	{
		return;
	}

	if( !is_power_of_2( SampleCount ) )
	{
		return;
	}

	mSampleCount    = SampleCount;
	mSamplePosition = 0;

	calculateWindow();

	fftwFree();
}

void FFTNode::calculateWindow()
{
	const	float	nn = float( samples() - 1 );

	mWindow.resize( samples() );

	switch( mWindowType )
	{
		case NONE:
			for( int i = 0 ; i < samples() ; i++ )
			{
				mWindow[ i ] = 1.0f;
			}
			break;

		case HANNING:
			for( int i = 0 ; i < samples() ; i++ )
			{
				mWindow[ i ] = hanning( i, nn );
			}
			break;

		case HAMMING:
			for( int i = 0 ; i < samples() ; i++ )
			{
				mWindow[ i ] = hamming( i, nn );
			}
			break;

		case BLACKMAN:
			for( int i = 0 ; i < samples() ; i++ )
			{
				mWindow[ i ] = blackman( i, nn );
			}
			break;

		case PARZEN:
			for( int i = 0 ; i < samples() ; i++ )
			{
				mWindow[ i ] = parzen( i, nn );
			}
			break;

		case WELCH:
			for( int i = 0 ; i < samples() ; i++ )
			{
				mWindow[ i ] = welch( i, nn );
			}
			break;

		case STEEPER:
			for( int i = 0 ; i < samples() ; i++ )
			{
				mWindow[ i ] = steeper( i, nn );
			}
			break;
	}
}

void FFTNode::fftwFree()
{
#if defined( FFTW_PLUGIN_SUPPORTED )
	if( mBufSrc )
	{
		fftwf_free( mBufSrc );

		mBufSrc = nullptr;
	}

	if( mBufDst )
	{
		fftwf_free( mBufDst );

		mBufDst = nullptr;
	}

	if( mPlan )
	{
		fftwf_destroy_plan( mPlan );

		mPlan = nullptr;
	}
#endif
}
