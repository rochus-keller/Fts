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
#include "Tokenizer.h"
#include <QStringList>
using namespace Fts;

Tokenizer::Tokenizer(QObject *parent) :
	QObject(parent)
{
}

LetterOrNumberTok::LetterOrNumberTok(QObject *p):Tokenizer(p),d_pos(0)
{
}

QStringList LetterOrNumberTok::parse(const QString & str)
{
	setString( str );
	QStringList res;
	QString tok = nextToken();
	while( !tok.isEmpty() )
	{
		res.append(tok);
		tok = nextToken();
	}
	return res;
}

void LetterOrNumberTok::setString(const QString & str)
{
	d_pos = 0;
	d_str = str;
}

QString LetterOrNumberTok::nextToken()
{
	// TODO: mit "&" oder "-" verbundene Wörter in ihren Bestandteilen und zusammen zurückgeben

	enum Status { Idle, InString } status = Idle;
	int start = 0;
	while( d_pos < d_str.size() )
	{
		const QChar ch = d_str[d_pos];
		switch( status )
		{
		case Idle:
			if( ch.isLetterOrNumber() )
			{
				start = d_pos;
				status = InString;
			}
			d_pos++;
			break;
		case InString:
			if( !ch.isLetterOrNumber() )
			{
				const QString str = d_str.mid( start, d_pos - start ).toLower();
				d_pos++;
				return str;
			}else
				d_pos++;
			break;
		}
	}
	if( status == InString )
		return d_str.mid( start).toLower();
	else
		return QString();
}
