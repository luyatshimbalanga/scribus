/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          pageitem.h  -  description
                             -------------------
    copyright            : Scribus Team
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "scribuscore.h"

#include <cassert>
#include <iostream>

#include <QByteArray>
#include <QDebug>
#include <QGlobalStatic>
#include <QMessageBox>
#include <QScreen>
#include <QStyleFactory>
#include <QStyleHints>

#include "colormgmt/sccolormgmtenginefactory.h"
#include "commonstrings.h"
#include "filewatcher.h"
#include "iconmanager.h"
#include "langmgr.h"
#include "localemgr.h"
#include "pluginmanager.h"
#include "prefsmanager.h"
#include "scimagecachemanager.h"
#include "scpaths.h"
#include "scribus.h"
#include "scribusapp.h"
#include "ui/splash.h"
#include "undomanager.h"
#include "util_debug.h"
#include "util_ghostscript.h"

//extern ScribusQApp* ScQApp;

ScribusCore::ScribusCore() : defaultEngine(colorMgmtEngineFactory.createDefaultEngine()),
							m_iconManager(IconManager::instance()),
							m_prefsManager(PrefsManager::instance())
{
	ScColorMgmtStrategy strategy;
	strategy.setUseBlackPointCompensation(true);
	strategy.setUseBlackPreservation(false);
	defaultEngine.setStrategy(strategy);
}

ScribusCore::~ScribusCore()
{

	while (m_ScMWList.count() > 0)
	{
		ScribusMainWindow *mainWindow = m_ScMWList.takeAt(0);
		delete mainWindow;
	}

	delete pluginManager;
}

#ifndef NDEBUG
static void abort_on_error(QtMsgType t, const QMessageLogContext&, const QString & m)
{
	std::cerr << m.toLocal8Bit().data() << "\n";
	if(t == QtFatalMsg) assert(false);
//	if(t != QtDebugMsg) assert(false);
}
#endif

int ScribusCore::init(bool useGUI, const QList<QString>& filesToUse)
{
	m_useGUI = useGUI;
	m_Files = filesToUse;
#if !defined(NDEBUG) && !defined(_WIN32)
	qInstallMessageHandler( & abort_on_error );
#endif
	return 0;
}

int ScribusCore::startGUI(bool showSplash, bool showFontInfo, bool showProfileInfo, const QString& newGuiLanguage)
{
	auto* scribus = new ScribusMainWindow();
	Q_CHECK_PTR(scribus);
	if (!scribus)
		return EXIT_FAILURE;
	m_ScMWList.append(scribus);
	int retVal = initScribusCore(showSplash, showFontInfo, showProfileInfo, newGuiLanguage);
	if (retVal == EXIT_FAILURE)
		return EXIT_FAILURE;

	retVal = scribus->initScMW(true);
	if (retVal == EXIT_FAILURE)
		return EXIT_FAILURE;

	closeSplash();
	m_scribusInitialized = true;
	connect(ScQApp, SIGNAL(lastWindowClosed()), ScQApp, SLOT(quit()));

	scribus->show();
	scribus->setupMainWindow();

	QStringList recoverFiles = scribus->findRecoverableFile();
	int subsRet = scribus->ShowSubs();
	if (subsRet == 0)
	{
		if (!m_Files.isEmpty())
		{
			for (int i = 0; i < m_Files.size(); ++i)
				scribus->loadDoc(m_Files.at(i));
		}
		else if ((recoverFiles.count() > 0) && usingGUI())
		{
			if (!scribus->recoverFile(recoverFiles))
			{
				if (PrefsManager::instance().appPrefs.uiPrefs.showStartupDialog)
					scribus->startUpDialog();
			}
		}
		else
		{
			if (PrefsManager::instance().appPrefs.uiPrefs.showStartupDialog && usingGUI())
				scribus->startUpDialog();
			else
				scribus->setFocus();
		}
	}
	else if (subsRet == QMessageBox::Help)
	{
		scribus->slotRaiseOnlineHelp();
	}

	return EXIT_SUCCESS;
}

