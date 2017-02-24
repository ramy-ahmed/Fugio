#include "videocaptureplugin.h"

#include <fugio/global_interface.h>
#include <fugio/global_signals.h>

#include <fugio/videocapture/uuid.h>

#include <fugio/nodecontrolbase.h>

#include "videocapturenode.h"

QList<QUuid>	NodeControlBase::PID_UUID;

ClassEntry	NodeClasses[] =
{
	ClassEntry( "Video Capture", NID_VIDEOCAPTURE, &VideoCaptureNode::staticMetaObject ),
	ClassEntry()
};

ClassEntry PinClasses[] =
{
	ClassEntry()
};

PluginInterface::InitResult VideoCapturePlugin::initialise( fugio::GlobalInterface *pApp, bool pLastChance )
{
	Q_UNUSED( pLastChance )

	mApp = pApp;

	mApp->registerNodeClasses( NodeClasses );

	mApp->registerPinClasses( PinClasses );

	return( INIT_OK );
}

void VideoCapturePlugin::deinitialise( void )
{
	mApp->unregisterPinClasses( PinClasses );

	mApp->unregisterNodeClasses( NodeClasses );

	mApp = 0;
}
