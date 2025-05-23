/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

// include the python header first because on OSX that uses the name "slots"
#include "cmdvar.h"

#include <memory>
#include <QColorDialog>
#include <QFileDialog>
#include <QListWidgetItem>
#include <QPalette>

#include "prefs_scripter.h"
#include "scriptercore.h"
#include "prefsfile.h"
#include "prefscontext.h"
#include "prefsmanager.h"
#include "prefsstructs.h"

Prefs_Scripter::Prefs_Scripter(QWidget* parent)
	: Prefs_Pane(parent)
{
	setupUi(this);
	languageChange();

	m_caption = tr("Scripter");
	m_icon = "pref-scripter";

	setupSyntaxColors();

	// Set the state of the ext script enable checkbox
	extensionScriptsChk->setChecked(scripterCore->extensionsEnabled());
	// The startup script box should be disabled  if ext scripts are off
	startupScriptEdit->setEnabled(extensionScriptsChk->isChecked());
	startupScriptEdit->setText(scripterCore->startupScript());
	scriptPathsListWidget->addItems(scripterCore->scriptPaths.get());
	// signals and slots connections
	connect(extensionScriptsChk, SIGNAL(toggled(bool)), startupScriptEdit, SLOT(setEnabled(bool)));
	// colors
	connect(textButton, SIGNAL(clicked()), this, SLOT(setColor()));
	connect(commentButton, SIGNAL(clicked()), this, SLOT(setColor()));
	connect(keywordButton, SIGNAL(clicked()), this, SLOT(setColor()));
	connect(errorButton, SIGNAL(clicked()), this, SLOT(setColor()));
	connect(signButton, SIGNAL(clicked()), this, SLOT(setColor()));
	connect(stringButton, SIGNAL(clicked()), this, SLOT(setColor()));
	connect(numberButton, SIGNAL(clicked()), this, SLOT(setColor()));
	connect(extensionScriptsChk, SIGNAL(toggled(bool)), startupScriptChangeButton, SLOT(setEnabled(bool)));
	connect(startupScriptChangeButton, SIGNAL(clicked()), this, SLOT(changeStartupScript()));

	changePathButton->setEnabled(false);
	removePathButton->setEnabled(false);
	connect(scriptPathsListWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(selectPath(QListWidgetItem*)));
	connect(addPathButton, SIGNAL(clicked()), this, SLOT(addPath()));
	connect(changePathButton, SIGNAL(clicked()), this, SLOT(changePath()));
	connect(removePathButton, SIGNAL(clicked()), this, SLOT(removePath()));


}

Prefs_Scripter::~Prefs_Scripter() = default;

void Prefs_Scripter::languageChange()
{
}

void Prefs_Scripter::restoreDefaults(struct ApplicationPrefs *prefsData)
{
}

void Prefs_Scripter::saveGuiToPrefs(struct ApplicationPrefs *prefsData) const
{
}

// Apply changes to prefs. Auto connected.
void Prefs_Scripter::apply()
{
	scripterCore->setExtensionsEnabled(extensionScriptsChk->isChecked());
	scripterCore->setStartupScript(startupScriptEdit->text());
	// colors
	PrefsContext* prefs = PrefsManager::instance().prefsFile->getPluginContext("scriptplugin");
	if (prefs)
	{
		prefs->set("syntaxerror", errorColor.name());
		prefs->set("syntaxcomment", commentColor.name());
		prefs->set("syntaxkeyword", keywordColor.name());
		prefs->set("syntaxsign", signColor.name());
		prefs->set("syntaxnumber", numberColor.name());
		prefs->set("syntaxstring", stringColor.name());
		prefs->set("syntaxtext", textColor.name());

		if (pathsChanged)
		{
			scripterCore->scriptPaths.clear();
			for (int i = 0; i < scriptPathsListWidget->count(); i++)
				scripterCore->scriptPaths.append(scriptPathsListWidget->item(i)->text());

			scripterCore->scriptPaths.buildMenu();
		}

		scripterCore->scriptPaths.saveToPrefs();

		emit prefsChanged();
	}
}

