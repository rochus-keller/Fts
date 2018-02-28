/*
* Copyright 2016-2017 Rochus Keller <mailto:me@rochus-keller.info>
*
* This file is part of the CrossLine full-text search Fts library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.info.
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

#include "IndexEngine.h"
#include "Tokenizer.h"
#include "Stemmer.h"
#include "Stopper.h"
#include <Udb/Transaction.h>
#include <Udb/Idx.h>
#include <Udb/Global.h>
#include <Stream/Helper.h>
#include <QLocale> // wegen Test
#include <QtDebug>
#include <QBuffer>
#include <limits>
using namespace Fts;

static const char s_rev = 0x07; // BEL
static QHash<Udb::Transaction*,IndexEngine*> s_cache;

static QString _reverse( const QString& in)
{
	QString out;
	out.reserve(in.size());
	for( int i = in.size() - 1; i >= 0; i-- )
		out.push_back( in[i] );
	return out;
}

static QByteArray writeKey2( quint32 nr, Udb::OID oid )
{
	QBuffer buf;
	buf.open( QIODevice::WriteOnly );
	Stream::Helper::writeMultibyte32( &buf, nr );
	Stream::Helper::writeMultibyte64( &buf, oid );
	buf.close();
	return buf.buffer();
}

static QByteArray writeKey3( quint32 nr, Udb::OID doc, Udb::OID item )
{
	QBuffer buf;
	buf.open( QIODevice::WriteOnly );
	Stream::Helper::writeMultibyte32( &buf, nr );
	Stream::Helper::writeMultibyte64( &buf, doc );
	Stream::Helper::writeMultibyte64( &buf, item );
	buf.close();
	return buf.buffer();
}

static int readKey3( const QByteArray& in, quint32& nr, Udb::OID& doc, Udb::OID& item )
{
	QBuffer buf;
	buf.buffer() = in;
	buf.open( QIODevice::ReadOnly );
	int n = 0;
	if( Stream::Helper::readMultibyte32( &buf, nr ) > 0 )
		n++;
	if( Stream::Helper::readMultibyte64( &buf, doc ) > 0 )
		n++;
	if( Stream::Helper::readMultibyte64( &buf, item ) > 0 )
		n++;
	return n;
}

static QByteArray writeFreq( quint32 f )
{
	QBuffer buf;
	buf.open( QIODevice::WriteOnly );
	Stream::Helper::writeMultibyte32( &buf, f );
	buf.close();
	return buf.buffer();
}

static quint32 readFreq( const QByteArray& in )
{
	quint32 f = 0;
	Stream::Helper::readMultibyte32( in, f, in.size() );
	return f;
}

IndexEngine::IndexEngine(const Udb::Obj& index, Udb::Transaction* txn, QObject *parent) :
	QObject(parent), d_index(index), d_txn(txn), d_tok(0), d_ste(0), d_sto(0),
	d_useReverseIndex(false),d_resolveDocuments(false),d_checkEmpty(false)
{
	Q_ASSERT( !index.isNull() );
	// Damit index in anderer Db sein kann als die Daten, hier txn optional separat
	if( d_txn == 0 )
		d_txn = d_index.getTxn();
	else
		d_checkEmpty = true;
	s_cache.insert( txn, this );
	s_cache.insert( index.getTxn(), this );
	quint32 dict = d_index.getValue(AttrDict).getId32();
	quint32 post = d_index.getValue(AttrPosts).getId32();
	d_dict = new Udb::Global( d_index.getDb(), this );
	d_post = new Udb::Global( d_index.getDb(), this );
	if( dict == 0 || post == 0 )
	{
		if( !d_index.getTxn()->isReadOnly() )
		{
			dict = d_dict->create();
			post = d_post->create();
			d_index.setValue(AttrDict, Stream::DataCell().setId32(dict) );
			d_index.setValue(AttrPosts, Stream::DataCell().setId32(post) );
			d_index.commit();
		}else
			return;
	}else
	{
		d_dict->open(dict);
		d_post->open(post);
	}

	d_txn->addObserver( this, SLOT(onDbUpdate( Udb::UpdateInfo ) ), false );
}

IndexEngine::~IndexEngine()
{
	s_cache.remove( d_txn );
	s_cache.remove( d_index.getTxn() );
}

void IndexEngine::addAttrToWatch(Udb::Atom a)
{
	d_attrsToWatch.insert( a );
}

void IndexEngine::addTypeToWatch(Udb::Atom a)
{
	d_typesToWatch.insert( a );
}

void IndexEngine::setTokenizer(Tokenizer *t)
{
	if( d_tok && d_tok->parent() == this )
		delete d_tok;
	d_tok = t;
}

void IndexEngine::setStemmer(Stemmer *t)
{
	if( d_ste && d_ste->parent() == this )
		delete d_ste;
	d_ste = t;
}

void IndexEngine::setStopper(Stopper *s)
{
	if( d_sto && d_sto->parent() == this )
		delete d_sto;
	d_sto = s;
}

void IndexEngine::indexObject(const Udb::Obj & o, bool removeOldValues)
{
	if( o.isNull() || o.equals( d_index ) )
		return;
	if( d_typesToWatch.isEmpty() || d_typesToWatch.contains( o.getType() ) )
	{
		QSet<Udb::Atom>::const_iterator i;
		for( i = d_attrsToWatch.begin(); i != d_attrsToWatch.end(); ++i )
		{
			if( removeOldValues )
				process( o.getValue( (*i), true ), o, true );
			process( o.getValue( (*i), false ), o, false );
		}
	}
}

IndexEngine::DocHits IndexEngine::findWithJoker(const QString & str, bool itemAnd, bool partial) const
{
	QStringList terms = str.toLower().split( QChar('*') ); // keep empty parts
	if( terms.size() > 2 )
	{
		qWarning() << "IndexEngine::findWithJoker: only one joker supported:" << str;
		return DocHits();
	}else if( terms.size() == 1 )
		return find( terms.first(), partial );
	Q_ASSERT( !terms.isEmpty() );
	if( !d_useReverseIndex && !terms.last().isEmpty() )
	{
		qWarning() << "IndexEngine::findWildcard: not using reverse index; only trailing wildcard supported:" << str;
		return DocHits();
	}
	if( terms.first().isEmpty() && terms.last().isEmpty() )
		return DocHits();
	if( terms.last().isEmpty() )
		return find( terms.first(), !partial ); // invertiere die Wirkung
	else if( terms.first().isEmpty() )
		return find( terms.last(), !partial, true );
	else
		return intersect( find( terms.first(), true ), find( terms.last(), true, true ), !itemAnd );

}

IndexEngine::DocHits IndexEngine::find(const QString & s, bool partial, bool reverse) const
{
	Q_ASSERT( !reverse || partial ); // reverse ist immer auch partial!

	if( !d_dict->isOpen() )
		return DocHits();

	const QString str = s.toLower();
	QString term = str;
	if( term.endsWith(QLatin1String("*")) ) // "trailing wildcard query" kann auch hier direkt verarbeitet werden
	{
		partial = true;
		term.chop(1);
	}else if( term.endsWith(QLatin1String("!")) )
	{
		partial = false;
		term.chop(1);
	}
	if( !partial && d_ste != 0 )
		term = d_ste->stem( term );
	QList<quint32> nrs;
	if( partial || reverse )
	{
		if( reverse )
			term = _reverse(term);
		QByteArray key;
		Udb::Idx::collate( key, 0, term ); // Udb::IndexMeta::NFKD_CanonicalBase, s.toLower() );
		if( reverse )
			key.prepend(s_rev);
		Udb::Git git = d_dict->findCells( key );
		if( !git.isNull() ) do
		{
			nrs.append( readFreq( git.getValue() ) );
		}while( git.nextKey() );
	}else
	{
		const quint32 nr = const_cast<IndexEngine*>(this)->termId( str, false );
		if( nr != 0 )
			nrs.append( nr );
	}
	QMap<Udb::OID,DocHit> hits;
	foreach( quint32 nr, nrs )
	{
		Udb::Git m = d_post->findCells( writeFreq( nr ) );
		DocHit* hit1 = 0;
		ItemHits hit2;
		if( !m.isNull() ) do
		{
			// Hier kommen die ItemHits bereits nach OID sortiert.
			// Zuerst kommt immer der DocHit, gefolgt von allen ItemHits des Doc
			// Es kann sein, dass mehrere nr auf dasselbe Doc zeigen; darum unite der Teilergebnisse
			Udb::OID doc = 0, item = 0;
			const int n = readKey3( m.getKey(), nr, doc, item );
			if( n == 2 )
			{
				if( hit1 )
				{
					hit1->d_items = unite( hit1->d_items, hit2 );
					hit2.clear();
				}
				hit1 = &hits[ doc ];
				hit1->d_doc = doc;
				hit1->d_rank += readFreq( m.getValue() );
			}else if( n == 3 )
			{
				Q_ASSERT( hit1 != 0 && hit1->d_doc == doc );
				ItemHit h;
				h.d_item = item;
				h.d_rank = readFreq( m.getValue() );
				hit2.append( h );
			}
		}while( m.nextKey() );
		if( hit1 )
			hit1->d_items = unite( hit1->d_items, hit2 );
	}
	DocHits res;
	QMap<Udb::OID,DocHit>::const_iterator i;
	for( i = hits.begin(); i != hits.end(); ++i )
		res.append(i.value());
	return res;
}

IndexEngine::DocHits IndexEngine::find(const QStringList & l, bool docAnd, bool itemAnd, bool joker, bool partial) const
{
	DocHits res;
	if( docAnd )
	{
		// AND
		for( int i = 0; i < l.size(); i++ )
		{
			if( i == 0 )
				res = (joker)?findWithJoker(l[i],itemAnd,partial):find( l[i], partial );
			else
				res = intersect( res, (joker)?findWithJoker(l[i],itemAnd,partial):find( l[i], partial ), !itemAnd );
		}
	}else
	{
		// OR
		foreach( const QString& s, l )
			res = unite( res, (joker)?findWithJoker(s,itemAnd,partial):find( s, partial ), !itemAnd );
	}
	return res;
}

void IndexEngine::commit(bool force)
{
	if( !d_dict->isOpen() )
		return;
	d_dict->commit();
	d_post->commit();
	if( force || d_index.getTxn() != d_txn )
	{
		d_index.commit();
	}
}

void IndexEngine::clearIndex()
{
	if( !d_dict->isOpen() )
		return;
	d_dict->clearAllCells();
	d_post->clearAllCells();
	d_index.clearValue(AttrMaxTerm);
	d_index.commit();
}

bool IndexEngine::isEmpty() const
{
	return d_index.getValue(AttrMaxTerm).getUInt32() == 0;
}

static QString _kb( quint32 b )
{
	return QLocale::c().toString( b / 1024.0, 'f', 1 );
}

void IndexEngine::test() const
{
	quint32 mkSize = 0, mvSize = 0, mCount = 0;
	Udb::Git mCur = d_post->findCells( QByteArray() );
	if( !mCur.isNull() ) do
	{
		const QByteArray k = mCur.getKey();
		mkSize += k.size();
		mvSize += mCur.getValue().size();
		mCount++;
	}while( mCur.nextKey() );
	qDebug() << "MapTable:" << mCount << _kb(mkSize) << _kb(mvSize);
	quint32 xkSize = 0, xvSize = 0, xCount = 0;
	Udb::Git xCur = d_dict->findCells( QByteArray() );
	if( !xCur.isNull() ) do
	{
		const QByteArray k = xCur.getKey();
		qDebug() << k;
		xkSize += k.size();
		xvSize += xCur.getValue().size();
		xCount++;
	}while( xCur.nextKey() );
	//qDebug() << "OixTable:" << xCount << _kb(xkSize) << _kb(xvSize);
	//qDebug() << "Total (KB):" << _kb(mkSize+xkSize) << _kb(mvSize+xvSize) << _kb(mkSize+xkSize+mvSize+xvSize);
}

IndexEngine *IndexEngine::getIndex(Udb::Transaction * txn)
{
	return s_cache.value( txn );
}

void IndexEngine::onDbUpdate(const Udb::UpdateInfo & info)
{
	if( info.d_kind != Udb::UpdateInfo::PreCommit )
		return;
	if( d_checkEmpty && isEmpty() )
		return;
	Udb::Transaction::Changes::const_iterator i;
	Udb::Obj o;
	bool erased = false;
	bool toProcess = false;
	bool changes = false;
	for( i = d_txn->getChanges().begin(); i != d_txn->getChanges().end(); ++i )
	{
		if( i.key().first != o.getOid() )
		{
			erased = ( i.key().second == 0 && i.value().isNull() );
			o = d_txn->getObject( i.key().first );
			toProcess = d_typesToWatch.isEmpty() || d_typesToWatch.contains( o.getType() );
			if( o.equals( d_index ) )
				toProcess = false;
		}
		if( toProcess )
		{
			if( erased )
			{
				QSet<Udb::Atom>::const_iterator j;
				for( j = d_attrsToWatch.begin(); j != d_attrsToWatch.end(); ++j )
				{
					process( o.getValue( (*j), true ), o, true );
					changes = true;
				}
			}else if( d_attrsToWatch.contains( i.key().second ) )
			{
				process( o.getValue( i.key().second, true ), o, true );
				process( o.getValue( i.key().second, false ), o, false );
				changes = true;
			}
		}
	}
	if( changes )
		commit();
}

void IndexEngine::index(const QString & s, const Udb::Obj & o, bool remove)
{
	const quint32 tid = termId(s,true);

	Udb::Obj doc;
	if( d_resolveDocuments )
		doc = getDocument(o);
	if( doc.isNull() )
		doc = o;

	const QByteArray key2 = writeKey2( tid, doc.getOid() );
	qint32 freq = qint32(readFreq(d_post->getCell( key2 )) );
	if( remove )
		freq--;
	else if( freq < std::numeric_limits<qint32>::max() )
		freq++;
	else
		qWarning() << "IndexEngine::index: frequency out of qint32 range";
	if( freq > 0 )
		d_post->setCell( key2, writeFreq(freq) ); // term, oid -> freq
	else
		d_post->setCell( key2, QByteArray() ); // loeschen

	if( d_resolveDocuments && !doc.equals(o) )
	{
		const QByteArray key3 = writeKey3( tid, doc.getOid(), o.getOid() );
		qint32 freq = qint32(readFreq(d_post->getCell( key3 )));
		if( remove )
			freq--;
		else if( freq < std::numeric_limits<qint32>::max() )
			freq++;
		else
			qWarning() << "IndexEngine::index: frequency out of qint32 range";
		if( freq > 0 )
			d_post->setCell( key3, writeFreq(freq) ); // term, oid -> freq
		else
			d_post->setCell( key3, QByteArray() ); // loeschen
	}
}

quint32 IndexEngine::termId(const QString & term, bool create)
{
	// Terms sind im Array-indizierten Teil von d_index gespeichert und haben als Wert die ID
	QByteArray key;
	if( d_ste )
		Udb::Idx::collate( key, 0, d_ste->stem( term ) );
	else
		Udb::Idx::collate( key, 0, term ); // Udb::IndexMeta::NFKD_CanonicalBase, s.toLower() );
	QByteArray nrv = d_dict->getCell( key );
	quint32 nr = 0;
	if( nrv.isEmpty() && create )
	{
		// Term ist noch nicht enthalten; loese neue Nummer und fuege ihn ein
		nr = d_index.incCounter( AttrMaxTerm );
		nrv = writeFreq( nr );
		d_dict->setCell( key, nrv );
	}else
		nr = readFreq(nrv);
	if( d_useReverseIndex && create )
	{
		// das muss hier kommen, da ansonsten wegen stemming nicht alle Terms im Index landen
		Udb::Idx::collate( key, 0, _reverse(term) ); // hier wird absichtlich die Originalversion verwendet, nicht stemmed.
		key.prepend(s_rev);
		d_dict->setCell( key, nrv );
	}
	return nr;
}

void IndexEngine::process(const Stream::DataCell & v, const Udb::Obj & o, bool remove)
{
	if( d_tok == 0 )
	{
		qWarning() << "IndexEngine::process: no Tokenizer set";
		return;
	}
	d_tok->setString( v.toString(true) );
	QString tok = d_tok->nextToken();
	while( !tok.isEmpty() )
	{
		if( d_sto == 0 || !d_sto->isStopword( tok ) )
		{
			index( tok, o, remove );
		}
		tok = d_tok->nextToken();
	}
}

static Udb::Obj _getDocument(const Udb::Obj & o)
{
//	if( o.getType() == Oln::OutlineItem::TID )
//		return o.getValueAsObj( Oln::OutlineItem::AttrHome );
//	else
		return Udb::Obj();
}

Udb::Obj (*IndexEngine::s_getDocument)(const Udb::Obj & ) = _getDocument;

Udb::Obj IndexEngine::getDocument(const Udb::Obj & o)
{
	return s_getDocument(o);
}

// Quelle: http://stackoverflow.com/questions/2400157/the-intersection-of-two-sorted-arrays
IndexEngine::DocHits IndexEngine::intersect(const IndexEngine::DocHits &lhs, const IndexEngine::DocHits &rhs, bool uniteItems)
{
	DocHits res;
	int i = 0, j = 0;
	while( i < lhs.size() && j < rhs.size() )
	{
		if( lhs[i].d_doc == rhs[j].d_doc )
		{
			DocHit hit;
			hit.d_doc = lhs[i].d_doc;
			hit.d_rank = lhs[i].d_rank + rhs[j].d_rank; // RISK
			if( uniteItems )
				hit.d_items = unite( lhs[i].d_items, rhs[j].d_items );
			else
				hit.d_items = intersect( lhs[i].d_items, rhs[j].d_items );
			res.append(hit);
			i++;
			j++;
		}else if( lhs[i].d_doc > rhs[j].d_doc )
			j++;
		else
			i++;
	}
	return res;
}

IndexEngine::DocHits IndexEngine::unite(const IndexEngine::DocHits &lhs, const IndexEngine::DocHits &rhs, bool uniteItems)
{
	DocHits res;
	int i = 0, j = 0;
	while( i < lhs.size() && j < rhs.size() )
	{
		if( lhs[i].d_doc < rhs[j].d_doc )
			res.append( lhs[i++] );
		else if( rhs[j].d_doc < lhs[i].d_doc )
			res.append( rhs[j++] );
		else // lhs==rhs
		{
			DocHit hit;
			hit.d_doc = lhs[i].d_doc;
			hit.d_rank = lhs[i].d_rank + rhs[j].d_rank; // RISK
			if( uniteItems )
				hit.d_items = unite( lhs[i].d_items, rhs[j].d_items );
			else
				hit.d_items = intersect( lhs[i].d_items, rhs[j].d_items );
			res.append(hit);
			i++;
			j++;
		}
	}
	while( i < lhs.size() )
		res.append( lhs[i++] );
	while( j < rhs.size() )
		res.append( rhs[j++] );
	return res;
}

IndexEngine::ItemHits IndexEngine::intersect(const IndexEngine::ItemHits &lhs, const IndexEngine::ItemHits &rhs)
{
	ItemHits res;
	int i = 0, j = 0;
	while( i < lhs.size() && j < rhs.size() )
	{
		if( lhs[i].d_item == rhs[j].d_item )
		{
			ItemHit hit;
			hit.d_item = lhs[i].d_item;
			hit.d_rank = lhs[i].d_rank + rhs[j].d_rank; // RISK
			res.append(hit);
			i++;
			j++;
		}else if( lhs[i].d_item > rhs[j].d_item )
			j++;
		else
			i++;
	}
	return res;
}

// Quelle: http://www.geeksforgeeks.org/union-and-intersection-of-two-sorted-arrays-2/
IndexEngine::ItemHits IndexEngine::unite(const IndexEngine::ItemHits &lhs, const IndexEngine::ItemHits &rhs)
{
	ItemHits res;
	int i = 0, j = 0;
	while( i < lhs.size() && j < rhs.size() )
	{
		if( lhs[i].d_item < rhs[j].d_item )
			res.append( lhs[i++] );
		else if( rhs[j].d_item < lhs[i].d_item )
			res.append( rhs[j++] );
		else // lhs==rhs
		{
			ItemHit hit;
			hit.d_item = lhs[i].d_item;
			hit.d_rank = lhs[i].d_rank + rhs[j].d_rank; // RISK
			res.append(hit);
			i++;
			j++;
		}
	}
	while( i < lhs.size() )
		res.append( lhs[i++] );
	while( j < rhs.size() )
		res.append( rhs[j++] );
	return res;
}



