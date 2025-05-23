
/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "cmykfw.h"

#include <QAction>
#include <QByteArray>
#include <QCheckBox>
#include <QCursor>
#include <QDomDocument>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStackedWidget>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpacerItem>
#include <QToolTip>
#include <QTextStream>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <cstdlib>

#include "colorchart.h"
#include "colorlistbox.h"
#include "commonstrings.h"
#include "sccolorengine.h"
#include "scconfig.h"
#include "scpaths.h"
#include "scribusdoc.h"
#include "scrspinbox.h"
#include "swatchcombo.h"
#include "ui/scmessagebox.h"
#include "util.h"
#include "util_color.h"
#include "util_formats.h"
#include "iconmanager.h"


CMYKChoose::CMYKChoose(QWidget* parent, ScribusDoc* doc, const ScColor& orig, const QString& name, ColorList *Colors, bool newCol)
		: QDialog(parent),
		  m_color(orig),
	      isNew(newCol),
	      EColors(Colors),
	      CurrSwatch(doc),
	      m_doc(doc)
{
	setModal(true);

	CurrSwatch.clear();
	alertIcon = IconManager::instance().loadPixmap("alert-warning");
	resize( 498, 306 );
	setWindowTitle( tr( "Edit Color" ));
	setWindowIcon(IconManager::instance().loadIcon("app-icon"));

	setupUi(this);
	ColorMap->setDoc(doc);
	ColorName->setText(name);
	ColorSwatch->setPixmapType(ColorListBox::fancyPixmap);
	
	ComboBox1->addItem( tr( "CMYK" ));
	ComboBox1->addItem( tr( "RGB" ));
	ComboBox1->addItem( tr( "Web Safe RGB" ));
	ComboBox1->addItem( tr( "Lab" ));
	ComboBox1->addItem( tr( "HLC" ));

	Separations->setChecked(orig.isSpotColor());
	imageA.fill( ScColorEngine::getDisplayColor(orig, m_doc));
	if ( ScColorEngine::isOutOfGamut(orig, m_doc))
		paintAlert(alertIcon,imageA, 2, 2);
	OldC->setPixmap( imageA );
	updateNewColorImage(orig);

	buttonOK->setText(CommonStrings::tr_OK);
	buttonOK->setDefault( true );
	buttonCancel->setText(CommonStrings::tr_Cancel);

	hsvSelector = Swatches->addTopLevelItem( tr( "Color Map" ));
	hsvSelector->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
	m_CSM.findPaletteLocations();
	systemSwatches = Swatches->addTopLevelItem( tr("Scribus Swatches"));
	m_CSM.findPalettes(systemSwatches);
	Swatches->addSubItem("Scribus Small", systemSwatches);
	systemSwatches->setExpanded(true);
	userSwatches = Swatches->addTopLevelItem( tr("User Swatches"));
	m_CSM.findUserPalettes(userSwatches);
	customColSet = m_CSM.userPaletteNames();
	userSwatches->setExpanded(true);
	// Swatches combo uses elided text strings, so we cannot
	// set default combo item in constructor: at that point
	// the swatch combo widget does not know its width yet.
	// We set it in the dialog showEvent().
	// Swatches->setCurrentComboItem( tr( "Color Map" ));

	slidersLayout->setSpacing(6);
	slidersLayout->setContentsMargins(0, 0, 0, 0);

	CyanSp->setNewUnit(0);
	CyanSp->setMinimum(0);
	CyanSp->setMaximum(100);
	CyanSp->setSuffix( tr(" %"));

	CyanSL->setAutoFillBackground(true);
	CyanSL->setMinimumSize(QSize(255, 16));
	CyanSL->setMinimum(0);
	CyanSL->setMaximum(100);
	CyanSL->setPalette(sliderPix(180));

	MagentaSp->setNewUnit(0);
	MagentaSp->setMinimum(0);
	MagentaSp->setMaximum(100);
	MagentaSp->setSuffix( tr(" %"));

	MagentaSL->setAutoFillBackground(true);
	MagentaSL->setMinimumSize(QSize(255, 16));
	MagentaSL->setMinimum(0);
	MagentaSL->setMaximum(100);
	MagentaSL->setPalette(sliderPix(300));

	YellowSp->setNewUnit(0);
	YellowSp->setMinimum(0);
	YellowSp->setMaximum(100);
	YellowSp->setSuffix( tr(" %"));

	YellowSL->setAutoFillBackground(true);
	YellowSL->setMinimumSize(QSize(255, 16));
	YellowSL->setMinimum(0);
	YellowSL->setMaximum(100);
	YellowSL->setPalette(sliderPix(60));

	BlackSp->setNewUnit(0);
	BlackSp->setMinimum(0);
	BlackSp->setMaximum(100);
	BlackSp->setSuffix( tr(" %"));

	BlackSL->setAutoFillBackground(true);
	BlackSL->setMinimumSize(QSize(255, 16));
	BlackSL->setMinimum(0);
	BlackSL->setMaximum(100);
	BlackSL->setPalette(sliderBlack());

	if (orig.getColorModel () == colorModelCMYK)
	{
		CMYKColorF cmyk;
		ScColorEngine::getCMYKValues(orig, m_doc, cmyk);
		double ccd = cmyk.c * 100.0;
		double cmd = cmyk.m * 100.0;
		double cyd = cmyk.y * 100.0;
		double ckd = cmyk.k * 100.0;
		CyanSp->setValue(ccd);
		CyanSL->setValue(qRound(ccd * 1000));
		MagentaSp->setValue(cmd);
		MagentaSL->setValue(qRound(cmd * 1000));
		YellowSp->setValue(cyd);
		YellowSL->setValue(qRound(cyd * 1000));
		BlackSp->setValue(ckd);
		BlackSL->setValue(qRound(ckd * 1000));
		BlackComp = qRound(cmyk.k * 255.0);
	}
	int h, s, v;
	ScColorEngine::getRGBColor(orig, m_doc).getHsv(&h, &s, &v);
	ColorMap->setFixedWidth(180);
	ColorMap->drawPalette(v);
	ColorMap->setMark(h, s);
	Fnam = name;
	ColorName->selectAll();
	ColorName->setFocus();
	TabStack->setCurrentIndex(0);
	setFixedSize(minimumSizeHint());
	setContextMenuPolicy(Qt::CustomContextMenu);
	if (orig.getColorModel () == colorModelRGB)
	{
		ComboBox1->setCurrentIndex(1);
		selModel ( tr( "RGB" ));
	}
	else if (orig.getColorModel() == colorModelCMYK)
	{
		ComboBox1->setCurrentIndex(0);
		selModel ( tr( "CMYK" ));
	}
	else if (orig.getColorModel() == colorModelLab)
	{
		ComboBox1->setCurrentIndex(3);
		selModel ( tr( "Lab" ));
	}
	isRegistration = m_color.isRegistrationColor();
	if (m_color.isRegistrationColor())
	{
		ComboBox1->setEnabled(false);
		Separations->setEnabled(false);
	}

	// QRegularExpression regex(R"(^#?([A-Fa-f0-9]{3}|[A-Fa-f0-9]{6})$)");
	//6 digits for now
	QRegularExpression regex(R"(^#?([A-Fa-f0-9]{6})$)");
	QValidator *validator = new QRegularExpressionValidator(regex, this);
	hexLineEdit->setValidator(validator);


	// signals and slots connections
//	Regist->setToolTip( "<qt>" + tr( "Choosing this will enable printing this on all plates. Registration colors are used for printer marks such as crop marks, registration marks and the like. These are not typically used in the layout itself." ) + "</qt>");
	Separations->setToolTip( "<qt>" + tr( "Choosing this will make this color a spot color, thus creating another spot when creating plates or separations. This is used most often when a logo or other color needs exact representation or cannot be replicated with CMYK inks. Metallic and fluorescent inks are good examples which cannot be easily replicated with CMYK inks." ) + "</qt>");
	connect( buttonCancel, SIGNAL(clicked()), this, SLOT(reject()));
	connect( buttonOK, SIGNAL(clicked()), this, SLOT(leave()));
	connect( CyanSp, SIGNAL(valueChanged(double)), this, SLOT(setValSliders(double)));
	connect( MagentaSp, SIGNAL(valueChanged(double)), this, SLOT(setValSliders(double)));
	connect( YellowSp, SIGNAL(valueChanged(double)), this, SLOT(setValSliders(double)));
	connect( BlackSp, SIGNAL(valueChanged(double)), this, SLOT(setValSliders(double)));
	connect( CyanSL, SIGNAL(valueChanged(int)), this, SLOT(setValueS(int)));
	connect( MagentaSL, SIGNAL(valueChanged(int)), this, SLOT(setValueS(int)));
	connect( YellowSL, SIGNAL(valueChanged(int)), this, SLOT(setValueS(int)));
	connect( BlackSL, SIGNAL(valueChanged(int)), this, SLOT(setValueS(int)));
	connect( CyanSL, SIGNAL(valueChanged(int)), this, SLOT(setColor()));
	connect( MagentaSL, SIGNAL(valueChanged(int)), this, SLOT(setColor()));
	connect( YellowSL, SIGNAL(valueChanged(int)), this, SLOT(setColor()));
	connect( BlackSL, SIGNAL(valueChanged(int)), this, SLOT(setColor()));
	connect( ColorMap, SIGNAL(ColorVal(int,int,bool)), this, SLOT(setColor2(int,int,bool)));
	connect( ComboBox1, SIGNAL(textActivated(QString)), this, SLOT(selModel(QString)));
	connect(Swatches, SIGNAL(activated(QString)), this, SLOT(selSwatch()));
	connect(ColorSwatch, SIGNAL(itemClicked(int)), this, SLOT(selFromSwatch(int)));
	connect(Separations, SIGNAL(clicked()), this, SLOT(setSpot()));
	connect(hexLineEdit, SIGNAL(editingFinished()), this, SLOT(setValueFromHex()));
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotRightClick()));
	layout()->activate();
}

