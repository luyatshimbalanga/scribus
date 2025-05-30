/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QEvent>


#include "iconmanager.h"
#include "numeration.h"
#include "smpstylewidget.h"
#include "scribus.h"
#include "scribusapp.h"
#include "units.h"
#include "util.h"
#include "ui/charselectenhanced.h"

static bool isEqual(double a, double b)
{
	Q_ASSERT(a >  -21473 && b > -21473 && a < 21474 && b < 21474);
	long al = static_cast<long>(10000 * a);
	long bl = static_cast<long>(10000 * b);
	return al == bl;
}


SMPStyleWidget::SMPStyleWidget(ScribusDoc* doc, StyleSet<CharStyle> *cstyles) :
	m_Doc(doc),
	m_cstyles(cstyles)
{
	setupUi(this);

	//Not used yet
// 	optMarginCheckLeftProtruding->setVisible(false);

	iconSetChange();

	backgroundColor->colorButton->setContext(Context::TextBackground);
	backgroundColor->setColor(CommonStrings::tr_NoneColor);
	backgroundColor->setText(tr("Background"));

	lineSpacingMode->addItem( tr("Fixed"));
	lineSpacingMode->addItem( tr("Automatic"));
	lineSpacingMode->addItem( tr("Baseline"));
	connect(lineSpacingMode, SIGNAL(highlighted(int)), this, SLOT(slotLineSpacingModeChanged(int)));

	lineSpacing->setSuffix(unitGetSuffixFromIndex(0));
	spaceAbove->setSuffix(unitGetSuffixFromIndex(0));
	spaceBelow->setSuffix(unitGetSuffixFromIndex(0));

//	optMarginCombo->addItem(tr("None"), ParagraphStyle::OM_None);
//	optMarginCombo->addItem(tr("Left Protruding"), ParagraphStyle::OM_LeftProtruding);
//	optMarginCombo->addItem(tr("Right Protruding"), ParagraphStyle::OM_RightProtruding);
//	optMarginCombo->addItem(tr("Left Hanging Punctuation"), ParagraphStyle::OM_LeftHangingPunct);
//	optMarginCombo->addItem(tr("Right Hanging Punctuation"), ParagraphStyle::OM_RightHangingPunct);
//	optMarginCombo->addItem(tr("Default"), ParagraphStyle::OM_Default);

	parEffectOffset->setSuffix(unitGetSuffixFromIndex(0));

	fillBulletStrEditCombo();

	numStartSpin->setMinimum(1);
	numStartSpin->setMaximum(9999);
	numLevelSpin->setMinimum(1);
	numLevelSpin->setMaximum(1);
	numPrefix->setMaxLength(5);
	numPrefix->setMaximumWidth(QFontMetrics(numPrefix->font()).averageCharWidth() * 7);
	numSuffix->setMaxLength(5);
	numSuffix->setMaximumWidth(QFontMetrics(numSuffix->font()).averageCharWidth() * 7);
	fillNumRestartCombo();
	dropCapLines->setMinimum(2);
	dropCapLines->setMaximum(99);

	fillPECombo();
	connect(peCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(handleParEffectUse(int)));
	connect(parentParEffectsButton, SIGNAL(clicked()), this, SLOT(slotParentParEffects()));

	minSpaceSpin->setSuffix(unitGetSuffixFromIndex(SC_PERCENT));
	minGlyphExtSpin->setSuffix(unitGetSuffixFromIndex(SC_PERCENT));
	maxGlyphExtSpin->setSuffix(unitGetSuffixFromIndex(SC_PERCENT));

	connect(ScQApp, SIGNAL(iconSetChanged()), this, SLOT(iconSetChange()));

	if (m_Doc)
		connect(m_Doc->scMW(), SIGNAL(UpdateRequest(int)), this , SLOT(handleUpdateRequest(int)));
	m_enhanced = nullptr;
}

void SMPStyleWidget::slotLineSpacingModeChanged(int i)
{
	lineSpacing->setEnabled(i == 0);
}

void SMPStyleWidget::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
	{
		languageChange();
	}
	else
		QWidget::changeEvent(e);
}

void SMPStyleWidget::iconSetChange()
{
	IconManager& iconManager = IconManager::instance();

	lineSpacingLabel->setPixmap(iconManager.loadPixmap("paragraph-line-height"));
	spaceAboveLabel->setPixmap(iconManager.loadPixmap("paragraph-space-above") );
	spaceBelowLabel->setPixmap(iconManager.loadPixmap("paragraph-space-below") );
	keepLabelStart->setPixmap(iconManager.loadPixmap("paragraph-orphan") );
	keepLabelEnd->setPixmap(iconManager.loadPixmap("paragraph-widow") );
	minSpaceLabel->setPixmap(iconManager.loadPixmap("character-space-width-min"));
	minGlyphExtLabel->setPixmap(iconManager.loadPixmap("character-scale-width-min"));
	maxGlyphExtLabel->setPixmap(iconManager.loadPixmap("character-scale-width-max"));
	consecutiveHyphenLabel->setPixmap(iconManager.loadPixmap("hyphen-consecutive"));

	bulletCharTableButton->setIcon(iconManager.loadIcon("character-table"));
	distFromTextLabel->setPixmap(iconManager.loadPixmap("paragraph-list-offset"));
	parEffectCharStyleComboLabel->setPixmap(iconManager.loadPixmap("character-style"));
	peLabel->setPixmap(iconManager.loadPixmap("paragraph-effects-style"));
	parentParEffectsButton->setIcon(iconManager.loadIcon("reset-style-changes"));
}

