/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          fontcombo.cpp  -  description
                             -------------------
    begin                : Die Jun 17 2003
    copyright            : (C) 2003 by Franz Schmid
    email                : Franz.Schmid@altmuehlnet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QAbstractItemView>
#include <QEvent>
#include <QFont>
#include <QFontInfo>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QPixmapCache>
#include <QStringList>
#include <QToolTip>

#include "fontcombo.h"
#include "iconmanager.h"
#include "prefsmanager.h"
#include "scribusapp.h"
#include "scribusdoc.h"
#include "util.h"

#include "fonts/scface.h"

FontCombo::FontCombo(QWidget* pa) : QComboBox(pa),
									prefsManager(PrefsManager::instance())
{
	iconSetChange();
	setEditable(true);
	setValidator(new FontComboValidator(this));
	setInsertPolicy(QComboBox::NoInsert);
	setItemDelegate(new FontFamilyDelegate(this));
	RebuildList(nullptr);

	connect(ScQApp, SIGNAL(iconSetChanged()), this, SLOT(iconSetChange()));
}

void FontCombo::iconSetChange()
{
	IconManager& iconManager = IconManager::instance();
	ttfFont = iconManager.loadPixmap("font-truetype");
	otfFont = iconManager.loadPixmap("font-otf");
	psFont = iconManager.loadPixmap("font-postscript");
	substFont = iconManager.loadPixmap("font-substitute");
}

void FontCombo::RebuildList(ScribusDoc *currentDoc, bool forAnnotation, bool forSubstitute)
{
	clear();
	QMap<QString, QString> rlist;
	SCFontsIterator it(prefsManager.appPrefs.fontPrefs.AvailFonts);
	for ( ; it.hasNext(); it.next())
	{
		if (it.current().usable())
		{
			if (currentDoc != nullptr)
			{
				if (currentDoc->documentFileName() == it.current().localForDocument() || it.current().localForDocument().isEmpty())
					rlist.insert(it.currentKey().toLower(), it.currentKey());
			}
			else
				rlist.insert(it.currentKey().toLower(), it.currentKey());
		}
	}
	for (auto it2 = rlist.cbegin(); it2 != rlist.cend(); ++it2)
	{
		ScFace fon = prefsManager.appPrefs.fontPrefs.AvailFonts[it2.value()];
		if (!fon.usable())
			continue;
		ScFace::FontType type = fon.type();
		if (forAnnotation && ((type == ScFace::TYPE1) || (type == ScFace::OTF) || fon.subset()))
			continue;
		if (forSubstitute && fon.isReplacement())
			continue;
		if (fon.isReplacement())
			addItem(substFont, it2.value());
		else if (type == ScFace::OTF)
			addItem(otfFont, it2.value());
		else if (type == ScFace::TYPE1)
			addItem(psFont, it2.value());
		else if (type == ScFace::TTF)
			addItem(ttfFont, it2.value());
	}
	QAbstractItemView *tmpView = view();
	int tmpWidth = tmpView->sizeHintForColumn(0);
	if (tmpWidth > 0)
		tmpView->setMinimumWidth(tmpWidth + 24);
}