CMYKChoose::~CMYKChoose()
{
	if (hexLineEdit->validator())
		delete hexLineEdit->validator();
}

QString CMYKChoose::colorName() const
{
	return ColorName->text();
}

bool CMYKChoose::isSpotColor() const
{
	return Separations->isChecked();
}

void CMYKChoose::setValSliders(double value)
{
	if (CyanSp == sender())
		CyanSL->setValue(value * 1000);
	if (MagentaSp == sender())
		MagentaSL->setValue(value * 1000);
	if (YellowSp == sender())
		YellowSL->setValue(value * 1000);
	if (BlackSp == sender())
		BlackSL->setValue(value * 1000);
}

void CMYKChoose::slotRightClick()
{
	QMenu *pmen = new QMenu();
	QAction* dynAct;
	if (dynamic)
		dynAct = pmen->addAction( tr("Static Color Bars"));
	else
		dynAct = pmen->addAction( tr("Dynamic Color Bars"));
	connect(dynAct, SIGNAL(triggered()), this, SLOT(toggleSL()));
	pmen->exec(QCursor::pos());
	pmen->deleteLater();
}

void CMYKChoose::setValueS(int val)
{
	QSignalBlocker sb1(CyanSp);
	QSignalBlocker sb2(MagentaSp);
	QSignalBlocker sb3(YellowSp);
	QSignalBlocker sb4(BlackSp);
	if (CyanSL == sender())
		CyanSp->setValue(val / 1000.0);
	if (MagentaSL == sender())
		MagentaSp->setValue(val / 1000.0);
	if (YellowSL == sender())
		YellowSp->setValue(val / 1000.0);
	if (BlackSL == sender())
		BlackSp->setValue(val / 1000.0);
	setColor();
}

