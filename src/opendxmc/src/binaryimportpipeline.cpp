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

void BinaryImportPipeline::setMaterialArrayPath(const QString& path)
{
	bool test = false;
	// read data here
}

void BinaryImportPipeline::setDensityArrayPath(const QString& path)
{
	//read data here
}

void BinaryImportPipeline::setMaterialMapPath(const QString& path)
{
	//read data here
}
