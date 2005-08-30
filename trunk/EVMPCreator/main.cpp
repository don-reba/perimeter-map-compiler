#include <FreeImage\Wrapper\FreeImagePlus\FreeImagePlus.h>
#include <fstream>
#include <iostream>

typedef unsigned int  uint;
typedef unsigned char byte;
#define ri_cast reinterpret_cast

struct MapPixel
{
	enum Flag { F_HARD };
	uint top_texture    : 24;
	uint flags          : 8;
	uint bottom_texture : 24;
	uint heightmap      : 8;
};

uint ExchangeComponents(uint colour)
{
	return RGB(GetBValue(colour), GetGValue(colour), GetRValue(colour));
}

int main(int argc, char* argv[])
{
	if (argc != 7)
		return 1;
	FreeImage_Initialise(TRUE);
	// load images
	fipImage hardness_img;
	fipImage heightmap_img;
	fipImage ok_img;
	fipImage top_texture_img;
	fipImage bottom_texture_img;
	hardness_img.load(      argv[1]);
	heightmap_img.load(     argv[2]);
	top_texture_img.load(   argv[3]);
	bottom_texture_img.load(argv[4]);
	ok_img.load(            argv[5]);
	if (
		!hardness_img.isValid()    ||
		!heightmap_img.isValid()   ||
		!ok_img.isValid()          ||
		!top_texture_img.isValid() ||
		!bottom_texture_img.isValid())
		return 2;
	// make sure the dimensions are the same
	const uint width (hardness_img.getWidth());
	const uint height(hardness_img.getHeight());
	if (
		width  != heightmap_img.getWidth()      ||
		width  != ok_img.getWidth()             ||
		width  != top_texture_img.getWidth()    ||
		width  != bottom_texture_img.getWidth() ||
		height != heightmap_img.getHeight()     ||
		height != ok_img.getHeight()            ||
		height != top_texture_img.getHeight()   ||
		height != bottom_texture_img.getHeight())
		return 3;
	// flip all images vertically
	hardness_img.flipVertical();
	heightmap_img.flipVertical();
	ok_img.flipVertical();
	top_texture_img.flipVertical();
	bottom_texture_img.flipVertical();
	// convert to corrett formats, if necessary
	if (!hardness_img.isGrayscale())
		hardness_img.convertToGrayscale();
	if (!heightmap_img.isGrayscale())
		heightmap_img.convertToGrayscale();
	if (!ok_img.isGrayscale())
		ok_img.convertToGrayscale();
	if (FIC_RGB != bottom_texture_img.getColorType() || 32 != bottom_texture_img.getBitsPerPixel())
		bottom_texture_img.convertTo32Bits();
	if (FIC_RGB != top_texture_img.getColorType() || 32 != top_texture_img.getBitsPerPixel())
		top_texture_img.convertTo32Bits();
	// write dimensionsto file
	std::ofstream evmp(argv[6], std::ios_base::binary | std::ios_base::out);
	evmp.write(ri_cast<const char*>(&width),  sizeof(int));
	evmp.write(ri_cast<const char*>(&height), sizeof(int));
	// write main data to the file
	// the image is to have a two-pixel border all around
	byte *hardness_iter      (hardness_img.accessPixels());
	byte *heightmap_iter     (heightmap_img.accessPixels());
	byte *ok_iter            (ok_img.accessPixels());
	uint *top_texture_iter   (ri_cast<uint*>(top_texture_img.accessPixels()));
	uint *bottom_texture_iter(ri_cast<uint*>(bottom_texture_img.accessPixels()));
	MapPixel pixel;
	for (uint y(0); y != height; ++y)
	{
		for (uint x(0); x != width; ++x)
		{
			pixel.top_texture     = ExchangeComponents(*top_texture_iter++);
			pixel.bottom_texture  = ExchangeComponents(*bottom_texture_iter++);
			pixel.flags           = *hardness_iter++ ? 1 : 0;
			pixel.flags          |= *ok_iter++       ? 2 : 0;
			pixel.heightmap       = *heightmap_iter++;
			evmp.write(ri_cast<const char*>(&pixel), sizeof(pixel));
		}
		evmp.write(ri_cast<const char*>(&pixel), sizeof(pixel));
	}
	FreeImage_DeInitialise();
	std::cout << "success\n";
	return 0;
}