void SMPStyleWidget::languageChange()
{
	retranslateUi(this);
	alignment->languageChange();
	tabList->firstLineSpin->setToolTip( tr("First Line Indent"));
	tabList->leftIndentSpin->setToolTip(  tr("Left Indent"));
	tabList->rightIndentSpin->setToolTip( tr("Right Indent"));

	int  oldLineSpacingModeIndex = lineSpacingMode->currentIndex();
	bool lineSpacingModeBlocked = lineSpacingMode->blockSignals(true);
	lineSpacingMode->clear();
	lineSpacingMode->addItem( tr("Fixed"));
	lineSpacingMode->addItem( tr("Automatic"));
	lineSpacingMode->addItem( tr("Baseline"));
	lineSpacingMode->setCurrentIndex(oldLineSpacingModeIndex);
	lineSpacingMode->blockSignals(lineSpacingModeBlocked);

	lineSpacing->setSuffix(unitGetSuffixFromIndex(0));
	spaceAbove->setSuffix(unitGetSuffixFromIndex(0));
	spaceBelow->setSuffix(unitGetSuffixFromIndex(0));

	keepLinesStart->setSuffix( tr(" lines"));
	keepLinesEnd->setSuffix( tr(" lines"));

	int  oldNumRestartIndex = numRestartCombo->currentIndex();
	bool numRestartComboBlocked = numRestartCombo->blockSignals(true);
	fillNumRestartCombo();
	numRestartCombo->setCurrentIndex(oldNumRestartIndex);
	numRestartCombo->blockSignals(numRestartComboBlocked);

	fillPECombo();

	backgroundColor->colorButton->setPersistentToolTip( tr("Background color of selected text"));
	backgroundColor->setText(tr("Background"));
}

void SMPStyleWidget::unitChange(double oldRatio, double newRatio, int unitIndex)
{
	parEffectOffset->setNewUnit(unitIndex);
	tabList->unitChange(unitIndex);
}

void SMPStyleWidget::setDoc(ScribusDoc *doc)
{
	if (m_Doc)
		disconnect(m_Doc->scMW(), SIGNAL(UpdateRequest(int)), this , SLOT(handleUpdateRequest(int)));

	m_Doc = doc;
	if (m_Doc)
	{
		connect(m_Doc->scMW(), SIGNAL(UpdateRequest(int)), this , SLOT(handleUpdateRequest(int)));
		fillNumerationsCombo();
	}

	cpage->setDoc(m_Doc);
	backgroundColor->colorButton->setDoc(m_Doc);
}

