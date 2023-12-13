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

Copyright 2024 Erlend Andersen
*/

#include <beamsettingsdelegate.hpp>

#include <QComboBox>
#include <QModelIndex>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

BeamSettingsDelegate::BeamSettingsDelegate(BeamSettingsModel* model, QObject* parent)
    : m_model(model)
    , QStyledItemDelegate(parent)
{
}
QWidget* BeamSettingsDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto userType = index.data(Qt::DisplayRole).userType();
    if (userType == qMetaTypeId<BeamSettingsModel::BowtieSelection>()) {
        QComboBox* cb = new QComboBox(parent);
        const auto& filters = m_model->bowtieFilters();
        for (auto i = filters.cbegin(), end = filters.cend(); i != end; ++i)
            cb->addItem(i.key());
        return cb;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}
void BeamSettingsDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto userType = index.data(Qt::DisplayRole).userType();
    if (userType == qMetaTypeId<BeamSettingsModel::BowtieSelection>()) {
        auto currentFilter = qvariant_cast<BeamSettingsModel::BowtieSelection>(index.data(Qt::EditRole));
        auto cb = qobject_cast<QComboBox*>(editor);
        cb->setCurrentText(currentFilter.currentKey);
        return;
    }
    QStyledItemDelegate::setEditorData(editor, index);
}
void BeamSettingsDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto userType = index.data(Qt::DisplayRole).userType();
    if (userType == qMetaTypeId<BeamSettingsModel::BowtieSelection>()) {
        auto cb = qobject_cast<QComboBox*>(editor);
        auto idx = cb->currentText();
        BeamSettingsModel::BowtieSelection selection {
            .currentKey = idx,
            .bowtieMap = m_model->bowtieFiltersPtr()
        };

        auto data = QVariant::fromValue(selection);
        model->setData(index, data, Qt::EditRole);

    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}
QString BeamSettingsDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    auto userType = value.userType();
    if (userType == qMetaTypeId<BeamSettingsModel::BowtieSelection>()) {
        auto filter = value.value<BeamSettingsModel::BowtieSelection>();
        return filter.currentKey;
    }
    return QStyledItemDelegate::displayText(value, locale);
}
