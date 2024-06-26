/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
 *   Copyright (C) 2004 by Riku Leino                                      *
 *   tsoots@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#ifndef CONTENTREADER_H
#define CONTENTREADER_H

#include "scconfig.h"

#ifdef HAVE_XML

#include <utility>
#include <vector>

#include <libxml/SAX2.h>
#include <QMap>

#include <gtstyle.h>
#include <gtwriter.h>
#include "stylereader.h"

using Properties = std::vector<std::pair<QString, QString> >;
using SXWAttributesMap = QMap<QString, QString>;
using TMap = QMap<QString, Properties>;

class ContentReader
{
public:
	ContentReader(const QString& documentName, StyleReader* s, gtWriter *w, bool textOnly);
	~ContentReader();
	
	void parse(const QString& fileName);

	static void startElement(void *user_data, const xmlChar *fullname, const xmlChar ** atts);
	static void endElement(void *user_data, const xmlChar *name);
	static void characters(void *user_data, const xmlChar *ch, int len);
	bool startElement(const QString &name, const SXWAttributesMap &attrs);
	bool endElement(const QString &name);
	bool characters(const QString &ch);

private:
	static ContentReader *creader;

	TMap tmap;
	QString docname;
	StyleReader* sreader { nullptr };
	gtWriter *writer { nullptr };
	gtStyle *defaultStyle { nullptr };
	gtStyle *currentStyle { nullptr };
	gtStyle *lastStyle { nullptr };
	gtStyle *pstyle { nullptr };
	bool importTextOnly { false };
	bool inList { false };
	bool inNote { false };
	bool inNoteBody { false };
	bool inSpan { false };
	int  append { 0 };
	int  listLevel { 0 };
	int  listIndex { 0 };
	std::vector<int> listIndex2;
	std::vector<bool> isOrdered2;
	bool inT { false };
	std::vector<QString> styleNames;
	QString tName;
	QString currentList;

	void write(const QString& text);
	QString getName() const;
	void getStyle();
};

#endif // HAVE_XML

#endif