FontComboH::FontComboH(QWidget* parent, bool labels) :
		QWidget(parent),
		prefsManager(PrefsManager::instance()),
		showLabels(labels)
{	
	fontFamily = new QComboBox(this);
	fontFamily->setEditable(true);
	fontFamily->setValidator(new FontComboValidator(fontFamily));
	fontFamily->setInsertPolicy(QComboBox::NoInsert);
	fontFamily->setItemDelegate(new FontFamilyDelegate(this));

	fontFaceLabel = new FormWidget(this);
	fontFaceLabel->setLabelVisibility(false);
	fontFaceLabel->addWidget(fontFamily);

	fontStyle = new QComboBox(this);
	fontStyle->setSizeAdjustPolicy(QComboBox::AdjustToContents);

	fontStyleLabel = new FormWidget(this);
	fontStyleLabel->addWidget(fontStyle);

	fontComboLayout = new QGridLayout(this);
	fontComboLayout->setContentsMargins(0, 0, 0, 0);
	fontComboLayout->setSpacing(8);
	fontComboLayout->addWidget(fontFaceLabel, 0, 0, 1, 4);
	fontComboLayout->addWidget(fontStyleLabel, 1, 0);
	fontComboLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding), 1, 1);
	fontComboLayout->addWidget(new QWidget(), 1, 2);
	fontComboLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding), 1, 3);

	isForAnnotation = true;  // this is merely to ensure that the list is rebuilt

	iconSetChange();
	languageChange();
	rebuildList(nullptr);

	connect(fontFamily, SIGNAL(activated(int)), this, SLOT(familySelected(int)));
	connect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
	connect(ScQApp, SIGNAL(iconSetChanged()), this, SLOT(iconSetChange()));
	//connect(ScQApp, SIGNAL(labelVisibilityChanged(bool)), this, SLOT(toggleLabelVisibility(bool)));

}

void FontComboH::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
		languageChange();
	else
		QWidget::changeEvent(e);
}

void FontComboH::languageChange()
{
	fontFaceLabel->setText( tr("Family"));
	fontStyleLabel->setText( tr("Style"));
	fontFamily->setToolTip( tr("Font Family of Selected Text or Text Frame"));
	fontStyle->setToolTip( tr("Font Style of Selected Text or Text Frame"));
}

void FontComboH::iconSetChange()
{
	IconManager &im = IconManager::instance();

	ttfFont = im.loadPixmap("font-truetype");
	otfFont = im.loadPixmap("font-otf");
	psFont = im.loadPixmap("font-postscript");
	substFont = im.loadPixmap("font-substitute");

	fontFaceLabel->setPixmap(im.loadPixmap("font-face"));
	fontStyleLabel->setPixmap(im.loadPixmap("font-style"));
}

void FontComboH::toggleLabelVisibility(bool v)
{
	fontStyleLabel->setLabelVisibility(v);
}

void FontComboH::familySelected(int id)
{
	disconnect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
	QString curr = fontStyle->currentText();
	fontStyle->clear();
	QString fntFamily = fontFamily->itemText(id);
	QStringList slist, styleList = prefsManager.appPrefs.fontPrefs.AvailFonts.fontMap[fntFamily];
	for (QStringList::ConstIterator it = styleList.constBegin(); it != styleList.constEnd(); ++it)
	{
		SCFonts::ConstIterator fIt = prefsManager.appPrefs.fontPrefs.AvailFonts.find(fntFamily + " " + *it);
		if (fIt != prefsManager.appPrefs.fontPrefs.AvailFonts.constEnd())
		{
			if (!fIt->usable() || fIt->isReplacement())
				continue;
			slist.append(*it);
		}
	}
	slist.sort();
	fontStyle->addItems(slist);
	if (slist.contains(curr))
		setCurrentComboItem(fontStyle, curr);
	else if (slist.contains("Regular"))
		setCurrentComboItem(fontStyle, "Regular");
	else if (slist.contains("Roman"))
		setCurrentComboItem(fontStyle, "Roman");
	emit fontSelected(fontFamily->itemText(id) + " " + fontStyle->currentText());
	connect(fontStyle, SIGNAL(activated(int)), this, SLOT(styleSelected(int)));
}

void FontComboH::styleSelected(int id)
{
	emit fontSelected(fontFamily->currentText() + " " + fontStyle->itemText(id));
}

QString FontComboH::currentFont() const
{
	return fontFamily->currentText() + " " + fontStyle->currentText();
}

