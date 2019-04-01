#pragma once

#include <memory>
#include <vector>
#include <array>
#include <chrono>

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkType.h>
#include <vtkImageImport.h>



//used onlu for viz
class ImageContainer
{
public:
	enum ImageType
	{
		CTImage,
		DensityImage,
		MaterialImage,
		DoseImage,
		OrganImage,
		Empty
	};
	vtkSmartPointer<vtkImageData> image;
	std::array<double, 6> directionCosines{ 1,0,0,0,1,0 };
	std::array<double, 2> minMax;
	ImageType imageType = Empty;
	std::uint64_t ID = 0;
	
	ImageContainer() {}
	ImageContainer(ImageType imageType, vtkSmartPointer<vtkImageData> imageData)
	{
		image = imageData;
		this->imageType = imageType;
		auto* minmax = image->GetScalarRange();
		minMax[0] = minmax[0];
		minMax[1] = minmax[1];
	}
	static std::uint64_t generateID(void)
	{
		auto timepoint = std::chrono::system_clock::now();
		auto interval = timepoint.time_since_epoch();
		return std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count();
	}
protected:
	

	ImageContainer(ImageType imageType, std::shared_ptr<std::vector<double>> imageData, const std::array<std::size_t, 3> &dimensions, const std::array<double, 3> &dataSpacing, const std::array<double, 3> &origin)
	{
		//image_data_double = imageData;
		this->imageType = imageType;
		registerVector(imageData, dimensions, dataSpacing, origin, VTK_DOUBLE);
	}
	ImageContainer(ImageType imageType, std::shared_ptr<std::vector<float>> imageData, const std::array<std::size_t, 3> &dimensions, const std::array<double, 3> &dataSpacing, const std::array<double, 3> &origin)
	{
		//image_data_float = imageData;
		this->imageType = imageType;
		registerVector(imageData, dimensions, dataSpacing, origin, VTK_FLOAT);
	}
	ImageContainer(ImageType imageType, std::shared_ptr<std::vector<unsigned char>> imageData, const std::array<std::size_t, 3> &dimensions, const std::array<double, 3> &dataSpacing, const std::array<double, 3> &origin)
	{
		//image_data_uchar = imageData;
		this->imageType = imageType;
		registerVector(imageData, dimensions, dataSpacing, origin, VTK_UNSIGNED_CHAR);
	}

private:
	template<typename T>
	void registerVector(std::shared_ptr<std::vector<T>> imageData, const std::array<std::size_t, 3> &dimensions, const std::array<double, 3> &dataSpacing, const std::array<double, 3> &origin, int vtkType)
	{
		if (imageData)
		{
			vtkSmartPointer<vtkImageImport> importer = vtkSmartPointer<vtkImageImport>::New();
			importer->SetDataSpacing(dataSpacing[0], dataSpacing[1], dataSpacing[2]);
			importer->SetDataOrigin(0.0, 0.0, 0.0);
			importer->SetWholeExtent(0, static_cast<int>(dimensions[0]) - 1, 0, static_cast<int>(dimensions[1]) - 1, 0, static_cast<int>(dimensions[2]) - 1);
			importer->SetDataExtentToWholeExtent();
			importer->SetDataScalarType(vtkType);
			importer->SetNumberOfScalarComponents(1);
			importer->SetImportVoidPointer(static_cast<void*>(imageData->data()));
			importer->SetDataOrigin(origin[0], origin[1], origin[2]);
			
			importer->Update();
			image = importer->GetOutput();
			image->GetDimensions();
			auto* minmax = image->GetScalarRange();
			minMax[0] = minmax[0];
			minMax[1] = minmax[1];

		}
	}


};


class DensityImageContainer :public ImageContainer
{
public:
	DensityImageContainer() :ImageContainer() { imageType = DensityImage; }
	DensityImageContainer(std::shared_ptr<std::vector<double>> imageData, const std::array<std::size_t, 3> &dimensions, const std::array<double, 3> &dataSpacing, const std::array<double, 3> &origin)
		:ImageContainer(DensityImage, imageData, dimensions, dataSpacing, origin)
	{
		m_image_data = imageData;
	}
	std::shared_ptr<std::vector<double>> imageData(void)
	{
		return m_image_data;
	}
private:
	std::shared_ptr<std::vector<double>> m_image_data;
};


class DoseImageContainer :public ImageContainer
{
public:
	DoseImageContainer() :ImageContainer() { imageType = DoseImage; }
	DoseImageContainer(std::shared_ptr<std::vector<double>> imageData, const std::array<std::size_t, 3> &dimensions, const std::array<double, 3> &dataSpacing, const std::array<double, 3> &origin)
		:ImageContainer(DoseImage, imageData, dimensions, dataSpacing, origin)
	{
		m_image_data = imageData;
	}
	std::shared_ptr<std::vector<double>> imageData(void)
	{
		return m_image_data;
	}
private:
	std::shared_ptr<std::vector<double>> m_image_data;
};


class OrganImageContainer :public ImageContainer
{
public:
	OrganImageContainer() :ImageContainer() { imageType = OrganImage; }
	OrganImageContainer(std::shared_ptr<std::vector<unsigned char>> imageData, const std::array<std::size_t, 3> &dimensions, const std::array<double, 3> &dataSpacing, const std::array<double, 3> &origin)
		:ImageContainer(OrganImage, imageData, dimensions, dataSpacing, origin)
	{
		m_image_data = imageData;
	}
	std::shared_ptr<std::vector<unsigned char>> imageData(void)
	{
		return m_image_data;
	}
private:
	std::shared_ptr<std::vector<unsigned char>> m_image_data;
};

class MaterialImageContainer :public ImageContainer
{
public:
	MaterialImageContainer() :ImageContainer() { imageType = MaterialImage; }
	MaterialImageContainer(std::shared_ptr<std::vector<unsigned char>> imageData, const std::array<std::size_t, 3> &dimensions, const std::array<double, 3> &dataSpacing, const std::array<double, 3> &origin)
		:ImageContainer(MaterialImage, imageData, dimensions, dataSpacing, origin)
	{
		m_image_data = imageData;
	}
	std::shared_ptr<std::vector<unsigned char>> imageData(void)
	{
		return m_image_data;
	}
private:
	std::shared_ptr<std::vector<unsigned char>> m_image_data;
};