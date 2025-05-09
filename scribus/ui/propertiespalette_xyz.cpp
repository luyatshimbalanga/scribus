/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "propertiespalette_xyz.h"

#include <QMessageBox>

#if defined(_MSC_VER) && !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include "appmodehelper.h"
#include "appmodes.h"
#include "basepointwidget.h"
#include "iconmanager.h"
#include "localemgr.h"
#include "pageitem.h"
#include "pageitem_arc.h"
#include "pageitem_line.h"
#include "pageitem_spiral.h"
#include "propertiespalette_utils.h"
#include "scraction.h"
#include "scribus.h"
#include "scribusapp.h"
#include "scribusdoc.h"
#include "scribusview.h"
#include "selection.h"
#include "undomanager.h"
#include "units.h"

//using namespace std;

PropertiesPalette_XYZ::PropertiesPalette_XYZ( QWidget* parent) : QWidget(parent)
{
	setupUi(this);
	setSizePolicy( QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

	userActionSniffer = new UserActionSniffer(this);
	connect(userActionSniffer, SIGNAL(actionStart()), this, SLOT(spinboxStartUserAction()));
	connect(userActionSniffer, SIGNAL(actionEnd()), this, SLOT(spinboxFinishUserAction()));

	installSniffer(xposSpin);
	installSniffer(yposSpin);
	installSniffer(widthSpin);
	installSniffer(heightSpin);

	keepFrameWHRatioButton->setCheckable( true );
	keepFrameWHRatioButton->setChecked(false);

	rotationSpin->setNewUnit(SC_DEG);
	rotationSpin->setWrapping( true );
	installSniffer(rotationSpin);
//	rotationLabel->setBuddy(rotationSpin);

	levelLabel->setAlignment( Qt::AlignCenter );

	flipH->setCheckable(true);
	flipV->setCheckable(true);
	
	doLock->setCheckable(true);
	noResize->setCheckable(true);

	buttonLineBasePoint->setVisible(false);

	iconSetChange();
	languageChange();

	connect(ScQApp, SIGNAL(iconSetChanged()), this, SLOT(iconSetChange()));
	connect(ScQApp, SIGNAL(localeChanged()), this, SLOT(localeChange()));
	connect(ScQApp, SIGNAL(labelVisibilityChanged(bool)), this, SLOT(toggleLabelVisibility(bool)));

	connect(xposSpin, SIGNAL(valueChanged(double)), this, SLOT(handleNewX()));
	connect(yposSpin, SIGNAL(valueChanged(double)), this, SLOT(handleNewY()));
	connect(widthSpin, SIGNAL(valueChanged(double)), this, SLOT(handleNewW()));
	connect(heightSpin, SIGNAL(valueChanged(double)), this, SLOT(handleNewH()));
	connect(rotationSpin, SIGNAL(valueChanged(double)), this, SLOT(handleRotation()));
	connect(rotateCCW, SIGNAL(clicked()), this, SLOT(handleRotateCCW()));
	connect(rotateCW, SIGNAL(clicked()), this, SLOT(handleRotateCW()));
	connect(flipH, SIGNAL(clicked()), this, SLOT(handleFlipH()));
	connect(flipV, SIGNAL(clicked()), this, SLOT(handleFlipV()));
	connect(levelUp, SIGNAL(clicked()), this, SLOT(handleRaise()));
	connect(levelDown, SIGNAL(clicked()), this, SLOT(handleLower()));
	connect(levelTop, SIGNAL(clicked()), this, SLOT(handleFront()));
	connect(levelBottom, SIGNAL(clicked()), this, SLOT(handleBack()));
	connect(basePointWidget, SIGNAL(buttonClicked(AnchorPoint)), this, SLOT(handleBasePoint(AnchorPoint)));
	connect(buttonLineBasePoint, SIGNAL(toggled(bool)), this, SLOT(handleLineMode()));

	connect(doLock   , SIGNAL(clicked()), this, SLOT(handleLock()));
	connect(noResize , SIGNAL(clicked()), this, SLOT(handleLockSize()));
	connect(doGroup  , SIGNAL(clicked()), this, SLOT(handleGrouping()) );
	connect(doUnGroup, SIGNAL(clicked()), this, SLOT(handleUngrouping()) );

	xposSpin->showValue(0);
	yposSpin->showValue(0);
	widthSpin->showValue(0);
	heightSpin->showValue(0);
	rotationSpin->showValue(0);
}

void PropertiesPalette_XYZ::setMainWindow(ScribusMainWindow* mw)
{
	m_ScMW = mw;

	connect(mw->appModeHelper, SIGNAL(AppModeChanged(int,int)), this, SLOT(handleAppModeChanged(int,int)));
}

void PropertiesPalette_XYZ::setDoc(ScribusDoc *d)
{
	if ((d == (ScribusDoc*) m_doc) || (m_ScMW && m_ScMW->scriptIsRunning()))
		return;

	if (m_doc)
	{
		disconnect(m_doc->m_Selection, SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
		disconnect(m_doc             , SIGNAL(docChanged())      , this, SLOT(handleSelectionChanged()));
	}

	m_doc  = d;
	m_item = nullptr;
	m_unitRatio   = m_doc->unitRatio();
	m_unitIndex   = m_doc->unitIndex();
	int precision = unitGetPrecisionFromIndex(m_unitIndex);
//qt4 FIXME here
	double maxXYWHVal =  16777215 * m_unitRatio;
	double minXYVal   = -16777215 * m_unitRatio;

	m_haveDoc = true;
	m_haveItem = false;

	QMap<QString, double>* docConstants = m_doc? &m_doc->constants()  : nullptr;
	xposSpin->setValues( minXYVal, maxXYWHVal, precision, minXYVal);
	xposSpin->setConstants(docConstants);
	yposSpin->setValues( minXYVal, maxXYWHVal, precision, minXYVal);
	yposSpin->setConstants(docConstants);
	widthSpin->setValues( m_unitRatio, maxXYWHVal, precision, m_unitRatio);
	widthSpin->setConstants(docConstants);
	heightSpin->setValues( m_unitRatio, maxXYWHVal, precision, m_unitRatio);
	heightSpin->setConstants(docConstants);

	rotationSpin->setValues( 0, 359.99, 1, 0);
	basePointWidget->setAngle(0);

	updateSpinBoxConstants();

	connect(m_doc->m_Selection, SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
	connect(m_doc             , SIGNAL(docChanged())      , this, SLOT(handleSelectionChanged()));
}

void PropertiesPalette_XYZ::unsetDoc()
{
	if (m_doc)
	{
		disconnect(m_doc->m_Selection, SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
		disconnect(m_doc             , SIGNAL(docChanged())      , this, SLOT(handleSelectionChanged()));
	}

	m_haveDoc  = false;
	m_haveItem = false;
	m_doc   = nullptr;
	m_item  = nullptr;
	xposSpin->setConstants(nullptr);
	yposSpin->setConstants(nullptr);
	widthSpin->setConstants(nullptr);
	heightSpin->setConstants(nullptr);
	doGroup->setEnabled(false);
	doUnGroup->setEnabled(false);
	flipH->setEnabled(false);
	flipV->setEnabled(false);
	xposLabel->setText( tr( "&X:" ) );
	yposLabel->setText( tr( "&Y:" ) );
	widthLabel->setText( tr( "&W:" ) );
	heightLabel->setText( tr( "&H:" ) );
	xposSpin->showValue(0);
	yposSpin->showValue(0);
	widthSpin->showValue(0);
	heightSpin->showValue(0);
	rotationSpin->showValue(0);
	basePointWidget->setAngle(0);
	setEnabled(false);
}

void PropertiesPalette_XYZ::unsetItem()
{
	m_haveItem = false;
	m_item     = nullptr;
	handleSelectionChanged();
}

void PropertiesPalette_XYZ::setLineMode(int lineMode)
{
	if (lineMode == 0)
	{
		xposLabel->setText( tr( "&X:" ) );
		yposLabel->setText( tr( "&Y:" ) );
		widthLabel->setText( tr( "&W:" ) );
		heightLabel->setText( tr( "&H:" ) );
		rotationSpin->setEnabled(true);
		heightSpin->setEnabled(false);
		heightSpin->setVisible(false);
		heightLabel->setVisible(false);
		m_lineMode = false;
		basePointWidget->setEnabled(true);
	}
	else
	{
		xposLabel->setText( tr( "&X1:" ) );
		yposLabel->setText( tr( "Y&1:" ) );
		widthLabel->setText( tr( "X&2:" ) );
		heightLabel->setText( tr( "&Y2:" ) );
		rotationSpin->setEnabled(false);
		heightSpin->setEnabled(true);
		heightSpin->setVisible(true);
		heightLabel->setVisible(true);
		m_lineMode = true;
		basePointWidget->setEnabled(false);
	}
}

PageItem* PropertiesPalette_XYZ::currentItemFromSelection()
{
	PageItem *currentItem = nullptr;

	if (m_doc)
	{
		if (m_doc->m_Selection->count() > 1)
		{
			currentItem = m_doc->m_Selection->itemAt(0);
		}
		else if (m_doc->m_Selection->count() == 1)
		{
			currentItem = m_doc->m_Selection->itemAt(0);
		}
	}

	return currentItem;
}

void PropertiesPalette_XYZ::setCurrentItem(PageItem *item)
{
	if (!m_ScMW || m_ScMW->scriptIsRunning())
		return;
	//CB We shouldn't really need to process this if our item is the same one
	//maybe we do if the item has been changed by scripter.. but that should probably
	//set some status if so.
	//FIXME: This won't work until when a canvas deselect happens, m_item must be nullptr.
	//if (m_item == i)
	//	return;

	if (!m_doc)
		setDoc(item->doc());

	m_haveItem = false;
	m_item = item;

	levelLabel->setText( QString::number(m_item->level()) );

//CB replaces old emits from PageItem::emitAllToGUI()
	disconnect(xposSpin, SIGNAL(valueChanged(double)), this, SLOT(handleNewX()));
	disconnect(yposSpin, SIGNAL(valueChanged(double)), this, SLOT(handleNewY()));
	disconnect(widthSpin, SIGNAL(valueChanged(double)), this, SLOT(handleNewW()));
	disconnect(heightSpin, SIGNAL(valueChanged(double)), this, SLOT(handleNewH()));
	disconnect(doLock, SIGNAL(clicked()), this, SLOT(handleLock()));
	disconnect(noResize, SIGNAL(clicked()), this, SLOT(handleLockSize()));
	disconnect(flipH, SIGNAL(clicked()), this, SLOT(handleFlipH()));
	disconnect(flipV, SIGNAL(clicked()), this, SLOT(handleFlipV()));
	disconnect(rotationSpin, SIGNAL(valueChanged(double)), this, SLOT(handleRotation()));

	double selX = m_item->xPos();
	double selY = m_item->yPos();
	double selW = m_item->width();
	double selH = m_item->height();
	if (m_doc->m_Selection->count() > 1)
		m_doc->m_Selection->getGroupRect(&selX, &selY, &selW, &selH);
//	showXY(selX, selY);
//	showWH(selW, selH);
	
	bool checkableFlip = (item->isImageFrame() || item->isTextFrame() || item->isLatexFrame() || item->isOSGFrame() || item->isSymbol() || item->isGroup() || item->isSpiral());
	flipH->setCheckable(checkableFlip);
	flipV->setCheckable(checkableFlip);

	showFlippedH(item->imageFlippedH());
	showFlippedV(item->imageFlippedV());

	double rr = item->rotation();
	if (item->rotation() > 0)
		rr = 360 - rr;
	m_oldRotation = fabs(rr);
	rotationSpin->setValue(fabs(rr));
	if (m_doc->m_Selection->count() == 1)
		basePointWidget->setAngle(fabs(rr));
	else
		basePointWidget->setAngle(0);

//CB TODO reconnect PP signals from here
	connect(xposSpin    , SIGNAL(valueChanged(double)), this, SLOT(handleNewX()), Qt::UniqueConnection);
	connect(yposSpin    , SIGNAL(valueChanged(double)), this, SLOT(handleNewY()), Qt::UniqueConnection);
	connect(widthSpin   , SIGNAL(valueChanged(double)), this, SLOT(handleNewW()), Qt::UniqueConnection);
	connect(heightSpin  , SIGNAL(valueChanged(double)), this, SLOT(handleNewH()), Qt::UniqueConnection);
	connect(doLock  , SIGNAL(clicked()), this, SLOT(handleLock()), Qt::UniqueConnection);
	connect(noResize, SIGNAL(clicked()), this, SLOT(handleLockSize()), Qt::UniqueConnection);
	connect(flipH   , SIGNAL(clicked()), this, SLOT(handleFlipH()), Qt::UniqueConnection);
	connect(flipV   , SIGNAL(clicked()), this, SLOT(handleFlipV()), Qt::UniqueConnection);
	connect(rotationSpin, SIGNAL(valueChanged(double)), this, SLOT(handleRotation()), Qt::UniqueConnection);

	bool setter = false;
	xposSpin->setEnabled(!setter);
	yposSpin->setEnabled(!setter);
	bool haveSameParent = m_doc->m_Selection->objectsHaveSameParent();
	labelLevel->setEnabled(haveSameParent && !item->locked());
	if ((m_item->isGroup()) && (!m_item->isSingleSel))
	{
		setEnabled(true);
	}
	if ((m_item->itemType() == PageItem::Line) && m_lineMode)
	{
		xposLabel->setText( tr( "&X1:" ) );
		yposLabel->setText( tr( "Y&1:" ) );
		widthLabel->setText( tr( "X&2:" ) );
		heightLabel->setText( tr( "&Y2:" ) );
		rotationSpin->setEnabled(false);
		basePointWidget->setEnabled(false);
	}
	else
	{
		xposLabel->setText( tr( "&X:" ) );
		yposLabel->setText( tr( "&Y:" ) );
		widthLabel->setText( tr( "&W:" ) );
		heightLabel->setText( tr( "&H:" ) );
		rotationSpin->setEnabled(true);
		basePointWidget->setEnabled(true);
	}
	m_haveItem = true;
	if (m_item->asLine())
	{
		heightSpin->setEnabled(m_lineMode && !m_item->locked());
		heightSpin->setVisible(m_lineMode);
		heightLabel->setVisible(m_lineMode);
		buttonLineBasePoint->setVisible(true);
		keepFrameWHRatioButton->setVisible(false);
	}
	else
	{
		heightSpin->setEnabled(true);
		heightSpin->setVisible(true);
		heightLabel->setVisible(true);
		buttonLineBasePoint->setVisible(false);
		keepFrameWHRatioButton->setVisible(true);
	}
	showXY(selX, selY);
	showWH(selW, selH);
	showLocked(item->locked());
	showSizeLocked(item->sizeLocked());
	double rrR = item->imageRotation();
	if (item->imageRotation() > 0)
		rrR = 360 - rrR;

	doGroup->setEnabled(false);
	doUnGroup->setEnabled(false);
	if ((m_doc->m_Selection->count() > 1) && haveSameParent)
		doGroup->setEnabled(true);
	if (m_doc->m_Selection->count() == 1)
		doUnGroup->setEnabled(m_item->isGroup());
	if ((m_doc->appMode == modeEditClip) && (m_item->isGroup()))
		doUnGroup->setEnabled(false);
	if (m_item->isOSGFrame())
	{
		setEnabled(true);
		rotationSpin->setEnabled(false);
	}
	if (m_item->asSymbol())
	{
		setEnabled(true);
	}
	updateSpinBoxConstants();
}

void PropertiesPalette_XYZ::handleAppModeChanged(int oldMode, int mode)
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;
	doUnGroup->setEnabled(mode != modeEdit && mode != modeEditClip && m_item->isGroup());
	doLock->setEnabled(mode != modeEditClip);
}

void PropertiesPalette_XYZ::handleSelectionChanged()
{
	if (!m_haveDoc || !m_ScMW || m_ScMW->scriptIsRunning())
		return;

	PageItem* currItem = currentItemFromSelection();
	if (m_doc->m_Selection->count() > 1)
	{
		m_oldRotation = 0.0;
		double gx, gy, gh, gw;
		m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);

		switch(basePointWidget->selectedAnchor()){
		case AnchorPoint::None:
		case AnchorPoint::TopLeft:
			m_ScMW->view->RCenter = FPoint(gx, gy);
			break;
		case AnchorPoint::Top:
			m_ScMW->view->RCenter = FPoint(gx + gw / 2.0, gy);
			break;
		case AnchorPoint::TopRight:
			m_ScMW->view->RCenter = FPoint(gx + gw, gy);
			break;
		case AnchorPoint::Left:
			m_ScMW->view->RCenter = FPoint(gx, gy + gh / 2.0);
			break;
		case AnchorPoint::Center:
			m_ScMW->view->RCenter = FPoint(gx + gw / 2.0, gy + gh / 2.0);
			break;
		case AnchorPoint::Right:
			m_ScMW->view->RCenter = FPoint(gx + gw, gy + gh / 2.0);
			break;
		case AnchorPoint::BottomLeft:
			m_ScMW->view->RCenter = FPoint(gx, gy + gh);
			break;
		case AnchorPoint::Bottom:
			m_ScMW->view->RCenter = FPoint(gx + gw / 2.0, gy + gh);
			break;
		case AnchorPoint::BottomRight:
			m_ScMW->view->RCenter = FPoint(gx + gw, gy + gh);
			break;
		}

		xposLabel->setText( tr( "&X:" ) );
		yposLabel->setText( tr( "&Y:" ) );
		widthLabel->setText( tr( "&W:" ) );
		heightLabel->setText( tr( "&H:" ) );

		xposSpin->showValue(gx);
		yposSpin->showValue(gy);
		widthSpin->showValue(gw);
		heightSpin->showValue(gh);
		rotationSpin->showValue(0);
		basePointWidget->setAngle(0);

		xposSpin->setEnabled(true);
		yposSpin->setEnabled(true);
		widthSpin->setEnabled(true);
		heightSpin->setEnabled(true);
		rotationSpin->setEnabled(true);

		flipH->setCheckable( false );
		flipV->setCheckable( false );
		flipH->setChecked(false);
		flipV->setChecked(false);

		flipH->setEnabled(true);
		flipV->setEnabled(true);

		keepFrameWHRatioButton->setVisible(true);
		buttonLineBasePoint->setVisible(false);

		setEnabled(true);
	}
	else
	{
		int itemType = currItem ? (int) currItem->itemType() : -1;
		
		m_haveItem = (itemType!=-1);
		if (itemType == -1)
		{
			doGroup->setEnabled(false);
			doUnGroup->setEnabled(false);
		}
		basePointWidget->setEnabled(true);
		basePointWidget->setMode(BasePointWidget::Full);
		keepFrameWHRatioButton->setVisible(true);
		buttonLineBasePoint->setVisible(false);

		setEnabled(true);

		//CB If Toggle is not possible, then we need to enable it so we can turn it off
		//It then gets reset below for items where its valid
		flipH->setCheckable(true);
		flipV->setCheckable(true);
		if ((itemType == 2) || (itemType == 4) || ((itemType >= 9) && (itemType <= 12)) || (itemType == 15))
		{
			flipH->setCheckable(true);
			flipV->setCheckable(true);
		}
		else
		{
			flipH->setCheckable(false);
			flipV->setCheckable(false);
			flipH->setChecked(false);
			flipV->setChecked(false);
		}
		
		//CB Why can't we do this for lines?
//		flipH->setEnabled((itemType !=-1) && (itemType != 5));
//		flipV->setEnabled((itemType !=-1) && (itemType != 5));
		flipH->setEnabled(itemType !=-1);
		flipV->setEnabled(itemType !=-1);
		switch (itemType)
		{
		case -1:
			xposLabel->setText( tr( "&X:" ) );
			yposLabel->setText( tr( "&Y:" ) );
			widthLabel->setText( tr( "&W:" ) );
			heightLabel->setText( tr( "&H:" ) );
			levelLabel->setText("  ");

			xposSpin->showValue(0);
			yposSpin->showValue(0);
			widthSpin->showValue(0);
			heightSpin->showValue(0);			
			rotationSpin->showValue(0);
			basePointWidget->setAngle(0);			

			heightSpin->setVisible(true);
			heightLabel->setVisible(true);

			setEnabled(false);
			break;
		case PageItem::ImageFrame:
		case PageItem::LatexFrame:
		case PageItem::OSGFrame:
			if (currItem->isOSGFrame())
			{
				setEnabled(true);
				rotationSpin->setEnabled(false);
			}
			break;
		case PageItem::Line:
			//basePointWidget->setEnabled(true);
			basePointWidget->setMode(BasePointWidget::Line);
			keepFrameWHRatioButton->setVisible(false);
			buttonLineBasePoint->setVisible(true);
			break;
		}
	}
	if (currItem)
	{
		setCurrentItem(currItem);
	}
	updateGeometry();
}

void PropertiesPalette_XYZ::unitChange()
{
	if (!m_haveDoc)
		return;
	bool tmp = m_haveItem;
	m_haveItem = false;
	m_unitRatio = m_doc->unitRatio();
	m_unitIndex = m_doc->unitIndex();
	xposSpin->setNewUnit( m_unitIndex );
	yposSpin->setNewUnit( m_unitIndex );
	widthSpin->setNewUnit( m_unitIndex );
	heightSpin->setNewUnit( m_unitIndex );
	m_haveItem = tmp;
}

void PropertiesPalette_XYZ::localeChange()
{
	const QLocale& l(LocaleManager::instance().userPreferredLocale());
	xposSpin->setLocale(l);
	yposSpin->setLocale(l);
	widthSpin->setLocale(l);
	heightSpin->setLocale(l);
	rotationSpin->setLocale(l);
}

void PropertiesPalette_XYZ::toggleLabelVisibility(bool visibility)
{
	labelRotation->setLabelVisibility(visibility);
	labelFlip->setLabelVisibility(visibility);
	labelLock->setLabelVisibility(visibility);
	labelGroup->setLabelVisibility(visibility);
	labelLevel->setLabelVisibility(visibility);
}

void PropertiesPalette_XYZ::showXY(double x, double y)
{
	if (!m_ScMW || m_ScMW->scriptIsRunning())
		return;
	bool sigBlocked1 = xposSpin->blockSignals(true);
	bool sigBlocked2 = yposSpin->blockSignals(true);
	bool useLineMode = false;
	bool tmp = m_haveItem;
	double b = 0.0, h = 0.0, r = 0.0;
	QTransform ma;
	FPoint n;
	if (m_haveItem)
	{
		if (m_doc->m_Selection->isMultipleSelection())
		{
			double dummy1, dummy2;
			m_doc->m_Selection->getGroupRect(&dummy1, &dummy2, &b, &h);
			ma.translate(dummy1, dummy2);
		}
		else
		{
			b = m_item->width();
			h = m_item->height();
			r = m_item->rotation();
			ma.translate(x, y);
			useLineMode = (m_lineMode && m_item->isLine());
		}
	}
	else
		ma.translate(x, y);
	m_haveItem = false;
	ma.rotate(r);
//	AnchorPoint bp = basePointWidget->selectedAnchor();
	// #8890 : basepoint is meaningless when lines use "end points" mode

	switch (basePointWidget->selectedAnchor())
	{
		case AnchorPoint::None:
		case AnchorPoint::TopLeft:
			n = FPoint(0.0, 0.0);
			break;
		case AnchorPoint::Top:
			n = FPoint(b / 2.0, 0.0);
			break;
		case AnchorPoint::TopRight:
			n = FPoint(b, 0.0);
			break;
		case AnchorPoint::Left:
			n = FPoint(0.0, h / 2.0);
			break;
		case AnchorPoint::Center:
			n = FPoint(b / 2.0, h / 2.0);
			break;
		case AnchorPoint::Right:
			n = FPoint(b, h / 2.0);
			break;
		case AnchorPoint::BottomLeft:
			n = FPoint(0.0, h);
			break;
		case AnchorPoint::Bottom:
			n = FPoint(b / 2.0, h);
			break;
		case AnchorPoint::BottomRight:
			n = FPoint(b, h);
			break;
	}

	if (useLineMode)
		n = FPoint(0.0, 0.0);

//	if (bp == AnchorPoint::TopLeft || useLineMode)
//		n = FPoint(0.0, 0.0);
//	else if (bp == AnchorPoint::TopRight)
//		n = FPoint(b, 0.0);
//	else if (bp == AnchorPoint::Center)
//		n = FPoint(b / 2.0, h / 2.0);
//	else if (bp == AnchorPoint::BottomLeft)
//		n = FPoint(0.0, h);
//	else if (bp == AnchorPoint::BottomRight)
//		n = FPoint(b, h);

	double inX = ma.m11() * n.x() + ma.m21() * n.y() + ma.dx();
	double inY = ma.m22() * n.y() + ma.m12() * n.x() + ma.dy();
	if (tmp)
	{
		inX -= m_doc->rulerXoffset;
		inY -= m_doc->rulerYoffset;
		if (m_doc->guidesPrefs().rulerMode)
		{
			inX -= m_doc->currentPage()->xOffset();
			inY -= m_doc->currentPage()->yOffset();
		}
	}
	xposSpin->setValue(inX * m_unitRatio);
	yposSpin->setValue(inY * m_unitRatio);
	if (useLineMode)
		showWH(m_item->width(), m_item->height());
	m_haveItem = tmp;
	xposSpin->blockSignals(sigBlocked1);
	yposSpin->blockSignals(sigBlocked2);
}

void PropertiesPalette_XYZ::showWH(double x, double y)
{
	if (!m_ScMW || m_ScMW->scriptIsRunning())
		return;
	bool sigBlocked1 = widthSpin->blockSignals(true);
	bool sigBlocked2 = heightSpin->blockSignals(true);
	if (m_lineMode && m_item->isLine())
	{
		QTransform ma;
		ma.translate(m_item->xPos(), m_item->yPos());
		ma.rotate(m_item->rotation());
		QPointF dp = QPointF(x, 0.0) * ma;
		dp -= QPointF(m_doc->rulerXoffset, m_doc->rulerYoffset);
		if (m_doc->guidesPrefs().rulerMode)
			dp -= QPointF(m_doc->currentPage()->xOffset(), m_doc->currentPage()->yOffset());
		widthSpin->setValue(dp.x() * m_unitRatio);
		heightSpin->setValue(dp.y() * m_unitRatio);
	}
	else
	{
		widthSpin->setValue(x * m_unitRatio);
		heightSpin->setValue(y * m_unitRatio);
	}
	widthSpin->blockSignals(sigBlocked1);
	heightSpin->blockSignals(sigBlocked2);
}

void PropertiesPalette_XYZ::showRotation(double r)
{
	if (!m_ScMW || m_ScMW->scriptIsRunning())
		return;
	double rr = r;
	if (r > 0)
		rr = 360 - rr;
	bool sigBlocked = rotationSpin->blockSignals(true);
	rotationSpin->setValue(fabs(rr));
	rotationSpin->blockSignals(sigBlocked);
}

void PropertiesPalette_XYZ::handleNewX()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;

	QTransform ma;
	double x = (xposSpin->value() / m_unitRatio) + m_doc->rulerXoffset;
	double base = 0.0;
	if (m_doc->guidesPrefs().rulerMode)
		x += m_doc->currentPage()->xOffset();
	if (m_doc->m_Selection->isMultipleSelection())
	{
		double gx, gy, gh, gw;
		m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);

		switch(basePointWidget->selectedAnchor()){
		case AnchorPoint::None:
		case AnchorPoint::TopLeft:
		case AnchorPoint::Left:
		case AnchorPoint::BottomLeft:
			base = gx;
			break;
		case AnchorPoint::Top:
		case AnchorPoint::Center:
		case AnchorPoint::Bottom:
			base = gx + gw / 2.0;
			break;
		case AnchorPoint::TopRight:
		case AnchorPoint::Right:
		case AnchorPoint::BottomRight:
			base = gx + gw;
			break;
		}

		if (!m_userActionOn)
			m_ScMW->view->startGroupTransaction();
		m_doc->moveGroup(x - base, 0);
		if (!m_userActionOn)
		{
			m_ScMW->view->endGroupTransaction();
		}
		m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);
		showXY(gx, gy);
	}
	else
	{
		if ((m_item->asLine()) && m_lineMode)
		{
			QPointF endPoint = m_item->asLine()->endPoint();
			double r = atan2(endPoint.y() - m_item->yPos(), endPoint.x() - x) * (180.0 / M_PI);
			double length = sqrt(pow(endPoint.x() - x, 2) + pow(endPoint.y() - m_item->yPos(), 2));
			m_item->setXYPos(x, m_item->yPos(), true);
			m_item->setRotation(r, true);
			m_doc->sizeItem(length, m_item->height(), m_item, true);
		}
		else
		{
			ma.translate(m_item->xPos(), m_item->yPos());
			ma.rotate(m_item->rotation());

			switch(basePointWidget->selectedAnchor()){
			case AnchorPoint::None:
			case AnchorPoint::TopLeft:
				base = m_item->xPos();
				break;
			case AnchorPoint::Top:
				base = ma.m11() * (m_item->width() / 2.0) + ma.m21() * 0.0 + ma.dx();
				break;
			case AnchorPoint::TopRight:
				base = ma.m11() * m_item->width() +			ma.m21() * 0.0 + ma.dx();
				break;
			case AnchorPoint::Left:
				base = ma.m11() * 0.0 +						ma.m21() * (m_item->height() / 2.0) + ma.dx();
				break;
			case AnchorPoint::Center:
				base = ma.m11() * (m_item->width() / 2.0) + ma.m21() * (m_item->height() / 2.0) + ma.dx();
				break;
			case AnchorPoint::Right:
				base = ma.m11() * m_item->width() +			ma.m21() * (m_item->height() / 2.0) + ma.dx();
				break;
			case AnchorPoint::BottomLeft:
				base = ma.m11() * 0.0 +						ma.m21() * m_item->height() + ma.dx();
				break;
			case AnchorPoint::Bottom:
				base = ma.m11() * (m_item->width() / 2.0) + ma.m21() * m_item->height() + ma.dx();
				break;
			case AnchorPoint::BottomRight:
				base = ma.m11() * m_item->width() +			ma.m21() * m_item->height() + ma.dx();
				break;
			}

			m_doc->moveItem(x - base, 0, m_item);
		}
	}
	m_doc->regionsChanged()->update(QRect());
	m_doc->changed();
	m_doc->changedPagePreview();
}

