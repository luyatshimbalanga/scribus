/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "effectsdialog.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QPixmap>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpacerItem>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QTime>
#include <QToolButton>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>

#include "cmsettings.h"
#include "colorcombo.h"
#include "curvewidget.h"
#include "iconmanager.h"
#include "scclocale.h"
#include "scpage.h"
#include "scribusdoc.h"
#include "scrspinbox.h"
#include "sctextstream.h"
#include "shadebutton.h"
#include "util.h"
#include "util_color.h"



EffectsDialog::EffectsDialog( QWidget* parent, PageItem* item, ScribusDoc* docc )
	: QDialog( parent ),
	  m_doc(docc),
	  m_item(item)
{
	setModal(true);
	setWindowTitle( tr( "Image Effects" ) );
	setWindowIcon(IconManager::instance().loadIcon("app-icon"));

	effectsList = m_item->effectsInUse;

//	CMSettings cms(docc, "", Intent_Perceptual);
//	cms.allowColorManagement(false);
	bool mode = false;
	CMSettings cms(m_doc, m_item->ImageProfile, m_item->ImageIntent);
	cms.setUseEmbeddedProfile(m_item->UseEmbedded);
	cms.allowSoftProofing(true);
	m_image.loadPicture(m_item->Pfile, m_item->pixm.imgInfo.actualPageNumber, cms, ScImage::RGBData, 72, &mode);
	int iw = m_image.width();
	int ih = m_image.height();
	if ((iw > 220) || (ih > 220))
	{
		double sx = iw / 220.0;
		double sy = ih / 220.0;
		if (sy < sx)
			m_image.createLowRes(sx);
		else
			m_image.createLowRes(sy);
		m_imageScale = qMin(sx, sy);
	}
	EffectsDialogLayout = new QVBoxLayout( this );
	EffectsDialogLayout->setContentsMargins(9, 9, 9, 9);
	EffectsDialogLayout->setSpacing(6);
	layoutGrid = new QGridLayout();
	layoutGrid->setContentsMargins(0, 0, 0, 0);
	layoutGrid->setSpacing(6);

	textLabel5 = new QLabel( this );
	textLabel5->setText( tr( "Preview" ) );
	pixmapLabel1 = new QLabel( this );
	pixmapLabel1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	pixmapLabel1->setMinimumSize( QSize( 220, 220 ) );
	pixmapLabel1->setMaximumSize( QSize( 220, 220 ) );
	pixmapLabel1->setFrameShape( QLabel::StyledPanel );
	pixmapLabel1->setFrameShadow( QLabel::Sunken );
	pixmapLabel1->setScaledContents( false );

	optionStack = new QStackedWidget( this );
//	optionStack->setSizePolicy( QSizePolicy( (QSizePolicy::SizeType)7, (QSizePolicy::SizeType)7, 0, 0, optionStack->sizePolicy().hasHeightForWidth() ) );
//	optionStack->setMinimumSize( QSize( 220, 280 ) );
	optionStack->setFrameShape( QFrame::NoFrame );
	WStackPage = new QWidget( optionStack );
	optionStack->addWidget( WStackPage );

	WStackPage_2 = new QWidget( optionStack );
	WStackPageLayout = new QVBoxLayout( WStackPage_2 );
	WStackPageLayout->setContentsMargins(0, 0, 0, 0);
	WStackPageLayout->setSpacing(6);
	WStackPageLayout->setAlignment( Qt::AlignTop );
	layout17 = new QHBoxLayout;
	layout17->setContentsMargins(0, 0, 0, 0);
	layout17->setSpacing(6);
	textLabel3 = new QLabel( tr( "Color:" ), WStackPage_2 );
	layout17->addWidget( textLabel3 );

	colData = new ColorCombo(false, WStackPage_2);
	colData->setPixmapType(ColorCombo::fancyPixmaps);
	colData->setColors(m_doc->PageColors, false);
	layout17->addWidget( colData );
	WStackPageLayout->addLayout( layout17 );

	layout19 = new QHBoxLayout;
	layout19->setContentsMargins(0, 0, 0, 0);
	layout19->setSpacing(6);
	textLabel4 = new QLabel( tr( "Shade:" ), WStackPage_2 );
	layout19->addWidget( textLabel4 );
	shade = new ShadeButton(WStackPage_2);
	shade->setValue(100);
	layout19->addWidget( shade );
	WStackPageLayout->addLayout( layout19 );
	optionStack->addWidget( WStackPage_2 );

	WStackPage_3 = new QWidget( optionStack );
	WStackPage3Layout = new QVBoxLayout( WStackPage_3 );
	WStackPage3Layout->setContentsMargins(0, 0, 0, 0);
	WStackPage3Layout->setSpacing(6);
	WStackPage3Layout->setAlignment( Qt::AlignTop );
	layout20 = new QHBoxLayout;
	layout20->setContentsMargins(0, 0, 0, 0);
	layout20->setSpacing(6);
	textLabel6 = new QLabel( tr( "Brightness:" ), WStackPage_3 );
	layout20->addWidget( textLabel6, Qt::AlignLeft );
	textLabel7 = new QLabel( "0", WStackPage_3 );
	layout20->addWidget( textLabel7, Qt::AlignRight );
	WStackPage3Layout->addLayout( layout20 );
	brightnessSlider = new QSlider( WStackPage_3 );
	brightnessSlider->setMinimum(-255);
	brightnessSlider->setMaximum(255);
	brightnessSlider->setValue(0);
	brightnessSlider->setOrientation( Qt::Horizontal );
	brightnessSlider->setTickPosition( QSlider::TicksBelow );
	WStackPage3Layout->addWidget( brightnessSlider );
	optionStack->addWidget( WStackPage_3 );

	WStackPage_4 = new QWidget( optionStack );
	WStackPage4Layout = new QVBoxLayout( WStackPage_4 );
	WStackPage4Layout->setContentsMargins(0, 0, 0, 0);
	WStackPage4Layout->setSpacing(6);
	WStackPage4Layout->setAlignment( Qt::AlignTop );
	layout21 = new QHBoxLayout;
	layout21->setContentsMargins(0, 0, 0, 0);
	layout21->setSpacing(6);
	textLabel8 = new QLabel( tr( "Contrast:" ), WStackPage_4 );
	layout21->addWidget( textLabel8, Qt::AlignLeft );
	textLabel9 = new QLabel( "0", WStackPage_4 );
	layout21->addWidget( textLabel9, Qt::AlignRight );
	WStackPage4Layout->addLayout( layout21 );
	contrastSlider = new QSlider( WStackPage_4 );
	contrastSlider->setMinimum(-127);
	contrastSlider->setMaximum(127);
	contrastSlider->setValue(0);
	contrastSlider->setOrientation( Qt::Horizontal );
	contrastSlider->setTickPosition( QSlider::TicksBelow );
	WStackPage4Layout->addWidget( contrastSlider );
	optionStack->addWidget( WStackPage_4 );

	WStackPage_5 = new QWidget( optionStack );
	WStackPage5Layout = new QVBoxLayout( WStackPage_5 );
	WStackPage5Layout->setContentsMargins(0, 0, 0, 0);
	WStackPage5Layout->setSpacing(6);
	WStackPage5Layout->setAlignment( Qt::AlignTop );
	layout22 = new QHBoxLayout;
	layout22->setContentsMargins(0, 0, 0, 0);
	layout22->setSpacing(6);
	textLabel10 = new QLabel( tr( "Radius:" ), WStackPage_5 );
	layout22->addWidget( textLabel10 );
	shRadius = new ScrSpinBox( 0.0, 10.0, WStackPage_5, 1 );
	shRadius->setDecimals(1);
	shRadius->setSuffix("");
	shRadius->setValue(0);
	layout22->addWidget( shRadius );
	WStackPage5Layout->addLayout( layout22 );
	layout23 = new QHBoxLayout;
	layout23->setContentsMargins(0, 0, 0, 0);
	layout23->setSpacing(6);
	textLabel11 = new QLabel( tr("Value:"), WStackPage_5 );
	layout23->addWidget( textLabel11 );
	shValue = new ScrSpinBox( 0.0, 5.0, WStackPage_5, 1 );
	shValue->setDecimals(1);
	shValue->setSuffix("");
	shValue->setValue(1.0);
	layout23->addWidget( shValue );
	WStackPage5Layout->addLayout( layout23 );
	optionStack->addWidget( WStackPage_5 );

	WStackPage_6 = new QWidget( optionStack );
	WStackPage6Layout = new QVBoxLayout( WStackPage_6 );
	WStackPage6Layout->setContentsMargins(0, 0, 0, 0);
	WStackPage6Layout->setSpacing(6);
	WStackPage6Layout->setAlignment( Qt::AlignTop );
	layout24 = new QHBoxLayout;
	layout24->setContentsMargins(0, 0, 0, 0);
	layout24->setSpacing(6);
	textLabel12 = new QLabel( tr( "Radius:" ), WStackPage_6 );
	layout24->addWidget( textLabel12 );
	blRadius = new ScrSpinBox( 0.0, 30.0, WStackPage_6, 1 );
	blRadius->setDecimals(1);
	blRadius->setSuffix("");
	blRadius->setValue(0);
	layout24->addWidget( blRadius );
	WStackPage6Layout->addLayout( layout24 );
	optionStack->addWidget( WStackPage_6 );

	WStackPage_7 = new QWidget( optionStack );
	WStackPage7Layout = new QVBoxLayout( WStackPage_7 );
	WStackPage7Layout->setContentsMargins(0, 0, 0, 0);
	WStackPage7Layout->setSpacing(6);
	WStackPage7Layout->setAlignment( Qt::AlignTop );
	layout26 = new QHBoxLayout;
	layout26->setContentsMargins(0, 0, 0, 0);
	layout26->setSpacing(6);
	textLabel14 = new QLabel( tr( "Posterize:" ), WStackPage_7 );
	layout26->addWidget( textLabel14, Qt::AlignLeft );
	textLabel15 = new QLabel( "0", WStackPage_7 );
	layout26->addWidget( textLabel15, Qt::AlignRight );
	WStackPage7Layout->addLayout( layout26 );
	solarizeSlider = new QSlider( WStackPage_7 );
	solarizeSlider->setMinimum(1);
	solarizeSlider->setMaximum(255);
	solarizeSlider->setValue(255);
	solarizeSlider->setOrientation( Qt::Horizontal );
	solarizeSlider->setTickPosition( QSlider::TicksBelow );
	WStackPage7Layout->addWidget( solarizeSlider );
	optionStack->addWidget( WStackPage_7 );

	WStackPage_8 = new QWidget( optionStack );
	WStackPage8Layout = new QGridLayout( WStackPage_8 );
	WStackPage8Layout->setContentsMargins(0, 0, 0, 0);
	WStackPage8Layout->setSpacing(6);
	WStackPage8Layout->setAlignment( Qt::AlignTop );
	textLabel1d = new QLabel( tr( "Color 1:" ), WStackPage_8 );
	WStackPage8Layout->addWidget( textLabel1d, 0, 0 );
	colData1 = new ColorCombo(false, WStackPage_8);
	colData1->setPixmapType(ColorCombo::fancyPixmaps);
	colData1->setColors(m_doc->PageColors, false);
	WStackPage8Layout->addWidget( colData1, 0, 1, 1, 2);
	shade1 = new ShadeButton(WStackPage_8);
	shade1->setValue(100);
	WStackPage8Layout->addWidget( shade1, 1, 1 );
	CurveD1 = new CurveWidget( nullptr );
	CurveD1Pop = new QMenu();
	CurveD1Act = new QWidgetAction(this);
	CurveD1Act->setDefaultWidget(CurveD1);
	CurveD1Pop->addAction(CurveD1Act);
	CurveD1Button = new QToolButton( WStackPage_8 );
	CurveD1Button->setText( "" );
	CurveD1Button->setMaximumSize( QSize( 22, 22 ) );
	CurveD1Button->setIcon(IconManager::instance().loadIcon("curve"));
	CurveD1Button->setMenu(CurveD1Pop);
	CurveD1Button->setPopupMode(QToolButton::InstantPopup);
	WStackPage8Layout->addWidget( CurveD1Button, 1, 2 );

	textLabel2d = new QLabel( tr( "Color 2:" ), WStackPage_8 );
	WStackPage8Layout->addWidget( textLabel2d, 2, 0 );
	colData2 = new ColorCombo(false, WStackPage_8);
	colData2->setPixmapType(ColorCombo::fancyPixmaps);
	colData2->setColors(m_doc->PageColors, false);
	WStackPage8Layout->addWidget( colData2, 2, 1, 1, 2);
	shade2 = new ShadeButton(WStackPage_8);
	shade2->setValue(100);
	WStackPage8Layout->addWidget( shade2, 3, 1 );
	CurveD2 = new CurveWidget( nullptr );
	CurveD2Pop = new QMenu();
	CurveD2Act = new QWidgetAction(this);
	CurveD2Act->setDefaultWidget(CurveD2);
	CurveD2Pop->addAction(CurveD2Act);
	CurveD2Button = new QToolButton( WStackPage_8 );
	CurveD2Button->setText( "" );
	CurveD2Button->setMaximumSize( QSize( 22, 22 ) );
	CurveD2Button->setIcon(IconManager::instance().loadIcon("curve"));
	CurveD2Button->setMenu(CurveD2Pop);
	CurveD2Button->setPopupMode(QToolButton::InstantPopup);
	WStackPage8Layout->addWidget( CurveD2Button, 3, 2 );
	auto *spacerD1 = new QSpacerItem( 1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding );
	WStackPage8Layout->addItem( spacerD1, 4, 0 );
	optionStack->addWidget( WStackPage_8 );

	WStackPage_9 = new QWidget( optionStack );
	WStackPage9Layout = new QGridLayout( WStackPage_9 );
	WStackPage9Layout->setContentsMargins(0, 0, 0, 0);
	WStackPage9Layout->setSpacing(6);
	WStackPage9Layout->setAlignment( Qt::AlignTop );
	textLabel1t = new QLabel( tr( "Color 1:" ), WStackPage_9 );
	WStackPage9Layout->addWidget( textLabel1t, 0, 0 );
	colDatat1 = new ColorCombo(false, WStackPage_9);
	colDatat1->setPixmapType(ColorCombo::fancyPixmaps);
	colDatat1->setColors(m_doc->PageColors, false);
	WStackPage9Layout->addWidget( colDatat1, 0, 1, 1, 2 );
	shadet1 = new ShadeButton(WStackPage_9);
	shadet1->setValue(100);
	WStackPage9Layout->addWidget( shadet1, 1, 1 );
	CurveT1 = new CurveWidget( nullptr );
	CurveT1Pop = new QMenu();
	CurveT1Act = new QWidgetAction(this);
	CurveT1Act->setDefaultWidget(CurveT1);
	CurveT1Pop->addAction(CurveT1Act);
	CurveT1Button = new QToolButton( WStackPage_9 );
	CurveT1Button->setText( "" );
	CurveT1Button->setMaximumSize( QSize( 22, 22 ) );
	CurveT1Button->setIcon(IconManager::instance().loadIcon("curve"));
	CurveT1Button->setMenu(CurveT1Pop);
	CurveT1Button->setPopupMode(QToolButton::InstantPopup);
	WStackPage9Layout->addWidget( CurveT1Button, 1, 2 );
	textLabel2t = new QLabel( tr( "Color 2:" ), WStackPage_9 );
	WStackPage9Layout->addWidget( textLabel2t, 2, 0 );
	colDatat2 = new ColorCombo(false, WStackPage_9);
	colDatat2->setPixmapType(ColorCombo::fancyPixmaps);
	colDatat2->setColors(m_doc->PageColors, false);
	WStackPage9Layout->addWidget( colDatat2, 2, 1, 1, 2 );
	shadet2 = new ShadeButton(WStackPage_9);
	shadet2->setValue(100);
	WStackPage9Layout->addWidget( shadet2, 3, 1 );
	CurveT2 = new CurveWidget( nullptr );
	CurveT2Pop = new QMenu();
	CurveT2Act = new QWidgetAction(this);
	CurveT2Act->setDefaultWidget(CurveT2);
	CurveT2Pop->addAction(CurveT2Act);
	CurveT2Button = new QToolButton( WStackPage_9 );
	CurveT2Button->setText( "" );
	CurveT2Button->setMaximumSize( QSize( 22, 22 ) );
	CurveT2Button->setIcon(IconManager::instance().loadIcon("curve"));
	CurveT2Button->setMenu(CurveT2Pop);
	CurveT2Button->setPopupMode(QToolButton::InstantPopup);
	WStackPage9Layout->addWidget( CurveT2Button, 3, 2 );
	textLabel3t = new QLabel( tr( "Color 3:" ), WStackPage_9 );
	WStackPage9Layout->addWidget( textLabel3t, 4, 0 );
	colDatat3 = new ColorCombo(false, WStackPage_9);
	colDatat3->setPixmapType(ColorCombo::fancyPixmaps);
	colDatat3->setColors(m_doc->PageColors, false);
	WStackPage9Layout->addWidget( colDatat3, 4, 1, 1, 2 );
	shadet3 = new ShadeButton(WStackPage_9);
	shadet3->setValue(100);
	WStackPage9Layout->addWidget( shadet3, 5, 1 );
	CurveT3 = new CurveWidget( nullptr );
	CurveT3Act = new QWidgetAction(this);
	CurveT3Pop = new QMenu();
	CurveT3Act->setDefaultWidget(CurveT3);
	CurveT3Pop->addAction(CurveT3Act);
	CurveT3Button = new QToolButton( WStackPage_9 );
	CurveT3Button->setText( "" );
	CurveT3Button->setMaximumSize( QSize( 22, 22 ) );
	CurveT3Button->setIcon(IconManager::instance().loadIcon("curve"));
	CurveT3Button->setMenu(CurveT3Pop);
	CurveT3Button->setPopupMode(QToolButton::InstantPopup);
	WStackPage9Layout->addWidget( CurveT3Button, 5, 2 );
	optionStack->addWidget( WStackPage_9 );

	WStackPage_10 = new QWidget( optionStack );
	WStackPage10Layout = new QGridLayout( WStackPage_10 );
	WStackPage10Layout->setContentsMargins(0, 0, 0, 0);
	WStackPage10Layout->setSpacing(6);
	textLabel1q = new QLabel( tr( "Color 1:" ), WStackPage_10 );
	WStackPage10Layout->addWidget( textLabel1q, 0, 0 );
	colDataq1 = new ColorCombo(false, WStackPage_10);
	colDataq1->setPixmapType(ColorCombo::fancyPixmaps);
	colDataq1->setColors(m_doc->PageColors, false);
	WStackPage10Layout->addWidget( colDataq1, 0, 1, 1, 2 );
	shadeq1 = new ShadeButton(WStackPage_10);
	shadeq1->setValue(100);
	WStackPage10Layout->addWidget( shadeq1, 1, 1 );
	CurveQ1 = new CurveWidget( nullptr );
	CurveQ1Pop = new QMenu();
	CurveQ1Act = new QWidgetAction(this);
	CurveQ1Act->setDefaultWidget(CurveQ1);
	CurveQ1Pop->addAction(CurveQ1Act);
	CurveQ1Button = new QToolButton( WStackPage_10 );
	CurveQ1Button->setText( "" );
	CurveQ1Button->setMaximumSize( QSize( 22, 22 ) );
	CurveQ1Button->setIcon(IconManager::instance().loadIcon("curve"));
	CurveQ1Button->setMenu(CurveQ1Pop);
	CurveQ1Button->setPopupMode(QToolButton::InstantPopup);
	WStackPage10Layout->addWidget( CurveQ1Button, 1, 2 );
	textLabel2q = new QLabel( tr( "Color 2:" ), WStackPage_10 );
	WStackPage10Layout->addWidget( textLabel2q, 2, 0 );
	colDataq2 = new ColorCombo(false, WStackPage_10);
	colDataq2->setPixmapType(ColorCombo::fancyPixmaps);
	colDataq2->setColors(m_doc->PageColors, false);
	WStackPage10Layout->addWidget( colDataq2, 2, 1, 1, 2 );
	shadeq2 = new ShadeButton(WStackPage_10);
	shadeq2->setValue(100);
	WStackPage10Layout->addWidget( shadeq2, 3, 1 );
	CurveQ2 = new CurveWidget( nullptr );
	CurveQ2Pop = new QMenu();
	CurveQ2Act = new QWidgetAction(this);
	CurveQ2Act->setDefaultWidget(CurveQ2);
	CurveQ2Pop->addAction(CurveQ2Act);
	CurveQ2Button = new QToolButton( WStackPage_10 );
	CurveQ2Button->setText( "" );
	CurveQ2Button->setMaximumSize( QSize( 22, 22 ) );
	CurveQ2Button->setIcon(IconManager::instance().loadIcon("curve"));
	CurveQ2Button->setMenu(CurveQ2Pop);
	CurveQ2Button->setPopupMode(QToolButton::InstantPopup);
	WStackPage10Layout->addWidget( CurveQ2Button, 3, 2 );
	textLabel3q = new QLabel( tr( "Color 3:" ), WStackPage_10 );
	WStackPage10Layout->addWidget( textLabel3q, 4, 0 );
	colDataqc3 = new ColorCombo(false, WStackPage_10);
	colDataqc3->setPixmapType(ColorCombo::fancyPixmaps);
	colDataqc3->setColors(m_doc->PageColors, false);
	WStackPage10Layout->addWidget( colDataqc3, 4, 1, 1, 2 );
	shadeqc3 = new ShadeButton(WStackPage_10);
	shadeqc3->setValue(100);
	WStackPage10Layout->addWidget( shadeqc3, 5, 1 );
	CurveQc3 = new CurveWidget( nullptr );
	CurveQc3Pop = new QMenu();
	CurveQc3Act = new QWidgetAction(this);
	CurveQc3Act->setDefaultWidget(CurveQc3);
	CurveQc3Pop->addAction(CurveQc3Act);
	CurveQc3Button = new QToolButton( WStackPage_10 );
	CurveQc3Button->setText( "" );
	CurveQc3Button->setMaximumSize( QSize( 22, 22 ) );
	CurveQc3Button->setIcon(IconManager::instance().loadIcon("curve"));
	CurveQc3Button->setMenu(CurveQc3Pop);
	CurveQc3Button->setPopupMode(QToolButton::InstantPopup);
	WStackPage10Layout->addWidget( CurveQc3Button, 5, 2 );
	textLabel4q = new QLabel( tr( "Color 4:" ), WStackPage_10 );
	WStackPage10Layout->addWidget( textLabel4q, 6, 0 );
	colDataq4 = new ColorCombo(false, WStackPage_10);
	colDataq4->setPixmapType(ColorCombo::fancyPixmaps);
	colDataq4->setColors(m_doc->PageColors, false);
	WStackPage10Layout->addWidget( colDataq4, 6, 1, 1, 2 );
	shadeq4 = new ShadeButton(WStackPage_10);
	shadeq4->setValue(100);
	WStackPage10Layout->addWidget( shadeq4, 7, 1 );
	CurveQ4 = new CurveWidget( nullptr );
	CurveQ4Pop = new QMenu();
	CurveQ4Act = new QWidgetAction(this);
	CurveQ4Act->setDefaultWidget(CurveQ4);
	CurveQ4Pop->addAction(CurveQ4Act);
	CurveQ4Button = new QToolButton( WStackPage_10 );
	CurveQ4Button->setText( "" );
	CurveQ4Button->setMaximumSize( QSize( 22, 22 ) );
	CurveQ4Button->setIcon(IconManager::instance().loadIcon("curve"));
	CurveQ4Button->setMenu(CurveQ4Pop);
	CurveQ4Button->setPopupMode(QToolButton::InstantPopup);
	WStackPage10Layout->addWidget( CurveQ4Button, 7, 2 );
	optionStack->addWidget( WStackPage_10 );

	WStackPage_11 = new QWidget( optionStack );
	WStackPage11Layout = new QVBoxLayout( WStackPage_11 );
	WStackPage11Layout->setContentsMargins(0, 0, 0, 0);
	WStackPage11Layout->setSpacing(6);
	WStackPage11Layout->setAlignment( Qt::AlignTop );
	Kdisplay = new CurveWidget(WStackPage_11);
	WStackPage11Layout->addWidget( Kdisplay );
	spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Expanding );
	WStackPage11Layout->addItem( spacer );
	optionStack->addWidget( WStackPage_11 );

	textLabel1 = new QLabel( this );
	textLabel1->setText( tr( "Available Effects" ) );
	availableEffects = new QListWidget( this );
	availableEffects->clear();
	availableEffects->addItem( tr("Blur"));
	availableEffects->addItem( tr("Brightness"));
	availableEffects->addItem( tr("Colorize"));
	availableEffects->addItem( tr("Duotone"));
	availableEffects->addItem( tr("Tritone"));
	availableEffects->addItem( tr("Quadtone"));
	availableEffects->addItem( tr("Contrast"));
	availableEffects->addItem( tr("Grayscale"));
	availableEffects->addItem( tr("Curves"));
	availableEffects->addItem( tr("Invert"));
	availableEffects->addItem( tr("Posterize"));
	availableEffects->addItem( tr("Sharpen"));

	QFontMetrics ftMetrics = this->fontMetrics();
	int availableEffectsAdvance = ftMetrics.horizontalAdvance( tr("Available Effects"));

	availableEffects->setMinimumSize(availableEffectsAdvance + 40, 180);
	toEffects = new QPushButton( this );
	toEffects->setText( tr( "Add" ) );
	toEffects->setEnabled(false);

	textLabel2 = new QLabel( this );
	textLabel2->setText( tr( "Applied Effects" ) );
	usedEffects = new QListWidget( this );
	usedEffects->setMinimumSize(availableEffectsAdvance + 40, 180);
	usedEffects->clear();
	m_effectValMap.clear();
	for (int i = 0; i < effectsList.count(); ++i)
	{
		if (effectsList.at(i).effectCode == ImageEffect::EF_INVERT)
		{
			usedEffects->addItem( tr("Invert"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), "");
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_GRAYSCALE)
		{
			usedEffects->addItem( tr("Grayscale"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), "");
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_COLORIZE)
		{
			usedEffects->addItem( tr("Colorize"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), effectsList.at(i).effectParameters);
			setItemSelectable(availableEffects, 2, false);
			setItemSelectable(availableEffects, 3, false);
			setItemSelectable(availableEffects, 4, false);
			setItemSelectable(availableEffects, 5, false);
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_BRIGHTNESS)
		{
			usedEffects->addItem( tr("Brightness"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), effectsList.at(i).effectParameters);
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_CONTRAST)
		{
			usedEffects->addItem( tr("Contrast"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), effectsList.at(i).effectParameters);
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_SHARPEN)
		{
			usedEffects->addItem( tr("Sharpen"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), effectsList.at(i).effectParameters);
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_BLUR)
		{
			usedEffects->addItem( tr("Blur"));
			QString tmpstr = effectsList.at(i).effectParameters;
			double radius;
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> radius; // has to be read from stream, as two numbers are stored in effectParameters
			blRadius->setValue(radius / m_imageScale);
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), QString("%1 1.0").arg(radius / m_imageScale));
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_SOLARIZE)
		{
			usedEffects->addItem( tr("Posterize"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), effectsList.at(i).effectParameters);
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_DUOTONE)
		{
			usedEffects->addItem( tr("Duotone"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), effectsList.at(i).effectParameters);
			setItemSelectable(availableEffects, 2, false);
			setItemSelectable(availableEffects, 3, false);
			setItemSelectable(availableEffects, 4, false);
			setItemSelectable(availableEffects, 5, false);
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_TRITONE)
		{
			usedEffects->addItem( tr("Tritone"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), effectsList.at(i).effectParameters);
			setItemSelectable(availableEffects, 2, false);
			setItemSelectable(availableEffects, 3, false);
			setItemSelectable(availableEffects, 4, false);
			setItemSelectable(availableEffects, 5, false);
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_QUADTONE)
		{
			usedEffects->addItem( tr("Quadtone"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), effectsList.at(i).effectParameters);
			setItemSelectable(availableEffects, 2, false);
			setItemSelectable(availableEffects, 3, false);
			setItemSelectable(availableEffects, 4, false);
			setItemSelectable(availableEffects, 5, false);
		}
		if (effectsList.at(i).effectCode == ImageEffect::EF_GRADUATE)
		{
			usedEffects->addItem( tr("Curves"));
			m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), effectsList.at(i).effectParameters);
		}
	}
	layout7 = new QHBoxLayout;
	layout7->setContentsMargins(0, 0, 0, 0);
	layout7->setSpacing(6);
	fromEffects = new QPushButton( this );
	fromEffects->setText( tr( "Remove" ) );
	fromEffects->setEnabled(false);
	layout7->addWidget( fromEffects );
	effectUp = new QPushButton( this );
	effectUp->setText( "" );
	effectUp->setIcon(IconManager::instance().loadIcon("go-up"));
	effectUp->setEnabled(false);
	layout7->addWidget( effectUp );
	effectDown = new QPushButton( this );
	effectDown->setText( "" );
	effectDown->setIcon(IconManager::instance().loadIcon("go-down"));
	effectDown->setEnabled(false);
	layout7->addWidget( effectDown );

	groupBox = new QGroupBox( this );
	groupBox->setTitle( tr( "Options:" ) );
	layout8 = new QVBoxLayout( groupBox );
	layout8->addWidget( optionStack );
	layout8->setAlignment( Qt::AlignTop );
	layout8->setSpacing(6);
	layout8->setContentsMargins(9, 9, 9, 9);

	layoutGrid->addWidget(textLabel1,       0, 0, 1, 1);
	layoutGrid->addWidget(textLabel2,       0, 1, 1, 1);
	layoutGrid->addWidget(textLabel5,       0, 2, 1, 1);
	layoutGrid->addWidget(availableEffects, 1, 0, 2, 1);
	layoutGrid->addWidget(usedEffects,      1, 1, 2, 1);
	layoutGrid->addWidget(pixmapLabel1,     1, 2, 1, 1);
	layoutGrid->addWidget(groupBox,         2, 2, 2, 1);
	layoutGrid->addWidget(toEffects,        3, 0, 1, 1);
	layoutGrid->addLayout(layout7,          3, 1, 1, 1);
	EffectsDialogLayout->addLayout( layoutGrid );

	layoutDialogButtonBox = new QHBoxLayout;
	layoutDialogButtonBox->setContentsMargins(0, 0, 0, 0);
	layoutDialogButtonBox->setSpacing(6);
	spacer3 = new QSpacerItem( 1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum );
	layoutDialogButtonBox->addItem( spacer3 );
	okButton = new QPushButton( this );
	okButton->setText( tr( "OK" ) );
	layoutDialogButtonBox->addWidget( okButton );
	cancelButton = new QPushButton( this );
	cancelButton->setText( tr( "Cancel" ) );
	layoutDialogButtonBox->addWidget( cancelButton );
	EffectsDialogLayout->addLayout( layoutDialogButtonBox );


	optionStack->setCurrentIndex(0);
	usedEffects->clearSelection();
	availableEffects->clearSelection();
	resize( minimumSizeHint() );
	ScImage im(m_image);
	saveValues(false);
	im.applyEffect(effectsList, m_doc->PageColors, false);
	QPixmap Bild = QPixmap(pixmapLabel1->width(), pixmapLabel1->height());
	int x = (pixmapLabel1->width() - im.qImage().width()) / 2;
	int y = (pixmapLabel1->height() - im.qImage().height()) / 2;
	QPainter p;
	QBrush b(QColor(205,205,205), IconManager::instance().loadPixmap("testfill"));
	p.begin(&Bild);
	p.fillRect(0, 0, pixmapLabel1->width(), pixmapLabel1->height(), b);
	p.drawImage(x, y, im.qImage());
	p.end();
	pixmapLabel1->setPixmap( Bild );

	// signals and slots connections
	connect( CurveD1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveD2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveQ1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveQ2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveQ4->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveQc3->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveT1->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveT2->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( CurveT3->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( Kdisplay->cDisplay, SIGNAL(modified()), this, SLOT(createPreview()));
	connect( availableEffects, SIGNAL( itemClicked(QListWidgetItem*) ), this, SLOT( selectAvailEffect(QListWidgetItem*) ) );
	connect( availableEffects, SIGNAL( itemDoubleClicked(QListWidgetItem*) ), this, SLOT( selectAvailEffectDbl(QListWidgetItem*) ) );
	connect( blRadius, SIGNAL(valueChanged(double)), this, SLOT(createPreview()));
	connect( brightnessSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
	connect( brightnessSlider, SIGNAL(valueChanged(int)), this, SLOT(updateBright(int)));
	connect( cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
	connect( colData, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( colData1, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( colData2, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( colDataq1, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( colDataq2, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( colDataq4, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( colDataqc3, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( colDatat1, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( colDatat2, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( colDatat3, SIGNAL(activated(int)), this, SLOT( createPreview()));
	connect( contrastSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
	connect( contrastSlider, SIGNAL(valueChanged(int)), this, SLOT(updateContrast(int)));
	connect( effectDown, SIGNAL( clicked() ), this, SLOT( moveEffectDown() ) );
	connect( effectUp, SIGNAL( clicked() ), this, SLOT( moveEffectUp() ) );
	connect( fromEffects, SIGNAL( clicked() ), this, SLOT( moveFromEffects() ) );
	connect( okButton, SIGNAL( clicked() ), this, SLOT( leaveOK() ) );
	connect( shRadius, SIGNAL(valueChanged(double)), this, SLOT(createPreview()));
	connect( shValue, SIGNAL(valueChanged(double)), this, SLOT(createPreview()));
	connect( shade, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( shade1, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( shade2, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( shadeq1, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( shadeq2, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( shadeq4, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( shadeqc3, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( shadet1, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( shadet2, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( shadet3, SIGNAL(clicked()), this, SLOT(createPreview()));
	connect( solarizeSlider, SIGNAL(sliderReleased()), this, SLOT(createPreview()));
	connect( solarizeSlider, SIGNAL(valueChanged(int)), this, SLOT(updateSolarize(int)));
	connect( toEffects, SIGNAL( clicked() ), this, SLOT( moveToEffects() ) );
	connect( usedEffects, SIGNAL( itemClicked(QListWidgetItem*) ), this, SLOT( selectEffect(QListWidgetItem*) ) );
	connect( usedEffects, SIGNAL( itemDoubleClicked(QListWidgetItem*) ), this, SLOT( moveFromEffects() ) );
	m_time.start();
}

void EffectsDialog::setItemSelectable(QListWidget* widget, int itemNr, bool enable)
{
	if (enable)
		widget->item(itemNr)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	else
		widget->item(itemNr)->setFlags(Qt::NoItemFlags);
}

void EffectsDialog::leaveOK()
{
	saveValues(true);
	accept();
}

void EffectsDialog::updateSolarize(int val)
{
	QString tmp;
	tmp.setNum(val);
	textLabel15->setText(tmp);
	createPreview();
}

void EffectsDialog::updateContrast(int val)
{
	QString tmp;
	tmp.setNum(val);
	textLabel9->setText(tmp);
	createPreview();
}

void EffectsDialog::updateBright(int val)
{
	QString tmp;
	tmp.setNum(val);
	textLabel7->setText(tmp);
	createPreview();
}

void EffectsDialog::createPreview()
{
	if (m_time.elapsed() < 50)
		return;
	ScImage im(m_image);
	saveValues(false);
	im.applyEffect(effectsList, m_doc->PageColors, false);
	QPixmap Bild = QPixmap(pixmapLabel1->width(), pixmapLabel1->height());
	int x = (pixmapLabel1->width() - im.qImage().width()) / 2;
	int y = (pixmapLabel1->height() - im.qImage().height()) / 2;
	QPainter p;
	QBrush b(QColor(205,205,205), IconManager::instance().loadPixmap("testfill"));
	p.begin(&Bild);
	p.fillRect(0, 0, pixmapLabel1->width(), pixmapLabel1->height(), b);
	p.drawImage(x, y, im.qImage());
	p.end();
	pixmapLabel1->setPixmap( Bild );
	m_time.start();
}

void EffectsDialog::saveValues(bool finalValues)
{
	selectEffectHelper(finalValues);
	effectsList.clear();
	struct ImageEffect ef;
	for (int e = 0; e < usedEffects->count(); ++e)
	{
		if (usedEffects->item(e)->text() == tr("Invert"))
		{
			ef.effectCode = ImageEffect::EF_INVERT;
			ef.effectParameters = "";
		}
		if (usedEffects->item(e)->text() == tr("Grayscale"))
		{
			ef.effectCode = ImageEffect::EF_GRAYSCALE;
			ef.effectParameters = "";
		}
		if (usedEffects->item(e)->text() == tr("Colorize"))
		{
			ef.effectCode = ImageEffect::EF_COLORIZE;
			ef.effectParameters = m_effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Brightness"))
		{
			ef.effectCode = ImageEffect::EF_BRIGHTNESS;
			ef.effectParameters = m_effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Contrast"))
		{
			ef.effectCode = ImageEffect::EF_CONTRAST;
			ef.effectParameters = m_effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Sharpen"))
		{
			ef.effectCode = ImageEffect::EF_SHARPEN;
			ef.effectParameters = m_effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Blur"))
		{
			ef.effectCode = ImageEffect::EF_BLUR;
			if (finalValues)
				ef.effectParameters = QString("%1 1.0").arg(blRadius->value() * m_imageScale);
			else
				ef.effectParameters = QString("%1 1.0").arg(blRadius->value());
		}
		if (usedEffects->item(e)->text() == tr("Posterize"))
		{
			ef.effectCode = ImageEffect::EF_SOLARIZE;
			ef.effectParameters = m_effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Duotone"))
		{
			ef.effectCode = ImageEffect::EF_DUOTONE;
			ef.effectParameters = m_effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Tritone"))
		{
			ef.effectCode = ImageEffect::EF_TRITONE;
			ef.effectParameters = m_effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Quadtone"))
		{
			ef.effectCode = ImageEffect::EF_QUADTONE;
			ef.effectParameters = m_effectValMap[usedEffects->item(e)];
		}
		if (usedEffects->item(e)->text() == tr("Curves"))
		{
			ef.effectCode = ImageEffect::EF_GRADUATE;
			ef.effectParameters = m_effectValMap[usedEffects->item(e)];
		}
		effectsList.append(ef);
	}
}

void EffectsDialog::selectAvailEffectDbl(QListWidgetItem* c)
{
	if (!c)
		return;
	if (!(c->flags() & Qt::ItemIsSelectable))
		return;
	moveToEffects();
}

void EffectsDialog::moveToEffects()
{
	QSignalBlocker blocker(usedEffects);
	usedEffects->addItem(availableEffects->currentItem()->text());
	if (availableEffects->currentItem()->text() == tr("Invert"))
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), "");
	if (availableEffects->currentItem()->text() == tr("Grayscale"))
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), "");
	if (availableEffects->currentItem()->text() == tr("Brightness"))
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), "0");
	if (availableEffects->currentItem()->text() == tr("Contrast"))
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), "0");
	if (availableEffects->currentItem()->text() == tr("Sharpen"))
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), "0 1");
	if (availableEffects->currentItem()->text() == tr("Blur"))
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), "0 1");
	if (availableEffects->currentItem()->text() == tr("Posterize"))
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), "255");
	if (availableEffects->currentItem()->text() == tr("Colorize"))
	{
		ColorList::Iterator it;
		it = m_doc->PageColors.begin();
		QString efval = it.key()+"\n100";
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), efval);
		setItemSelectable(availableEffects, 2, false);
		setItemSelectable(availableEffects, 3, false);
		setItemSelectable(availableEffects, 4, false);
		setItemSelectable(availableEffects, 5, false);
	}
	if (availableEffects->currentItem()->text() == tr("Duotone"))
	{
		ColorList::Iterator it;
		it = m_doc->PageColors.begin();
		QString efval = it.key()+"\n"+it.key()+"\n100 100 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0";
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), efval);
		setItemSelectable(availableEffects, 2, false);
		setItemSelectable(availableEffects, 3, false);
		setItemSelectable(availableEffects, 4, false);
		setItemSelectable(availableEffects, 5, false);
	}
	if (availableEffects->currentItem()->text() == tr("Tritone"))
	{
		ColorList::Iterator it;
		it = m_doc->PageColors.begin();
		QString efval = it.key()+"\n"+it.key()+"\n"+it.key()+"\n100 100 100 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0";
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), efval);
		setItemSelectable(availableEffects, 2, false);
		setItemSelectable(availableEffects, 3, false);
		setItemSelectable(availableEffects, 4, false);
		setItemSelectable(availableEffects, 5, false);
	}
	if (availableEffects->currentItem()->text() == tr("Quadtone"))
	{
		ColorList::Iterator it;
		it = m_doc->PageColors.begin();
		QString efval = it.key()+"\n"+it.key()+"\n"+it.key()+"\n"+it.key()+"\n100 100 100 100 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0 2 0.0 0.0 1.0 1.0 0";
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), efval);
		setItemSelectable(availableEffects, 2, false);
		setItemSelectable(availableEffects, 3, false);
		setItemSelectable(availableEffects, 4, false);
		setItemSelectable(availableEffects, 5, false);
	}
	if (availableEffects->currentItem()->text() == tr("Curves"))
		m_effectValMap.insert(usedEffects->item(usedEffects->count()-1), "2 0.0 0.0 1.0 1.0 0");
	usedEffects->setCurrentItem(usedEffects->item(usedEffects->count()-1));
	selectEffect(usedEffects->item(usedEffects->count()-1));
	createPreview();
}

void EffectsDialog::moveFromEffects()
{
	QSignalBlocker blocker(usedEffects);
	if ((usedEffects->currentItem()->text() == tr("Colorize")) || (usedEffects->currentItem()->text() == tr("Duotone")) || (usedEffects->currentItem()->text() == tr("Tritone")) || (usedEffects->currentItem()->text() == tr("Quadtone")))
	{
		setItemSelectable(availableEffects, 2, true);
		setItemSelectable(availableEffects, 3, true);
		setItemSelectable(availableEffects, 4, true);
		setItemSelectable(availableEffects, 5, true);
	}
	m_effectValMap.remove(usedEffects->currentItem());
	int curr = usedEffects->currentRow();
	QListWidgetItem *it = usedEffects->takeItem(curr);
	delete it;
	currentOptions = nullptr;
	usedEffects->clearSelection();
	if (usedEffects->count() == 0)
	{
		fromEffects->setEnabled(false);
		toEffects->setEnabled(false);
		selectEffectHelper();
		optionStack->setCurrentIndex(0);
		QSignalBlocker blocker2(availableEffects);
		availableEffects->clearSelection();
	}
	else
	{
		usedEffects->setCurrentItem(usedEffects->item(qMax(curr-1, 0)));
		usedEffects->currentItem()->setSelected(true);
		selectEffect(usedEffects->currentItem());
	}
	if (usedEffects->count() < 2)
	{
		effectUp->setEnabled(false);
		effectDown->setEnabled(false);
	}
	createPreview();
}

void EffectsDialog::moveEffectUp()
{
	int curr = usedEffects->currentRow();
	if (curr == 0)
		return;
	QSignalBlocker blocker(usedEffects);
	QListWidgetItem *it = usedEffects->takeItem(curr);
	usedEffects->insertItem(curr-1, it);
	usedEffects->setCurrentItem(it);
	selectEffect(usedEffects->currentItem());
	createPreview();
}

void EffectsDialog::moveEffectDown()
{
	int curr = usedEffects->currentRow();
	if (curr == static_cast<int>(usedEffects->count())-1)
		return;
	QSignalBlocker blocker(usedEffects);
	QListWidgetItem *it = usedEffects->takeItem(curr);
	usedEffects->insertItem(curr+1, it);
	usedEffects->setCurrentItem(it);
	selectEffect(usedEffects->currentItem());
	createPreview();
}

void EffectsDialog::selectEffect(QListWidgetItem* c)
{
	QString s;
	toEffects->setEnabled(false);
	selectEffectHelper();
	if (c)
	{
		fromEffects->setEnabled(true);
		if (usedEffects->count() > 1)
		{
			effectUp->setEnabled(true);
			effectDown->setEnabled(true);
			if (usedEffects->currentItem() == nullptr)
				effectUp->setEnabled(false);
			if (usedEffects->currentRow() == static_cast<int>(usedEffects->count())-1)
				effectDown->setEnabled(false);
		}
		if (c->text() == tr("Grayscale"))
			optionStack->setCurrentIndex(0);
		else if (c->text() == tr("Invert"))
			optionStack->setCurrentIndex(0);
		else if (c->text() == tr("Colorize"))
		{
			QSignalBlocker blocker(colData);
			QSignalBlocker blocker2(shade);
			QString tmpstr = m_effectValMap[c];
			QString col;
			int shading;
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
		//	fp >> col;
			col = fp.readLine();
			fp >> shading;
			setCurrentComboItem(colData, col);
			shade->setValue(shading);
			optionStack->setCurrentIndex(1);
		}
		else if (c->text() == tr("Duotone"))
		{
			QSignalBlocker blocker1(colData1);
			QSignalBlocker blocker2(colData2);
			QSignalBlocker blocker3(shade1);
			QSignalBlocker blocker4(shade2);
			QSignalBlocker blocker5(CurveD1->cDisplay);
			QSignalBlocker blocker6(CurveD2->cDisplay);

			QString tmpstr = m_effectValMap[c];
			QString col1, col2;
			int shading1, shading2;
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
			col1 = fp.readLine();
			col2 = fp.readLine();
			fp >> shading1;
			fp >> shading2;
			setCurrentComboItem(colData1, col1);
			shade1->setValue(shading1);
			setCurrentComboItem(colData2, col2);
			shade2->setValue(shading2);
			int numVals;
			double xval, yval;
			FPointArray curve;
			fp >> numVals;
			for (int i = 0; i < numVals; i++)
			{
				fp >> s;
				xval = ScCLocale::toDoubleC(s);
				fp >> s;
				yval = ScCLocale::toDoubleC(s);
				curve.addPoint(xval, yval);
			}
			CurveD1->cDisplay->setCurve(curve);
			int lin;
			fp >> lin;
			CurveD1->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int i = 0; i < numVals; i++)
			{
				fp >> s;
				xval = ScCLocale::toDoubleC(s);
				fp >> s;
				yval = ScCLocale::toDoubleC(s);
				curve.addPoint(xval, yval);
			}
			CurveD2->cDisplay->setCurve(curve);
			fp >> lin;
			CurveD2->setLinear(lin);
			optionStack->setCurrentIndex(7);
		}
		else if (c->text() == tr("Tritone"))
		{
			QSignalBlocker blocker1(colDatat1);
			QSignalBlocker blocker2(colDatat2);
			QSignalBlocker blocker3(colDatat3);
			QSignalBlocker blocker4(shadet1);
			QSignalBlocker blocker5(shadet2);
			QSignalBlocker blocker6(shadet3);
			QSignalBlocker blocker7(CurveT1->cDisplay);
			QSignalBlocker blocker8(CurveT2->cDisplay);
			QSignalBlocker blocker9(CurveT3->cDisplay);

			QString tmpstr = m_effectValMap[c];
			QString col1, col2, col3;
			int shading1, shading2, shading3;
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
			col1 = fp.readLine();
			col2 = fp.readLine();
			col3 = fp.readLine();
			fp >> shading1;
			fp >> shading2;
			fp >> shading3;
			setCurrentComboItem(colDatat1, col1);
			shadet1->setValue(shading1);
			setCurrentComboItem(colDatat2, col2);
			shadet2->setValue(shading2);
			setCurrentComboItem(colDatat3, col3);
			shadet3->setValue(shading3);
			int numVals;
			double xval, yval;
			FPointArray curve;
			fp >> numVals;
			for (int i = 0; i < numVals; i++)
			{
				fp >> s;
				xval = ScCLocale::toDoubleC(s);
				fp >> s;
				yval = ScCLocale::toDoubleC(s);
				curve.addPoint(xval, yval);
			}
			CurveT1->cDisplay->setCurve(curve);
			int lin;
			fp >> lin;
			CurveT1->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int i = 0; i < numVals; i++)
			{
				fp >> s;
				xval = ScCLocale::toDoubleC(s);
				fp >> s;
				yval = ScCLocale::toDoubleC(s);
				curve.addPoint(xval, yval);
			}
			CurveT2->cDisplay->setCurve(curve);
			fp >> lin;
			CurveT2->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int i = 0; i < numVals; i++)
			{
				fp >> s;
				xval = ScCLocale::toDoubleC(s);
				fp >> s;
				yval = ScCLocale::toDoubleC(s);
				curve.addPoint(xval, yval);
			}
			CurveT3->cDisplay->setCurve(curve);
			fp >> lin;
			CurveT3->setLinear(lin);
			optionStack->setCurrentIndex(8);
		}
		else if (c->text() == tr("Quadtone"))
		{
			QSignalBlocker blocker1(colDataq1);
			QSignalBlocker blocker2(colDataq2);
			QSignalBlocker blocker3(colDataqc3);
			QSignalBlocker blocker4(colDataq4);
			QSignalBlocker blocker5(shadeq1);
			QSignalBlocker blocker6(shadeq2);
			QSignalBlocker blocker7(shadeqc3);
			QSignalBlocker blocker8(shadeq4);
			QSignalBlocker blocker9(CurveQ1->cDisplay);
			QSignalBlocker blocker10(CurveQ2->cDisplay);
			QSignalBlocker blocker11(CurveQc3->cDisplay);
			QSignalBlocker blocker12(CurveQ4->cDisplay);

			QString tmpstr = m_effectValMap[c];
			QString col1, col2, col3, col4;
			int shading1, shading2, shading3, shading4;
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
			col1 = fp.readLine();
			col2 = fp.readLine();
			col3 = fp.readLine();
			col4 = fp.readLine();
			fp >> shading1;
			fp >> shading2;
			fp >> shading3;
			fp >> shading4;
			setCurrentComboItem(colDataq1, col1);
			shadeq1->setValue(shading1);
			setCurrentComboItem(colDataq2, col2);
			shadeq2->setValue(shading2);
			setCurrentComboItem(colDataqc3, col3);
			shadeqc3->setValue(shading3);
			setCurrentComboItem(colDataq4, col4);
			shadeq4->setValue(shading4);
			int numVals;
			double xval, yval;
			FPointArray curve;
			fp >> numVals;
			for (int i = 0; i < numVals; i++)
			{
				fp >> s;
				xval = ScCLocale::toDoubleC(s);
				fp >> s;
				yval = ScCLocale::toDoubleC(s);
				curve.addPoint(xval, yval);
			}
			CurveQ1->cDisplay->setCurve(curve);
			int lin;
			fp >> lin;
			CurveQ1->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int i = 0; i < numVals; i++)
			{
				fp >> s;
				xval = ScCLocale::toDoubleC(s);
				fp >> s;
				yval = ScCLocale::toDoubleC(s);
				curve.addPoint(xval, yval);
			}
			CurveQ2->cDisplay->setCurve(curve);
			fp >> lin;
			CurveQ2->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int i = 0; i < numVals; i++)
			{
				fp >> s;
				xval = ScCLocale::toDoubleC(s);
				fp >> s;
				yval = ScCLocale::toDoubleC(s);
				curve.addPoint(xval, yval);
			}
			CurveQc3->cDisplay->setCurve(curve);
			fp >> lin;
			CurveQc3->setLinear(lin);
			curve.resize(0);
			fp >> numVals;
			for (int i = 0; i < numVals; i++)
			{
				fp >> s;
				xval = ScCLocale::toDoubleC(s);
				fp >> s;
				yval = ScCLocale::toDoubleC(s);
				curve.addPoint(xval, yval);
			}
			CurveQ4->cDisplay->setCurve(curve);
			fp >> lin;
			CurveQ4->setLinear(lin);
			optionStack->setCurrentIndex(9);
		}
		else if (c->text() == tr("Brightness"))
		{
			QSignalBlocker blocker(brightnessSlider);
			QString tmpstr = m_effectValMap[c];
			int brightness;
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> brightness;
			brightnessSlider->setValue(brightness);
			QString tmp;
			tmp.setNum(brightness);
			textLabel7->setText(tmp);
			optionStack->setCurrentIndex(2);
		}
		else if (c->text() == tr("Contrast"))
		{
			QSignalBlocker blocker(contrastSlider);
			QString tmpstr = m_effectValMap[c];
			int contrast;
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> contrast;
			contrastSlider->setValue(contrast);
			QString tmp;
			tmp.setNum(contrast);
			textLabel9->setText(tmp);
			optionStack->setCurrentIndex(3);
		}
		else if (c->text() == tr("Sharpen"))
		{
			QSignalBlocker blocker1(shRadius);
			QSignalBlocker blocker2(shValue);
			QString tmpstr = m_effectValMap[c];
			double radius, sigma;
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> s;
			radius = ScCLocale::toDoubleC(s);
			fp >> s;
			sigma = ScCLocale::toDoubleC(s);
			shRadius->setValue(radius);
			shValue->setValue(sigma);
			optionStack->setCurrentIndex(4);
		}
		else if (c->text() == tr("Blur"))
		{
			QSignalBlocker blocker1(blRadius);
			QString tmpstr = m_effectValMap[c];
			double radius;
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> s;
			radius = ScCLocale::toDoubleC(s);
			blRadius->setValue(radius);
			optionStack->setCurrentIndex(5);
		}
		else if (c->text() == tr("Posterize"))
		{
			QSignalBlocker blocker1(solarizeSlider);
			QString tmpstr = m_effectValMap[c];
			int solarize;
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
			fp >> solarize;
			solarizeSlider->setValue(solarize);
			QString tmp;
			tmp.setNum(solarize);
			textLabel15->setText(tmp);
			optionStack->setCurrentIndex(6);

		}
		else if (c->text() == tr("Curves"))
		{
			QSignalBlocker blocker1(Kdisplay->cDisplay);
			QString tmpstr = m_effectValMap[c];
			ScTextStream fp(&tmpstr, QIODevice::ReadOnly);
			int numVals;
			double xval, yval;
			FPointArray curve;
			fp >> numVals;
			for (int i = 0; i < numVals; i++)
			{
				fp >> s;
				xval = ScCLocale::toDoubleC(s);
				fp >> s;
				yval = ScCLocale::toDoubleC(s);
				curve.addPoint(xval, yval);
			}
			Kdisplay->cDisplay->setCurve(curve);
			int lin;
			fp >> lin;
			Kdisplay->setLinear(lin == 1);
			optionStack->setCurrentIndex(10);
		}
		else
			optionStack->setCurrentIndex(0);
		currentOptions = c;
	}
	else
		optionStack->setCurrentIndex(0);
	QSignalBlocker blocker1(availableEffects);
	availableEffects->clearSelection();
}

void EffectsDialog::selectAvailEffect(QListWidgetItem* c)
{
	if (c)
	{
		if (!(c->flags() & Qt::ItemIsSelectable))
			toEffects->setEnabled(false);
		else
			toEffects->setEnabled(true);
	}
	fromEffects->setEnabled(false);
	effectUp->setEnabled(false);
	effectDown->setEnabled(false);
	QSignalBlocker blocker1(usedEffects);
	selectEffectHelper();
	currentOptions = nullptr;
	usedEffects->clearSelection();
	optionStack->setCurrentIndex(0);
}

void EffectsDialog::selectEffectHelper(bool finalValues)
{
	if (currentOptions != nullptr)
	{
		if (currentOptions->text() == tr("Colorize"))
		{
			QString efval = colData->currentText();
			QString tmp;
			tmp.setNum(shade->getValue());
			efval += "\n"+tmp;
			m_effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Brightness"))
		{
			QString tmp;
			tmp.setNum(brightnessSlider->value());
			m_effectValMap[currentOptions] = tmp;
		}
		if (currentOptions->text() == tr("Contrast"))
		{
			QString tmp;
			tmp.setNum(contrastSlider->value());
			m_effectValMap[currentOptions] = tmp;
		}
		if (currentOptions->text() == tr("Sharpen"))
		{
			QString efval;
			QString tmp;
			tmp.setNum(shRadius->value());
			efval += tmp;
			tmp.setNum(shValue->value());
			efval += " "+tmp;
			m_effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Blur"))
		{
			QString efval;
			QString tmp;
			if (!finalValues)
				tmp.setNum(blRadius->value());
			else
				tmp.setNum(blRadius->value()*m_imageScale);
			efval += tmp;
			tmp.setNum(1.0);
			efval += " "+tmp;
			m_effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Posterize"))
		{
			QString tmp;
			tmp.setNum(solarizeSlider->value());
			m_effectValMap[currentOptions] = tmp;
		}
		if (currentOptions->text() == tr("Duotone"))
		{
			QString efval = colData1->currentText()+"\n";
			efval += colData2->currentText()+"\n";
			QString tmp;
			tmp.setNum(shade1->getValue());
			efval += tmp;
			tmp.setNum(shade2->getValue());
			efval += " "+tmp;
			FPointArray Vals = CurveD1->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (int i = 0; i < Vals.size(); i++)
			{
				const FPoint& pv = Vals.point(i);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveD1->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveD2->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (int i = 0; i < Vals.size(); i++)
			{
				const FPoint& pv = Vals.point(i);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveD2->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			m_effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Tritone"))
		{
			QString efval;
			efval = colDatat1->currentText()+"\n";
			efval += colDatat2->currentText()+"\n";
			efval += colDatat3->currentText()+"\n";
			QString tmp;
			tmp.setNum(shadet1->getValue());
			efval += tmp;
			tmp.setNum(shadet2->getValue());
			efval += " "+tmp;
			tmp.setNum(shadet3->getValue());
			efval += " "+tmp;
			FPointArray Vals = CurveT1->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (int p = 0; p < Vals.size(); p++)
			{
				const FPoint& pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveT1->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveT2->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (int p = 0; p < Vals.size(); p++)
			{
				const FPoint& pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveT2->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveT3->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (int p = 0; p < Vals.size(); p++)
			{
				const FPoint& pv = Vals.point(p);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveT3->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			m_effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Quadtone"))
		{
			QString efval = colDataq1->currentText()+"\n";
			efval += colDataq2->currentText()+"\n";
			efval += colDataqc3->currentText()+"\n";
			efval += colDataq4->currentText()+"\n";
			QString tmp;
			tmp.setNum(shadeq1->getValue());
			efval += tmp;
			tmp.setNum(shadeq2->getValue());
			efval += " "+tmp;
			tmp.setNum(shadeqc3->getValue());
			efval += " "+tmp;
			tmp.setNum(shadeq4->getValue());
			efval += " "+tmp;
			FPointArray Vals = CurveQ1->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (int i = 0; i < Vals.size(); i++)
			{
				const FPoint& pv = Vals.point(i);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveQ1->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveQ2->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (int i = 0; i < Vals.size(); i++)
			{
				const FPoint& pv = Vals.point(i);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveQ2->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveQc3->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (int i = 0; i < Vals.size(); i++)
			{
				const FPoint& pv = Vals.point(i);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveQc3->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			Vals = CurveQ4->cDisplay->getCurve();
			tmp.setNum(Vals.size());
			efval += " "+tmp;
			for (int i = 0; i < Vals.size(); i++)
			{
				const FPoint& pv = Vals.point(i);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (CurveQ4->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			m_effectValMap[currentOptions] = efval;
		}
		if (currentOptions->text() == tr("Curves"))
		{
			QString efval;
			FPointArray Vals = Kdisplay->cDisplay->getCurve();
			QString tmp;
			tmp.setNum(Vals.size());
			efval += tmp;
			for (int i = 0; i < Vals.size(); i++)
			{
				const FPoint& pv = Vals.point(i);
				efval += QString(" %1 %2").arg(pv.x()).arg(pv.y());
			}
			if (Kdisplay->cDisplay->isLinear())
				efval += " 1";
			else
				efval += " 0";
			m_effectValMap[currentOptions] = efval;
		}
	}
}