void CMYKChoose::toggleSL()
{
	dynamic = !dynamic;
	CyanSL->setPalette(sliderPix(m_color.getColorModel() == colorModelCMYK ? 180 : 0));
	MagentaSL->setPalette(sliderPix(m_color.getColorModel() == colorModelCMYK? 300 : 120));
	YellowSL->setPalette(sliderPix(m_color.getColorModel() == colorModelCMYK? 60 : 240));
	if (m_color.getColorModel() == colorModelCMYK)
		BlackSL->setPalette(sliderBlack());
}

QPalette CMYKChoose::sliderPix(int color)
{
	RGBColor rgb;
	CMYKColor cmyk;
	QImage image0(255, 10, QImage::Format_ARGB32);
	QPainter p;
	p.begin(&image0);
	p.setPen(Qt::NoPen);
	int r, g, b, treeItem, m, y, k;
	QColor tmp;
	for (int x = 0; x < 255; x += 5)
	{
		if (m_color.getColorModel() == colorModelCMYK)
		{
			ScColorEngine::getCMYKValues(m_color, m_doc, cmyk);
			cmyk.getValues(treeItem, m, y, k);
			if (dynamic)
			{
				switch (color)
				{
				case 180:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(x, m, y, k), m_doc);
					break;
				case 300:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(treeItem, x, y, k), m_doc);
					break;
				case 60:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(treeItem, m, x, k), m_doc);
					break;
				}
				p.setBrush(tmp);
			}
			else
			{
				switch (color)
				{
				case 180:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(x, 0, 0, 0), m_doc);
					break;
				case 300:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(0, x, 0, 0), m_doc);
					break;
				case 60:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(0, 0, x, 0), m_doc);
					break;
				}
				p.setBrush(tmp);
			}
		}
		else if (m_color.getColorModel() == colorModelRGB)
		{
			ScColorEngine::getRGBValues(m_color, m_doc, rgb);
			rgb.getValues(r, g, b);
			if (dynamic)
			{
				switch (color)
				{
				case 0:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(x, g, b), m_doc);
					break;
				case 120:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(r, x, b), m_doc);
					break;
				case 240:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(r, g, x), m_doc);
					break;
				}
				p.setBrush(tmp);
			}
			else
			{
				switch (color)
				{
				case 0:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(x, 0, 0), m_doc);
					break;
				case 120:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(0, x, 0), m_doc);
					break;
				case 240:
					tmp = ScColorEngine::getDisplayColorGC(ScColor(0, 0, x), m_doc);
					break;
				}
				p.setBrush(tmp);
			}
		}
		else if (m_color.getColorModel() == colorModelLab)
		{
			double L, a, b;
			double val = static_cast<double>(x) / 255.0;
			m_color.getLab(&L, &a, &b);
			if (m_isHLC)
			{
				QLineF lin;
				lin.setP1(QPointF(0.0, 0.0));
				lin.setP2(QPointF(a, b));
				double H = lin.angle();
				double C = lin.length();
				double tmpA, tmpB;
				if (dynamic)
				{
					switch (color)
					{
					case 0:
						lin = QLineF::fromPolar(C, -360 * val);
						tmpA = lin.p2().x();
						tmpB = lin.p2().y();
						tmp = ScColorEngine::getDisplayColorGC(ScColor(L, tmpA, tmpB), m_doc);
						break;
					case 120:
						tmp = ScColorEngine::getDisplayColorGC(ScColor(100 * val, a, b), m_doc);
						break;
					case 240:
						lin = QLineF::fromPolar(128 * val, H);
						tmpA = lin.p2().x();
						tmpB = lin.p2().y();
						tmp = ScColorEngine::getDisplayColorGC(ScColor(L, tmpA, tmpB), m_doc);
						break;
					}
					p.setBrush(tmp);
				}
				else
				{
					switch (color)
					{
					case 0:
						lin = QLineF::fromPolar(128, -360 * val);
						tmpA = lin.p2().x();
						tmpB = lin.p2().y();
						tmp = ScColorEngine::getDisplayColorGC(ScColor(100.0, tmpA, tmpB), m_doc);
						break;
					case 120:
						tmp = ScColorEngine::getDisplayColorGC(ScColor(100 * val, 0.0, 0.0), m_doc);
						break;
					case 240:
						lin = QLineF::fromPolar(128 * val, 0);
						tmpA = lin.p2().x();
						tmpB = lin.p2().y();
						tmp = ScColorEngine::getDisplayColorGC(ScColor(100.0, tmpA, tmpB), m_doc);
						break;
					}
					p.setBrush(tmp);
				}
			}
			else
			{
				if (dynamic)
				{
					switch (color)
					{
					case 0:
						tmp = ScColorEngine::getDisplayColorGC(ScColor(100 * val, a, b), m_doc);
						break;
					case 120:
						tmp = ScColorEngine::getDisplayColorGC(ScColor(L, 256 * val - 128.0, b), m_doc);
						break;
					case 240:
						tmp = ScColorEngine::getDisplayColorGC(ScColor(L, a, 256 * val - 128.0), m_doc);
						break;
					}
					p.setBrush(tmp);
				}
				else
				{
					switch (color)
					{
					case 0:
						tmp = ScColorEngine::getDisplayColorGC(ScColor(100 * val, 0.0, 0.0), m_doc);
						break;
					case 120:
						tmp = ScColorEngine::getDisplayColorGC(ScColor(100.0, 256 * val - 128.0, 0.0), m_doc);
						break;
					case 240:
						tmp = ScColorEngine::getDisplayColorGC(ScColor(100.0, 0.0, 256 * val - 128.0), m_doc);
						break;
					}
				}
				p.setBrush(tmp);
			}
		}
		p.drawRect(x, 0, 5, 10);
	}
	p.end();
	QPalette pal;
	pal.setBrush(QPalette::Window, QBrush(image0));
	return pal;
}