void PropertiesPalette_XYZ::handleNewY()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;

	QTransform ma;
	double y = (yposSpin->value() / m_unitRatio) + m_doc->rulerYoffset;
	double base = 0;
	if (m_doc->guidesPrefs().rulerMode)
		y += m_doc->currentPage()->yOffset();
	if (m_doc->m_Selection->isMultipleSelection())
	{
		double gx, gy, gh, gw;
		m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);

		switch(basePointWidget->selectedAnchor()){
		case AnchorPoint::None:
		case AnchorPoint::TopLeft:
		case AnchorPoint::Top:
		case AnchorPoint::TopRight:
			base = gy;
			break;
		case AnchorPoint::Left:
		case AnchorPoint::Center:
		case AnchorPoint::Right:
			base = gy + gh / 2.0;
			break;
		case AnchorPoint::BottomLeft:
		case AnchorPoint::Bottom:
		case AnchorPoint::BottomRight:
			base = gy + gh;
			break;
		}

		if (!m_userActionOn)
			m_ScMW->view->startGroupTransaction();
		m_doc->moveGroup(0, y - base);
		if (!m_userActionOn)
		{
			m_ScMW->view->endGroupTransaction();
		}
		m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);
		showXY(gx, gy);
	}
	else
	{
		if ((m_item->asLine()) && m_lineMode)
		{
			QPointF endPoint = m_item->asLine()->endPoint();
			double r = atan2(endPoint.y() - y, endPoint.x() - m_item->xPos()) * (180.0 / M_PI);
			double length = sqrt(pow(endPoint.x() - m_item->xPos(), 2) + pow(endPoint.y() - y, 2));
			m_item->setXYPos(m_item->xPos(), y, true);
			m_item->setRotation(r, true);
			m_doc->sizeItem(length, m_item->height(), m_item, true);
		}
		else
		{
			ma.translate(m_item->xPos(), m_item->yPos());
			ma.rotate(m_item->rotation());

			switch(basePointWidget->selectedAnchor()){
			case AnchorPoint::None:
			case AnchorPoint::TopLeft:
				base = m_item->yPos();
				break;
			case AnchorPoint::Top:
				base = ma.m22() * 0.0 +							ma.m12() * (m_item->width() / 2.0) + ma.dy();
				break;
			case AnchorPoint::TopRight:
				base = ma.m22() * 0.0 +							ma.m12() * m_item->width() + ma.dy();
				break;
			case AnchorPoint::Left:
				base = ma.m22() * (m_item->height() / 2.0) +	ma.m12() * 0.0 + ma.dy();
				break;
			case AnchorPoint::Center:
				base = ma.m22() * (m_item->height() / 2.0) +	ma.m12() * (m_item->width() / 2.0) + ma.dy();
				break;
			case AnchorPoint::Right:
				base = ma.m22() * (m_item->height() / 2.0) +	ma.m12() * m_item->width() + ma.dy();
				break;
			case AnchorPoint::BottomLeft:
				base = ma.m22() * m_item->height() +			ma.m12() * 0.0 + ma.dy();
				break;
			case AnchorPoint::Bottom:
				base = ma.m22() * m_item->height() +			ma.m12() * (m_item->width() / 2.0) + ma.dy();
				break;
			case AnchorPoint::BottomRight:
				base = ma.m22() * m_item->height() +			ma.m12() * m_item->width() + ma.dy();
				break;
			}

			m_doc->moveItem(0, y - base, m_item);
		}
	}
	m_doc->regionsChanged()->update(QRect());
	m_doc->changed();
	m_doc->changedPagePreview();
}

