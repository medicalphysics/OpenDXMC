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

Copyright 2023 Erlend Andersen
*/

#include <dosetablewidget.hpp>

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QList>
#include <QStringList>
#include <QTableWidgetItem>

DoseTableWidget::DoseTableWidget(QWidget* parent)
    : QTableWidget(parent)
{
    setSortingEnabled(true);
}

void DoseTableWidget::setDoseDataHeader(QStringList header)
{
    if (columnCount() < header.size()) {
        setColumnCount(header.size());
    }
    setHorizontalHeaderLabels(header);
}

void DoseTableWidget::setDoseData(int col, int row, QVariant data)
{
    setSortingEnabled(false);

    if (columnCount() <= col) {
        setColumnCount(col + 1);
    }
    if (rowCount() <= row) {
        setRowCount(row + 1);
    }

    if (auto citem = item(row, col); citem) {
        citem->setData(Qt::DisplayRole, data);
    } else {
        auto nitem = new QTableWidgetItem;
        nitem->setData(Qt::DisplayRole, data);
        setItem(row, col, nitem);
    }
    setSortingEnabled(true);
}

void DoseTableWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Copy)) {
        copyToClipBoard();
    } else {
        QTableWidget::keyPressEvent(event);
    }
}

void DoseTableWidget::copyToClipBoard()
{

    auto sItems = selectedItems();

    int max_row = 0;
    int max_col = 0;
    for (auto& s : sItems) {
        max_row = std::max(max_row, s->row());
        max_col = std::max(max_col, s->column());
    }
    int min_row = max_row;
    int min_col = max_col;
    for (auto& s : sItems) {
        min_row = std::min(min_row, s->row());
        min_col = std::min(min_col, s->column());
    }

    QList<QStringList> data;
    data.resize(max_row - min_row + 1);

    for (auto item : sItems) {
        auto r = item->row() - min_row;
        auto c = item->column() - min_col;
        if (data[r].size() <= c)
            data[r].resize(c + 1);
        data[r][c] = item->data(Qt::DisplayRole).toString();
    }

    QString clipboardString;

    QStringList header(max_col - min_col + 1);
    for (int col = min_col; col <= max_col; col++) {
        auto hitem = horizontalHeaderItem(col);
        if (hitem)
            header[col - min_col] = hitem->data(Qt::DisplayRole).toString();
    }
    clipboardString.append(header.join("\t"));
    clipboardString.append("\n");

    for (auto& line : data) {
        clipboardString.append(line.join("\t"));
        clipboardString.append("\n");
    }

    QApplication::clipboard()->setText(clipboardString);
}
