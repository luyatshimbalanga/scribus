/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
    begin                : Jan 2005
    copyright            : (C) 2005 by Craig Bradney
    email                : cbradney@scribus.info
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QMenu>
#include <QIcon>
#include <QSysInfo>
#include <QVector>

#include "iconmanager.h"
#include "scraction.h"
#include "scribus.h"
#include "scribusapp.h"
#include "scribusdoc.h"

ScrAction::ScrAction( QObject * parent ) : QAction( parent )
{
}

ScrAction::ScrAction(const QString & menuText, const QKeySequence& accel, QObject * parent ) : QAction(menuText, parent)
{
	setShortcut(accel);
}

ScrAction::ScrAction(ActionType aType, const QString & menuText, const QKeySequence& accel, QObject * parent, const QVariant& d) : QAction(menuText, parent)
{
	setShortcut(accel);
	setData(d);

	m_actionType = aType;
	if (m_actionType != Normal)
		connect (this, SIGNAL(triggered()), this, SLOT(triggeredToTriggeredData()));
}

ScrAction::ScrAction(ActionType aType, const QPixmap& icon16, const QPixmap& icon22, const QString & menuText, const QKeySequence& accel, QObject * parent, const QVariant& d)
          : QAction(QIcon(icon16), menuText, parent)
{
	setShortcut(accel);
	icon().addPixmap(icon22, QIcon::Normal, QIcon::On);

	m_actionType = aType;
	setData(d);
	if (m_actionType != Normal)
		connect (this, SIGNAL(triggered()), this, SLOT(triggeredToTriggeredData()));
}

ScrAction::ScrAction(ActionType aType, const QString& icon16Path, const QString& icon22Path, const QString & menuText, const QKeySequence& accel, QObject * parent, const QVariant& d)
         : QAction(menuText, parent), m_iconPath16(icon16Path), m_iconPath22(icon22Path)
{
	setShortcut(accel);
	loadIcon();

	m_actionType = aType;
	setData(d);
	if (m_actionType != Normal)
		connect (this, SIGNAL(triggered()), this, SLOT(triggeredToTriggeredData()));
	if (!m_iconPath16.isEmpty() || !m_iconPath22.isEmpty())
		connect(ScQApp, SIGNAL(iconSetChanged()), this, SLOT(loadIcon()));
}

ScrAction::ScrAction(const QString& icon16Path, const QString& icon22Path, const QString & menuText, const QKeySequence& accel, QObject * parent )
         : QAction(menuText, parent), m_iconPath16(icon16Path), m_iconPath22(icon22Path)
{
	setShortcut(accel);
	setMenuRole(QAction::NoRole);
	loadIcon();
	if (!m_iconPath16.isEmpty() || !m_iconPath22.isEmpty())
		connect(ScQApp, SIGNAL(iconSetChanged()), this, SLOT(loadIcon()));
}


ScrAction::ScrAction(const QKeySequence& accel, QObject * parent, const QVariant& d)
	: QAction( QIcon(QPixmap()), "", parent )
{
	setShortcut(accel);
	icon().addPixmap(QPixmap(), QIcon::Normal, QIcon::On);
	m_actionType = UnicodeChar;
	setData(d);
	connect (this, SIGNAL(triggered()), this, SLOT(triggeredToTriggeredData()));
}

void ScrAction::triggeredToTriggeredData()
{
	if (m_actionType == ScrAction::DataInt)
		emit triggeredData(data().toInt());
	if (m_actionType == ScrAction::DataDouble)
		emit triggeredData(data().toDouble());
	if (m_actionType == ScrAction::DataQString)
		emit triggeredData(data().toString());
	if (m_actionType == ScrAction::DLL)
		qDebug() << "if (_actionType == ScrAction::DLL): please fix in ScrAction::triggeredToTriggeredData()";
//		emit triggeredData(pluginID);
	if (m_actionType == ScrAction::Window)
		emit triggeredData(data().toInt());
	if (m_actionType == ScrAction::RecentFile)
		emit triggeredData(data().toString());
	if (m_actionType == ScrAction::RecentPaste)
		emit triggeredData(data().toString());
	if (m_actionType == ScrAction::RecentScript)
		emit triggeredData(data().toString());
	if (m_actionType == ScrAction::OwnScript)
		emit triggeredData(data().toString());
	if (m_actionType == ScrAction::UnicodeChar)
		emit triggeredUnicodeShortcut(data().toInt());
	if (m_actionType == ScrAction::Layer)
		emit triggeredData(data().toInt());
	if (m_actionType == ScrAction::ActionDLL)
		emit triggeredData(((ScribusMainWindow*)parent())->doc);
}