void SMPStyleWidget::show(ParagraphStyle *pstyle, QList<ParagraphStyle> &pstyles, QList<CharStyle> &cstyles, int unitIndex, const QString &defLang)
{
	m_currPStyle = pstyle;
	double unitRatio = unitGetRatioFromIndex(unitIndex);
	parentCombo->setEnabled(!pstyle->isDefaultStyle());
	const ParagraphStyle *parent = dynamic_cast<const ParagraphStyle*>(pstyle->parentStyle());
	m_hasParent = pstyle->hasParent() && parent != nullptr && parent->hasName() && pstyle->parent() != "";

	int  oldLineSpacingModeIndex = lineSpacingMode->currentIndex();
	bool lineSpacingModeBlocked = lineSpacingMode->blockSignals(true);
	lineSpacingMode->clear();
	lineSpacingMode->addItem( tr("Fixed"));
	lineSpacingMode->addItem( tr("Automatic"));
	lineSpacingMode->addItem( tr("Baseline"));
	lineSpacingMode->setCurrentIndex(oldLineSpacingModeIndex);
	lineSpacingMode->blockSignals(lineSpacingModeBlocked);
	
//	optMarginCombo->clear();
//	optMarginCombo->addItem(tr("None"), ParagraphStyle::OM_None);
//	optMarginCombo->addItem(tr("Left Protruding"), ParagraphStyle::OM_LeftProtruding);
//	optMarginCombo->addItem(tr("Right Protruding"), ParagraphStyle::OM_RightProtruding);
//	optMarginCombo->addItem(tr("Left Hanging Punctuation"), ParagraphStyle::OM_LeftHangingPunct);
//	optMarginCombo->addItem(tr("Right Hanging Punctuation"), ParagraphStyle::OM_RightHangingPunct);
//	optMarginCombo->addItem(tr("Default"), ParagraphStyle::OM_Default);
	
	// One could think it’s too much (aesthetic) or not enough (freedom)!
	minSpaceSpin->setRange(1.0,100.0);
	minGlyphExtSpin->setRange(90.0,100.0);
	maxGlyphExtSpin->setRange(100.0,110.0);

	maxConsecutiveCountSpinBox->clear();
	
	//fillBulletStrEditCombo();
	//fillNumerationsCombo();
	//fillNumRestartCombo();

	optMarginWidget->setOpticalMarginSets(m_Doc->opticalMarginSets());

	if (m_hasParent)
	{
		lineSpacingMode->setCurrentItem(pstyle->lineSpacingMode(), pstyle->isInhLineSpacingMode());
		lineSpacingMode->setParentItem(parent->lineSpacingMode());
		
//		optMarginCombo->setCurrentItemByData( pstyle->opticalMargins(),  pstyle->isInhOpticalMargins() );
//		optMarginCombo->setParentItem(optMarginCombo->getItemIndexForData( parent->opticalMargins()));
		optMarginWidget->setOpticalMargins(pstyle->opticalMargins(), parent->opticalMargins(), pstyle->isInhOpticalMargins());		
		optMarginWidget->setOpticalMarginSetId(pstyle->opticalMarginSetId(), parent->opticalMarginSetId(), pstyle->isInhOpticalMarginSetId());
		
		minSpaceSpin->setValue(pstyle->minWordTracking() * 100.0,  pstyle->isInhMinWordTracking());
		minSpaceSpin->setParentValue(parent->minWordTracking());
		minGlyphExtSpin->setValue(pstyle->minGlyphExtension() * 100.0,  pstyle->isInhMinGlyphExtension());
		minGlyphExtSpin->setParentValue(parent->minGlyphExtension());
		maxGlyphExtSpin->setValue(pstyle->maxGlyphExtension() * 100.0,  pstyle->isInhMaxGlyphExtension());
		maxGlyphExtSpin->setParentValue(parent->maxGlyphExtension());

		maxConsecutiveCountSpinBox->setValue(pstyle->hyphenConsecutiveLines(), pstyle->isInhHyphenConsecutiveLines());
		maxConsecutiveCountSpinBox->setParentValue(parent->hyphenConsecutiveLines());

		lineSpacing->setValue(pstyle->lineSpacing(), pstyle->isInhLineSpacing());
		lineSpacing->setParentValue(parent->lineSpacing());

		spaceAbove->setValue(pstyle->gapBefore(), pstyle->isInhGapBefore());
		spaceAbove->setParentValue(parent->gapBefore());

		spaceBelow->setValue(pstyle->gapAfter(), pstyle->isInhGapAfter());
		spaceBelow->setParentValue(parent->gapAfter());

		alignment->setStyle(pstyle->alignment(), direction->getStyle(), pstyle->isInhAlignment());
		alignment->setParentItem(parent->alignment(), direction->getStyle());

		direction->setStyle(pstyle->direction());
		direction->setParentItem(parent->direction());

		bool hasParentTabs = pstyle->isInhTabValues();
		QList<ParagraphStyle::TabRecord> tabs;
		if (hasParentTabs)
			tabs = QList<ParagraphStyle::TabRecord>(parent->tabValues());
		else
			tabs = pstyle->tabValues();

		tabList->setTabs(tabs, unitIndex, hasParentTabs);
		tabList->setParentTabs(parent->tabValues());

		tabList->setLeftIndentValue(pstyle->leftMargin() * unitRatio,pstyle->isInhLeftMargin());
		tabList->setParentLeftIndent(parent->leftMargin() * unitRatio);

		tabList->setFirstLineValue(pstyle->firstIndent() * unitRatio, pstyle->isInhFirstIndent());
		tabList->setParentFirstLine(parent->firstIndent() * unitRatio);

		tabList->setRightIndentValue(pstyle->rightMargin() * unitRatio, pstyle->isInhRightMargin());
		tabList->setParentRightIndent(parent->rightMargin() * unitRatio);

		keepLinesStart->setValue (pstyle->keepLinesStart(), pstyle->isInhKeepLinesStart());
		keepLinesEnd->setValue (pstyle->keepLinesEnd(), pstyle->isInhKeepLinesEnd());
		keepTogether->setChecked (pstyle->keepTogether(), pstyle->isInhKeepTogether());
		keepWithNext->setChecked (pstyle->keepWithNext(), pstyle->isInhKeepWithNext());
		keepLinesStart->setParentValue (parent->keepLinesStart());
		keepLinesEnd->setParentValue (parent->keepLinesEnd());
		keepTogether->setParentValue (parent->keepTogether());
		keepWithNext->setParentValue (parent->keepWithNext());
		
//Effects Gropup Box
		m_parentDC = parent->hasDropCap();
		m_parentBul = parent->hasBullet();
		m_parentNum = parent->hasNum();
		//parentParEffects_ = (m_parentDC || m_parentBul || m_parentNum);
		if (pstyle->isInhHasDropCap() && pstyle->isInhHasBullet() && pstyle->isInhHasNum())
		{
			parentParEffectsButton->hide();
			disconnect(parentParEffectsButton, SIGNAL(clicked()), this, SLOT(slotParentParEffects()));
		}
		else
		{
			parentParEffectsButton->show();
		}
		connect(parentParEffectsButton, SIGNAL(clicked()), this, SLOT(slotParentParEffects()));

		parEffectOffset->setValue(pstyle->parEffectOffset() * unitRatio, pstyle->isInhParEffectOffset());
		parEffectOffset->setParentValue(parent->parEffectOffset() * unitRatio);
		parEffectIndentBox->setChecked(pstyle->parEffectIndent(),pstyle->isInhParEffectIndent());
		parEffectIndentBox->setParentValue(parent->parEffectIndent());
		dropCapLines->setValue(pstyle->dropCapLines(), pstyle->isInhDropCapLines());
		dropCapLines->setParentValue(parent->dropCapLines());
		bulletStrEdit->setEditText(pstyle->bulletStr());
		setWidgetBoldFont(bulletCharLabel, !pstyle->isInhBulletStr());

		QString numName = pstyle->numName();
		if (numName.isEmpty())
			numName = "default";
		numComboBox->setCurrentItem(numComboBox->findText(numName), pstyle->isInhNumName());
		if (!parent->numName().isEmpty())
			numComboBox->setParentItem(numComboBox->findText(parent->numName()));
		else
			numComboBox->setParentItem(0);
		numFormatCombo->setCurrentFormat((NumFormat) pstyle->numFormat());
		numFormatCombo->setParentFormat((NumFormat) parent->numFormat());
		numLevelSpin->setValue(pstyle->numLevel() +1, pstyle->isInhNumLevel());
		NumStruct * numS = m_Doc->numerations.value(pstyle->numName());
		if (numS)
			numLevelSpin->setMaximum(numS->m_counters.count()+1);
		else
			numLevelSpin->setMaximum(1);
		numLevelSpin->setParentValue(parent->numLevel()+1);
		numPrefix->setText(pstyle->numPrefix());
		setWidgetBoldFont(numPrefixLabel, !pstyle->isInhNumPrefix());
		numSuffix->setText(pstyle->numSuffix());
		setWidgetBoldFont(numSuffixLabel, !pstyle->isInhNumSuffix());
		numStartSpin->setValue(pstyle->numStart(), pstyle->isInhNumStart());
		numStartSpin->setParentValue(parent->numStart());
		int numRestartIndex = numRestartCombo->findData(pstyle->numRestart());
		numRestartCombo->setCurrentItem(numRestartIndex, pstyle->isInhNumRestart());
		numRestartCombo->setParentItem(numRestartIndex);
		numRestartOtherBox->setChecked(pstyle->numOther(), pstyle->isInhNumOther());
		numRestartOtherBox->setParentValue(parent->numOther());
		numRestartHigherBox->setChecked(pstyle->numHigher(), pstyle->isInhNumHigher());
		numRestartHigherBox->setParentValue(parent->numHigher());
		backgroundColor->setColor(pstyle->backgroundColor(), qRound(pstyle->backgroundShade()), parent->backgroundColor(), qRound(pstyle->backgroundShade()), pstyle->isInhBackgroundColor());
	}
	else
	{
		lineSpacingMode->setCurrentItem(pstyle->lineSpacingMode());
		lineSpacing->setValue(pstyle->lineSpacing());
		spaceAbove->setValue(pstyle->gapBefore());
		spaceBelow->setValue(pstyle->gapAfter());
//		optMarginCombo->setCurrentItemByData( pstyle->opticalMargins() );
		optMarginWidget->setOpticalMargins(pstyle->opticalMargins());
		optMarginWidget->setOpticalMarginSetId(pstyle->opticalMarginSetId());
		minSpaceSpin->setValue(pstyle->minWordTracking() * 100.0);
		minGlyphExtSpin->setValue(pstyle->minGlyphExtension() * 100.0);
		maxGlyphExtSpin->setValue(pstyle->maxGlyphExtension() * 100.0);
		maxConsecutiveCountSpinBox->setValue(pstyle->hyphenConsecutiveLines());
		parEffectOffset->setValue(pstyle->parEffectOffset() * unitRatio);
		parEffectIndentBox->setChecked(pstyle->parEffectIndent());
		parentParEffectsButton->hide();
		disconnect(parentParEffectsButton, SIGNAL(clicked()), this, SLOT(slotParentParEffects()));

		dropCapLines->setValue(pstyle->dropCapLines());
		bulletStrEdit->setEditText(pstyle->bulletStr());
		setWidgetBoldFont(bulletCharLabel, false);
		QString numName = pstyle->numName();
		if (numName.isEmpty())
			numName = "default";
		numComboBox->setCurrentItem(numComboBox->findText(numName));
		numNewLineEdit->clear();
		numFormatCombo->setCurrentFormat((NumFormat) pstyle->numFormat());
		numLevelSpin->setValue(pstyle->numLevel()+1);
		NumStruct * numS = m_Doc->numerations.value(pstyle->numName());
		if (numS)
			numLevelSpin->setMaximum(numS->m_counters.count()+1);
		else
			numLevelSpin->setMaximum(1);
		numPrefix->setText(pstyle->numPrefix());
		setWidgetBoldFont(numPrefixLabel, false);
		numSuffix->setText(pstyle->numSuffix());
		setWidgetBoldFont(numSuffixLabel, false);
		numStartSpin->setValue(pstyle->numStart());
		int numRestartIndex = numRestartCombo->findData(pstyle->numRestart());
		numRestartCombo->setCurrentItem((numRestartIndex >= 0) ? numRestartIndex : 0);
		numRestartOtherBox->setChecked(pstyle->numOther());
		numRestartHigherBox->setChecked(pstyle->numHigher());

		alignment->setStyle(pstyle->alignment(), direction->getStyle());
		tabList->setTabs(pstyle->tabValues(), unitIndex);
		tabList->setLeftIndentValue(pstyle->leftMargin() * unitRatio);
		tabList->setFirstLineValue(pstyle->firstIndent() * unitRatio);
		tabList->setRightIndentValue(pstyle->rightMargin() * unitRatio);

		direction->setStyle(pstyle->direction());

		keepLinesStart->setValue (pstyle->keepLinesStart());
		keepLinesEnd->setValue (pstyle->keepLinesEnd());
		keepTogether->setChecked (pstyle->keepTogether());
		keepWithNext->setChecked (pstyle->keepWithNext());
		backgroundColor->setColor(pstyle->backgroundColor(), qRound(pstyle->backgroundShade()));

	}

	lineSpacing->setEnabled(pstyle->lineSpacingMode() == ParagraphStyle::FixedLineSpacing);

	if (pstyle->hasDropCap())
		setParagraphEffect(1);
	else if (pstyle->hasBullet())
		setParagraphEffect(2);
	else if (pstyle->hasNum())
		setParagraphEffect(3);
	else
		setParagraphEffect(0);

	parEffectCharStyleCombo->clear();
	parEffectCharStyleCombo->addItem(tr("No Style"));
	for (int i =0; i < cstyles.count(); i++)
		parEffectCharStyleCombo->addItem(cstyles.at(i).name());
	setCurrentComboItem(parEffectCharStyleCombo, pstyle->peCharStyleName().isEmpty() ? tr("No Style") : pstyle->peCharStyleName());

	cpage->parentLabel->setText( tr("Based On:"));
	cpage->show(&pstyle->charStyle(), cstyles, defLang, unitIndex);

	parentCombo->clear();
	parentCombo->addItem( pstyle->isDefaultStyle()? tr("A default style cannot be assigned a parent style") : "");
	if (!pstyle->isDefaultStyle())
	{
		QStringList styleNames;
		for (int i = 0; i < pstyles.count(); ++i)
		{
			if (pstyles[i].hasName() && pstyles[i].name() != pstyle->name())
				styleNames.append(pstyles[i].name());
		}
		styleNames.sort(Qt::CaseSensitive);
		parentCombo->addItems(styleNames);
	}

	if (pstyle->isDefaultStyle() || !m_hasParent)
		parentCombo->setCurrentIndex(0);
	else
	{
		int index = parentCombo->findText(parent->name());
		if (index < 0)
			index = 0;
		parentCombo->setCurrentIndex(index);
	}

}