int ScribusCore::initScribusCore(bool showSplash, bool showFontInfo, bool showProfileInfo, const QString& newGuiLanguage)
{
	CommonStrings::languageChange();
	LanguageManager::instance()->languageChange();

	if (!m_iconManager.setup())
		return EXIT_FAILURE;

	// FIXME: Splash needs the prefs loaded by initDefaults() to know if it must force the image to grayscale
	initSplash(showSplash);
	LocaleManager::instance();
	m_prefsManager.setup();
	//CB #4428 Get fonts before prefs are set to default
	bool haveFonts = false;
	haveFonts = ScCore->initFonts(showFontInfo);
	if (!haveFonts)
		return EXIT_FAILURE;
	m_prefsManager.initDefaults();
	m_prefsManager.initDefaultGUIFont(QApplication::font());
	m_prefsManager.initArrowStyles();
	m_undoManager = UndoManager::instance();
	fileWatcher = new FileWatcher(this);
	pluginManager = new PluginManager();

	setSplashStatus( tr("Initializing Keyboard Shortcuts") );
	m_prefsManager.initDefaultActionKeys();
	setSplashStatus( tr("Reading Preferences") );
	m_prefsManager.readPrefs();

	// This is a very basic implemenation of the UI theme palette.
	// If necessary implement platform specific theme configurations here.

	// Apply different palette only if the system UI palette is different.
#if (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
	if (m_prefsManager.appPrefs.uiPrefs.stylePalette == "dark" && QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light)
	{
		QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);

	// https://bugreports.qt.io/browse/QTBUG-132929
#if (defined Q_OS_LINUX)
		QPalette pal(QColor(49, 49, 49));
		QApplication::setPalette(pal);
#endif
		m_SplashScreen->setPixmap(m_iconManager.splashScreen());
	}
	else if (m_prefsManager.appPrefs.uiPrefs.stylePalette == "light" && QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark)
	{
		QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);

	// https://bugreports.qt.io/browse/QTBUG-132929
#if (defined Q_OS_LINUX)
		QPalette pal(QColor(239, 239, 239));
		QApplication::setPalette(pal);
#endif
		m_SplashScreen->setPixmap(m_iconManager.splashScreen());
	}
#endif

	// Configure GUI
	const QString& prefsUiStyle = m_prefsManager.appPrefs.uiPrefs.style;
	if (prefsUiStyle.length() > 0)
	{
		QStyle* qtStyle = nullptr;
		QStringList availableStyles = QStyleFactory::keys();
		if (availableStyles.contains(prefsUiStyle))
			qtStyle = QStyleFactory::create(prefsUiStyle);
		if (qtStyle)
			QApplication::setStyle(qtStyle);
		else
			m_prefsManager.appPrefs.uiPrefs.style.clear();
	}
	QFont apf = QApplication::font();
	apf.setPointSize(m_prefsManager.appPrefs.uiPrefs.applicationFontSize);
	QApplication::setFont(apf);

	LocaleManager::instance().setUserPreferredLocale(m_prefsManager.appPrefs.uiPrefs.userPreferredLocale);
	m_prefsManager.appPrefs.uiPrefs.showSplashOnStartup = showSplash;
	if (!m_iconManager.setActiveFromPrefs(m_prefsManager.appPrefs.uiPrefs.iconSet))
	{
		//reset prefs name to chosen name based on version, when prefs is empty or not found
		m_prefsManager.appPrefs.uiPrefs.iconSet = m_iconManager.activeSetBasename();
	}

	m_haveGS = testGSAvailability();
	m_havePNGAlpha = testGSDeviceAvailability("pngalpha");
	m_haveTiffSep = testGSDeviceAvailability("tiffsep");
	setSplashStatus( tr("Initializing Plugins") );
	pluginManager->initPlugs();
	
	ScCore->setSplashStatus( tr("Reading Color Profiles") );
	m_haveCMS = false;
	getCMSProfiles(showProfileInfo);
	initCMS();

	setSplashStatus( tr("Initializing Image Cache") );
	ScImageCacheManager & icm = ScImageCacheManager::instance();
	icm.setEnabled(m_prefsManager.appPrefs.imageCachePrefs.cacheEnabled);
	icm.setMaxCacheSizeMiB(m_prefsManager.appPrefs.imageCachePrefs.maxCacheSizeMiB);
	icm.setMaxCacheEntries(m_prefsManager.appPrefs.imageCachePrefs.maxCacheEntries);
	icm.setCompressionLevel(m_prefsManager.appPrefs.imageCachePrefs.compressionLevel);
	icm.initialize();
	return 0;
}