void ScrAction::toggledToToggledData(bool ison)
{

	if (!isCheckable())
		return;
	if (m_actionType == ScrAction::DataInt)
		emit toggledData(ison, data().toInt());
	if (m_actionType == ScrAction::DataDouble)
		emit toggledData(ison, data().toDouble());
	if (m_actionType == ScrAction::DataQString)
		emit toggledData(ison, data().toString());
	if (m_actionType == ScrAction::DLL)
		qDebug() << "if (_actionType == ScrAction::DLL): please fix in ScrAction::toggledToToggledData(bool ison)";
	//			emit toggledData(ison, pluginID);
	if (m_actionType == ScrAction::Window)
		emit toggledData(ison, data().toInt());
	if (m_actionType == ScrAction::RecentFile)
		emit toggledData(ison, data().toString());
	if (m_actionType == ScrAction::RecentPaste)
		emit toggledData(ison, data().toString());
	if (m_actionType == ScrAction::RecentScript)
		emit toggledData(ison, text());
	if (m_actionType == ScrAction::OwnScript)
		emit toggledData(ison, text());
	if (m_actionType == ScrAction::Layer)
		emit toggledData(ison, data().toInt());
	// no toggle for UnicodeChar
}

void ScrAction::addedTo(int index, QMenu * menu)
{
	if (m_menuIndex == -1) // Add the first time, not for secondary popups.
	{
		m_menuIndex = index;
		m_popupMenuAddedTo = menu;
	}
}

QString ScrAction::cleanMenuText()
{
	return text().remove('&').remove("...");
}

void ScrAction::setToolTipFromTextAndShortcut()
{
	QString sct(shortcut().toString(QKeySequence::NativeText));
	if (sct.isEmpty())
		QAction::setToolTip("<qt>" + cleanMenuText() + "</qt>");
	else
		QAction::setToolTip("<qt>" + cleanMenuText() + " (" + sct + ")" + "</qt>");
}

void ScrAction::setStatusTextAndShortcut(const QString& statusText)
{
	QString sct(shortcut().toString(QKeySequence::NativeText));
	if (sct.isEmpty())
		QAction::setStatusTip(statusText);
	else
		QAction::setStatusTip(statusText + " (" + sct + ")");
}

int ScrAction::getMenuIndex() const
{
	return m_menuIndex;
}

bool ScrAction::isDLLAction() const
{
	return m_actionType == ScrAction::DLL;
}

int ScrAction::dllID() const
{
	if (m_actionType == ScrAction::DLL)
		return data().toInt();
	return -1;
}

void ScrAction::setToggleAction(bool isToggle, bool isFakeToggle)
{
	if (m_actionType != Normal)
	{
		if (isToggle)
			connect(this, SIGNAL(toggled(bool)), this, SLOT(toggledToToggledData(bool)));
		else
			disconnect(this, SIGNAL(toggled(bool)), this, SLOT(toggledToToggledData(bool)));
	}
	QAction::setCheckable(isToggle);
	setChecked(isToggle); // set default state of the action's checkbox - PV
	m_fakeToggle = isFakeToggle;
	//if (fakeToggle)
		//connect(this, toggled(bool), this, triggered());
}

void ScrAction::saveShortcut()
{
	if (!m_shortcutSaved)
	{
		m_savedKeySequence = shortcut();
		setShortcut(QKeySequence(""));
		m_shortcutSaved = true;
	}
}

void ScrAction::restoreShortcut()
{
	if (m_shortcutSaved)
	{
		setShortcut(m_savedKeySequence);
		m_savedKeySequence = QKeySequence("");
		m_shortcutSaved = false;
	}
}

ScrAction::ActionType ScrAction::actionType()
{
	return m_actionType;
}

int ScrAction::actionInt() const
{
	return data().toInt();
}

double ScrAction::actionDouble() const
{
	return data().toDouble();
}

const QString ScrAction::actionQString()
{
	return data().toString();
}

void ScrAction::setTexts(const QString &newText)//#9114, qt3-qt4 change of behaviour bug:, bool setTextToo)
{
	QAction::setText(newText);
//	if (setTextToo)
	QAction::setIconText(cleanMenuText());
}

void ScrAction::toggle()
{
	QAction::toggle();
	if (m_fakeToggle)
		emit triggered();
}

void ScrAction::setActionQString(const QString &s)
{
	setData(s);
}

void ScrAction::loadIcon()
{
	if (m_iconPath16.isEmpty() && m_iconPath22.isEmpty())
		return;

	IconManager& iconManager = IconManager::instance();
	QIcon newIcon;

	if (!m_iconPath16.isEmpty())
	{
		QPixmap pix = iconManager.loadPixmap(m_iconPath16, 16);
		if (!pix.isNull())
			newIcon.addPixmap(pix, QIcon::Normal, QIcon::On);
	}

	if (!m_iconPath22.isEmpty())
	{
		QPixmap pix = iconManager.loadPixmap(m_iconPath22, 20);
		if (!pix.isNull())
			newIcon.addPixmap(pix, QIcon::Normal, QIcon::On);
	}

	setIcon(newIcon);
}
