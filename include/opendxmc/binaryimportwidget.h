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
#include <QLabel>

#include <array>

class FileSelectWidget : public QWidget
{
	Q_OBJECT
public:
	FileSelectWidget(QWidget* parent = nullptr, const QString& title="");
	const QLineEdit* getLineEditWidget() const { return m_lineEdit; }
signals:
	void pathChanged(const QString& path);
private:
	QLineEdit* m_lineEdit=nullptr;
};

class DimensionSpacingWidget :public QWidget
{
	Q_OBJECT
public:
	DimensionSpacingWidget(QWidget* parent = nullptr, 
		const std::array<double, 3>& spacing = { 1,1,1 }, 
		const std::array<std::size_t, 3> dimensions = {64,64,64});
	~DimensionSpacingWidget();
signals:
	void dimensionChanged(int position, int value);
	void spacingChanged(int position, double value);
private:
	std::array<std::size_t, 3> m_dimension = { 64,64,64 };
	std::array<double, 3> m_spacing = { 1, 1, 1 };
};

class BinaryImportWidget : public QWidget
{
	Q_OBJECT
public:
	BinaryImportWidget(QWidget* parent = nullptr);

	void setErrorMessage(const QString& message);

signals:
	void dimensionChanged(int position, int value);
	void spacingChanged(int position, double spacing);
	void materialArrayPathChanged(const QString& array);
	void densityArrayPathChanged(const QString& array);
	void materialMapPathChanged(const QString& array);


private:
	DimensionSpacingWidget* m_dsWidget = nullptr;
	QLabel* m_errorTxt = nullptr;
};