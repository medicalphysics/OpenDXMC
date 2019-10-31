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

#include "binaryimportpipeline.h"
#include <fstream>
#include <string>

BinaryImportPipeline::BinaryImportPipeline(QObject* parent)
	:QObject(parent)
{

}

void BinaryImportPipeline::setDimension(const std::array<std::size_t, 3>& dimensions)
{
	for (int i = 0; i < 3; ++i)
	{
		if (dimensions[i] == 0 && dimensions[i] > 2048)
			return;
	}
	m_dimensions = dimensions;
}

void BinaryImportPipeline::setSpacing(const std::array<double, 3>& spacing)
{
	for (int i = 0; i < 3; ++i)
	{
		if (spacing[i] <= 0.0)
			return;
	}
	m_spacing = spacing;
}

template<typename T>
std::vector<T> BinaryImportPipeline::readBinaryArray(const QString& path) const
{
	auto std_path = path.toStdString();
	std::ifstream ifs(std_path, std::ios::binary | std::ios::ate);

	if (!ifs)
	{
		auto msq = QString(tr("Error opening file: ")) + path;
		emit errorMessage(msq);
		return std::vector<T>();
	}

	auto end = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	auto size = std::size_t(end - ifs.tellg());

	auto dim_size = m_dimensions[0] * m_dimensions[1] * m_dimensions[2] * sizeof(T);
	if (dim_size != size)
	{
		auto msq = QString(tr("Dimensions and file size do not match for: ")) + path;
		emit errorMessage(msq);
		return std::vector<T>();
	}

	if (size == 0) // avoid undefined behavior 
	{
		auto msq = QString(tr("Error reading file: ")) + path;
		emit errorMessage(msq);
		return std::vector<T>();
	}

	std::vector<T> buffer(size);
	buffer.resize(m_dimensions[0] * m_dimensions[1] * m_dimensions[2]);

	if (!ifs.read(reinterpret_cast<char*>(buffer.data()), size))
	{
		auto msq = QString(tr("Error reading file: ")) + path;
		emit errorMessage(msq);
		return std::vector<T>();
	}
	return buffer;
}

void BinaryImportPipeline::setMaterialArrayPath(const QString& path)
{
	auto data = readBinaryArray<unsigned char>(path);
	if (data.size() == 0)
		return;
}

void BinaryImportPipeline::setDensityArrayPath(const QString& path)
{
	auto data = readBinaryArray<double>(path);
	if (data.size() == 0)
		return;
}

void BinaryImportPipeline::setMaterialMapPath(const QString& path)
{
	auto std_path = path.toStdString();
	std::ifstream ifs(std_path);

	if (!ifs)
	{
		auto msq = QString(tr("Error opening file: ")) + path;
		emit errorMessage(msq);
	}

	std::vector<std::string> lines;
	std::string str;
	while (std::getline(in, str))
	{
		if(str.size() > 0)
			lines.push_back(str);
	}

}
