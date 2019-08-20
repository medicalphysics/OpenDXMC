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

#include "imagecontainer.h"


#include <QWidget>
#include <QLineEdit>

#include <vector>
#include <memory>
#include <string>

constexpr std::size_t EXPORT_HEADER_SIZE = 4096;

class ExportWidget : public QWidget
{
	Q_OBJECT
public:
	ExportWidget(QWidget* parent = nullptr);
	void registerImage(std::shared_ptr<ImageContainer> image);
signals:
	void rawExportFolderSelected(QString folderPath);

protected:
	void browseForRawExportFolder();
	void exportAllRawData();
	static std::array<char, EXPORT_HEADER_SIZE> getHeaderData(std::shared_ptr<ImageContainer> image);
	static void writeArrayBin(std::shared_ptr<ImageContainer> image, const std::string& path, bool includeHeader);
private:
	bool m_rawExportIncludeHeader = true;
	QLineEdit* m_exportRawLineEdit;
	std::vector<std::shared_ptr<ImageContainer>> m_images;
};
