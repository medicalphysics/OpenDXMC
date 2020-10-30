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

#include "opendxmc/saveload.h"
#include "opendxmc/h5wrapper.h"

SaveLoad::SaveLoad(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<std::shared_ptr<ImageContainer>>();
    qRegisterMetaType<std::vector<Material>>();
    qRegisterMetaType<std::vector<std::string>>();
    qRegisterMetaType<std::shared_ptr<ImageContainer>>();
    qRegisterMetaType<DoseReportContainer>();
    qRegisterMetaType<std::shared_ptr<BowTieFilter>>();
}

void SaveLoad::loadFromFile(const QString& path)
{
    emit processingDataStarted();
    clear();
    m_sources.clear();
    m_images.clear();
    H5Wrapper wrapper(path.toStdString(), H5Wrapper::FileOpenType::ReadOnly);
    constexpr std::array<ImageContainer::ImageType, 7> types({ ImageContainer::ImageType::CTImage,
        ImageContainer::ImageType::DensityImage, ImageContainer::ImageType::MaterialImage,
        ImageContainer::ImageType::DoseImage, ImageContainer::ImageType::OrganImage,
        ImageContainer::ImageType::TallyImage, ImageContainer::ImageType::VarianceImage });
    for (auto type : types) {
        auto im = wrapper.loadImage(type);
        if (im)
            m_images.push_back(im);
    }

    m_materialList = wrapper.loadMaterials();
    m_organList = wrapper.loadOrganList();

    std::shared_ptr<ImageContainer> matImage = nullptr;
    std::shared_ptr<ImageContainer> orgImage = nullptr;
    std::shared_ptr<ImageContainer> densImage = nullptr;
    std::shared_ptr<ImageContainer> doseImage = nullptr;
    std::shared_ptr<ImageContainer> tallyImage = nullptr;
    for (auto im : m_images) {
        if (im->imageType == ImageContainer::ImageType::MaterialImage)
            matImage = im;
        ;
        if (im->imageType == ImageContainer::ImageType::OrganImage)
            orgImage = im;
        if (im->imageType == ImageContainer::ImageType::DensityImage)
            densImage = im;
        if (im->imageType == ImageContainer::ImageType::DoseImage)
            doseImage = im;
        if (im->imageType == ImageContainer::ImageType::TallyImage)
            tallyImage = im;
    }

    //creating dose data
    if (matImage && densImage && doseImage && tallyImage) {
        if (orgImage) {
            DoseReportContainer cont(
                m_materialList,
                m_organList,
                std::static_pointer_cast<MaterialImageContainer>(matImage),
                std::static_pointer_cast<OrganImageContainer>(orgImage),
                std::static_pointer_cast<DensityImageContainer>(densImage),
                std::static_pointer_cast<DoseImageContainer>(doseImage),
                std::static_pointer_cast<TallyImageContainer>(tallyImage));
            emit doseDataChanged(cont);
        } else {
            DoseReportContainer cont(
                m_materialList,
                std::static_pointer_cast<MaterialImageContainer>(matImage),
                std::static_pointer_cast<DensityImageContainer>(densImage),
                std::static_pointer_cast<DoseImageContainer>(doseImage),
                std::static_pointer_cast<TallyImageContainer>(tallyImage));
            emit doseDataChanged(cont);
        }
    }
    m_sources = wrapper.loadSources();
    for (auto im : m_images)
        emit imageDataChanged(im);

    emit materialDataChanged(m_materialList);
    emit organDataChanged(m_organList);
    emit sourcesChanged(m_sources);

    for (auto src : m_sources) {
        if (src->type() == Source::Type::CTAxial || src->type() == Source::Type::CTSpiral || src->type() == Source::Type::CTDual) {
            auto ctsrc = std::static_pointer_cast<CTSource>(src);
            auto aecFilter = ctsrc->aecFilter();
            if (aecFilter) {
                emit aecFilterChanged(aecFilter);
            }
            auto bowtie = ctsrc->bowTieFilter();
            if (bowtie) {
                emit bowtieFilterChanged(bowtie);
            }
            if (src->type() == Source::Type::CTDual) {
                auto ctdualsrc = std::static_pointer_cast<CTSpiralDualSource>(src);
                auto bowtieB = ctdualsrc->bowTieFilterB();
                if (bowtieB) {
                    emit bowtieFilterChanged(bowtieB);
                }
            }
        }
    }
    emit processingDataEnded();
}

void SaveLoad::saveToFile(const QString& path)
{
    emit processingDataStarted();

    H5Wrapper wrapper(path.toStdString());
    for (auto image : m_images)
        wrapper.saveImage(image);

    wrapper.saveMaterials(m_materialList);
    wrapper.saveOrganList(m_organList);
    wrapper.saveSources(m_sources);
    emit processingDataEnded();
}

void SaveLoad::setImageData(std::shared_ptr<ImageContainer> image)
{
    if (!image)
        return;
    if (!image->image)
        return;
    if (m_currentImageID != image->ID) {
        m_images.clear();
        m_images.push_back(image);
        m_currentImageID = image->ID;
    } else {
        //find if image is present
        for (std::size_t i = 0; i < m_images.size(); ++i) {
            if (m_images[i]->imageType == image->imageType) {
                m_images[i] = image;
                return;
            }
        }
        //did not find image
        m_images.push_back(image);
    }
}

void SaveLoad::setMaterials(const std::vector<Material>& materials)
{
    m_materialList = materials;
}

void SaveLoad::clear(void)
{
    m_currentImageID = 0;
    m_images.clear();
    // we do not clear m_sources here
}

void SaveLoad::addSource(std::shared_ptr<Source> source)
{
    auto pos = std::find(m_sources.begin(), m_sources.end(), source);
    if (pos == m_sources.end())
        m_sources.push_back(source);
}

void SaveLoad::removeSource(std::shared_ptr<Source> source)
{
    auto pos = std::find(m_sources.begin(), m_sources.end(), source);
    if (pos != m_sources.end()) // we found it
        m_sources.erase(pos);
}
