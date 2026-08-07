#include "markindex.h"

#include <QPushButton>

MarkIndex::MarkIndex(const QString& indexEntry, const QString& index, const QStringList& indexList, QWidget *parent) :
	MarkInsert(parent)
{
	setupUi(this);
	setWindowTitle(tr("Index Mark"));
	setIndexValues(indexEntry, indexList);

	QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
	if (okButton)
	{
		okButton->setEnabled(!indexEntry.isEmpty());
		connect(entryEdit, &QLineEdit::textChanged, this, &MarkIndex::onEntryTextChanged);
	}
}

void MarkIndex::onEntryTextChanged(const QString& entryText)
{
	bool emptyText = entryText.isEmpty();

	QPushButton* okButton = buttonBox->button(QDialogButtonBox::Ok);
	if (okButton)
		okButton->setEnabled(!emptyText);
}

void MarkIndex::values(QString& label)
{
	label = entryEdit->text();
}

void MarkIndex::setValues(const QString label)
{
	entryEdit->setText(label);
}

void MarkIndex::indexValues(QString &indexEntry, QString &index)
{
	indexEntry = entryEdit->text();
	index = indexComboBox->currentText();
}

void MarkIndex::setIndexValues(const QString &indexEntry, const QStringList &indexes)
{
	indexComboBox->addItems(indexes);
	entryEdit->setText(indexEntry);
}

void MarkIndex::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type())
	{
		case QEvent::LanguageChange:
			retranslateUi(this);
			break;
		default:
			break;
	}
}