void FontComboH::setCurrentFont(const QString& f)
{
	QString family = prefsManager.appPrefs.fontPrefs.AvailFonts[f].family();
	QString style = prefsManager.appPrefs.fontPrefs.AvailFonts[f].style();
	// If we already have the correct font+style, nothing to do
	if ((fontFamily->currentText() == family) && (fontStyle->currentText() == style))
		return;
	bool familySigBlocked = fontFamily->blockSignals(true);
	bool styleSigBlocked = fontStyle->blockSignals(true);
	setCurrentComboItem(fontFamily, family);
	fontStyle->clear();
	QStringList slist = prefsManager.appPrefs.fontPrefs.AvailFonts.fontMap[family];
	slist.sort();
	QStringList ilist;
	if (currDoc != nullptr)
	{
		for (QStringList::ConstIterator it3 = slist.constBegin(); it3 != slist.constEnd(); ++it3)
		{
			SCFonts::ConstIterator fIt = prefsManager.appPrefs.fontPrefs.AvailFonts.find(family + " " + *it3);
			if (fIt != prefsManager.appPrefs.fontPrefs.AvailFonts.constEnd())
			{
				if (!fIt->usable() || fIt->isReplacement())
					continue;
				if ((currDoc->documentFileName() == fIt->localForDocument()) || (fIt->localForDocument().isEmpty()))
					ilist.append(*it3);
			}
		}
		fontStyle->addItems(ilist);
	}
	else
		fontStyle->addItems(slist);
	setCurrentComboItem(fontStyle, style);
	fontFamily->blockSignals(familySigBlocked);
	fontStyle->blockSignals(styleSigBlocked);
}

void FontComboH::rebuildList(ScribusDoc *currentDoc, bool forAnnotation, bool forSubstitute)
{
	// if we already have the proper fonts loaded, we need to do nothing
	if ((currDoc == currentDoc) && (forAnnotation == isForAnnotation) && (isForSubstitute == forSubstitute))
		return;
	currDoc = currentDoc;
	isForAnnotation = forAnnotation;
	isForSubstitute = forSubstitute;
	bool familySigBlocked = fontFamily->blockSignals(true);
	bool styleSigBlocked = fontStyle->blockSignals(true);
	fontFamily->clear();
	fontStyle->clear();
	QStringList rlist = prefsManager.appPrefs.fontPrefs.AvailFonts.fontMap.keys();
	QMap<QString, ScFace::FontType> flist;
	flist.clear();
	for (QStringList::ConstIterator it2 = rlist.constBegin(); it2 != rlist.constEnd(); ++it2)
	{
		if (currentDoc != nullptr)
		{
			QStringList slist = prefsManager.appPrefs.fontPrefs.AvailFonts.fontMap[*it2];
			slist.sort();
			for (QStringList::ConstIterator it3 = slist.constBegin(); it3 != slist.constEnd(); ++it3)
			{
				if ( prefsManager.appPrefs.fontPrefs.AvailFonts.contains(*it2 + " " + *it3))
				{
					const ScFace& fon(prefsManager.appPrefs.fontPrefs.AvailFonts[*it2 + " " + *it3]);
					ScFace::FontType type = fon.type();
					if (!fon.usable() || fon.isReplacement() || !(currentDoc->documentFileName() == fon.localForDocument() || fon.localForDocument().isEmpty()))
						continue;
					if (forAnnotation && ((type == ScFace::TYPE1) || (type == ScFace::OTF) || (fon.subset())))
						continue;
					if (forSubstitute && fon.isReplacement())
						continue;
					flist.insert(*it2, fon.type());
					break;
				}
			}
		}
		else
		{
			QMap<QString, QStringList>::ConstIterator fmIt = prefsManager.appPrefs.fontPrefs.AvailFonts.fontMap.find(*it2);
			if (fmIt == prefsManager.appPrefs.fontPrefs.AvailFonts.fontMap.constEnd())
				continue;
			if (fmIt->count() <= 0)
				continue;
			QString fullFontName = (*it2) + " " + fmIt->at(0);
			ScFace fon = prefsManager.appPrefs.fontPrefs.AvailFonts[fullFontName];
			if ( !fon.usable() || fon.isReplacement() )
				continue;
			ScFace::FontType type = fon.type();
			if (forAnnotation && ((type == ScFace::TYPE1) || (type == ScFace::OTF) || (fon.subset())))
				continue;
			if (forSubstitute && fon.isReplacement())
				continue;
			flist.insert(*it2, fon.type());
		}
	}
	for (auto it2a = flist.constBegin(); it2a != flist.constEnd(); ++it2a)
	{
		ScFace::FontType type = it2a.value();
		if (type == ScFace::OTF)
			fontFamily->addItem(otfFont, it2a.key());
		else if (type == ScFace::TYPE1)
			fontFamily->addItem(psFont, it2a.key());
		else if (type == ScFace::TTF)
			fontFamily->addItem(ttfFont, it2a.key());
	}
	QString family = fontFamily->currentText();
	QStringList slist = prefsManager.appPrefs.fontPrefs.AvailFonts.fontMap[family];
	slist.sort();
	QStringList ilist;
	if (currentDoc != nullptr)
	{
		for (QStringList::ConstIterator it = slist.constBegin(); it != slist.constEnd(); ++it)
		{
			SCFonts::ConstIterator fIt = prefsManager.appPrefs.fontPrefs.AvailFonts.find(family + " " + *it);
			if (fIt != prefsManager.appPrefs.fontPrefs.AvailFonts.constEnd())
			{
				if (!fIt->usable() || fIt->isReplacement())
					continue;
				ilist.append(*it);
			}
		}
		fontStyle->addItems(ilist);
	}
	else
		fontStyle->addItems(slist);
	fontFamily->blockSignals(familySigBlocked);
	fontStyle->blockSignals(styleSigBlocked);
}