void PropertiesPalette_XYZ::handleNewW()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;
	
	double x = xposSpin->value() / m_unitRatio;
	double y = yposSpin->value() / m_unitRatio;
	double w = widthSpin->value() / m_unitRatio;
	double h = heightSpin->value() / m_unitRatio;
	double oldW = (m_item->width()  != 0.0) ? m_item->width()  : 1.0;
	double oldH = (m_item->height() != 0.0) ? m_item->height() : 1.0;
	if (m_doc->m_Selection->isMultipleSelection())
	{
		double gx, gy, gh, gw;
		if (!m_userActionOn)
			m_ScMW->view->startGroupTransaction();
		m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);
		if (keepFrameWHRatioButton->isChecked())
		{
			m_doc->scaleGroup(w / gw, w / gw, false);
			showWH(w, (w / gw) * gh);
		}
		else
		{
			m_doc->scaleGroup(w / gw, 1.0, false);
			m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);
			showWH(gw, gh);
		}
		if (!m_userActionOn)
		{
			m_ScMW->view->endGroupTransaction();
		}
	}
	else
	{
		bool oldS = m_item->Sizing;
		m_item->Sizing = false;
		m_item->OldB2 = m_item->width();
		m_item->OldH2 = m_item->height();
		if (m_item->asLine())
		{
			if (m_lineMode)
			{
				double r = atan2(h - y, w - x) * (180.0 / M_PI);
				m_item->setRotation(r, true);
				w = sqrt(pow(w - x, 2) + pow(h - y, 2));
			}
			m_doc->sizeItem(w, m_item->height(), m_item, true, true, false);
		}
		else
		{
			if (keepFrameWHRatioButton->isChecked())
			{
				showWH(w, (w / oldW) * m_item->height());
				m_doc->sizeItem(w, (w / oldW) * m_item->height(), m_item, true, true, false);
			}
			else
				m_doc->sizeItem(w, m_item->height(), m_item, true, true, false);
		}
		if (m_item->isArc())
		{
			double dw = w - oldW;
			double dh = h - oldH;
			PageItem_Arc* item = m_item->asArc();
			double dsch = item->arcHeight / oldH;
			double dscw = item->arcWidth / oldW;
			item->arcWidth += dw * dscw;
			item->arcHeight += dh * dsch;
			item->recalcPath();
		}
		if (m_item->isSpiral())
		{
			PageItem_Spiral* item = m_item->asSpiral();
			item->recalcPath();
		}
		m_item->Sizing = oldS;
	}
	m_doc->changed();
	m_doc->regionsChanged()->update(QRect());
	m_ScMW->setStatusBarTextSelectedItemInfo();
	m_doc->changedPagePreview();
}

