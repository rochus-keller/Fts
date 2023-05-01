#ifndef STOPPER_H
#define STOPPER_H

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
#include <QSet>

namespace Fts
{
	class Stopper : public QObject
	{
	public:
		explicit Stopper(QObject *parent = 0);
		// to override
		virtual bool isStopword( const QString& ) = 0;
	};

	class GermanStopper : public Stopper
	{
	public:
		GermanStopper( QObject* );
		// override
		bool isStopword( const QString& );
	private:
		QSet<QString> d_hash;
	};
}

#endif // STOPPER_H