QPalette CMYKChoose::sliderBlack()
{
	QImage image0(255, 10, QImage::Format_ARGB32);
	QPainter p;
	p.begin(&image0);
	p.setPen(Qt::NoPen);
	int treeItem, m, y, k;
	CMYKColor cmyk;
	ScColorEngine::getCMYKValues(m_color, m_doc, cmyk);
	cmyk.getValues(treeItem, m, y, k);
	for (int x = 0; x < 255; x += 5)
	{
		if (dynamic)
			p.setBrush( ScColorEngine::getDisplayColorGC(ScColor(treeItem, m, y, x), m_doc));
		else
			p.setBrush( ScColorEngine::getDisplayColorGC(ScColor(0, 0, 0, x), m_doc));
		p.drawRect(x, 0, 5, 10);
	}
	p.end();
	QPalette pal;
	pal.setBrush(QPalette::Window, QBrush(image0));
	return pal;
}

void CMYKChoose::selSwatch()
{
	const QTreeWidgetItem * treeItem = Swatches->currentItem();
	if (treeItem == hsvSelector)
	{
		TabStack->setCurrentIndex(0);
		return;
	}

	CurrSwatch.clear();
	QString pfadC2;
	QString txt = treeItem->data(0, Qt::UserRole).toString() + "/" + treeItem->text(0);
	if (!customColSet.contains(txt))
		pfadC2 = m_CSM.paletteFileFromName(txt);
	else
		pfadC2 = m_CSM.userPaletteFileFromName(txt);
	if (importColorsFromFile(pfadC2, CurrSwatch))
	{
		CurrSwatch.insert("White", ScColor(0, 0, 0, 0));
		CurrSwatch.insert("Black", ScColor(0, 0, 0, 255));
	}
	else
	{
		CurrSwatch.insert("White", ScColor(0, 0, 0, 0));
		CurrSwatch.insert("Black", ScColor(0, 0, 0, 255));
		ScColor cc(255, 255, 255, 255);
		cc.setRegistrationColor(true);
		CurrSwatch.insert("Registration", cc);
		CurrSwatch.insert("Blue", ScColor(255, 255, 0, 0));
		CurrSwatch.insert("Cyan", ScColor(255, 0, 0, 0));
		CurrSwatch.insert("Green", ScColor(255, 0, 255, 0));
		CurrSwatch.insert("Red", ScColor(0, 255, 255, 0));
		CurrSwatch.insert("Yellow", ScColor(0, 0, 255, 0));
		CurrSwatch.insert("Magenta", ScColor(0, 255, 0, 0));
	}
	ColorSwatch->setColors(CurrSwatch, false);
	ColorSwatch->setCurrentRow(0);
	TabStack->setCurrentIndex(1);
}

void CMYKChoose::setSpot()
{
	QSignalBlocker comboBox1Blocker(ComboBox1);
	if (Separations->isChecked())
	{
//		Commented out to allow RGB Spot-Colors
//		ComboBox1->setCurrentIndex( 0 );
//		selModel( tr("CMYK"));
		if (ComboBox1->currentIndex() == 2) // Web Safe RGB
		{
			ComboBox1->setCurrentIndex(1);
			selModel( tr("RGB"));
		}
	}
}