void PropertiesPalette_XYZ::handleNewH()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;

	double x = xposSpin->value() / m_unitRatio;
	double y = yposSpin->value() / m_unitRatio;
	double w = widthSpin->value() / m_unitRatio;
	double h = heightSpin->value() / m_unitRatio;
	double oldW = (m_item->width()  != 0.0) ? m_item->width()  : 1.0;
	double oldH = (m_item->height() != 0.0) ? m_item->height() : 1.0;
	if (m_doc->m_Selection->isMultipleSelection())
	{
		double gx, gy, gh, gw;
		if (!m_userActionOn)
			m_ScMW->view->startGroupTransaction();
		m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);
		if (keepFrameWHRatioButton->isChecked())
		{
			m_doc->scaleGroup(h / gh, h / gh, false);
			showWH((h / gh) * gw, h);
		}
		else
		{
			m_doc->scaleGroup(1.0, h / gh, false);
			m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);
			showWH(gw, gh);
		}
		if (!m_userActionOn)
		{
			m_ScMW->view->endGroupTransaction();
		}
	}
	else
	{
		bool oldS = m_item->Sizing;
		m_item->Sizing = false;
		m_item->OldB2 = m_item->width();
		m_item->OldH2 = m_item->height();
		if (m_item->asLine())
		{
			if (m_lineMode)
			{
				double r = atan2(h - y, w - x) * (180.0 / M_PI);
				m_item->setRotation(r, true);
				w = sqrt(pow(w - x, 2) + pow(h - y, 2));
			}
			m_doc->sizeItem(w, m_item->height(), m_item, true, true, false);
		}
		else
		{
			if (keepFrameWHRatioButton->isChecked())
			{
				showWH((h / oldH) * m_item->width(), h);
				m_doc->sizeItem((h / oldH) * m_item->width(), h, m_item, true, true, false);
			}
			else
				m_doc->sizeItem(m_item->width(), h, m_item, true, true, false);
		}
		if (m_item->isArc())
		{
			double dw = w - oldW;
			double dh = h - oldH;
			PageItem_Arc* item = m_item->asArc();
			double dsch = item->arcHeight / oldH;
			double dscw = item->arcWidth / oldW;
			item->arcWidth += dw * dscw;
			item->arcHeight += dh * dsch;
			item->recalcPath();
		}
		if (m_item->isSpiral())
		{
			PageItem_Spiral* item = m_item->asSpiral();
			item->recalcPath();
		}
		m_item->Sizing = oldS;
	}
	m_doc->changed();
	m_doc->regionsChanged()->update(QRect());
	m_ScMW->setStatusBarTextSelectedItemInfo();
	m_doc->changedPagePreview();
}

