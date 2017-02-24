#include "videocapturenode.h"

#include <QComboBox>

#include <fugio/core/uuid.h>
#include <fugio/image/uuid.h>

#include <fugio/context_signals.h>

VideoCaptureNode::VideoCaptureNode( QSharedPointer<fugio::NodeInterface> pNode )
	: NodeControlBase( pNode ), mDeviceIndex( -1 )
#if defined( VIDEOCAPTURE_SUPPORTED )
	 , mCapture( &VideoCaptureNode::frameCallbackStatic, this )
#endif
{
	FUGID( PIN_OUTPUT_IMAGE, "9e154e12-bcd8-4ead-95b1-5a59833bcf4e" );

	mValOutputImage = pinOutput<fugio::ImageInterface *>( "Image", mPinOutputImage, PID_IMAGE, PIN_OUTPUT_IMAGE );
}

bool VideoCaptureNode::initialise( void )
{
	if( !NodeControlBase::initialise() )
	{
		return( false );
	}

	return( true );
}

bool VideoCaptureNode::deinitialise( void )
{
	mCapture.stop();

	disconnect( mNode->context()->qobject(), SIGNAL(frameStart()), this, SLOT(frameStart()) );

	mCapture.close();

	return( NodeControlBase::deinitialise() );
}

void VideoCaptureNode::inputsUpdated( qint64 pTimeStamp )
{
	NodeControlBase::inputsUpdated( pTimeStamp );
}

void VideoCaptureNode::frameCallbackStatic( ca::PixelBuffer &pBuffer )
{
	static_cast<VideoCaptureNode *>( pBuffer.user )->frameCallback( pBuffer );
}

void VideoCaptureNode::frameCallback( ca::PixelBuffer &pBuffer )
{
	if( mValOutputImage->width() != pBuffer.width[ 0 ] || mValOutputImage->height() != pBuffer.height[ 0 ] )
	{
		mValOutputImage->setSize( pBuffer.width[ 0 ], pBuffer.height[ 0 ] );

		switch( pBuffer.pixel_format )
		{
			case CA_YUV422P:                                                             /* YUV422 Planar */
			case CA_YUV420BP:                                                            /* YUV420 Bi Planar */
			case CA_YUVJ420P:                                                          /* YUV420 Planar Full Range (JPEG), J comes from the JPEG. (values 0-255 used) */
			case CA_YUVJ420BP:                                                          /* YUV420 Bi-Planer Full Range (JPEG), J comes fro the JPEG. (values: luma = [16,235], chroma=[16,240]) */
			case CA_JPEG_OPENDML:                                                          /* JPEG with Open-DML extensions */
			case CA_H264:                                                                  /* H264 */
			case CA_MJPEG:                                                                /* MJPEG 2*/
				return;

			case CA_YUV420P:                                                           /* YUV420 Planar */
				mValOutputImage->setFormat( fugio::ImageInterface::FORMAT_YUV420P );
				break;

			case CA_UYVY422:                                                              /* Cb Y0 Cr Y1 */
				mValOutputImage->setFormat( fugio::ImageInterface::FORMAT_YUYV422 );
				break;

			case CA_ARGB32:                                                              /* ARGB 8:8:8:8 32bpp, ARGBARGBARGB... */
			case CA_RGBA32:                                                              /* RGBA 8:8:8:8 32bpp. */
				mValOutputImage->setFormat( fugio::ImageInterface::FORMAT_RGBA8 );
				break;

			case CA_BGRA32:                                                             /* BGRA 8:8:8:8 32bpp, BGRABGRABGRA... */
				mValOutputImage->setFormat( fugio::ImageInterface::FORMAT_BGRA8 );
				break;

			case CA_RGB24:                                                              /* RGB 8:8:8 24bit */
				mValOutputImage->setFormat( fugio::ImageInterface::FORMAT_RGB8 );
				break;
		}

		for( int i = 0 ; i < 3 ; i++ )
		{
			mValOutputImage->setLineSize( i, pBuffer.stride[ i ] );
		}
	}

	if( !mValOutputImage->isValid() )
	{
		return;
	}

	for( int i = 0 ; i < 3 ; i++ )
	{
		const uint8_t		*SrcPtr = pBuffer.plane[ i ];
		uint8_t				*DstPtr = mValOutputImage->internalBuffer( i );

		if( SrcPtr && DstPtr )
		{
			memcpy( DstPtr, SrcPtr, mValOutputImage->bufferSize( i ) );
		}
	}

	pinUpdated( mPinOutputImage );
}

