
#include "h5wrapper.h"

#include <memory>
#include <vector>


std::vector<std::shared_ptr<ImageContainer>> getImages(void)
{
	std::array<std::size_t, 3> dim{ 50,50,50 };
	std::array<double, 3> spacing{ .5,.5,.5 };
	std::array<double, 3> origin{ 1,1,1 };
	
	std::vector<std::shared_ptr<ImageContainer>> images;

	auto size = dim[0] * dim[1] * dim[2];
	auto im1 = std::make_shared<std::vector<float>>(size, 0.0);
	std::shared_ptr<ImageContainer> ct = std::make_shared<CTImageContainer>(im1, dim, spacing, origin);
	images.push_back(ct);

	auto im2 = std::make_shared<std::vector<double>>(size, 0.0);
	std::shared_ptr<ImageContainer> dens = std::make_shared<DensityImageContainer>(im2, dim, spacing, origin);
	images.push_back(dens);

	auto im3 = std::make_shared<std::vector<unsigned char>>(size, 0);
	std::shared_ptr<ImageContainer> mat = std::make_shared<MaterialImageContainer>(im3, dim, spacing, origin);
	images.push_back(mat);
	return images;
}




int main(int argc, char* argv[])
{

	H5Wrapper w("test.h5");
	auto images = getImages();
	for (auto im : images)
	{
		w.saveImage(im);
	}
	

	auto ctImageLoaded = w.loadImage(ImageContainer::CTImage);
	auto densImageLoaded = w.loadImage(ImageContainer::DensityImage);
	auto matImageLoaded = w.loadImage(ImageContainer::MaterialImage);
	return 0;

}