void PropertiesPalette_XYZ::handleRotation()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;

	if (!m_userActionOn)
		m_ScMW->view->startGroupTransaction(Um::Rotate, "", Um::IRotate);
	if (m_doc->m_Selection->isMultipleSelection())
	{
		double gx, gy, gh, gw;
		m_doc->rotateGroup(m_oldRotation - rotationSpin->value(), m_ScMW->view->RCenter);
		m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);
		showXY(gx, gy);
	}
	else
		m_doc->rotateItem(-1.0 * rotationSpin->value(), m_item);
	if (!m_userActionOn)
	{
		for (int i = 0; i < m_doc->m_Selection->count(); ++i)
			m_doc->m_Selection->itemAt(i)->checkChanges(true);
		m_ScMW->view->endGroupTransaction();
	}
	m_doc->changed();
	m_doc->regionsChanged()->update(QRect());
	m_doc->changedPagePreview();
	m_oldRotation = rotationSpin->value();
	basePointWidget->setAngle(m_oldRotation);
}

void PropertiesPalette_XYZ::handleRotateCCW()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;

	if (!m_userActionOn)
		m_ScMW->view->startGroupTransaction(Um::Rotate, "", Um::IRotate);
	if (m_doc->m_Selection->isMultipleSelection())
	{
		double gx, gy, gh, gw;
		m_doc->rotateGroup(-90.0, m_ScMW->view->RCenter);
		m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);
		showXY(gx, gy);
	}
	else
	{
		double rr = m_item->rotation() - 90.0;
		m_doc->rotateItem(rr, m_item);
	}
	if (!m_userActionOn)
	{
		for (int i = 0; i < m_doc->m_Selection->count(); ++i)
			m_doc->m_Selection->itemAt(i)->checkChanges(true);
		m_ScMW->view->endGroupTransaction();
	}
	m_doc->changed();
	m_doc->regionsChanged()->update(QRect());
	m_doc->changedPagePreview();
	m_oldRotation = rotationSpin->value();
	basePointWidget->setAngle(m_oldRotation);
}