void FontComboH::setGuestWidget(QWidget *widget)
{
	fontComboLayout->addWidget(widget, 1, 2);
}

QWidget *FontComboH::guestWidget()
{
	if (fontComboLayout->itemAtPosition(1, 2)->widget() == nullptr)
		return new QWidget();

	return fontComboLayout->itemAtPosition(1, 2)->widget();
}

// This code borrowed from Qt project qfontcombobox.cpp

static QFontDatabase::WritingSystem writingSystemFromScript(QLocale::Script script)
{
	switch (script) {
	case QLocale::ArabicScript:
		return QFontDatabase::Arabic;
	case QLocale::CyrillicScript:
		return QFontDatabase::Cyrillic;
	case QLocale::GurmukhiScript:
		return QFontDatabase::Gurmukhi;
	case QLocale::SimplifiedHanScript:
		return QFontDatabase::SimplifiedChinese;
	case QLocale::TraditionalHanScript:
		return QFontDatabase::TraditionalChinese;
	case QLocale::LatinScript:
		return QFontDatabase::Latin;
	case QLocale::ArmenianScript:
		return QFontDatabase::Armenian;
	case QLocale::BengaliScript:
		return QFontDatabase::Bengali;
	case QLocale::DevanagariScript:
		return QFontDatabase::Devanagari;
	case QLocale::GeorgianScript:
		return QFontDatabase::Georgian;
	case QLocale::GreekScript:
		return QFontDatabase::Greek;
	case QLocale::GujaratiScript:
		return QFontDatabase::Gujarati;
	case QLocale::HebrewScript:
		return QFontDatabase::Hebrew;
	case QLocale::JapaneseScript:
		return QFontDatabase::Japanese;
	case QLocale::KhmerScript:
		return QFontDatabase::Khmer;
	case QLocale::KannadaScript:
		return QFontDatabase::Kannada;
	case QLocale::KoreanScript:
		return QFontDatabase::Korean;
	case QLocale::LaoScript:
		return QFontDatabase::Lao;
	case QLocale::MalayalamScript:
		return QFontDatabase::Malayalam;
	case QLocale::MyanmarScript:
		return QFontDatabase::Myanmar;
	case QLocale::TamilScript:
		return QFontDatabase::Tamil;
	case QLocale::TeluguScript:
		return QFontDatabase::Telugu;
	case QLocale::ThaanaScript:
		return QFontDatabase::Thaana;
	case QLocale::ThaiScript:
		return QFontDatabase::Thai;
	case QLocale::TibetanScript:
		return QFontDatabase::Tibetan;
	case QLocale::SinhalaScript:
		return QFontDatabase::Sinhala;
	case QLocale::SyriacScript:
		return QFontDatabase::Syriac;
	case QLocale::OriyaScript:
		return QFontDatabase::Oriya;
	case QLocale::OghamScript:
		return QFontDatabase::Ogham;
	case QLocale::RunicScript:
		return QFontDatabase::Runic;
	case QLocale::NkoScript:
		return QFontDatabase::Nko;
	default:
		return QFontDatabase::Any;
	}
}

