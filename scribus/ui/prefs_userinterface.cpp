/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QColorDialog>
#include <QFontDialog>
#include <QPixmap>
#include <QStyleFactory>
#include <QStyleHints>

#include "iconmanager.h"
#include "langmgr.h"
#include "prefs_userinterface.h"
#include "prefsstructs.h"
#include "scribusapp.h"
#include "scribusdoc.h"
#include "util.h"
#include "util_text.h"

Prefs_UserInterface::Prefs_UserInterface(QWidget* parent, ScribusDoc* /*doc*/)
	: Prefs_Pane(parent)
{
	setupUi(this);
	languageChange();
	m_caption = tr("User Interface");
	m_icon = "pref-ui";

	// qt styles
	QStringList styleList = QStyleFactory::keys();
	themeComboBox->addItem("");
	themeComboBox->addItems(styleList);
	QStringList iconSetList;
	iconSetList = IconManager::instance().nameList(ScQApp->currGUILanguage());
	iconSetComboBox->addItems(iconSetList);

	QStringList languageList;
	LanguageManager::instance()->fillInstalledGUIStringList(&languageList);
	if (languageList.isEmpty())
	{
		QString currentGUILang = ScQApp->currGUILanguage();
		if (!currentGUILang.isEmpty())
			languageList << LanguageManager::instance()->getLangFromAbbrev(currentGUILang);
		else
			languageList << LanguageManager::instance()->getLangFromAbbrev("en_GB");
	}
	std::sort(languageList.begin(), languageList.end(), localeAwareLessThan);
	languageComboBox->addItems(languageList);

	numberFormatComboBox->addItem(tr("Use System Format"),"System");
	numberFormatComboBox->addItem(tr("Use Interface Language Format"),"Language");

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
	themePaletteComboBox->setVisible(false);
	themePaletteLabel->setVisible(false);
#endif

	connect(languageComboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(setSelectedGUILang(QString)));
	connect(storyEditorFontPushButton, SIGNAL(clicked()), this, SLOT(changeStoryEditorFont()));
}

Prefs_UserInterface::~Prefs_UserInterface() = default;

void Prefs_UserInterface::languageChange()
{
	themePaletteComboBox->clear();
	themePaletteComboBox->addItem(tr("Auto"), "auto");
	themePaletteComboBox->addItem(tr("Light"), "light");
	themePaletteComboBox->addItem(tr("Dark"), "dark");
	themePaletteComboBox->setToolTip( "<qt>" + tr( "Choose the default theme palette. Auto uses the systems default." ) + "</qt>");

	themeComboBox->setToolTip( "<qt>" + tr( "Choose the default window decoration and looks. Scribus inherits any available KDE or Qt themes, if Qt is configured to search KDE plugins." ) + "</qt>");
	iconSetComboBox->setToolTip( "<qt>" + tr( "Choose the default icon set" ) + "</qt>");
	useSmallWidgetsCheckBox->setToolTip( "<qt>" + tr( "Palette windows will use smaller (space savvy) widgets. Requires application restart." ) + "</qt>");
	recentDocumentsSpinBox->setToolTip( "<qt>" + tr( "Number of recently edited documents to show in the File menu" ) + "</qt>");
	languageComboBox->setToolTip( "<qt>" + tr( "Select your default language for Scribus to run with. Leave this blank to choose based on environment variables. You can still override this by passing a command line option when starting Scribus." )+"</qt>");
	numberFormatComboBox->setToolTip( "<qt>" + tr( "Use either the system or selected language related definition for number formats for decimals for numbers in the interface" ) + "</qt>");
	fontSizeMenuSpinBox->setToolTip( "<qt>" + tr( "Default font size for the menus and windows" ) + "</qt>");
	fontSizePaletteSpinBox->setToolTip( "<qt>" + tr( "Default font size for the tool windows" ) + "</qt>");
	resizeMoveDelaySpinBox->setToolTip( "<qt>" + tr( "Time before resize or move starts allows for a slight delay between when you click and the operation happens to avoid unintended moves. This can be helpful when dealing with mouse sensitivity settings or accessibility issues related to ergonomic mice, touch pads or moveability of the wrists and hands." ) + "</qt>");
	wheelJumpSpinBox->setToolTip( "<qt>" + tr( "Number of lines Scribus will scroll for each \"notch\" of the mouse wheel" ) + "</qt>");
	//showSplashCheckBox->setToolTip( "<qt>" + tr( "" ) + "</qt>");
	//showStartupDialogCheckBox->setToolTip( "<qt>" + tr( "" ) + "</qt>");
	storyEditorUseSmartSelectionCheckBox->setToolTip( "<qt>" + tr( "The default behavior when double-clicking on a word is to select the word and the first following space. Smart selection will select only the word, without the following space." ) + "</qt>");
	showLabels->setToolTip("<qt>" + tr( "Shows informational labels on palettes." ) + "</qt>");
	showLabelsOfInactiveTabs->setToolTip("<qt>" + tr( "Shows labels of inactive palette tabs." ) + "</qt>");
}

