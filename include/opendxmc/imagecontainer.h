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

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkSmartPointer.h>
#include <vtkType.h>

//used onlu for viz
class ImageContainer {
public:
    enum class ImageType :int {
        CTImage,
        DensityImage,
        MaterialImage,
        DoseImage,
        OrganImage,
        TallyImage,
        VarianceImage,
        MeasurementImage,
        CustomType,
        Empty
    };
    vtkSmartPointer<vtkImageData> image;
    std::array<double, 6> directionCosines { 1, 0, 0, 0, 1, 0 };
    std::array<double, 2> minMax { 0, 1 };
    ImageType imageType = ImageType::Empty;
    std::uint64_t ID = 0;
    std::string dataUnits = "";

    ImageContainer() { }
    ImageContainer(ImageType imageType, vtkSmartPointer<vtkImageData> imageData, const std::string& units = "")
    {
        image = imageData;
        this->imageType = imageType;
        auto* minmax = image->GetScalarRange();
        minMax[0] = minmax[0];
        minMax[1] = minmax[1];
        dataUnits = units;
    }
    static std::uint64_t generateID(void)
    {
        auto timepoint = std::chrono::system_clock::now();
        auto interval = timepoint.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count();
    }

    
    constexpr static std::underlying_type<ImageType>::type imageTypeToIndex(ImageType e)
    {
        return static_cast<typename std::underlying_type<ImageType>::type>(e);
    }

    std::string getImageName(void)
    {
        return getImageName(imageType);
    }
    static std::string getImageName(ImageContainer::ImageType type)
    {
        if (type == ImageContainer::ImageType::CTImage)
            return "CTImage";
        else if (type == ImageContainer::ImageType::DensityImage)
            return "DensityImage";
        else if (type == ImageContainer::ImageType::MaterialImage)
            return "MaterialImage";
        else if (type == ImageContainer::ImageType::DoseImage)
            return "DoseImage";
        else if (type == ImageContainer::ImageType::OrganImage)
            return "OrganImage";
        else if (type == ImageContainer::ImageType::TallyImage)
            return "DoseTallyImage";
        else if (type == ImageContainer::ImageType::VarianceImage)
            return "VarianceImage";
        else if (type == ImageContainer::ImageType::MeasurementImage)
            return "MeasurementImage";
        return "Unknown";
    }

    ImageContainer(ImageType imageType, std::shared_ptr<std::vector<double>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin, const std::string& units = "")
    {
        this->imageType = imageType;
        dataUnits = units;
        registerVector(imageData, dimensions, dataSpacing, origin, VTK_DOUBLE);
        m_image_data_double = imageData;
    }
    ImageContainer(ImageType imageType, std::shared_ptr<std::vector<float>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin, const std::string& units = "")
    {
        this->imageType = imageType;
        dataUnits = units;
        registerVector(imageData, dimensions, dataSpacing, origin, VTK_FLOAT);
        m_image_data_float = imageData;
    }
    ImageContainer(ImageType imageType, std::shared_ptr<std::vector<unsigned char>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin, const std::string& units = "")
    {
        this->imageType = imageType;
        dataUnits = units;
        registerVector(imageData, dimensions, dataSpacing, origin, VTK_UNSIGNED_CHAR);
        m_image_data_uchar = imageData;
    }
    ImageContainer(ImageType imageType, std::shared_ptr<std::vector<std::uint32_t>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin, const std::string& units = "")
    {
        this->imageType = imageType;
        dataUnits = units;
        registerVector(imageData, dimensions, dataSpacing, origin, VTK_UNSIGNED_INT);
        m_image_data_uint32 = imageData;
    }

protected:
    std::shared_ptr<std::vector<double>> m_image_data_double = nullptr;
    std::shared_ptr<std::vector<float>> m_image_data_float = nullptr;
    std::shared_ptr<std::vector<unsigned char>> m_image_data_uchar = nullptr;
    std::shared_ptr<std::vector<std::uint32_t>> m_image_data_uint32 = nullptr;

private:
    template <typename T>
    void registerVector(std::shared_ptr<std::vector<T>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin, int vtkType)
    {
        if (imageData) {
            vtkSmartPointer<vtkImageImport> importer = vtkSmartPointer<vtkImageImport>::New();
            importer->SetDataSpacing(dataSpacing[0], dataSpacing[1], dataSpacing[2]);
            importer->SetWholeExtent(0, static_cast<int>(dimensions[0]) - 1, 0, static_cast<int>(dimensions[1]) - 1, 0, static_cast<int>(dimensions[2]) - 1);
            importer->SetDataExtentToWholeExtent();
            importer->SetDataScalarType(vtkType);
            importer->SetNumberOfScalarComponents(1);
            importer->SetImportVoidPointer(static_cast<void*>(imageData->data()));
            importer->SetDataOrigin(origin[0], origin[1], origin[2]);

            importer->Update();
            image = importer->GetOutput();
            image->GetDimensions();
            image->ComputeBounds();
            auto* minmax = image->GetScalarRange();
            minMax[0] = minmax[0];
            minMax[1] = minmax[1];
        }
    }
};

