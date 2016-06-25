#include "opensoundcontrolplugin.h"

#include "decodernode.h"
#include "encodernode.h"

#include "bundlernode.h"

#include "splitnode.h"
#include "joinnode.h"

#include "splitpin.h"
#include "joinpin.h"

#include "namespacepin.h"

#include <fugio/global_signals.h>

QList<QUuid>				NodeControlBase::PID_UUID;

ClassEntry		OpenSoundControlPlugin::mNodeClasses[] =
{
	ClassEntry( "OSC Decoder", "OSC", NID_OSC_DECODER, &DecoderNode::staticMetaObject ),
	ClassEntry( "OSC Encoder", "OSC", NID_OSC_ENCODER, &EncoderNode::staticMetaObject ),
	ClassEntry( "OSC Bundler", "OSC", NID_OSC_BUNDLER, &BundlerNode::staticMetaObject ),
	ClassEntry( "OSC Split", "OSC", NID_OSC_SPLIT, &SplitNode::staticMetaObject ),
	ClassEntry( "OSC Join", "OSC", NID_OSC_JOIN, &JoinNode::staticMetaObject ),
	ClassEntry()
};

ClassEntry		OpenSoundControlPlugin::mPinClasses[] =
{
	ClassEntry( "OSC Split", PID_OSC_SPLIT, &SplitPin::staticMetaObject ),
	ClassEntry( "OSC Join", PID_OSC_JOIN, &JoinPin::staticMetaObject ),
	ClassEntry( "OSC Namespace", PID_OSC_NAMESPACE, &NamespacePin::staticMetaObject ),
	ClassEntry()
};

OpenSoundControlPlugin	*OpenSoundControlPlugin::mInstance = nullptr;

OpenSoundControlPlugin::OpenSoundControlPlugin( void )
	: mApp( 0 )
{
	mInstance = this;
}

OpenSoundControlPlugin::~OpenSoundControlPlugin()
{
	mInstance = 0;
}

OpenSoundControlPlugin *OpenSoundControlPlugin::instance()
{
	return( mInstance );
}

PluginInterface::InitResult OpenSoundControlPlugin::initialise( fugio::GlobalInterface *pApp )
{
	mApp = pApp;

	mApp->registerNodeClasses( mNodeClasses );

	mApp->registerPinClasses( mPinClasses );

	return( INIT_OK );
}

void OpenSoundControlPlugin::deinitialise()
{
	mApp->unregisterPinClasses( mPinClasses );

	mApp->unregisterNodeClasses( mNodeClasses );

	mApp = 0;
}