void CMYKChoose::selModel(const QString& mod)
{
	int h, s, v;

	QSignalBlocker sb1(CyanSp);
	QSignalBlocker sb2(MagentaSp);
	QSignalBlocker sb3(YellowSp);
	QSignalBlocker sb4(BlackSp);
	QSignalBlocker sb5(CyanSL);
	QSignalBlocker sb6(MagentaSL);
	QSignalBlocker sb7(YellowSL);
	QSignalBlocker sb8(BlackSL);
	QSignalBlocker sb9(CyanSL);
	QSignalBlocker sb10(MagentaSL);
	QSignalBlocker sb11(YellowSL);
	QSignalBlocker sb12(BlackSL);
	m_isHLC = false;
	if (mod == tr("CMYK"))
	{
		Wsave = false;
		CyanSL->setMaximum(100 * 1000);
		CyanSL->setMinimum(0 * 1000);
		CyanSL->setSingleStep(1 * 1000);
		CyanSL->setPageStep(10 * 1000);

		MagentaSL->setMaximum(100 * 1000);
		MagentaSL->setMinimum(0 * 1000);
		MagentaSL->setSingleStep(1 * 1000);
		MagentaSL->setPageStep(10 * 1000);

		YellowSL->setMaximum(100 * 1000);
		YellowSL->setMinimum(0 * 1000);
		YellowSL->setSingleStep(1 * 1000);
		YellowSL->setPageStep(10 * 1000);

		BlackSL->setMaximum(100 * 1000);
		BlackSL->setMinimum(0 * 1000);
		BlackSL->setSingleStep(1 * 1000);
		BlackSL->setPageStep(10 * 1000);

		CyanSp->setMaximum( 100 );
		CyanSp->setMinimum( 0 );
		CyanSp->setDecimals(1);
		CyanSp->setSingleStep(1);
		CyanSp->setSuffix( tr(" %"));

		MagentaSp->setMaximum( 100);
		MagentaSp->setMinimum( 0 );
		MagentaSp->setDecimals(1);
		MagentaSp->setSingleStep(1);
		MagentaSp->setSuffix( tr(" %"));

		YellowSp->setMaximum( 100 );
		YellowSp->setMinimum( 0 );
		YellowSp->setDecimals(1);
		YellowSp->setSingleStep(1);
		YellowSp->setSuffix( tr(" %"));

		BlackSp->setDecimals(1);
		
		CyanT->setText( tr("C:"));
		MagentaT->setText( tr("M:"));
		YellowT->setText( tr("Y:"));
		BlackSL->show();
		BlackSp->show();
		BlackT->show();
		hexLabel->hide();
		hexLineEdit->hide();

		if (m_color.getColorModel() != colorModelCMYK)
			m_color = ScColorEngine::convertToModel(m_color, m_doc, colorModelCMYK);
		CyanSL->setPalette(sliderPix(180));
		MagentaSL->setPalette(sliderPix(300));
		YellowSL->setPalette(sliderPix(60));
		BlackSL->setPalette(sliderBlack());
		ScColorEngine::getRGBColor(m_color, m_doc).getHsv(&h, &s, &v);
		setValues();
		ColorMap->drawMode = 0;
		ColorMap->drawPalette(v);
		ColorMap->setMark(h, s);
	}
	else if ((mod == tr("Web Safe RGB")) || (mod == tr("RGB")))
	{
		Wsave = false;
		CyanSL->setMaximum(255 * 1000);
		CyanSL->setMinimum(0 * 1000);
		CyanSL->setSingleStep(1 * 1000);
		CyanSL->setPageStep(1 * 1000);

		MagentaSL->setMaximum(255 * 1000);
		MagentaSL->setMinimum(0 * 1000);
		MagentaSL->setSingleStep(1 * 1000);
		MagentaSL->setPageStep(1 * 1000);

		YellowSL->setMaximum(255 * 1000);
		YellowSL->setMinimum(0 * 1000);
		YellowSL->setSingleStep(1 * 1000);
		YellowSL->setPageStep(1 * 1000);

		CyanSp->setSingleStep(1);
		CyanSp->setMaximum( 255 );
		CyanSp->setMinimum( 0 );
		CyanSp->setDecimals(0);
		CyanSp->setSuffix("");

		MagentaSp->setSingleStep(1);
		MagentaSp->setMaximum( 255 );
		MagentaSp->setMinimum( 0 );
		MagentaSp->setDecimals(0);
		MagentaSp->setSuffix("");

		YellowSp->setSingleStep(1);
		YellowSp->setMaximum( 255 );
		YellowSp->setMinimum( 0 );
		YellowSp->setDecimals(0);
		YellowSp->setSuffix("");
		
		CyanT->setText( tr("R:"));
		MagentaT->setText( tr("G:"));
		YellowT->setText( tr("B:"));
		BlackSL->hide();
		BlackSp->hide();
		BlackT->hide();
		hexLabel->show();
		hexLineEdit->show();
		if (mod == tr("Web Safe RGB"))
		{
			Wsave = true;
			CyanSL->setSingleStep(51 * 1000);
			MagentaSL->setSingleStep(51 * 1000);
			YellowSL->setSingleStep(51 * 1000);
			CyanSL->setPageStep(51 * 1000);
			MagentaSL->setPageStep(51 * 1000);
			YellowSL->setPageStep(51 * 1000);
			CyanSp->setSingleStep(51);
			MagentaSp->setSingleStep(51);
			YellowSp->setSingleStep(51);
		}
		if (m_color.getColorModel() != colorModelRGB)
			m_color = ScColorEngine::convertToModel(m_color, m_doc, colorModelRGB);
		CyanSL->setPalette(sliderPix(0));
		MagentaSL->setPalette(sliderPix(120));
		YellowSL->setPalette(sliderPix(240));
		ScColorEngine::getRGBColor(m_color, m_doc).getHsv(&h, &s, &v);
		setValues();
		ColorMap->drawMode = 0;
		ColorMap->drawPalette(v);
		ColorMap->setMark(h, s);
	}
	else if (mod == tr("Lab"))
	{
		Wsave = false;
		CyanSL->setSingleStep(1 * 1000);
		CyanSL->setPageStep(10 * 1000);
		CyanSL->setMaximum(100 * 1000);
		CyanSL->setMinimum(0 * 1000);
		MagentaSL->setSingleStep(1 * 1000);
		MagentaSL->setPageStep(10 * 1000);
		MagentaSL->setMaximum(128 * 1000);
		MagentaSL->setMinimum(-128 * 1000);
		YellowSL->setSingleStep(1 * 1000);
		YellowSL->setPageStep(10 * 1000);
		YellowSL->setMaximum(128 * 1000);
		YellowSL->setMinimum(-128 * 1000);

		CyanSp->setDecimals(2);
		CyanSp->setSingleStep(1);
		CyanSp->setMaximum( 100 );
		CyanSp->setSuffix( tr(""));

		MagentaSp->setDecimals(2);
		MagentaSp->setSingleStep(1);
		MagentaSp->setMaximum( 128);
		MagentaSp->setMinimum( -128 );
		MagentaSp->setSuffix("");

		YellowSp->setDecimals(2);
		YellowSp->setMaximum( 128 );
		YellowSp->setMinimum( -128 );
		YellowSp->setSingleStep(1);
		YellowSp->setSuffix("");

		CyanT->setText( tr("L:"));
		MagentaT->setText( tr("a:"));
		YellowT->setText( tr("b:"));

		BlackSL->hide();
		BlackSp->hide();
		BlackT->hide();
		hexLabel->hide();
		hexLineEdit->hide();

		if (m_color.getColorModel() != colorModelLab)
			m_color = ScColorEngine::convertToModel(m_color, m_doc, colorModelLab);
		CyanSL->setPalette(sliderPix(0));
		MagentaSL->setPalette(sliderPix(120));
		YellowSL->setPalette(sliderPix(240));
		setValues();
		ColorMap->drawMode = 1;
		ColorMap->drawPalette(CyanSp->value() * 2.55);
		ColorMap->setMark(MagentaSp->value(), YellowSp->value());
	}
	else if (mod == tr("HLC"))
	{
		Wsave = false;
		CyanSL->setSingleStep(1 * 1000);
		CyanSL->setPageStep(10 * 1000);
		CyanSL->setMaximum(360 * 1000);
		CyanSL->setMinimum(0 * 1000);
		MagentaSL->setSingleStep(1 * 1000);
		MagentaSL->setPageStep(10 * 1000);
		MagentaSL->setMaximum(100 * 1000);
		MagentaSL->setMinimum(0 * 1000);
		YellowSL->setSingleStep(1 * 1000);
		YellowSL->setPageStep(10 * 1000);
		YellowSL->setMaximum(128 * 1000);
		YellowSL->setMinimum(0 * 1000);

		CyanSp->setDecimals(2);
		CyanSp->setSingleStep(1);
		CyanSp->setMaximum( 360 );
		CyanSp->setSuffix( tr(""));

		MagentaSp->setDecimals(2);
		MagentaSp->setSingleStep(1);
		MagentaSp->setMaximum( 100);
		MagentaSp->setMinimum( 0 );
		MagentaSp->setSuffix("");

		YellowSp->setDecimals(2);
		YellowSp->setMaximum( 128 );
		YellowSp->setMinimum( 0 );
		YellowSp->setSingleStep(1);
		YellowSp->setSuffix("");

		CyanT->setText( tr("H:"));
		MagentaT->setText( tr("L:"));
		YellowT->setText( tr("C:"));

		BlackSL->hide();
		BlackSp->hide();
		BlackT->hide();
		hexLabel->hide();
		hexLineEdit->hide();

		if (m_color.getColorModel() != colorModelLab)
			m_color = ScColorEngine::convertToModel(m_color, m_doc, colorModelLab);
		m_isHLC = true;
		CyanSL->setPalette(sliderPix(0));
		MagentaSL->setPalette(sliderPix(120));
		YellowSL->setPalette(sliderPix(240));
		setValues();
		ColorMap->drawMode = 2;
		double L, a, b;
		m_color.getLab(&L, &a, &b);
		ColorMap->drawPalette(L * 2.55);
		ColorMap->setMark(a, b);
	}
	updateNewColorImage(m_color);
	NewC->setToolTip( "<qt>" + tr( "If color management is enabled, an exclamation mark indicates that the color may be outside of the color gamut of the current printer profile selected. What this means is the color may not print exactly as indicated on screen. More hints about gamut warnings are in the online help under Color Management." ) + "</qt>");
	OldC->setToolTip( "<qt>" + tr( "If color management is enabled, an exclamation mark indicates that the color may be outside of the color gamut of the current printer profile selected. What this means is the color may not print exactly as indicated on screen. More hints about gamut warnings are in the online help under Color Management." ) + "</qt>");
}