const QString &ScribusCore::getGuiLanguage() const
{
	return ScQApp->currGUILanguage();
}

void ScribusCore::initSplash(bool showSplash)
{
	m_SplashScreen = nullptr;
	if (!showSplash)
		return;

	QScreen* primaryScreeen = QApplication::primaryScreen();
	double pixelRatio = primaryScreeen ? primaryScreeen->devicePixelRatio() : 1.0;
	QPixmap pix = IconManager::instance().splashScreen();
	if (pixelRatio != 1.0)
	{
		int w = qRound(pix.width() * pixelRatio);
		int h = qRound(pix.height() * pixelRatio);
		double integralPart = 0;
		bool isIntegerRatio = (modf(pixelRatio, &integralPart) == 0.0);
		pix = pix.scaled(w, h, Qt::IgnoreAspectRatio, isIntegerRatio ? Qt::FastTransformation : Qt::SmoothTransformation);
		pix.setDevicePixelRatio(pixelRatio);
	}

	m_SplashScreen = new ScSplashScreen(pix, IconManager::instance().splashScreenRect(), Qt::WindowStaysOnTopHint);
	if (m_SplashScreen != nullptr)
	{
		m_SplashScreen->show();
		if (m_SplashScreen->isVisible())
			setSplashStatus(QObject::tr("Initializing..."));
	}
}

void ScribusCore::setSplashStatus(const QString& newText)
{
	if (m_SplashScreen != nullptr)
		m_SplashScreen->setStatus( newText );
}

void ScribusCore::showSplash(bool shown)
{
	if (m_SplashScreen != nullptr && shown != m_SplashScreen->isVisible())
		m_SplashScreen->setVisible(shown);
}

bool ScribusCore::splashShowing() const
{
	if (m_SplashScreen == nullptr)
		return false;
	return m_SplashScreen->isVisible();
}

void ScribusCore::closeSplash()
{
	if (m_SplashScreen == nullptr)
		return;
	m_SplashScreen->close();
	delete m_SplashScreen;
	m_SplashScreen = nullptr;
}

bool ScribusCore::usingGUI() const
{
	return m_useGUI;
}

bool ScribusCore::isMacGUI() const
{
	// Do it statically for now
#if defined(Q_OS_MACOS)
	return true;
#else
	return false;
#endif
}

bool ScribusCore::isWinGUI() const
{
	// Do it statically for now
#if defined(_WIN32)
	return true;
#else
	return false;
#endif
}

//Returns false when there are no fonts
bool ScribusCore::initFonts(bool showFontInfo)
{
	setSplashStatus( tr("Searching for Fonts") );
	bool haveFonts = m_prefsManager.GetAllFonts(showFontInfo);
	if (!haveFonts)
	{
		closeSplash();
		QString mess = tr("There are no fonts found on your system.");
		mess += "\n" + tr("Exiting now.");
		ScMessageBox::critical(nullptr, tr("Fatal Error"), mess);
	}
	else
		setSplashStatus( tr("Font System Initialized") );
	return haveFonts;
}

void ScribusCore::getCMSProfiles(bool showInfo)
{
	MonitorProfiles.clear();
	PrinterProfiles.clear();
	InputProfiles.clear();
	InputProfilesCMYK.clear();
	LabProfiles.clear();

	QStringList profDirs = ScPaths::systemProfilesDirs();
	profDirs.prepend( m_prefsManager.appPrefs.pathPrefs.colorProfiles );
	profDirs.prepend( ScPaths::instance().shareDir() + "profiles/");
	for (int i = 0; i < profDirs.count(); i++)
	{
		QString profDir = profDirs[i];
		if (profDir.isEmpty())
			continue;
		if (profDir.right(1) != "/")
			profDir += "/";
		getCMSProfilesDir(profDir, showInfo, true);
	}
	m_haveCMS = (!PrinterProfiles.isEmpty()) && (!InputProfiles.isEmpty()) && (!MonitorProfiles.isEmpty());
}