void PropertiesPalette_XYZ::handleRotateCW()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;

	if (!m_userActionOn)
		m_ScMW->view->startGroupTransaction(Um::Rotate, "", Um::IRotate);
	if (m_doc->m_Selection->isMultipleSelection())
	{
		double gx, gy, gh, gw;
		m_doc->rotateGroup(90.0, m_ScMW->view->RCenter);
		m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);
		showXY(gx, gy);
	}
	else
	{
		double rr = m_item->rotation() + 90.0;
		m_doc->rotateItem(rr, m_item);
	}
	if (!m_userActionOn)
	{
		for (int i = 0; i < m_doc->m_Selection->count(); ++i)
			m_doc->m_Selection->itemAt(i)->checkChanges(true);
		m_ScMW->view->endGroupTransaction();
	}
	m_doc->changed();
	m_doc->regionsChanged()->update(QRect());
	m_doc->changedPagePreview();
	m_oldRotation = rotationSpin->value();
	basePointWidget->setAngle(m_oldRotation);
}

void PropertiesPalette_XYZ::handleLower()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;
	m_doc->itemSelection_LowerItem();
	levelLabel->setText( QString::number(m_item->level()) );
}

void PropertiesPalette_XYZ::handleRaise()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;
	m_doc->itemSelection_RaiseItem();
	levelLabel->setText( QString::number(m_item->level()) );
}

