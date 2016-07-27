#ifndef CMDREMOVE_H
#define CMDREMOVE_H

#include <QUndoCommand>
#include <QUuid>
#include <QMultiHash>

#include <fugio/global_interface.h>
#include <fugio/context_interface.h>
#include <fugio/node_interface.h>

#include "noteitem.h"
#include "contextview.h"

class CmdRemove : public QUndoCommand
{
public:
	explicit CmdRemove( QSharedPointer<fugio::ContextInterface> pContext, QList< QSharedPointer<fugio::NodeInterface> > &pNodeList, QList<QSharedPointer<NodeItem>> &pGroupList, QMultiMap<QUuid,QUuid> &pLinkList, QList<QSharedPointer<NoteItem>> &pNoteList )
		: mContext( pContext ), mNodeList( pNodeList ), mGroupList( pGroupList ), mLinkList( pLinkList ), mNoteList( pNoteList )
	{
		setText( QObject::tr( "Remove Nodes/Links" ) );

		for( QSharedPointer<fugio::NodeInterface> mNode : mNodeList )
		{
			for( auto PinPtr : mNode->enumInputPins() )
			{
				for( auto SrcPin : PinPtr->connectedPins() )
				{
					if( !mLinkList.contains( SrcPin->globalId(), PinPtr->globalId() ) )
					{
						mLinkList.insert( SrcPin->globalId(), PinPtr->globalId() );
					}
				}
			}

			for( auto PinPtr : mNode->enumOutputPins() )
			{
				for( auto DstPin : PinPtr->connectedPins() )
				{
					if( !mLinkList.contains( PinPtr->globalId(), DstPin->globalId() ) )
					{
						mLinkList.insert( PinPtr->globalId(), DstPin->globalId() );
					}
				}
			}
		}
	}

	virtual ~CmdRemove( void )
	{

	}

	virtual void undo( void )
	{
		for( QSharedPointer<fugio::NodeInterface> mNode : mNodeList )
		{
			mContext->registerNode( mNode, mNode->uuid() );
		}

		for( QUuid Src : mLinkList.keys() )
		{
			for( QUuid Dst : mLinkList.values( Src ) )
			{
				mContext->connectPins( Src, Dst );
			}
		}

		for( QSharedPointer<NoteItem> P : mNoteList )
		{
			if( P )
			{
				P->view()->noteAdd( P );
			}
		}
	}

	virtual void redo( void )
	{
		for( QUuid Src : mLinkList.keys() )
		{
			for( QUuid Dst : mLinkList.values( Src ) )
			{
				mContext->disconnectPins( Src, Dst );
			}
		}

		for( QSharedPointer<fugio::NodeInterface> mNode : mNodeList )
		{
			mContext->unregisterNode( mNode->uuid() );
		}

		for( QSharedPointer<NoteItem> P : mNoteList )
		{
			if( P )
			{
				P->view()->noteRemove( P );
			}
		}
	}

private:
	QSharedPointer<fugio::ContextInterface>			 mContext;
	QList< QSharedPointer<fugio::NodeInterface> >	 mNodeList;
	QList<QSharedPointer<NodeItem>>					 mGroupList;
	QMultiMap<QUuid,QUuid>							 mLinkList;
	QList<QSharedPointer<NoteItem>>					 mNoteList;
};

#endif // CMDREMOVE_H