void ScribusCore::getCMSProfilesDir(const QString& pfad, bool showInfo, bool recursive)
{
	QList<ScColorProfileInfo> profileInfos = defaultEngine.getAvailableProfileInfo(pfad, recursive);
	for (int i = 0; i < profileInfos.count(); ++i)
	{
		const ScColorProfileInfo& profInfo = profileInfos.at(i);
		QString profileName = profInfo.description;
		if (profileName.isEmpty())
		{
			if (showInfo)
				sDebug(QString("Color profile %1 is broken : no valid description").arg(profInfo.file));
			continue;
		}
		if (!profInfo.debug.isEmpty())
		{
			if (showInfo)
				sDebug(profInfo.debug);
			continue;
		}
		switch (profInfo.deviceClass)
		{
		case Class_Input:
			if (profInfo.colorSpace == ColorSpace_Rgb)
			{
				if (!InputProfiles.contains(profileName))
					InputProfiles.insert(profileName, profInfo);
			}
			break;
		case Class_ColorSpace:
			if (profInfo.colorSpace == ColorSpace_Rgb)
			{
				if (!InputProfiles.contains(profileName))
					InputProfiles.insert(profileName, profInfo);
			}
			if (profInfo.colorSpace == ColorSpace_Cmyk)
			{
				if (!InputProfilesCMYK.contains(profileName))
					InputProfilesCMYK.insert(profileName, profInfo);
			}
			if (profInfo.colorSpace == ColorSpace_Lab)
			{
				if (!LabProfiles.contains(profileName))
					LabProfiles.insert(profileName, profInfo);
			}
			break;
		case Class_Display:
			if (profInfo.colorSpace == ColorSpace_Rgb)
			{
				if (!MonitorProfiles.contains(profileName))
					MonitorProfiles.insert(profileName, profInfo);
				if (!InputProfiles.contains(profileName))
					InputProfiles.insert(profileName, profInfo);
			}
			if (profInfo.colorSpace == ColorSpace_Cmyk)
			{
				if (!InputProfilesCMYK.contains(profileName))
					InputProfilesCMYK.insert(profileName, profInfo);
			}
			if (profInfo.colorSpace == ColorSpace_Lab)
			{
				if (!LabProfiles.contains(profileName))
					LabProfiles.insert(profileName, profInfo);
			}
			break;
		case Class_Output:
			// Disable rgb printer profile detection until effective support
			// PrinterProfiles.insert(nam, pfad + d[dc], false);
			if (profInfo.colorSpace == ColorSpace_Cmyk)
			{
				if (!PDFXProfiles.contains(profileName))
					PDFXProfiles.insert(profileName, profInfo);
				if (!InputProfilesCMYK.contains(profileName))
					InputProfilesCMYK.insert(profileName, profInfo);
				if (!PrinterProfiles.contains(profileName))
					PrinterProfiles.insert(profileName, profInfo);
			}
			break;
		}
		if (showInfo)
			sDebug( QString("Color profile %1 loaded from %2").arg(profileName, profInfo.file) );
	}
}