void VideoCaptureNode::frameStart()
{
	mCapture.update();
}

void VideoCaptureNode::currentIndexChanged( int pIndex )
{
	QComboBox		*GUI = qobject_cast<QComboBox *>( sender() );

	const int		 DevIdx = GUI->itemData( pIndex ).toInt();

	setCurrentDevice( DevIdx );
}

void VideoCaptureNode::setCurrentDevice( int pDevIdx )
{
	if( mDeviceIndex == pDevIdx )
	{
		return;
	}

	mCapture.stop();
	mCapture.close();

	mDeviceIndex = pDevIdx;

	const std::vector<ca::Capability>		CapLst = mCapture.getCapabilities( mDeviceIndex );

	if( CapLst.empty() )
	{
		return;
	}

	ca::Settings		DevCfg;

	DevCfg.device =  mDeviceIndex;
	DevCfg.format = -1;

	int					DevErr = -1;

	for( size_t i = 0 ; DevErr < 0 && i < CapLst.size() ; i++ )
	{
		const ca::Capability	&DevCap = CapLst[ i ];

		switch( DevCap.pixel_format )
		{
			case CA_JPEG_OPENDML:                                                          /* JPEG with Open-DML extensions */
			case CA_H264:                                                                  /* H264 */
			case CA_MJPEG:                                                                /* MJPEG 2*/
				break;

			case CA_UYVY422:                                                              /* Cb Y0 Cr Y1 */
			case CA_YUYV422:                                                           /* Y0 Cb Y1 Cr */
			case CA_YUV422P:                                                             /* YUV422 Planar */
			case CA_YUV420P:                                                           /* YUV420 Planar */
			case CA_YUV420BP:                                                            /* YUV420 Bi Planar */
			case CA_YUVJ420P:                                                          /* YUV420 Planar Full Range (JPEG), J comes from the JPEG. (values 0-255 used) */
			case CA_YUVJ420BP:                                                          /* YUV420 Bi-Planer Full Range (JPEG), J comes fro the JPEG. (values: luma = [16,235], chroma=[16,240]) */
			case CA_ARGB32:                                                              /* ARGB 8:8:8:8 32bpp, ARGBARGBARGB... */
			case CA_BGRA32:                                                             /* BGRA 8:8:8:8 32bpp, BGRABGRABGRA... */
			case CA_RGBA32:                                                              /* RGBA 8:8:8:8 32bpp. */
			case CA_RGB24:                                                              /* RGB 8:8:8 24bit */
				{
					DevCfg.capability = DevCap.capability_index;
					DevCfg.format     = DevCap.pixel_format;

					DevErr = mCapture.open( DevCfg );
				}
				break;
		}
	}

	if( DevErr < 0 )
	{
		return;
	}

	DevErr = mCapture.start();

	if( DevErr >= 0 )
	{
		connect( mNode->context()->qobject(), SIGNAL(frameStart()), this, SLOT(frameStart()) );
	}

	emit deviceIndexUpdated( mDeviceIndex );
}

QWidget *VideoCaptureNode::gui()
{
	QComboBox					*GUI = new QComboBox();

	std::vector<ca::Device>		 DevLst = mCapture.getDevices();

	for( const ca::Device &DevEnt : DevLst )
	{
		GUI->addItem( QString::fromStdString( DevEnt.name ), QVariant( DevEnt.index ) );
	}

	GUI->setCurrentIndex( GUI->findData( mDeviceIndex ) );

	connect( GUI, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged(int)) );
	connect( this, SIGNAL(deviceIndexUpdated(int)), GUI, SLOT(setCurrentIndex(int)) );

	return( GUI );
}

void VideoCaptureNode::loadSettings( QSettings &pSettings )
{
	int	DevIdx = pSettings.value( "index", mDeviceIndex ).toInt();

	setCurrentDevice( DevIdx );
}

void VideoCaptureNode::saveSettings( QSettings &pSettings ) const
{
	pSettings.setValue( "index", mDeviceIndex );
}
