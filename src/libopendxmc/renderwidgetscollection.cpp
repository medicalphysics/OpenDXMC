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

#include <renderwidgetscollection.hpp>

#include <QGridLayout>
#include <QSizePolicy>

std::shared_ptr<DataContainer> generateSampleData()
{
    auto data = std::make_shared<DataContainer>();
    data->setDimensions({ 8, 8, 8 });
    data->setSpacing({ 1, 1, 1 });
    std::vector<double> im(8 * 8 * 8, 0);
    for (int i = 0; i < im.size(); ++i)
        im[i] = i;
    data->setImageArray(DataContainer::ImageType::CT, im);
    return data;
}

RenderWidgetsCollection::RenderWidgetsCollection(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    for (std::size_t i = 0; i < 3; ++i)
        m_slice_widgets[i] = new SliceRenderWidget(i, this);

    layout->addWidget(m_slice_widgets[0], 0, 0);
    layout->addWidget(m_slice_widgets[1], 1, 0);
    layout->addWidget(m_slice_widgets[2], 1, 1);

    m_slice_widgets[0]->sharedViews(m_slice_widgets[1], m_slice_widgets[2]);

    this->setLayout(layout);
    updateImageData(generateSampleData());
}

void RenderWidgetsCollection::updateImageData(std::shared_ptr<DataContainer> data)
{
    for (auto& wid : m_slice_widgets)
        wid->updateImageData(data);
}

void RenderWidgetsCollection::useFXAA(bool on)
{
    for (auto& w : m_slice_widgets)
        w->useFXAA(on);
}

void RenderWidgetsCollection::setMultisampleAA(int samples)
{
    for (auto& w : m_slice_widgets)
        w->setMultisampleAA(samples);
}

void RenderWidgetsCollection::setInteractionStyleToSlicing()
{
    for (auto& w : m_slice_widgets)
        w->setInteractionStyleToSlicing();
}

void RenderWidgetsCollection::setInteractionStyleTo3D()
{
    for (auto& w : m_slice_widgets)
        w->setInteractionStyleTo3D();
}
