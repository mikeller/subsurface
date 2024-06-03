// SPDX-License-Identifier: GPL-2.0
/*
 * models.cpp
 *
 * classes for the equipment models of Subsurface
 *
 */
#include "qt-models/models.h"
#include "core/qthelper.h"
#include "core/dive.h"
#include "core/gettextfromc.h"

#include <QDir>
#include <QLocale>

Qt::ItemFlags GasSelectionModel::flags(const QModelIndex &index) const
{
	return listFlags[index.row()];
}

GasSelectionModel::GasSelectionModel(const dive &d, int dcNr, QObject *parent)
	: QStringListModel(parent)
{
	const divecomputer *dc = get_dive_dc_const(&d, dcNr);
	QStringList list;
	for (int i = 0; i < d.cylinders.nr; i++) {
		list.push_back(get_dive_gas(&d, dcNr, i));

		if (is_cylinder_use_appropriate(dc, get_cylinder(&d, i)))
			listFlags.push_back(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		else
			listFlags.push_back(Qt::NoItemFlags);
	}

	setStringList(list);
}

QVariant GasSelectionModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::FontRole)
		return defaultModelFont();
	return QStringListModel::data(index, role);
}

// Dive Type Model for the divetype combo box

Qt::ItemFlags DiveTypeSelectionModel::flags(const QModelIndex&) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

DiveTypeSelectionModel::DiveTypeSelectionModel(QObject *parent) : QStringListModel(parent)
{
	QStringList modes;
	for (int i = 0; i < FREEDIVE; i++)
		modes.append(gettextFromC::tr(divemode_text_ui[i]));
	setStringList(modes);
}

QVariant DiveTypeSelectionModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::FontRole)
		return defaultModelFont();
	return QStringListModel::data(index, role);
}

// Language Model, The Model to populate the list of possible Languages.

LanguageModel *LanguageModel::instance()
{
	static LanguageModel *self = new LanguageModel();
	QLocale l;
	return self;
}

LanguageModel::LanguageModel(QObject *parent) : QAbstractListModel(parent)
{
	QDir d(getSubsurfaceDataPath("translations"));
	for (const QString &s: d.entryList()) {
		if (s.startsWith("subsurface_") && s.endsWith(".qm")) {
			languages.push_back((s == "subsurface_source.qm") ? "English" : s);
		}
	}
}

QVariant LanguageModel::data(const QModelIndex &index, int role) const
{
	QLocale loc;
	QString currentString = languages.at(index.row());
	if (!index.isValid())
		return QVariant();
	switch (role) {
	case Qt::DisplayRole: {
		QLocale l(currentString.remove("subsurface_").remove(".qm"));
		return currentString == "English" ? currentString : QString("%1 (%2)").arg(l.languageToString(l.language())).arg(l.countryToString(l.country()));
	}
	case Qt::UserRole:
		return currentString == "English" ? "en_US" : currentString.remove("subsurface_").remove(".qm");
	}
	return QVariant();
}

int LanguageModel::rowCount(const QModelIndex&) const
{
	return languages.count();
}