void Prefs_Scripter::setColor()
{
	QPushButton* button = (QPushButton*) sender();

	QColor oldColor;
	if (button == textButton)    oldColor = textColor;
	if (button == commentButton) oldColor = commentColor;
 	if (button == keywordButton) oldColor = keywordColor;
 	if (button == errorButton)   oldColor = errorColor;
 	if (button == signButton)    oldColor = signColor;
 	if (button == stringButton)  oldColor = stringColor;
 	if (button == numberButton)  oldColor = numberColor;

	QColor color = QColorDialog::getColor(oldColor, this);
	if (color.isValid() && button)
	{
		QPixmap pm(54, 14);
		pm.fill(color);
		button->setIcon(pm);
		if (button == textButton)
			textColor = color;
		if (button == commentButton)
			commentColor = color;
		if (button == keywordButton)
			keywordColor = color;
		if (button == errorButton)
			errorColor = color;
		if (button == signButton)
			signColor = color;
		if (button == stringButton)
			stringColor = color;
		if (button == numberButton)
			numberColor = color;
	}
}

void Prefs_Scripter::setupSyntaxColors()
{
	auto syntax = std::make_unique<SyntaxColors>();

	textColor = syntax->textColor;
	commentColor = syntax->commentColor;
	keywordColor = syntax->keywordColor;
	errorColor = syntax->errorColor;
	signColor = syntax->signColor;
	stringColor = syntax->stringColor;
	numberColor = syntax->numberColor;

	QPixmap pm(54, 14);
	pm.fill(textColor);
	textButton->setIcon(pm);
	pm.fill(commentColor);
	commentButton->setIcon(pm);
	pm.fill(keywordColor);
	keywordButton->setIcon(pm);
	pm.fill(errorColor);
	errorButton->setIcon(pm);
	pm.fill(signColor);
	signButton->setIcon(pm);
	pm.fill(stringColor);
	stringButton->setIcon(pm);
	pm.fill(numberColor);
	numberButton->setIcon(pm);
}

void Prefs_Scripter::changeStartupScript()
{
	QString currentScript = startupScriptEdit->text();
	if (!QFileInfo::exists(startupScriptEdit->text()))
		currentScript = QDir::homePath();

	QString s = QFileDialog::getOpenFileName(this, tr("Locate Startup Script"), currentScript, "Python Scripts (*.py *.PY)");
	if (!s.isEmpty())
		startupScriptEdit->setText(s);
}

void Prefs_Scripter::selectPath(QListWidgetItem *c)
{
	changePathButton->setEnabled(true);
	removePathButton->setEnabled(true);
}

void Prefs_Scripter::addPath()
{
	QString s = QFileDialog::getExistingDirectory(this, tr("Choose a Directory"), latestPath);
	if (s.isEmpty())
		return;

	if (s.endsWith("/"))
		s.chop(1);

	s = QDir::toNativeSeparators(s);
	if (scriptPathsListWidget->findItems(s, Qt::MatchExactly).count() != 0)
		return;

	scriptPathsListWidget->addItem(s);

	scriptPathsListWidget->setCurrentRow(scriptPathsListWidget->count() - 1);
	changePathButton->setEnabled(true);
	removePathButton->setEnabled(true);
	latestPath = s;
	pathsChanged = true;
}

void Prefs_Scripter::changePath()
{
	QString s = QFileDialog::getExistingDirectory(this, tr("Choose a Directory"), latestPath);
	if (s.isEmpty())
		return;

	if (s.endsWith("/"))
		s.chop(1);

	s = QDir::toNativeSeparators(s);
	// if the new path is already in the list, just remove the old path
	if (scriptPathsListWidget->findItems(s, Qt::MatchExactly).count() != 0)
	{
		removePath();
		return;
	}

	scriptPathsListWidget->currentItem()->setText(s);

	latestPath = s;
	pathsChanged = true;
}

void Prefs_Scripter::removePath()
{
	const int i = scriptPathsListWidget->currentRow();
	if (scriptPathsListWidget->count() == 1)
		scriptPathsListWidget->clear();
	else
		delete scriptPathsListWidget->takeItem(i);

	if (scriptPathsListWidget->count() == 0)
	{
		changePathButton->setEnabled(false);
		removePathButton->setEnabled(false);
	}
	pathsChanged = true;
}
