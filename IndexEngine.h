#ifndef INDEXENGINE_H
#define INDEXENGINE_H

/*
* Copyright 2016-2017 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the CrossLine full-text search Fts library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <QObject>
#include <Udb/Obj.h>
#include <Udb/UpdateInfo.h>
#include <QSet>

namespace Fts
{
	class Tokenizer;
	class Stemmer;
	class Stopper;

	class IndexEngine : public QObject
	{
		Q_OBJECT
	public:
		struct ItemHit
		{
			Udb::OID d_item;
			quint32 d_rank;
		};
		typedef QList<ItemHit> ItemHits;
		struct DocHit
		{
			Udb::OID d_doc;
			quint32 d_rank;
			ItemHits d_items; // unique OID, ordered by OID
		};
		typedef QList<DocHit> DocHits; // unique OID, ordered by OID
		static ItemHits unite( const ItemHits& lhs, const ItemHits& rhs );
		static ItemHits intersect( const ItemHits& lhs, const ItemHits& rhs );
		static DocHits intersect( const DocHits& lhs, const DocHits& rhs, bool uniteItems );
		static DocHits unite( const DocHits& lhs, const DocHits& rhs, bool uniteItems );

		static Udb::Obj (*s_getDocument)( const Udb::Obj& );
		explicit IndexEngine( const Udb::Obj& index, Udb::Transaction* = 0, QObject *parent = 0);
		~IndexEngine();
		void addAttrToWatch( Udb::Atom );
		void addTypeToWatch( Udb::Atom );
		void setTokenizer( Tokenizer* t );
		void setStemmer( Stemmer* t );
		void setStopper( Stopper* s );
		void indexObject( const Udb::Obj&, bool removeOldValues = true );
		DocHits findWithJoker( const QString&, bool itemAnd, bool partial ) const; // '*' ist Joker
		DocHits find( const QString&, bool partial, bool reverse = false ) const;
		DocHits find( const QStringList&, bool docAnd, bool itemAnd, bool joker, bool partial ) const;
		Udb::Transaction* getTxn() const { return d_txn; }
		void commit(bool force = false);
		void clearIndex();
		bool isEmpty() const;
		void test() const;
		bool useReverseIndex() const { return d_useReverseIndex; }
		void useReverseIndex(bool on) { d_useReverseIndex = on; }
		bool resolveDocuments() const { return d_resolveDocuments; }
		void resolveDocuments(bool on) { d_resolveDocuments = on; }
		bool checkEmpty() const { return d_checkEmpty; }
		void checkEmpty(bool on) { d_checkEmpty = on; }
		static IndexEngine* getIndex( Udb::Transaction* ); // funktioniert sowohl für Db als auch Index Txn
	private slots:
		void onDbUpdate( const Udb::UpdateInfo& info );
	protected:
		enum Attrs
		{
			AttrMaxTerm = 20, // UInt32
			AttrDict = 21,    // Id32
			AttrPosts = 22    // Id32
		};

		void index( const QString&, const Udb::Obj&, bool remove = false );
		quint32 termId( const QString&, bool create = true );
		// to override
		virtual void process( const Stream::DataCell&, const Udb::Obj&, bool remove );
		virtual Udb::Obj getDocument( const Udb::Obj& );
	private:
		Udb::Obj d_index; // in Index-Db
		Udb::Global* d_dict; // in Index-Db
		Udb::Global* d_post; // in Index-Db
		Udb::Transaction* d_txn; // in Daten-Db
		QSet<Udb::Atom> d_typesToWatch, d_attrsToWatch; // wir brauchen Listen wegen erase
		Tokenizer* d_tok;
		Stemmer* d_ste;
		Stopper* d_sto;
		bool d_useReverseIndex;
		bool d_resolveDocuments;
		bool d_checkEmpty;
	};
}

#endif // INDEXENGINE_H