static QFontDatabase::WritingSystem writingSystemFromLocale()
{
	QStringList uiLanguages = QLocale::system().uiLanguages();
	QLocale::Script script;
	if (!uiLanguages.isEmpty())
		script = QLocale(uiLanguages.at(0)).script();
	else
		script = QLocale::system().script();

	return writingSystemFromScript(script);
}

QFontDatabase::WritingSystem writingSystemForFont(const QFont &font, bool *hasLatin)
{
	QList<QFontDatabase::WritingSystem> writingSystems = QFontDatabase::writingSystems(font.family());

	// this just confuses the algorithm below. Vietnamese is Latin with lots of special chars
	writingSystems.removeOne(QFontDatabase::Vietnamese);
	*hasLatin = writingSystems.removeOne(QFontDatabase::Latin);

	if (writingSystems.isEmpty())
		return QFontDatabase::Any;

	QFontDatabase::WritingSystem system = writingSystemFromLocale();

	if (writingSystems.contains(system))
		return system;

	if (system == QFontDatabase::TraditionalChinese && writingSystems.contains(QFontDatabase::SimplifiedChinese))
		return QFontDatabase::SimplifiedChinese;

	if (system == QFontDatabase::SimplifiedChinese && writingSystems.contains(QFontDatabase::TraditionalChinese))
		return QFontDatabase::TraditionalChinese;

	system = writingSystems.last();

	if (!*hasLatin)
		// we need to show something
		return system;

	if (writingSystems.count() == 1 && system > QFontDatabase::Cyrillic)
		return system;

	if (writingSystems.count() <= 2 && system > QFontDatabase::Armenian && system < QFontDatabase::Vietnamese)
		return system;

	if (writingSystems.count() <= 5 && system >= QFontDatabase::SimplifiedChinese && system <= QFontDatabase::Korean)
		return system;

	return QFontDatabase::Any;
}

const ScFace& getScFace(const QString& className, const QString& text)
{
	SCFonts& availableFonts = PrefsManager::instance().appPrefs.fontPrefs.AvailFonts;

	// Handle FontComboH class witch has only Family names in the combo class.
	if (className == "FontComboH" || className == "SMFontComboH")
	{
		// SMFontComboH's "Use Parent Font" case
		if (!availableFonts.fontMap.contains(text))
			return ScFace::none();
		QStringList styles = availableFonts.fontMap[text];
		QString style = styles[0];
		if (styles.contains("Regular"))
			style = "Regular";
		else if (styles.contains("Roman"))
			style = "Roman";
		else if (styles.contains("Medium"))
			style = "Medium";
		else if (styles.contains("Book"))
			style = "Book";
		const ScFace& fon = availableFonts.findFont(text, style);
		if (!QFontDatabase::families().contains(text))
			QFontDatabase::addApplicationFont(fon.fontFilePath());
		return fon;
	}
	const ScFace& scFace = availableFonts.findFont(text);
	if (!QFontDatabase::families().contains(scFace.family()))
		QFontDatabase::addApplicationFont(scFace.fontFilePath());
	return scFace;
}

FontFamilyDelegate::FontFamilyDelegate(QObject *parent)
	: QAbstractItemDelegate(parent)
{
	QPixmapCache::setCacheLimit(64 * 1024);
}

void FontFamilyDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	double pixelRatio = painter->device()->devicePixelRatioF();
	int pixmapW = qRound(pixelRatio * option.rect.width());
	int pixmapH = qRound(pixelRatio * option.rect.height());
	QString text(index.data(Qt::DisplayRole).toString());
	QString wh = QString("-w%1h%2").arg(pixmapW).arg(pixmapH);
	QPixmap cachedPixmap;
	QString cacheKey(text + wh);
	QString findKey(cacheKey);
	if (option.state & QStyle::State_Selected)
		findKey += "-selected";
	if (QPixmapCache::find(findKey, &cachedPixmap))
	{
		painter->drawPixmap(option.rect.x(), option.rect.y(), cachedPixmap);
		return;
	}

	QString className(this->parent()->metaObject()->className());
	const ScFace& scFace = getScFace(className, text);

	QPixmap pixmap(pixmapW, pixmapH);
	QPixmap invPixmap(pixmapW, pixmapH);
	pixmap.setDevicePixelRatio(pixelRatio);
	invPixmap.setDevicePixelRatio(pixelRatio);

	QPainter pixPainter(&pixmap);
	QPainter invpixPainter(&invPixmap);

	QRect r(0, 0, option.rect.width(), option.rect.height());
	pixPainter.fillRect(r, Qt::white); // #17514: protection if fill color has transparency
	pixPainter.fillRect(r, option.palette.window());
	pixPainter.setPen(QPen(option.palette.text(), 0));

	invpixPainter.fillRect(r, Qt::white); // #17514: protection if fill color has transparency
	invpixPainter.fillRect(r, option.palette.highlight());
	invpixPainter.setPen(QPen(option.palette.highlightedText(), 0));

	QFont font = option.font;
	font.setPointSize(QFontInfo(font).pointSize() * 3.0 / 2.0);

	QFont font2 = option.font;
	if (!scFace.isNone())
	{
		font2 = QFontDatabase::font(scFace.family(), scFace.style(), QFontInfo(option.font).pointSize());
		font2.setPointSize(QFontInfo(font2).pointSize() * 3.0 / 2.0);
	}

	bool hasLatin;
	QFontDatabase::WritingSystem system = writingSystemForFont(font2, &hasLatin);
	if (hasLatin)
		font = font2;

	QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
	QSize actualSize(icon.actualSize(r.size()));
	icon.paint(&pixPainter, r, Qt::AlignLeft|Qt::AlignVCenter);
	icon.paint(&invpixPainter, r, Qt::AlignLeft|Qt::AlignVCenter);
	if (option.direction == Qt::RightToLeft)
		r.setRight(r.right() - actualSize.width() - 4);
	else
		r.setLeft(r.left() + actualSize.width() + 4);

	pixPainter.setFont(font);
	invpixPainter.setFont(font);
	// If the ascent of the font is larger than the height of the rect,
	// we will clip the text, so it's better to align the tight bounding rect in this case
	// This is specifically for fonts where the ascent is very large compared to
	// the descent, like certain of the Stix family.
	QFontMetricsF fontMetrics(font);
	if (fontMetrics.ascent() > r.height())
	{
		QRectF tbr = fontMetrics.tightBoundingRect(text);
		QRectF rr (r.x(), r.y() - (tbr.bottom() + r.height()/2), r.width(),(r.height() + tbr.height()));
		pixPainter.drawText(rr, Qt::AlignVCenter|Qt::AlignLeading|Qt::TextSingleLine, text);
		invpixPainter.drawText(rr, Qt::AlignVCenter|Qt::AlignLeading|Qt::TextSingleLine, text);
	}
	else
	{
		pixPainter.drawText(r, Qt::AlignVCenter|Qt::AlignLeading|Qt::TextSingleLine, text);
		invpixPainter.drawText(r, Qt::AlignVCenter|Qt::AlignLeading|Qt::TextSingleLine, text);
	}

	if (writingSystem != QFontDatabase::Any)
		system = writingSystem;

	if (system != QFontDatabase::Any)
	{
		int w = pixPainter.fontMetrics().horizontalAdvance(text + QLatin1String("  "));
		pixPainter.setFont(font2);
		invpixPainter.setFont(font2);
		QString sample = QFontDatabase::writingSystemSample(system);
		if (system == QFontDatabase::Arabic)
			sample = "أبجدية عربية";

		if (fontMetrics.ascent() > r.height())
		{
			QRectF tbr = fontMetrics.tightBoundingRect(sample);
			QRectF rr (r.x(), r.y() - (tbr.bottom() + r.height()/2), r.width(), (r.height() + tbr.height()));
			if (option.direction == Qt::RightToLeft)
				rr.setRight(rr.right() - w);
			else
			{
				rr.setRight(rr.right() - 4);
				rr.setLeft(rr.left() + w);
			}
			pixPainter.drawText(rr, Qt::AlignVCenter|Qt::AlignRight|Qt::TextSingleLine, sample);
			invpixPainter.drawText(rr, Qt::AlignVCenter|Qt::AlignRight|Qt::TextSingleLine, sample);
		}
		else
		{
			if (option.direction == Qt::RightToLeft)
				r.setRight(r.right() - w);
			else
			{
				r.setRight(r.right() - 4);
				r.setLeft(r.left() + w);
			}
			pixPainter.drawText(r, Qt::AlignVCenter|Qt::AlignRight|Qt::TextSingleLine, sample);
			invpixPainter.drawText(r, Qt::AlignVCenter|Qt::AlignRight|Qt::TextSingleLine, sample);
		}
	}
	if (option.state & QStyle::State_Selected)
		painter->drawPixmap(option.rect.x(), option.rect.y(), invPixmap);
	else
		painter->drawPixmap(option.rect.x(), option.rect.y(), pixmap);
	QPixmapCache::insert(cacheKey, pixmap);
	QPixmapCache::insert(cacheKey + "-selected", invPixmap);
}

