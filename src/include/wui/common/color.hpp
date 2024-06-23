
#pragma once

namespace wui
{

typedef unsigned long color;

static inline color make_color(unsigned char red, unsigned char green, unsigned char blue)
{
    return ((red) | (static_cast<unsigned short>(green) << 8)) | (static_cast<unsigned long>(blue) << 16);
}

static inline color make_color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
{
	return ((red) | (static_cast<unsigned short>(green) << 8)) | (static_cast<unsigned long>(blue) << 16) | (static_cast<unsigned long>(alpha) << 24);
}

static inline unsigned char get_alpha(color rgb)
{
	return (rgb >> 24) & 0xFF;
}

static inline unsigned char get_red(color rgb)
{
	return (rgb >> 16) & 0xFF;
}

static inline unsigned char get_green(color rgb)
{
	return (rgb >> 8) & 0xFF;
}

static inline unsigned char get_blue(color rgb)
{
	return rgb & 0xFF;
}

}