void SMPStyleWidget::show(QList<ParagraphStyle*> &pstyles, QList<ParagraphStyle> &pstylesAll, QList<CharStyle> &cstyles, int unitIndex, const QString &defLang)
{
	if (pstyles.count() == 1)
		show(pstyles[0], pstylesAll, cstyles, unitIndex, defLang);
	else if (pstyles.count() > 1)
	{
		m_currPStyle = pstyles[0];
		showLineSpacing(pstyles);
		showSpaceAB(pstyles, unitIndex);
		showDropCap(pstyles, cstyles, unitIndex);
		showBullet(pstyles, cstyles, unitIndex);
		showNumeration(pstyles, cstyles, unitIndex);
		showAlignment(pstyles);
		showDirection(pstyles);
		showOpticalMargin(pstyles);
		showMinSpace(pstyles);
		showMinGlyphExt(pstyles);
		showMaxGlyphExt(pstyles);
		showConsecutiveLines(pstyles);
		showTabs(pstyles, unitIndex);
		showCStyle(pstyles, cstyles, defLang, unitIndex);
		showParent(pstyles);
		//checkParEffectState();
		setParagraphEffect(0);
		showColors(pstyles);
	}
}

void SMPStyleWidget::showColors(const QList<ParagraphStyle*> &cstyles)
{
	double d = -30000;
	for (int i = 0; i < cstyles.count(); ++i)
	{
		if (d != -30000 && cstyles[i]->backgroundShade() != d)
		{
			d = -30000;
			break;
		}
		d = cstyles[i]->backgroundShade();
	}

	QString s;
	for (int i = 0; i < cstyles.count(); ++i)
	{
		if (!s.isNull() && s != cstyles[i]->backgroundColor())
		{
			s.clear();
			break;
		}
		s = cstyles[i]->backgroundColor();
	}

	backgroundColor->setColor(s, qRound(d));
}

