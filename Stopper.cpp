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

#include "Stopper.h"
#include <QtDebug>
using namespace Fts;

Stopper::Stopper(QObject *parent) :
	QObject(parent)
{
}

// ******************************************************************************
static const char *words_de[] = {
	"aber", "alle", "allem", "allen", "aller", "alles", "als", "also", "am",
	"an", "ander", "andere", "anderem", "anderen", "anderer", "anderes",
	"anderm", "andern", "anderr", "anders", "auch", "auf", "aus", "bei",
	"bin", "bis", "bist", "da", "damit", "dann", "der", "den", "des", "dem",
	"die", "das", "da\303\237", "derselbe", "derselben", "denselben",
	"desselben", "demselben", "dieselbe", "dieselben", "dasselbe", "dazu",
	"dein", "deine", "deinem", "deinen", "deiner", "deines", "denn", "derer",
	"dessen", "dich", "dir", "du", "dies", "diese", "diesem", "diesen",
	"dieser", "dieses", "doch", "dort", "durch", "ein", "eine", "einem",
	"einen", "einer", "eines", "einig", "einige", "einigem", "einigen",
	"einiger", "einiges", "einmal", "er", "ihn", "ihm", "es", "etwas",
	"euer", "eure", "eurem", "euren", "eurer", "eures", "f\303\274r",
	"gegen", "gewesen", "hab", "habe", "haben", "hat", "hatte", "hatten",
	"hier", "hin", "hinter", "ich", "mich", "mir", "ihr", "ihre", "ihrem",
	"ihren", "ihrer", "ihres", "euch", "im", "in", "indem", "ins", "ist",
	"jede", "jedem", "jeden", "jeder", "jedes", "jene", "jenem", "jenen",
	"jener", "jenes", "jetzt", "kann", "kein", "keine", "keinem", "keinen",
	"keiner", "keines", "k\303\266nnen", "k\303\266nnte", "machen", "man",
	"manche", "manchem", "manchen", "mancher", "manches", "mein", "meine",
	"meinem", "meinen", "meiner", "meines", "mit", "muss", "musste", "nach",
	"nicht", "nichts", "noch", "nun", "nur", "ob", "oder", "ohne", "sehr",
	"sein", "seine", "seinem", "seinen", "seiner", "seines", "selbst",
	"sich", "sie", "ihnen", "sind", "so", "solche", "solchem", "solchen",
	"solcher", "solches", "soll", "sollte", "sondern", "sonst",
	"\303\274ber", "um", "und", "uns", "unse", "unsem", "unsen", "unser",
	"unses", "unter", "viel", "vom", "von", "vor", "w\303\244hrend", "war",
	"waren", "warst", "was", "weg", "weil", "weiter", "welche", "welchem",
	"welchen", "welcher", "welches", "wenn", "werde", "werden", "wie",
	"wieder", "will", "wir", "wird", "wirst", "wo", "wollen", "wollte",
	"w\303\274rde", "w\303\274rden", "zu", "zum", "zur", "zwar", "zwischen",
	NULL
};
static const char *words_en[] = {
	"i", "me", "my", "myself", "we", "our", "ours", "ourselves", "you",
	"your", "yours", "yourself", "yourselves", "he", "him", "his", "himself",
	"she", "her", "hers", "herself", "it", "its", "itself", "they", "them",
	"their", "theirs", "themselves", "what", "which", "who", "whom", "this",
	"that", "these", "those", "am", "is", "are", "was", "were", "be", "been",
	"being", "have", "has", "had", "having", "do", "does", "did", "doing",
	"would", "should", "could", "ought", "i'm", "you're", "he's", "she's",
	"it's", "we're", "they're", "i've", "you've", "we've", "they've", "i'd",
	"you'd", "he'd", "she'd", "we'd", "they'd", "i'll", "you'll", "he'll",
	"she'll", "we'll", "they'll", "isn't", "aren't", "wasn't", "weren't",
	"hasn't", "haven't", "hadn't", "doesn't", "don't", "didn't", "won't",
	"wouldn't", "shan't", "shouldn't", "can't", "cannot", "couldn't",
	"mustn't", "let's", "that's", "who's", "what's", "here's", "there's",
	"when's", "where's", "why's", "how's", "a", "an", "the", "and", "but",
	"if", "or", "because", "as", "until", "while", "of", "at", "by", "for",
	"with", "about", "against", "between", "into", "through", "during",
	"before", "after", "above", "below", "to", "from", "up", "down", "in",
	"out", "on", "off", "over", "under", "again", "further", "then", "once",
	"here", "there", "when", "where", "why", "how", "all", "any", "both",
	"each", "few", "more", "most", "other", "some", "such", "no", "nor",
	"not", "only", "own", "same", "so", "than", "too", "very", NULL
};

GermanStopper::GermanStopper(QObject * p):Stopper(p)
{
	for( int i = 0; words_de[i] != 0; i++ )
		d_hash.insert( QString::fromUtf8( words_de[i] ) );
//	for( int i = 0; words_en[i] != 0; i++ )
//		d_hash.insert( QString::fromUtf8( words_en[i] ) );
}

bool GermanStopper::isStopword(const QString & str)
{
	return d_hash.contains( str );
}