bool FontFamilyDelegate::helpEvent(QHelpEvent * event, QAbstractItemView * view,
	const QStyleOptionViewItem & option, const QModelIndex & index)
{
	if (!event || !view)
		return false;

	if (event->type() == QEvent::ToolTip)
	{
		QString text(index.data(Qt::DisplayRole).toString());
		QString className = this->parent()->metaObject()->className();
		const ScFace& scFace = getScFace(className, text);
		if (!scFace.isNone())
		{
			QString tooltip = scFace.family();
			if (className == QLatin1String("FontCombo"))
			{
				tooltip += QLatin1String(" ");
				tooltip += scFace.style();
			}
			QToolTip::showText(event->globalPos(), tooltip, view);
			return true;
		}
	}
	
	return QAbstractItemDelegate::helpEvent(event, view, option, index);
}

QSize FontFamilyDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QString text(index.data(Qt::DisplayRole).toString());
	QFont font(option.font);
	font.setPointSize(QFontInfo(font).pointSize() * 3/2);
	QFontMetrics fontMetrics(font);
	return QSize(fontMetrics.horizontalAdvance(text), fontMetrics.height() + 5);
}


FontComboValidator::FontComboValidator(QObject* parent)
	: QValidator(parent)
{
}

QValidator::State FontComboValidator::validate(QString & input, int & pos) const
{
	const QComboBox* comboBox = qobject_cast<QComboBox*>(this->parent());
	if (!comboBox)
		return QValidator::Invalid;

	int index = comboBox->findText(input, Qt::MatchFixedString);
	if (index >= 0)
	{
		input = comboBox->itemText(index); // Matching is performed case insensitively
		return QValidator::Acceptable;
	}

	for (int i = 0; i < comboBox->count(); ++i)
	{
		QString itemText = comboBox->itemText(i);
		if (itemText.startsWith(input, Qt::CaseInsensitive))
			return QValidator::Intermediate;
	}

	return QValidator::Invalid;
}
