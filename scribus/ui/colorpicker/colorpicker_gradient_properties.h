/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
For general Scribus copyright and licensing information please refer
to the COPYING file provided with the program.
*/

#ifndef COLORPICKER_GRADIENT_PROPERTIES_H
#define COLORPICKER_GRADIENT_PROPERTIES_H

#include <QEvent>
#include <QLabel>
#include <QLayout>
#include <QWidget>

#include "colorpicker_enum.h"
#include "scribusstructs.h"
#include "ui_colorpicker_gradient_properties.h"
#include "ui/widgets/stacked_container.h"

class ColorPickerGradientProperties :  public StackedContainer, Ui::ColorPickerGradientProperties
{
	Q_OBJECT

public:
	ColorPickerGradientProperties( QWidget* parent = nullptr);

	void selectLinear();
	void selectRadial();
	void selectConical();
	void selectFourColor();
	void selectDiamond();
	void selectMesh();
	void selectPatchMesh();

	const CPGradientVectorData& gradientVectorData() const { return m_data; }
	void setGradientVectorData(const CPGradientVectorData& data);

	void setConfig(int type, bool isMask);
	GradientEdit gradientEditType() const;

public slots:
	void languageChange();
	void handleEditButton();
	void handleEditControlButton();
	void handlePEditButton();
	void handlePEditControlButton();
	void handlePAddButton();
	void handleSnapToGridBox();
	void endPAddButton();
	void changebuttonRemovePatch(bool val);	
	void changeSpecialLinear();
	void changeSpecialRadial();
	void changeSpecialFourColors();
	void changeSpecialDiamond();

	void iconSetChange();
	void unitChange(int unitIndex);

protected:
	double m_unitRatio { 1.0 };
	GradientEdit m_gradientEditType { GradientEdit::Gradient };
	int m_type { -1 };
	CPGradientVectorData m_data;

	void connectSlots();
	void disconnectSlots();

	void changeEvent(QEvent *e) override;

signals:
	// Gradient Vector
	void gradientVectorChanged();

	// Mesh
	void gradientEditMode(GradientEdit);
	void createNewMesh();
	void resetMesh();
	void meshToShape();
	void reset1Control();
	void resetAllControl();
	void removePatch();
	void snapToMGrid(bool);	
};

#endif // COLORPICKER_GRADIENT_PROPERTIES_H