void PropertiesPalette_XYZ::handleFront()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;
	m_doc->bringItemSelectionToFront();
	levelLabel->setText( QString::number(m_item->level()) );
}

void PropertiesPalette_XYZ::handleBack()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;
	m_doc->sendItemSelectionToBack();
	levelLabel->setText( QString::number(m_item->level()) );
}

void PropertiesPalette_XYZ::handleBasePoint(AnchorPoint m)
{
	if (!m_ScMW || m_ScMW->scriptIsRunning() || !m_item)
		return;

	double inX = 0;
	double inY = 0;
	if (m_haveDoc && m_haveItem)
	{
		m_haveItem = false;
		m_doc->setRotationMode(m);
		if (m_doc->m_Selection->isMultipleSelection())
		{
			double gx, gy, gh, gw;
			m_doc->m_Selection->getGroupRect(&gx, &gy, &gw, &gh);

			switch(m){
			case AnchorPoint::None:
			case AnchorPoint::TopLeft:
				inX = gx;
				inY = gy;
				break;
			case AnchorPoint::Top:
				inX = gx + gw / 2.0;
				inY = gy;
				break;
			case AnchorPoint::TopRight:
				inX = gx+gw;
				inY = gy;
				break;
			case AnchorPoint::Left:
				inX = gx;
				inY = gy + gh / 2.0;
				break;
			case AnchorPoint::Center:
				inX = gx + gw / 2.0;
				inY = gy + gh / 2.0;
				break;
			case AnchorPoint::Right:
				inX = gx + gw;
				inY = gy + gh / 2.0;
				break;
			case AnchorPoint::BottomLeft:
				inX = gx;
				inY = gy+gh;
				break;
			case AnchorPoint::Bottom:
				inX = gx+gw / 2.0;
				inY = gy+gh;
				break;
			case AnchorPoint::BottomRight:
				inX = gx+gw;
				inY = gy+gh;
				break;
			}

			m_ScMW->view->RCenter = FPoint(inX, inY);

			inX -= m_doc->rulerXoffset;
			inY -= m_doc->rulerYoffset;
			if (m_doc->guidesPrefs().rulerMode)
			{
				inX -= m_doc->currentPage()->xOffset();
				inY -= m_doc->currentPage()->yOffset();
			}
			xposSpin->setValue(inX*m_unitRatio);
			yposSpin->setValue(inY*m_unitRatio);
		}
		else
		{
			QTransform ma;
			FPoint n(0.0, 0.0);
			double b = m_item->width();
			double h = m_item->height();
			double r = m_item->rotation();
			ma.translate(m_item->xPos(), m_item->yPos());
			ma.rotate(r);

			switch(basePointWidget->selectedAnchor()){
			case AnchorPoint::None:
			case AnchorPoint::TopLeft:
				n = FPoint(0.0, 0.0);
				break;
			case AnchorPoint::Top:
				n = FPoint(b / 2.0, 0.0);
				break;
			case AnchorPoint::TopRight:
				n = FPoint(b, 0.0);
				break;
			case AnchorPoint::Left:
				n = FPoint(0.0, h / 2.0);
				break;
			case AnchorPoint::Center:
				n = FPoint(b / 2.0, h / 2.0);
				break;
			case AnchorPoint::Right:
				n = FPoint(b, h / 2.0);
				break;
			case AnchorPoint::BottomLeft:
				n = FPoint(0.0, h);
				break;
			case AnchorPoint::Bottom:
				n = FPoint(b / 2.0, h);
				break;
			case AnchorPoint::BottomRight:
				n = FPoint(b, h);
				break;
			}

			inX = ma.m11() * n.x() + ma.m21() * n.y() + ma.dx();
			inY = ma.m22() * n.y() + ma.m12() * n.x() + ma.dy();
			inX -= m_doc->rulerXoffset;
			inY -= m_doc->rulerYoffset;
			if (m_doc->guidesPrefs().rulerMode)
			{
				inX -= m_doc->currentPage()->xOffset();
				inY -= m_doc->currentPage()->yOffset();
			}
			xposSpin->setValue(inX*m_unitRatio);
			yposSpin->setValue(inY*m_unitRatio);
		}
		if (m_item->itemType() == PageItem::ImageFrame)
		{
			// FIXME
			if (false /*!FreeScale->isChecked()*/)
			{
				m_item->adjustPictScale();
				m_item->update();
			}
		}
		m_haveItem = true;
	}
}