void ScribusCore::InitDefaultColorTransforms()
{
	QString defaultRGBString;
	QString defaultRGBString1("sRGB display profile (ICC v2.2)");
	QString defaultRGBString2("sRGB IEC61966-2.1");
	QString defaultCMYKString1("ISO Coated v2 300% (basICColor)");
	QString defaultCMYKString2("Fogra27L CMYK Coated Press");

	// Open the default RGB profile
	if (InputProfiles.contains(defaultRGBString1))
	{
		defaultRGBProfile = defaultEngine.openProfileFromFile(InputProfiles[defaultRGBString1].file);
		defaultRGBString = defaultRGBString1;
	}
	else if (InputProfiles.contains(defaultRGBString2))
	{
		defaultRGBProfile = defaultEngine.openProfileFromFile(InputProfiles[defaultRGBString2].file);
		defaultRGBString = defaultRGBString2;
	}
	else
	{
		defaultRGBProfile = defaultEngine.createProfile_sRGB();
		defaultRGBString = defaultRGBString2;
	}

	// Open the default CMYK profile
	if (InputProfilesCMYK.contains(defaultCMYKString1))
		defaultCMYKProfile = defaultEngine.openProfileFromFile(InputProfilesCMYK[defaultCMYKString1].file);
	else if (InputProfilesCMYK.contains(defaultCMYKString2))
		defaultCMYKProfile = defaultEngine.openProfileFromFile(InputProfilesCMYK[defaultCMYKString2].file);

	// Keep all chance to have monitor profile set
	monitorProfile = defaultRGBProfile;

	if (!defaultRGBProfile || !defaultCMYKProfile)
	{
		ResetDefaultColorTransforms();
		return;
	}

	// Default rgb profile may be a memory profile, if it is the case add it to the lists of 
	// rgb profiles (input and monitor) so that it can be used later in prefs
	if (!InputProfiles.contains(defaultRGBString))
	{
		ScColorProfileInfo profileInfo = defaultRGBProfile.info();
		InputProfiles.insert(defaultRGBString, profileInfo);
	}
	if (!MonitorProfiles.contains(defaultRGBString))
	{
		ScColorProfileInfo profileInfo = defaultRGBProfile.info();
		MonitorProfiles.insert(defaultRGBString, profileInfo);
	}

	// Open monitor profile as defined by user preferences
	QString displayProfile = m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultMonitorProfile;
	if (MonitorProfiles.contains(displayProfile))
		monitorProfile = defaultEngine.openProfileFromFile( MonitorProfiles[displayProfile].file);
	if (monitorProfile.isNull())
	{
		m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultMonitorProfile = defaultRGBString;
		monitorProfile = defaultRGBProfile;
	}

	// Now create default color transforms (used mainly when color management is "disabled")
	int dcmsFlags        = Ctf_BlackPointCompensation;
	eRenderIntent intent = Intent_Relative_Colorimetric;
//	if (!LabProfiles.isEmpty())
//		defaultLabProfile = defaultEngine.openProfileFromFile(LabProfiles.first());
//	else
		defaultLabProfile = defaultEngine.createProfile_Lab();

	defaultRGBToScreenSolidTrans  = defaultEngine.createTransform(defaultRGBProfile, Format_RGB_16,  defaultRGBProfile, Format_RGB_16, intent, dcmsFlags);
	defaultRGBToScreenImageTrans  = defaultEngine.createTransform(defaultRGBProfile, Format_RGBA_8,  defaultRGBProfile, Format_RGBA_8, intent, dcmsFlags);
	defaultCMYKToScreenImageTrans = defaultEngine.createTransform(defaultCMYKProfile, Format_CMYK_8, defaultRGBProfile, Format_RGBA_8, intent, dcmsFlags);
	defaultRGBToCMYKTrans         = defaultEngine.createTransform(defaultRGBProfile, Format_RGB_16, defaultCMYKProfile, Format_CMYK_16, intent, dcmsFlags);
	defaultCMYKToRGBTrans         = defaultEngine.createTransform(defaultCMYKProfile, Format_CMYK_16, defaultRGBProfile, Format_RGB_16, intent, dcmsFlags);
	defaultLabToRGBTrans          = defaultEngine.createTransform(defaultLabProfile, Format_Lab_Dbl, defaultRGBProfile, Format_RGB_16, Intent_Absolute_Colorimetric, dcmsFlags);
	defaultLabToCMYKTrans         = defaultEngine.createTransform(defaultLabProfile, Format_Lab_Dbl, defaultCMYKProfile, Format_CMYK_16, Intent_Absolute_Colorimetric, dcmsFlags);
	defaultLabToScreenTrans       = defaultLabToRGBTrans;
	if (!defaultRGBToScreenSolidTrans  || !defaultRGBToScreenImageTrans || 
		!defaultCMYKToScreenImageTrans || !defaultRGBToCMYKTrans || 
		!defaultCMYKToRGBTrans || !defaultLabToRGBTrans|| !defaultLabToCMYKTrans)
	{
		ResetDefaultColorTransforms();
	}
}

void ScribusCore::ResetDefaultColorTransforms()
{
	defaultRGBProfile  = ScColorProfile();
	defaultCMYKProfile = ScColorProfile();
	defaultRGBToScreenSolidTrans = ScColorTransform();
	defaultRGBToScreenImageTrans = ScColorTransform();
	defaultCMYKToScreenImageTrans = ScColorTransform();
	defaultRGBToCMYKTrans = ScColorTransform();
	defaultCMYKToRGBTrans = ScColorTransform();
	defaultLabToRGBTrans = ScColorTransform();
	defaultLabToCMYKTrans = ScColorTransform();
	defaultLabToScreenTrans = ScColorTransform();
}

