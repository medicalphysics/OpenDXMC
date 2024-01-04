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

#include <h5io.hpp>
#include <hdf5wrapper.hpp>
H5IO::H5IO(QObject* parent)
    : BasePipeline(parent)
{
}

void H5IO::updateImageData(std::shared_ptr<DataContainer> data)
{
    m_data = data;
}

void H5IO::addBeamActor(std::shared_ptr<BeamActorContainer> beam)
{
    auto pos = std::find(m_beams.cbegin(), m_beams.cend(), beam);
    if (pos == m_beams.cend())
        m_beams.push_back(beam);
}

void H5IO::removeBeamActor(std::shared_ptr<BeamActorContainer> beam)
{
    auto pos = std::find(m_beams.cbegin(), m_beams.cend(), beam);
    if (pos != m_beams.cend())
        m_beams.erase(pos);
}

void H5IO::saveData(QString path)
{
    emit dataProcessingStarted();

    HDF5Wrapper s(path.toStdString(), HDF5Wrapper::FileOpenMode::WriteOver);
    bool success = s.save(m_data);
    bool beam_success = true;
    for (auto beam : m_beams)
        beam_success = beam_success && s.save(beam);
    emit dataProcessingFinished();
}

void H5IO::loadData(QString path)
{
}
