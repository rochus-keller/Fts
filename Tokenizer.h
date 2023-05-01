#ifndef TOKENIZER_H
#define TOKENIZER_H

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

namespace Fts
{
	class Tokenizer : public QObject
	{
	public:
		explicit Tokenizer(QObject *parent = 0);
		// to override
		virtual void setString( const QString& ) = 0;
		virtual QString nextToken() = 0; // Gibt ein Token (lowercase) nach dem anderen zurück bis Leerstring
	};

	class LetterOrNumberTok : public Tokenizer
	{
	public:
		LetterOrNumberTok(QObject* p = 0);
		QStringList parse( const QString& );
		// override
		void setString( const QString& );
		QString nextToken();
	private:
		QString d_str;
		int d_pos;
	};
}

#endif // TOKENIZER_H