void CMYKChoose::setColor()
{
	int h, s, v;
	double treeItem, m, y;
	double k = 0;
	double L, a, b;
	ScColor tmp;
	if (m_color.getColorModel() == colorModelCMYK)
	{
		treeItem = CyanSp->value() / 100.0;
		m = MagentaSp->value() / 100.0;
		y = YellowSp->value() / 100.0;
		k = BlackSp->value() / 100.0;
		tmp.setColorF(treeItem, m, y, k);
		m_color = tmp;
		if (dynamic)
		{
			CyanSL->setPalette(sliderPix(180));
			MagentaSL->setPalette(sliderPix(300));
			YellowSL->setPalette(sliderPix(60));
			BlackSL->setPalette(sliderBlack());
		}
		BlackComp = qRound(k * 255.0);
		ScColorEngine::getRGBColor(tmp, m_doc).getHsv(&h, &s, &v);
		ColorMap->drawPalette(v);
		ColorMap->setMark(h, s);
	}
	else if (m_color.getColorModel() == colorModelRGB)
	{
		int ic = qRound(CyanSp->value());
		int im = qRound(MagentaSp->value());
		int iy = qRound(YellowSp->value());
		int ik = qRound(BlackSp->value());
		if (Wsave)
		{
			ic = ic / 51 * 51;
			im = im / 51 * 51;
			iy = iy / 51 * 51;
			blockSignals(true);
			CyanSp->setValue(ic);
			MagentaSp->setValue(im);
			YellowSp->setValue(iy);
			CyanSL->setValue(ic * 1000);
			MagentaSL->setValue(im * 1000);
			YellowSL->setValue(iy * 1000);
			blockSignals(false);
		}
		treeItem = ic / 255.0;
		m = im / 255.0;
		y = iy / 255.0;
		k = ik / 255.0;
		tmp.setRgbColorF(treeItem, m, y);
		QColor tmp2 = ScColorEngine::getRGBColor(tmp, m_doc);
		tmp2.getHsv(&h, &s, &v);
		BlackComp = 255 - v;
		m_color = tmp;
		if (dynamic)
		{
			CyanSL->setPalette(sliderPix(0));
			MagentaSL->setPalette(sliderPix(120));
			YellowSL->setPalette(sliderPix(240));
		}
		BlackComp = qRound(k * 255.0);
		ScColorEngine::getRGBColor(tmp, m_doc).getHsv(&h, &s, &v);
		ColorMap->drawPalette(v);
		ColorMap->setMark(h, s);
		QSignalBlocker sb(hexLineEdit);
		hexLineEdit->setText(tmp.name(false));
	}
	else if (m_color.getColorModel() == colorModelLab)
	{
		double Lalt;
		m_color.getLab(&Lalt, &a, &b);
		if (m_isHLC)
		{
			L = MagentaSp->value();
			double cv = 360 - CyanSp->value();
			double yv = YellowSp->value();
			QLineF lin = QLineF::fromPolar(yv, cv);
			a = lin.p2().x();
			b = lin.p2().y();
		}
		else
		{
			L = CyanSp->value();
			a = MagentaSp->value();
			b = YellowSp->value();
		}
		tmp.setLabColor(L, a, b);
		m_color = tmp;
		if (dynamic)
		{
			CyanSL->setPalette(sliderPix(0));
			MagentaSL->setPalette(sliderPix(120));
			YellowSL->setPalette(sliderPix(240));
		}
		BlackComp = qRound(L * 2.55);
		if (L != Lalt)
			ColorMap->drawPalette(L * 2.55);
		ColorMap->setMark(a, b);
	}
	updateNewColorImage(tmp);
}

