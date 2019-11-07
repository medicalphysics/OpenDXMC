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
#include <sstream>
#include <string>
#include <vector>
#include "material.h"
#include <numeric>

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
std::shared_ptr<std::vector<T>> BinaryImportPipeline::readBinaryArray(const QString& path) const
{
	auto std_path = path.toStdString();
	std::ifstream ifs(std_path, std::ios::binary | std::ios::ate);

	if (!ifs)
	{
		auto msq = QString(tr("Error opening file: ")) + path;
		emit errorMessage(msq);
		return nullptr;
	}

	auto end = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	auto size = std::size_t(end - ifs.tellg());

	auto dim_size = m_dimensions[0] * m_dimensions[1] * m_dimensions[2] * sizeof(T);
	if (dim_size != size)
	{
		auto msq = QString(tr("Image dimensions and file size do not match for: ")) + path;
		emit errorMessage(msq);
		return nullptr;
	}

	if (size == 0) // avoid undefined behavior 
	{
		auto msq = QString(tr("Error reading file: ")) + path;
		emit errorMessage(msq);
		return nullptr;
	}

	auto buffer = std::make_shared<std::vector<T>>(size);
	//std::vector<T> buffer(size);
	buffer->resize(m_dimensions[0] * m_dimensions[1] * m_dimensions[2]);

	if (!ifs.read(reinterpret_cast<char*>(buffer->data()), size))
	{
		auto msq = QString(tr("Error reading file: ")) + path;
		emit errorMessage(msq);
		return nullptr;
	}
	return buffer;
}

void BinaryImportPipeline::setMaterialArrayPath(const QString& path)
{
	emit resultsReady(false);
	m_materialArray = readBinaryArray<unsigned char>(path);
	if (!m_materialArray) {
		auto msg = QString(tr("Error opening material array file: ")) + path;
		emit errorMessage(msg);
		return;
	}
	validate();	
}

void BinaryImportPipeline::setDensityArrayPath(const QString& path)
{
	emit resultsReady(false);
	m_densityArray = readBinaryArray<double>(path);
	if (!m_densityArray) {
		auto msg = QString(tr("Error opening density array file: ")) + path;
		emit errorMessage(msg);
		return;
	}
	validate();
}

inline std::string trim(const std::string &s)
{
   auto wsfront=std::find_if_not(s.begin(),s.end(),[](int c){return std::isspace(c);});
   auto wsback=std::find_if_not(s.rbegin(),s.rend(),[](int c){return std::isspace(c);}).base();
   return (wsback<=wsfront ? std::string() : std::string(wsfront,wsback));
}
std::vector<std::string> split(const std::string& s, char delimiter)
{
   std::vector<std::string> tokens;
   std::string token;
   std::istringstream tokenStream(s);
   while (std::getline(tokenStream, token, delimiter))
   {
      tokens.push_back(trim(token));
   }
   return tokens;
}
void BinaryImportPipeline::setMaterialMapPath(const QString& path)
{
	emit resultsReady(false);
	m_materialMap.clear();
	auto std_path = path.toStdString();
	std::ifstream ifs(std_path);

	if (!ifs)
	{
		auto msq = QString(tr("Error opening material map file: ")) + path;
		emit errorMessage(msq);
		return;
	}
	std::string str;
	while (std::getline(ifs, str))
	{
		if(str.size() > 0)
		{
			auto strings = split(str, ',');
			if (strings.size() >= 3)
			{
				std::uint8_t ind = 0;
				try {
					ind = static_cast<std::uint8_t>(std::stoi(strings[0]));
				}
				catch (std::invalid_argument &e)
				{
					auto msq = QString(tr("Error in material map file: ")) + path + QString(tr(" when parsing material number: ")) + QString::fromStdString(strings[0]);
					msq += QString::fromStdString(e.what());
					emit errorMessage(msq);
					return;
				}
				catch (std::out_of_range &e)
				{
					auto msq = QString(tr("Error in material map file: ")) + path + QString(tr(" when parsing material number: ")) + QString::fromStdString(strings[0]);
					msq += QString::fromStdString(e.what());
					emit errorMessage(msq);
					return;
				}
				auto name = strings[1];
				auto composition = strings[2];
				Material material(composition, name);
				material.setStandardDensity(1.0);
				if (material.isValid())
				{
					// testing if index is taken
					for (auto& [i, m] : m_materialMap)
					{
						if (i == ind)
						{
							auto msq = QString(tr("Error in material map file: ")) + path + QString(tr(" index is already occupied: ")) + QString::fromStdString(strings[0]);
							return;
						}
					}
					m_materialMap.push_back(std::pair(ind, material));
				}
				else {
					auto msg = QString(tr("Error in material map file: ")) + path + QString(tr(" Not able to parse material definition ")) + QString::fromStdString(strings[2]);
					return;
				}
			}
		}
	}
	std::sort(m_materialMap.begin(), m_materialMap.end(), [](auto& a, auto& b) {return a.first < b.first; });
	validate();
}

void BinaryImportPipeline::validate()
{
	emit resultsReady(false);
	//test for valid pointers
	if (!m_densityArray || !m_materialArray)
		return;
	//test for size matching
	if (m_densityArray->size() != m_materialArray->size())
		return;
	//test for dimensions matching
	std::size_t dim = std::accumulate(m_dimensions.begin(), m_dimensions.end(), static_cast<std::size_t>(1), std::multiplies<std::size_t>());
	if (m_materialArray->size() != dim)
		return;

	//finding uniqe elements in material array
	// we must make a copy :(
	std::vector<unsigned char> matInd(m_materialArray->begin(), m_materialArray->end());
	std::sort(matInd.begin(), matInd.end());
	auto last = std::unique(matInd.begin(), matInd.end());
	matInd.erase(last, matInd.end());
	//making sure all indices have a corresponding material
	std::vector<unsigned char> matMapInd;
	matMapInd.reserve(matInd.size());
	for (auto &[ind, m] : m_materialMap)
	{
		matMapInd.push_back(ind);
	}
	std::vector<unsigned char> matInter;
	std::set_intersection(matInd.begin(), matInd.end(), matMapInd.begin(), matMapInd.end(), std::back_inserter(matInter));
	std::vector<unsigned char> matDiff;
	std::set_difference(matInd.begin(), matInd.end(), matInter.begin(), matInter.begin(), std::back_inserter(matDiff));
	if (matDiff.size() != 0) 
		return; // this means that there are material array values that does not have a corresponding material definition

	//making sure all indices are concecutive
	for (std::uint8_t i = 0; i < matInd.size(); ++i)
	{
		if (i != matInd[i])
		{
			//changing indices
			std::replace(m_materialArray->begin(), m_materialArray->end(), matInd[i], i);
			for (auto& [ind, m] : m_materialMap)
			{
				if (ind == matInd[i])
					ind = i;
			}
		}
	}

	//emitting data




}
