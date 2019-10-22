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

#include <QWidget>
#include <QLineEdit>

#include <array>

class FileSelectWidget : public QWidget
{
	Q_OBJECT
public:
	FileSelectWidget(QWidget* parent = nullptr);
private:
	QLineEdit* m_lineEdit=nullptr;
};

class DimensionSpacingWidget :public QWidget
{
	Q_OBJECT
public:
	DimensionSpacingWidget(QWidget* parent = nullptr);
signals:
	void dimensionChanged(const std::array<std::size_t, 3>& dimensions) const;
	void spacingChanged(const std::array<double, 3>& spacing) const;
private:
	std::array<std::size_t, 3> m_dimension = { 512, 512, 40 };
	std::array<double, 3> m_spacing = { 1, 1, 1 };
};

class BinaryImportWidget : public QWidget
{
	Q_OBJECT
public:
	BinaryImportWidget(QWidget* parent = nullptr);

signals:

private:

};