void CMYKChoose::setColor2(int h, int s, bool end)
{
	ScColor tmp;
	if (m_color.getColorModel() == colorModelLab)
	{
		if (m_isHLC)
			tmp = ScColor(MagentaSp->value(), static_cast<double>(h), static_cast<double>(s));
		else
			tmp = ScColor(CyanSp->value(), static_cast<double>(h), static_cast<double>(s));
	}
	else
	{
		int r, g, b;
		int ih = qMax(0, qMin(h, 359));
		int is = qMax(0, qMin(255 - s, 255));
		int iv = 255 - BlackComp;
		QColor tm = QColor::fromHsv(ih, is, iv);
		tm.getRgb(&r, &g, &b);
		tmp.fromQColor(tm);
		if (m_color.getColorModel() == colorModelCMYK)
		{
			CMYKColorF cmyk;
			ScColorEngine::getCMYKValues(tmp, m_doc, cmyk);
			tmp.setColorF(cmyk.c, cmyk.m, cmyk.y, cmyk.k);
		}
		else
		{
			QSignalBlocker sb(hexLineEdit);
			hexLineEdit->setText(tmp.name(false));
		}
	}
	updateNewColorImage(tmp);
	m_color = tmp;
	if (end)
		setValues();
}

void CMYKChoose::selFromSwatch(int itemIndex)
{
	QSignalBlocker comboBox1Blocker(ComboBox1);
	QString colorName = ColorSwatch->text(itemIndex);
	ScColor tmp = CurrSwatch[colorName];
	if (isRegistration)
	{
		if (tmp.getColorModel() != colorModelCMYK)
			tmp = ScColorEngine::convertToModel(tmp, m_doc, colorModelCMYK);
		selModel( tr("CMYK"));
	}
	else
	{
		if (tmp.getColorModel() == colorModelCMYK)
		{
			ComboBox1->setCurrentIndex( 0 );
			selModel( tr("CMYK"));
		}
		else if (tmp.getColorModel() == colorModelRGB)
		{
			ComboBox1->setCurrentIndex( 1 );
			selModel( tr("RGB"));
		}
		else
		{
			ComboBox1->setCurrentIndex( 3 );
			selModel( tr("Lab"));
		}
	}
	updateNewColorImage(tmp);
	m_color = tmp;
	setValues();
	Separations->setChecked(tmp.isSpotColor());
	if (isNew && !ColorName->isModified())
		ColorName->setText(colorName);
}