void PropertiesPalette_XYZ::handleLock()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;
	m_ScMW->scrActions["itemLock"]->toggle();
}

void PropertiesPalette_XYZ::handleLockSize()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;
	m_ScMW->scrActions["itemLockSize"]->toggle();
}

void PropertiesPalette_XYZ::handleFlipH()
{
	if (!m_haveDoc || !m_haveItem || !m_ScMW || m_ScMW->scriptIsRunning())
		return;
	m_ScMW->scrActions["itemFlipH"]->toggle();
}

void PropertiesPalette_XYZ::handleFlipV()
{
	if (!m_ScMW || m_ScMW->scriptIsRunning())
		return;
	m_ScMW->scrActions["itemFlipV"]->toggle();
}

void PropertiesPalette_XYZ::installSniffer(ScrSpinBox *spinBox)
{
	const QList<QObject*>& list = spinBox->children();
	if (list.isEmpty())
		return;
	QListIterator<QObject*> it(list);
	QObject *obj;
	while (it.hasNext())
	{
		obj = it.next();
		obj->installEventFilter(userActionSniffer);
	}
}

bool PropertiesPalette_XYZ::userActionOn() const
{
	return m_userActionOn;
}

void PropertiesPalette_XYZ::spinboxStartUserAction()
{
	m_userActionOn = true;
}

void PropertiesPalette_XYZ::spinboxFinishUserAction()
{
	m_userActionOn = false;

	for (int i = 0; i < m_doc->m_Selection->count(); ++i)
		m_doc->m_Selection->itemAt(i)->checkChanges(true);
	if (m_ScMW->view->groupTransactionStarted())
	{
		m_ScMW->view->endGroupTransaction();
	}
}

void PropertiesPalette_XYZ::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
	{
		languageChange();
	}
	else
		QWidget::changeEvent(e);
}

void PropertiesPalette_XYZ::iconSetChange()
{
	IconManager& im = IconManager::instance();

	levelUp->setIcon(im.loadIcon("layer-move-up"));
	levelDown->setIcon(im.loadIcon("layer-move-down"));
	levelTop->setIcon(im.loadIcon("go-top"));
	levelBottom->setIcon(im.loadIcon("go-bottom"));

	doGroup->setIcon(im.loadIcon("group"));
	doUnGroup->setIcon(im.loadIcon("ungroup"));

	flipH->setIcon(im.loadIcon("flip-object-horizontal"));
	flipV->setIcon(im.loadIcon("flip-object-vertical"));

	rotateCCW->setIcon(im.loadIcon("rotate-ccw"));
	rotateCW->setIcon(im.loadIcon("rotate-cw"));

	buttonLineBasePoint->setIcon(im.loadIcon("toggle-object-coordination"));
	labelRotation->setPixmap(im.loadPixmap("object-rotation"));
	
	QIcon a;
	a.addPixmap(im.loadPixmap("lock"), QIcon::Normal, QIcon::On);
	a.addPixmap(im.loadPixmap("lock-unlocked"), QIcon::Normal, QIcon::Off);
	doLock->setIcon(a);

	QIcon a3;
	a3.addPixmap(im.loadPixmap("frame-no-resize"), QIcon::Normal, QIcon::On);
	a3.addPixmap(im.loadPixmap("frame-resize"), QIcon::Normal, QIcon::Off);
	noResize->setIcon(a3);
}

void PropertiesPalette_XYZ::languageChange()
{
	retranslateUi(this);

	QString suffix = m_haveDoc ? unitGetSuffixFromIndex(m_doc->unitIndex()) : tr(" pt");

	xposSpin->setSuffix(suffix);
	yposSpin->setSuffix(suffix);
	widthSpin->setSuffix(suffix);
	heightSpin->setSuffix(suffix);

	buttonLineBasePoint->setToolTip( tr("Change settings for left or end points"));
}

void PropertiesPalette_XYZ::updateSpinBoxConstants()
{
	if (!m_haveDoc)
		return;
	if (m_doc->m_Selection->isEmpty())
		return;
	widthSpin->setConstants(&m_doc->constants());
	heightSpin->setConstants(&m_doc->constants());
	xposSpin->setConstants(&m_doc->constants());
	yposSpin->setConstants(&m_doc->constants());

}

void PropertiesPalette_XYZ::showLocked(bool isLocked)
{
	xposSpin->setReadOnly(isLocked);
	yposSpin->setReadOnly(isLocked);
	widthSpin->setReadOnly(isLocked);
	heightSpin->setReadOnly(isLocked);
	rotationSpin->setReadOnly(isLocked);
	QPalette pal(QApplication::palette());
	if (isLocked)
		pal.setCurrentColorGroup(QPalette::Disabled);

	xposSpin->setPalette(pal);
	yposSpin->setPalette(pal);
	widthSpin->setPalette(pal);
	heightSpin->setPalette(pal);
	rotationSpin->setPalette(pal);

	doLock->setChecked(isLocked);
}

void PropertiesPalette_XYZ::showSizeLocked(bool isSizeLocked)
{
	bool b = isSizeLocked;
	if (m_haveItem && m_item->locked())
		b = true;
	widthSpin->setReadOnly(b);
	heightSpin->setReadOnly(b);
	QPalette pal(QApplication::palette());
	
	if (b)
		pal.setCurrentColorGroup(QPalette::Disabled);

	widthSpin->setPalette(pal);
	heightSpin->setPalette(pal);
	noResize->setChecked(isSizeLocked);
}

void PropertiesPalette_XYZ::showFlippedH(bool isFlippedH)
{
	flipH->setChecked(isFlippedH);
}

void PropertiesPalette_XYZ::showFlippedV(bool isFlippedV)
{
	flipV->setChecked(isFlippedV);
}

void PropertiesPalette_XYZ::handleGrouping()
{
	m_ScMW->GroupObj();
	doGroup->setEnabled(false);
	doUnGroup->setEnabled(true);
	handleSelectionChanged();
	//FIXME
	//TabStack->setItemEnabled(idShapeItem, false);
}

void PropertiesPalette_XYZ::handleUngrouping()
{
	m_ScMW->UnGroupObj();
	m_doc->invalidateAll();
	m_doc->regionsChanged()->update(QRect());
}

void PropertiesPalette_XYZ::handleLineMode()
{
	if (!m_ScMW || m_ScMW->scriptIsRunning())
		return;

	int mode = buttonLineBasePoint->isChecked() ? 1 : 0;

	setLineMode(mode);
	showWH(m_item->width(), m_item->height());
}