void SMPStyleWidget::showLineSpacing(const QList<ParagraphStyle*> &pstyles)
{
	lineSpacingMode->clear();
	lineSpacingMode->addItem( tr("Fixed"));
	lineSpacingMode->addItem( tr("Automatic"));
	lineSpacingMode->addItem( tr("Baseline"));

	int tmpLP = -1;
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (tmpLP != -1 && pstyles[i]->lineSpacingMode() != tmpLP)
		{
			tmpLP = -1;
			break;
		}
		tmpLP = pstyles[i]->lineSpacingMode();
	}

	if (tmpLP == -1)
	{
		if (lineSpacingMode->itemText(lineSpacingMode->count() - 1) != "")
			lineSpacingMode->addItem("");
		lineSpacingMode->setCurrentIndex(lineSpacingMode->count() - 1);
	}
	else
		lineSpacingMode->setCurrentIndex(tmpLP);

	double tmpLS = -1.0;
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (tmpLS > 0 && !isEqual(pstyles[i]->lineSpacing(), tmpLS))
		{
			tmpLS = -1.0;
			break;
		}
		tmpLS = pstyles[i]->lineSpacing();
	}
	lineSpacing->setEnabled(true);
	if (tmpLS < 0)
		lineSpacing->clear();
	else
		lineSpacing->setValue(tmpLS);
}

void SMPStyleWidget::showSpaceAB(const QList<ParagraphStyle*> &pstyles, int unitIndex)
{
// 	double unitRatio = unitGetRatioFromIndex(unitIndex);
	double tmpA = -1.2;
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (tmpA > -1.0 && !isEqual(pstyles[i]->gapBefore(), tmpA))
		{
			tmpA = -1.2;
			break;
		}
		tmpA = pstyles[i]->gapBefore();
	}

	if (tmpA < 0)
		spaceAbove->clear();
	else
		spaceAbove->setValue(tmpA);

	tmpA = -1.2;
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (tmpA > -1.0 && !isEqual(pstyles[i]->gapAfter(), tmpA))
		{
			tmpA = -1.2;
			break;
		}
	}

	if (tmpA < 0)
		spaceBelow->clear();
	else
		spaceBelow->setValue(tmpA);
}