class CTImageContainer : public ImageContainer {
public:
    CTImageContainer()
        : ImageContainer()
    {
        imageType = ImageType::CTImage;
        dataUnits = "HU";
    }
    CTImageContainer(std::shared_ptr<std::vector<float>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin)
        : ImageContainer(ImageType::CTImage, imageData, dimensions, dataSpacing, origin, "HU")
    {
    }
    virtual ~CTImageContainer() = default;
    std::shared_ptr<std::vector<float>> imageData(void)
    {
        return m_image_data_float;
    }
};

class DensityImageContainer : public ImageContainer {
public:
    DensityImageContainer()
        : ImageContainer()
    {
        imageType = ImageType::DensityImage;
        dataUnits = "g/cm3";
    }
    DensityImageContainer(std::shared_ptr<std::vector<double>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin)
        : ImageContainer(ImageType::DensityImage, imageData, dimensions, dataSpacing, origin, "g/cm3")
    {
    }
    virtual ~DensityImageContainer() = default;
    std::shared_ptr<std::vector<double>> imageData(void)
    {
        return m_image_data_double;
    }
};

class DoseImageContainer : public ImageContainer {
public:
    DoseImageContainer()
        : ImageContainer()
    {
        imageType = ImageType::DoseImage;
    }
    DoseImageContainer(std::shared_ptr<std::vector<double>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin)
        : ImageContainer(ImageType::DoseImage, imageData, dimensions, dataSpacing, origin)
    {
    }
    virtual ~DoseImageContainer() = default;
    std::shared_ptr<std::vector<double>> imageData(void)
    {
        return m_image_data_double;
    }
};

class OrganImageContainer : public ImageContainer {
public:
    OrganImageContainer()
        : ImageContainer()
    {
        imageType = ImageType::OrganImage;
    }
    OrganImageContainer(std::shared_ptr<std::vector<unsigned char>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin)
        : ImageContainer(ImageType::OrganImage, imageData, dimensions, dataSpacing, origin)
    {
    }
    virtual ~OrganImageContainer() = default;
    std::shared_ptr<std::vector<unsigned char>> imageData(void)
    {
        return m_image_data_uchar;
    }
};

class MaterialImageContainer : public ImageContainer {
public:
    MaterialImageContainer()
        : ImageContainer()
    {
        imageType = ImageType::MaterialImage;
    }
    MaterialImageContainer(std::shared_ptr<std::vector<unsigned char>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin)
        : ImageContainer(ImageType::MaterialImage, imageData, dimensions, dataSpacing, origin)
    {
    }
    virtual ~MaterialImageContainer() = default;
    std::shared_ptr<std::vector<unsigned char>> imageData(void)
    {
        return m_image_data_uchar;
    }
};

class TallyImageContainer : public ImageContainer {
public:
    TallyImageContainer()
        : ImageContainer()
    {
        imageType = ImageType::TallyImage;
    }
    TallyImageContainer(std::shared_ptr<std::vector<std::uint32_t>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin)
        : ImageContainer(ImageType::TallyImage, imageData, dimensions, dataSpacing, origin)
    {
    }
    virtual ~TallyImageContainer() = default;
    std::shared_ptr<std::vector<std::uint32_t>> imageData(void)
    {
        return m_image_data_uint32;
    }
};

class VarianceImageContainer : public ImageContainer {
public:
    VarianceImageContainer()
        : ImageContainer()
    {
        imageType = ImageType::VarianceImage;
    }
    VarianceImageContainer(std::shared_ptr<std::vector<double>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin)
        : ImageContainer(ImageType::VarianceImage, imageData, dimensions, dataSpacing, origin)
    {
    }
    virtual ~VarianceImageContainer() = default;
    std::shared_ptr<std::vector<double>> imageData(void)
    {
        return m_image_data_double;
    }
};

class MeasurementImageContainer : public ImageContainer {
public:
    MeasurementImageContainer()
        : ImageContainer()
    {
        imageType = ImageType::MeasurementImage;
    }
    MeasurementImageContainer(std::shared_ptr<std::vector<unsigned char>> imageData, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin)
        : ImageContainer(ImageType::MeasurementImage, imageData, dimensions, dataSpacing, origin)
    {
    }
    virtual ~MeasurementImageContainer() = default;
    std::shared_ptr<std::vector<unsigned char>> imageData(void)
    {
        return m_image_data_uchar;
    }
};