void Prefs_UserInterface::restoreDefaults(struct ApplicationPrefs *prefsData)
{
	selectedGUILang = prefsData->uiPrefs.language;
	if (selectedGUILang.isEmpty())
		selectedGUILang = ScQApp->currGUILanguage();
	QString langString = LanguageManager::instance()->getLangFromAbbrev(selectedGUILang);
	if (languageComboBox->findText(langString) < 0)
	{
		selectedGUILang = ScQApp->currGUILanguage();
		langString = LanguageManager::instance()->getLangFromAbbrev(selectedGUILang);
	}
	if (languageComboBox->findText(langString) < 0)
	{
		selectedGUILang = "en_GB";
		langString = LanguageManager::instance()->getLangFromAbbrev(selectedGUILang);
	}

	int index = themePaletteComboBox->findData(prefsData->uiPrefs.stylePalette);
	if ( index != -1 )
		themePaletteComboBox->setCurrentIndex(index);
	else
		themePaletteComboBox->setCurrentIndex(0);

	setCurrentComboItem(languageComboBox, langString);
	numberFormatComboBox->setCurrentIndex(prefsData->uiPrefs.userPreferredLocale == "System" ? 0 : 1);
	setCurrentComboItem(themeComboBox, prefsData->uiPrefs.style);
	setCurrentComboItem(iconSetComboBox, prefsData->uiPrefs.iconSet);
	fontSizeMenuSpinBox->setValue( prefsData->uiPrefs.applicationFontSize );
	fontSizePaletteSpinBox->setValue( prefsData->uiPrefs.paletteFontSize);
	wheelJumpSpinBox->setValue( prefsData->uiPrefs.wheelJump );
	resizeMoveDelaySpinBox->setValue(prefsData->uiPrefs.mouseMoveTimeout);
	recentDocumentsSpinBox->setValue( prefsData->uiPrefs.recentDocCount );
	showStartupDialogCheckBox->setChecked(prefsData->uiPrefs.showStartupDialog);
	useTabsForDocumentsCheckBox->setChecked(prefsData->uiPrefs.useTabs);
	showSplashCheckBox->setChecked(prefsData->uiPrefs.showSplashOnStartup);
	useSmallWidgetsCheckBox->setChecked(prefsData->uiPrefs.useSmallWidgets);
	showLabels->setChecked(prefsData->uiPrefs.showLabels);
	showLabelsOfInactiveTabs->setChecked(prefsData->uiPrefs.showLabelsOfInactiveTabs);

	storyEditorUseSmartSelectionCheckBox->setChecked(prefsData->storyEditorPrefs.smartTextSelection);
	seFont.fromString(prefsData->storyEditorPrefs.guiFont);
	storyEditorFontPushButton->setText(seFont.family());
}

void Prefs_UserInterface::saveGuiToPrefs(struct ApplicationPrefs *prefsData) const
{
	prefsData->uiPrefs.language = selectedGUILang;
	prefsData->uiPrefs.userPreferredLocale = numberFormatComboBox->currentData().toString();
	prefsData->uiPrefs.style = themeComboBox->currentText();
	prefsData->uiPrefs.stylePalette = themePaletteComboBox->currentData().toString();
	prefsData->uiPrefs.iconSet = IconManager::instance().baseNameForTranslation(iconSetComboBox->currentText());
	prefsData->uiPrefs.applicationFontSize = fontSizeMenuSpinBox->value();
	prefsData->uiPrefs.paletteFontSize = fontSizePaletteSpinBox->value();
	prefsData->uiPrefs.wheelJump = wheelJumpSpinBox->value();
	prefsData->uiPrefs.mouseMoveTimeout = resizeMoveDelaySpinBox->value();
	prefsData->uiPrefs.recentDocCount = recentDocumentsSpinBox->value();
	prefsData->uiPrefs.showStartupDialog = showStartupDialogCheckBox->isChecked();
	prefsData->uiPrefs.useTabs = useTabsForDocumentsCheckBox->isChecked();
	prefsData->uiPrefs.showSplashOnStartup = showSplashCheckBox->isChecked();
	prefsData->uiPrefs.useSmallWidgets = useSmallWidgetsCheckBox->isChecked();
	prefsData->uiPrefs.showLabels = showLabels->isChecked();
	prefsData->uiPrefs.showLabelsOfInactiveTabs = showLabelsOfInactiveTabs->isChecked();

	prefsData->storyEditorPrefs.guiFont = seFont.toString();
	prefsData->storyEditorPrefs.smartTextSelection = storyEditorUseSmartSelectionCheckBox->isChecked();

}

void Prefs_UserInterface::setSelectedGUILang(const QString &newLang)
{
	selectedGUILang = LanguageManager::instance()->getAbbrevFromLang(newLang);
}

void Prefs_UserInterface::changeStoryEditorFont()
{
	bool ok;
	QFont newFont(QFontDialog::getFont( &ok, seFont, this ));
	if (!ok)
		return;
	seFont = newFont;
	storyEditorFontPushButton->setText(seFont.family());
}