void SMPStyleWidget::showAlignment(const QList<ParagraphStyle*> &pstyles)
{
	if (pstyles.isEmpty())
	{
		qDebug()<<"Warning showAlignment called with an empty list of styles";
		return;
	}
	ParagraphStyle::AlignmentType a = pstyles[0]->alignment();
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (a != pstyles[i]->alignment())
		{
			if (alignment->selectedId() > -1 && alignment->selectedId() < 5)
			{
				alignment->buttonGroup->setExclusive(false);
				alignment->buttonGroup->button(alignment->selectedId())->toggle();
				alignment->buttonGroup->setExclusive(true);
			}
			return;
		}
	}
	alignment->setStyle(a, direction->getStyle());
}

void SMPStyleWidget::showDirection(const QList<ParagraphStyle*> &pstyles)
{
	if (pstyles.isEmpty())
	{
		qDebug()<<"Warning showDirection called with an empty list of styles";
		return;
	}
	ParagraphStyle::DirectionType a = pstyles[0]->direction();
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (a != pstyles[i]->direction())
		{
			if (direction->selectedId() > -1 && direction->selectedId() < 2)
			{
				direction->buttonGroup->setExclusive(false);
				direction->buttonGroup->button(direction->selectedId())->toggle();
				direction->buttonGroup->setExclusive(true);
			}
			return;
		}
	}
	direction->setStyle(a);
}

void SMPStyleWidget::showOpticalMargin(const QList< ParagraphStyle * > & pstyles)
{
	if (pstyles.isEmpty())
	{
		qDebug()<<"Warning showOpticalMargin called with an empty list of styles";
		return;
	}
	
//	optMarginCombo->clear();
//	optMarginCombo->addItem(tr("None"), ParagraphStyle::OM_None);
//	optMarginCombo->addItem(tr("Left Protruding"), ParagraphStyle::OM_LeftProtruding);
//	optMarginCombo->addItem(tr("Right Protruding"), ParagraphStyle::OM_RightProtruding);
//	optMarginCombo->addItem(tr("Left Hanging Punctuation"), ParagraphStyle::OM_LeftHangingPunct);
//	optMarginCombo->addItem(tr("Right Hanging Punctuation"), ParagraphStyle::OM_RightHangingPunct);
//	optMarginCombo->addItem(tr("Default"), ParagraphStyle::OM_Default);
//
//	// the static cast should not be required if opticalMargins() would return OpticalMarginType. Why it does not? mystery
//	ParagraphStyle::OpticalMarginType o( static_cast<ParagraphStyle::OpticalMarginType>(pstyles[0]->opticalMargins()) );
//	for (int i = 0; i < pstyles.count(); ++i)
//	{
//		if (o != pstyles[i]->opticalMargins())
//		{
//			optMarginCombo->setCurrentItem(0);
//			return;
//		}
//	}
//	optMarginCombo->setCurrentItemByData(o);
	optMarginWidget->setOpticalMargins(pstyles[0]->opticalMargins());
	optMarginWidget->setOpticalMarginSets(m_Doc->opticalMarginSets());
	optMarginWidget->setOpticalMarginSetId(pstyles[0]->opticalMarginSetId());
}

void SMPStyleWidget::showMinSpace(const QList< ParagraphStyle * > & pstyles)
{
	if (pstyles.isEmpty())
	{
		qDebug()<<"Warning showMinSpace called with an empty list of styles";
		return;
	}
	
	double ms(pstyles[0]->minWordTracking());
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (ms != pstyles[i]->minWordTracking())
		{
			minSpaceSpin->setValue(100.0);
			return;
		}
	}
	minSpaceSpin->setValue(ms * 100.0);
}

void SMPStyleWidget::showMinGlyphExt(const QList< ParagraphStyle * > & pstyles)
{
	if (pstyles.isEmpty())
	{
		qDebug()<<"Warning showMinGlyphExt called with an empty list of styles";
		return;
	}
	
	double mge(pstyles[0]->minGlyphExtension());
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (mge != pstyles[i]->minGlyphExtension())
		{
			minGlyphExtSpin->setValue(100.0);
			return;
		}
	}
	minGlyphExtSpin->setValue(mge * 100.0);
}

void SMPStyleWidget::showMaxGlyphExt(const QList< ParagraphStyle * > & pstyles)
{
	if (pstyles.isEmpty())
	{
		qDebug()<<"Warning showMaxGlyphExt called with an empty list of styles";
		return;
	}
	
	double mge(pstyles[0]->maxGlyphExtension());
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (mge != pstyles[i]->maxGlyphExtension())
		{
			maxGlyphExtSpin->setValue(100.0);
			return;
		}
	}
	maxGlyphExtSpin->setValue(mge * 100.0);
}

void SMPStyleWidget::showConsecutiveLines(const QList<ParagraphStyle *> &pstyles)
{

	if (pstyles.isEmpty())
	{
		qDebug()<<"Warning showConsecutiveLines called with an empty list of styles";
		return;
	}

	double hyphenConsecutiveLines(pstyles[0]->hyphenConsecutiveLines());
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (hyphenConsecutiveLines != pstyles[i]->hyphenConsecutiveLines())
		{
			maxConsecutiveCountSpinBox->setValue(0);
			break;
		}
	}
	maxConsecutiveCountSpinBox->setValue(hyphenConsecutiveLines);

}


