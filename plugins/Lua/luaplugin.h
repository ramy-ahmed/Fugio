#ifndef LUAPLUGIN_H
#define LUAPLUGIN_H

#include <QMultiMap>
#include <QVector>

#if defined( LUA_PLUGIN_SUPPORTED )
#include <lua.hpp>
#endif

#include <fugio/plugin_interface.h>
#include <fugio/lua/lua_interface.h>

class LuaPlugin : public QObject, public fugio::PluginInterface, public fugio::LuaInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA( IID "com.bigfug.fugio.lua.plugin" )
	Q_INTERFACES( fugio::PluginInterface fugio::LuaInterface )

public:
	Q_INVOKABLE explicit LuaPlugin( void ) : mApp( 0 ) { mInstance = this; }

	virtual ~LuaPlugin( void ) {}

	static LuaPlugin *instance( void )
	{
		return( mInstance );
	}

	void registerNodeToState( fugio::NodeInterface *N, lua_State *L ) const;

	fugio::GlobalInterface *app( void )
	{
		return( mApp );
	}

	//-------------------------------------------------------------------------
	// fugio::PluginInterface

	virtual InitResult initialise( fugio::GlobalInterface *pApp ) Q_DECL_OVERRIDE;

	virtual void deinitialise( void ) Q_DECL_OVERRIDE;

	//-------------------------------------------------------------------------
	// InterfaceLua

	virtual void luaRegisterExtension( lua_CFunction pFunction ) Q_DECL_OVERRIDE;

	virtual void luaUnregisterExtension( lua_CFunction pFunction ) Q_DECL_OVERRIDE;

	virtual void luaRegisterLibrary( const char *pName, int (*pFunction) (lua_State *L) ) Q_DECL_OVERRIDE;

	virtual void luaAddExtensions( lua_State *L ) Q_DECL_OVERRIDE;

	virtual void luaAddFunction( const char *pName, int (*pFunction) (lua_State *L) ) Q_DECL_OVERRIDE;

	virtual void luaAddPinFunction( const QUuid &pPID, const char *pName, int (*pFunction) (lua_State *L) ) Q_DECL_OVERRIDE;

	virtual fugio::NodeInterface *node( lua_State *L ) Q_DECL_OVERRIDE;

	virtual QUuid checkpin( lua_State *L, int i ) Q_DECL_OVERRIDE;

	virtual void pushpin( lua_State *L, const QUuid &pUuid ) Q_DECL_OVERRIDE;

	//-------------------------------------------------------------------------

	const QVector<luaL_Reg> &functions( void )
	{
		return( mFunctions );
	}

	const QList<luaL_Reg> pinFunctions( const QUuid &pPID )
	{
		return( mPinFunctions.values( pPID ) );
	}

	const QMap<const char *,lua_CFunction> &libFunctions( void )
	{
		return( mLibFunctions );
	}

public:
	static int pushVariant( lua_State *L, const QVariant &V );

	static QVariant popVariant( lua_State *L, int idx );

private:
	static int luaLog( lua_State *L );

	static int luaTimestamp( lua_State *L );

private:
	static LuaPlugin						*mInstance;

	fugio::GlobalInterface					*mApp;

	QList<lua_CFunction>					 mExtensions;

	QVector<luaL_Reg>						 mFunctions;
	QMultiMap<QUuid,luaL_Reg>				 mPinFunctions;
	QMap<const char *,lua_CFunction>		 mLibFunctions;
};

#endif // LUAPLUGIN_H