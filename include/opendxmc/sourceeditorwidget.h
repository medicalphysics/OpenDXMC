/*This file is part of OpenDXMC.

OpenDXMC is free software : you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenDXMC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenDXMC. If not, see < https://www.gnu.org/licenses/>.

Copyright 2019 Erlend Andersen
*/

#pragma once

#include <QColumnView>
#include <QJsonObject>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QWidget>

#include <map>
#include <vector>

#include "opendxmc/sourceeditormodel.h"

#ifndef Q_DECLARE_METATYPE_SOURCEVECTOR
#define Q_DECLARE_METATYPE_SOURCEVECTOR
Q_DECLARE_METATYPE(std::vector<std::shared_ptr<Source>>)
#endif

class BowtieFilterReader : public QWidget {
    Q_OBJECT
public:
    BowtieFilterReader(QWidget* parent = nullptr);
    void addFilter(std::shared_ptr<BowTieFilter> filter);
    bool loadFilters();
    bool saveFilters();
    std::vector<std::shared_ptr<BowTieFilter>>& filters() { return m_bowtieFilters; }

protected:
    void readJson(const QJsonObject& json);
    void writeJson(QJsonObject& json) const;
    std::shared_ptr<BowTieFilter> readFilter(QJsonObject& json) const;
    void writeFilter(QJsonObject& json, std::shared_ptr<BowTieFilter> filter) const;

private:
    std::vector<std::shared_ptr<BowTieFilter>> m_bowtieFilters;
};

class SourceDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    SourceDelegate(QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    QString displayText(const QVariant& value, const QLocale& locale) const override;

    void addBowtieFilter(std::shared_ptr<BowTieFilter> filter);
    void addAecFilter(std::shared_ptr<AECFilter> filter);

private:
    std::vector<std::pair<QString, std::shared_ptr<BowTieFilter>>> m_bowtieFilters;
    std::vector<std::pair<QString, std::shared_ptr<AECFilter>>> m_aecFilters;
};

class SourceModelView : public QTreeView {
    Q_OBJECT
public:
    SourceModelView(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};

class SourceEditWidget : public QWidget {
    Q_OBJECT
public:
    SourceEditWidget(QWidget* parent = nullptr);
    SourceModel* model() { return m_model; }
    SourceDelegate* delegate() { return m_delegate; }
    void setLowEnergyCorrection(int value);

signals:
    void runSimulation(const std::vector<std::shared_ptr<Source>> sources);
    void lowEnergyCorrectionChanged(int value);

protected:
    void setCurrentSourceTypeSelected(int index) { m_currentSourceTypeSelected = index; }
    void addCurrentSourceType(void);
    void requestRunSimulation(void);

private:
    QMap<int, QString> m_sourceTypes;
    SourceModel* m_model = nullptr;
    SourceDelegate* m_delegate = nullptr;
    int m_currentSourceTypeSelected;
    int m_lowEnergyCorrection = 1;
};