void SMPStyleWidget::showTabs(const QList<ParagraphStyle*> &pstyles, int unitIndex)
{
	double unitRatio = unitGetRatioFromIndex(unitIndex);
	QList<ParagraphStyle::TabRecord> t = pstyles[0]->tabValues();
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (t != pstyles[i]->tabValues())
		{
			t = QList<ParagraphStyle::TabRecord>();
			break;
		}
	}
	tabList->setTabs(t, unitIndex);

	double l = -4000.0;
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (l > -3800.0 && !isEqual(pstyles[i]->leftMargin(), l))
		{
			l = -4000.0;
			break;
		}
		l = pstyles[i]->leftMargin();
	}
	if (l < -3800.0)
	{
		tabList->setLeftIndentValue(0.0);
		tabList->leftIndentSpin->clear();
	}
	else
		tabList->setLeftIndentValue(l * unitRatio);

	l = -4000.0;
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (l > -3800.0 && !isEqual(pstyles[i]->firstIndent(), l))
		{
			l = -4000.0;
			break;
		}
		l = pstyles[i]->firstIndent();
	}
	if (l < -3800.0)
	{
		tabList->setFirstLineValue(0.0);
		tabList->firstLineSpin->clear();
	}
	else
		tabList->setFirstLineValue(l * unitRatio);

	l = -4000.0;
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (l > -3800.0 && !isEqual(pstyles[i]->rightMargin(), l))
		{
			l = -4000.0;
			break;
		}
		l = pstyles[i]->rightMargin();
	}
	if (l < -3800.0)
	{
		tabList->setRightIndentData(0.0);
		tabList->rightIndentSpin->clear();
	}
	else
		tabList->setRightIndentValue(l * unitRatio);

}

void SMPStyleWidget::showCStyle(const QList<ParagraphStyle*> &pstyles, const QList<CharStyle> &cstyles, const QString &defLang, int unitIndex)
{
	cpage->parentLabel->setText( tr("Based On:"));

	QList<CharStyle*> cstyle;
	for (int i = 0; i < pstyles.count(); ++i)
		cstyle << &pstyles[i]->charStyle();

	cpage->show(cstyle, cstyles, defLang, unitIndex);
}

void SMPStyleWidget::showParent(const QList<ParagraphStyle*> &pstyles)
{
	parentCombo->setEnabled(false);

// 	parentCombo->clear();
// 	parentCombo->insertItem("");
// 	for (uint i = 0; i < pstyles.count(); ++i)
// 	{
// 		if (pstyles[i].hasName() && pstyles[i].name() != pstyle->name())
// 			parentCombo->insertItem(pstyles[i].name());
// 	}
// 
// 	if (hasParent_)
// 	{
// 		int index = 0;
// 		for (int i = 0; i < parentCombo->count(); ++i)
// 		{
// 			if (parentCombo->text(i) == parent->name())
// 			{
// 				index = i;
// 				break;
// 			}
// 		}
// 		parentCombo->setCurrentItem(index);
// 	}
// 	else
// 		parentCombo->setCurrentItem(0);
}


void SMPStyleWidget::clearAll()
{

}

/*********************************************************************
 *						Paragraph Effects
 * *******************************************************************/

void SMPStyleWidget::insertSpecialChars(const QVector<uint> &charCodes)
{
	QString chars = QString::fromUcs4(charCodes.data(), charCodes.length());
	bulletStrEdit->lineEdit()->setText(chars);
}

void SMPStyleWidget::slotParentParEffects()
{
	parentParEffectsButton->hide();

	if (m_parentDC)
		setParagraphEffect(1);
	else if (m_parentBul)
		setParagraphEffect(2);
	else if (m_parentNum)
		setParagraphEffect(3);
	else
		setParagraphEffect(0);

	emit useParentParaEffects();

}

void SMPStyleWidget::openEnhanced()
{
	if (m_enhanced)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m_enhanced = new CharSelectEnhanced(this);
	m_enhanced->setModal(true);
	connect(m_enhanced, SIGNAL(insertSpecialChars(QVector<uint>)), this, SLOT(insertSpecialChars(QVector<uint>)));
	connect(m_enhanced, SIGNAL(paletteShown(bool)), bulletCharTableButton, SLOT(setChecked(bool)));
	m_enhanced->setDoc(m_Doc);
	m_enhanced->setEnabled(true);
	QString styleName = parEffectCharStyleCombo->currentText();
	if (styleName != tr("No Style") && !styleName.isEmpty())
	{
		CharStyle chStyle = m_cstyles->get(styleName);
		setCurrentComboItem(m_enhanced->fontSelector, chStyle.font().scName());
	}
	else if (m_currPStyle)
		setCurrentComboItem(m_enhanced->fontSelector, m_currPStyle->charStyle().font().scName());
	m_enhanced->newFont(m_enhanced->fontSelector->currentIndex());
	m_enhanced->show();
	QApplication::restoreOverrideCursor();
}

void SMPStyleWidget::closeEnhanced(bool show)
{
	if (!m_enhanced || show)
		return;
	disconnect(m_enhanced, SIGNAL(insertSpecialChars(const QVector<uint> &)), this, SLOT(insertSpecialChars(const QVector<uint> &)));
	disconnect(m_enhanced, SIGNAL(paletteShown(bool)), bulletCharTableButton, SLOT(setChecked(bool)));
	m_enhanced->close();
	m_enhanced->deleteLater();
	m_enhanced = nullptr;
}