void CMYKChoose::setValues()
{
	QSignalBlocker cyanSpBlocker(CyanSp);
	QSignalBlocker cyanSLBlocker(CyanSL);
	QSignalBlocker magentaSpBlocker(MagentaSp);
	QSignalBlocker magentaSLBlocker(MagentaSL);
	QSignalBlocker yellowSpBlocker(YellowSp);
	QSignalBlocker yellowSLBlocker(YellowSL);
	QSignalBlocker blackSpBlocker(BlackSp);
	QSignalBlocker blackSLBlocker(BlackSL);

	if (m_color.getColorModel() == colorModelCMYK)
	{
		CMYKColorF cmyk;
		double cc, cm, cy, ck;
		ScColorEngine::getCMYKValues(m_color, m_doc, cmyk);
		cmyk.getValues(cc, cm, cy, ck);
		CyanSp->setValue(cc * 100.0);
		CyanSL->setValue(qRound(cc * 100.0 * 1000.0));
		MagentaSp->setValue(cm * 100.0);
		MagentaSL->setValue(qRound(cm * 100.0 * 1000.0));
		YellowSp->setValue(cy * 100.0);
		YellowSL->setValue(qRound(cy * 100.0 * 1000.0));
		BlackSp->setValue(ck * 100.0);
		BlackSL->setValue(qRound(ck * 100.0 * 1000.0));
		if (dynamic)
		{
			CyanSL->setPalette(sliderPix(180));
			MagentaSL->setPalette(sliderPix(300));
			YellowSL->setPalette(sliderPix(60));
			BlackSL->setPalette(sliderBlack());
		}
	}
	else if (m_color.getColorModel() == colorModelRGB)
	{
		RGBColor rgb;
		int r, g, b;
		ScColorEngine::getRGBValues(m_color, m_doc, rgb);
		rgb.getValues(r, g, b);
		CyanSp->setValue(static_cast<double>(r));
		CyanSL->setValue(r * 1000);
		MagentaSp->setValue(static_cast<double>(g));
		MagentaSL->setValue(g * 1000);
		YellowSp->setValue(static_cast<double>(b));
		YellowSL->setValue(b * 1000);
		int h, s, v;
		ScColorEngine::getRGBColor(m_color, m_doc).getHsv(&h, &s, &v);
		BlackComp = 255 - v;
		if (dynamic)
		{
			CyanSL->setPalette(sliderPix(0));
			MagentaSL->setPalette(sliderPix(120));
			YellowSL->setPalette(sliderPix(240));
		}
		QSignalBlocker sb(hexLineEdit);
		hexLineEdit->setText(m_color.name(false));
	}
	else if (m_color.getColorModel() == colorModelLab)
	{
		double L, a, b;
		m_color.getLab(&L, &a, &b);
		if (m_isHLC)
		{
			MagentaSp->setValue(L);
			MagentaSL->setValue(L * 1000.0);
			QLineF lin;
			lin.setP1(QPointF(0.0, 0.0));
			lin.setP2(QPointF(a, b));
			CyanSp->setValue(360 - lin.angle());
			CyanSL->setValue(360 - lin.angle() * 1000.0);
			YellowSp->setValue(lin.length());
			YellowSL->setValue(lin.length() * 1000.0);
		}
		else
		{
			CyanSp->setValue(L);
			CyanSL->setValue(L * 1000.0);
			MagentaSp->setValue(a);
			MagentaSL->setValue(a * 1000.0);
			YellowSp->setValue(b);
			YellowSL->setValue(b * 1000.0);
		}
		if (dynamic)
		{
			CyanSL->setPalette(sliderPix(0));
			MagentaSL->setPalette(sliderPix(120));
			YellowSL->setPalette(sliderPix(240));
		}
	}
}

void CMYKChoose::setValueFromHex()
{
	QString hexValue(hexLineEdit->text().trimmed());
	if (!hexValue.startsWith("#"))
		hexValue.insert(0, QChar('#'));
	m_color.setNamedColor(hexValue);
	setValues();
	updateNewColorImage(m_color);
}

void CMYKChoose::showEvent(QShowEvent * event)
{
	if (!event->spontaneous())
		Swatches->setCurrentComboItem( tr( "Color Map" ));
	QDialog::showEvent(event);
}

void CMYKChoose::leave()
{
	// if condition 10/21/2004 pv #1191 - just be sure that user cannot create "None" color
	if (ColorName->text().isEmpty())
	{
		ScMessageBox::information(this, CommonStrings::trWarning, tr("You cannot create a color without a name.\nPlease give it a name"));
		ColorName->setFocus();
		ColorName->selectAll();
		return;
	}
	if (ColorName->text() == CommonStrings::None || ColorName->text() == CommonStrings::tr_NoneColor)
	{
		ScMessageBox::information(this, CommonStrings::trWarning, tr("You cannot create a color named \"%1\".\nIt is a reserved name for transparent color").arg(ColorName->text()));
		ColorName->setFocus();
		ColorName->selectAll();
		return;
	}
	if ((Fnam != ColorName->text()) || isNew)
	{
		if (EColors->contains(ColorName->text()))
		{
			ScMessageBox::information(this, CommonStrings::trWarning, tr("The name of the color already exists.\nPlease choose another one."));
			ColorName->selectAll();
			ColorName->setFocus();
			return;
		}
	}
	accept();
}

void CMYKChoose::updateNewColorImage(const ScColor& color)
{
	imageN.fill( ScColorEngine::getDisplayColor(color, m_doc));
	if (ScColorEngine::isOutOfGamut(color, m_doc))
		paintAlert(alertIcon, imageN, 2, 2);
	NewC->setPixmap( imageN );
}