void ScribusCore::initCMS()
{
	if (!m_haveCMS)
		return;

	ScProfileInfoMap::Iterator ip;
	QString defaultRGBString1 = "sRGB display profile (ICC v2.2)";
	QString defaultRGBString2 = "sRGB IEC61966-2.1";
	QString defaultCMYKString1 = "ISO Coated v2 300% (basICColor)";
	QString defaultCMYKString2 = "Fogra27L CMYK Coated Press";

	QString defaultImageRGBProfile = m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultImageRGBProfile;
	if ((defaultImageRGBProfile.isEmpty()) || (!InputProfiles.contains(defaultImageRGBProfile)))
	{
		ip = InputProfiles.find(defaultRGBString1);
		if (ip == InputProfiles.end())
			ip = InputProfiles.find(defaultRGBString2);
		if (ip == InputProfiles.end())
			ip = InputProfiles.begin();
		m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultImageRGBProfile = ip.key();
	}

	QString defaultImageCMYKProfile = m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultImageCMYKProfile;
	if ((defaultImageCMYKProfile.isEmpty()) || (!InputProfilesCMYK.contains(defaultImageCMYKProfile)))
	{
		ip = InputProfilesCMYK.find(defaultCMYKString1);
		if (ip == InputProfilesCMYK.end())
			ip = InputProfilesCMYK.find(defaultCMYKString2);
		if (ip == InputProfilesCMYK.end())
			ip = InputProfilesCMYK.begin();
		m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultImageCMYKProfile = ip.key();
	}

	QString defaultSolidColorRGBProfile = m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultSolidColorRGBProfile;
	if ((defaultSolidColorRGBProfile.isEmpty()) || (!InputProfiles.contains(defaultSolidColorRGBProfile)))
	{
		ip = InputProfiles.find(defaultRGBString1);
		if (ip == InputProfiles.end())
			ip = InputProfiles.find(defaultRGBString2);
		if (ip == InputProfiles.end())
			ip = InputProfiles.begin();
		m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultSolidColorRGBProfile = ip.key();
	}

	QString defaultSolidColorCMYKProfile = m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultSolidColorCMYKProfile;
	if ((defaultSolidColorCMYKProfile.isEmpty()) || (!InputProfilesCMYK.contains(defaultSolidColorCMYKProfile)))
	{
		ip = InputProfilesCMYK.find(defaultCMYKString1);
		if (ip == InputProfilesCMYK.end())
			ip = InputProfilesCMYK.find(defaultCMYKString2);
		if (ip == InputProfilesCMYK.end())
			ip = InputProfilesCMYK.begin();
		m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultSolidColorCMYKProfile = ip.key();
	}

	QString defaultMonitorProfile = m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultMonitorProfile;
	if ((defaultMonitorProfile.isEmpty()) || (!MonitorProfiles.contains(defaultMonitorProfile)))
	{
		ip = MonitorProfiles.find(defaultRGBString1);
		if (ip == MonitorProfiles.end())
			ip = MonitorProfiles.find(defaultRGBString2);
		if (ip == MonitorProfiles.end())
			ip = MonitorProfiles.begin();
		m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultMonitorProfile = ip.key();
	}

	QString defaultPrinterProfile = m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultPrinterProfile;
	if ((defaultPrinterProfile.isEmpty()) || (!PrinterProfiles.contains(defaultPrinterProfile)))
	{
		ip = PrinterProfiles.find(defaultCMYKString1);
		if (ip == PrinterProfiles.end())
			ip = PrinterProfiles.find(defaultCMYKString2);
		if (ip == PrinterProfiles.end())
			ip = PrinterProfiles.begin();
		m_prefsManager.appPrefs.colorPrefs.DCMSset.DefaultPrinterProfile = ip.key();
	}

	InitDefaultColorTransforms();
}

ScribusMainWindow * ScribusCore::primaryMainWindow()
{
	if (m_ScMWList.count() == 0 || m_currScMW > m_ScMWList.count())
		return nullptr;
	ScribusMainWindow* mw = m_ScMWList.at(m_currScMW);
	if (!mw)
		return nullptr;
	return mw;
}

void ScribusCore::recheckGS()
{
	m_haveGS = testGSAvailability();
	m_havePNGAlpha = testGSDeviceAvailability("pngalpha");
	m_haveTiffSep = testGSDeviceAvailability("tiffsep");
}

bool ScribusCore::fileWatcherActive() const
{
	if (fileWatcher != nullptr)
		return fileWatcher->isActive();
	return false;
}