void SMPStyleWidget::on_bulletCharTableButton_toggled(bool checked)
{
	if (m_enhanced && !checked)
		closeEnhanced();
	else if (!m_enhanced && checked)
		openEnhanced();
}

void SMPStyleWidget::handleUpdateRequest(int updateFlags)
{
	if (!m_Doc)
		return;
	if (updateFlags & reqNumUpdate)
		fillNumerationsCombo();
}

void SMPStyleWidget::fillBulletStrEditCombo()
{
	bulletStrEdit->clear();
	bulletStrEdit->addItem(QChar(0x2022));
	bulletStrEdit->addItem("*");
	bulletStrEdit->addItem(QChar(0x2013));
	bulletStrEdit->setMinimumWidth(50);
	if (bulletStrEdit->currentText().isEmpty())
		bulletStrEdit->setEditText(QChar(0x2022));
}

void SMPStyleWidget::fillNumerationsCombo()
{
	QStringList numNames;
	const auto numerationKeys = m_Doc->numerations.keys();
	for (const QString& numName : numerationKeys)
	{
		if (numName.isEmpty())
			continue;
		numNames.append(numName);
	}
	numNames.sort();
	numComboBox->clear();
	numComboBox->insertItems(0, numNames);
	numComboBox->setCurrentItem(0);
}

void SMPStyleWidget::fillNumRestartCombo()
{
	numRestartCombo->clear();
	numRestartCombo->addItem(tr("Document"), static_cast<int>(NSRdocument));
	numRestartCombo->addItem(tr("Story") , static_cast<int>(NSRstory));
}

void SMPStyleWidget::fillPECombo()
{
	QSignalBlocker sb(peCombo);
	int currIndex = peCombo->currentIndex();
	peCombo->clear();
	peCombo->addItem(tr("No Paragraph Effects"), -1);
	peCombo->addItem(tr("Drop Caps"), 0);
	peCombo->addItem(tr("Bulleted List"), 1);
	peCombo->addItem(tr("Numbered List"), 2);
	peCombo->setCurrentIndex(currIndex);
}

void SMPStyleWidget::setParagraphEffect(int index)
{
	QSignalBlocker sigPECombo(peCombo);

	peCombo->setCurrentIndex(index);
	int id = peCombo->currentIndex();

	if (id == 1 || id == 2 || id == 3)
	{
		stackedWidget->setCurrentIndex(id - 1);
		stackedWidget->setVisible(true);
		peGroup->setVisible(true);

		if (id == 2) // Bullet List
		{
			if (bulletStrEdit->currentText().isEmpty())
				bulletStrEdit->setEditText(bulletStrEdit->itemText(0));
		}
		else if (id == 3) // Numbered List
		{
			if (numComboBox->currentIndex() < 0)
				numComboBox->setCurrentIndex(0);
		}
	}
	else // No effect
	{
		stackedWidget->setVisible(false);
		peGroup->setVisible(false);
		peCombo->setCurrentIndex(0);
	}
}

void SMPStyleWidget::showDropCap(const QList<ParagraphStyle*> &pstyles, const QList<CharStyle> &cstyles, int unitIndex)
{
	bool dc = pstyles[0]->hasDropCap();
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (dc != pstyles[i]->hasDropCap())
		{
			dc = false;
			break;
		}
	}

	if (dc)
		setParagraphEffect(1);

	int lines = -1;
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (lines > -1 && pstyles[i]->dropCapLines() != lines)
		{
			lines = -1;
			break;
		}
		lines = pstyles[i]->dropCapLines();
	}
	if (lines == -1)
		dropCapLines->clear();
	else
		dropCapLines->setValue(lines);
}

void SMPStyleWidget::showBullet(const QList<ParagraphStyle *> &pstyles, const QList<CharStyle> &cstyles, int unitIndex)
{
//	double unitRatio = unitGetRatioFromIndex(unitIndex);

	bool hb = pstyles[0]->hasBullet();
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (hb != pstyles[i]->hasBullet())
		{
			hb = false;
			break;
		}
	}

	if (hb)
		setParagraphEffect(2);

	QString chStr = pstyles[0]->bulletStr();
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (chStr != pstyles[i]->bulletStr())
		{
			chStr.clear();
			break;
		}
		chStr = pstyles[i]->bulletStr();
	}
	bulletStrEdit->setEditText(chStr);
}

void SMPStyleWidget::showNumeration(const QList<ParagraphStyle *> &pstyles, const QList<CharStyle> &cstyles, int unitIndex)
{

	QString prefix = pstyles[0]->numPrefix();
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (prefix != pstyles[i]->numPrefix())
		{
			prefix.clear();
			break;
		}
		prefix = pstyles[i]->numPrefix();
	}
	numPrefix->setText(prefix);

	QString suffix = pstyles[0]->numSuffix();
	for (int i = 0; i < pstyles.count(); ++i)
	{
		if (suffix != pstyles[i]->numSuffix())
		{
			suffix.clear();
			break;
		}
		suffix = pstyles[i]->numSuffix();
	}
	numSuffix->setText(suffix);
}

void SMPStyleWidget::handleParEffectUse(int index)
{
	if (m_hasParent)
		parentParEffectsButton->show();

	int id = peCombo->currentIndex();
	double newMinimum = -3000.0;
	if (id == 2 || id == 3)
		newMinimum = 0.0;
	parEffectOffset->setMinimum(newMinimum);

	setParagraphEffect(index);
}
