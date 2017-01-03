
#include "./surface.h"

FBSurface::FBSurface()
{
	pixels = NULL;
	pitch = 0;
    w = h = 0;
    sz = 0;
}

FBSurface::~FBSurface()
{
    free();
}

void FBSurface::alloc(int w, int h, const uInt32* data)
{
	free();

    if (w < 1 || h < 1) return;

    this->w = w;
    this->h = h;

    pitch = w * 4;
    sz = pitch * h;

    pixels = new uint32_t(w);

    if (NULL != data)
    {
        memcpy(pixels, data, sz);
    }
}

void FBSurface::free()
{
    if (NULL != pixels)
    {
        delete[] pixels;
        pixels = NULL;
    }

    pitch = 0;
    w = h = 0;
    sz = 0;
}

int FBSurface::getWidth() const
{
    return w;
}

int FBSurface::getHeight() const
{
    return h;
}

size_t FBSurface::getSize() const
{
    return sz;
}

uint32_t* FBSurface::getPixels() const
{
    return pixels;
}

int FBSurface::getPitch() const
{
    return pitch;
}

FBSurface::Attributes& FBSurface::attributes()
{
	return myAttributes;
}

void FBSurface::applyAttributes()
{ 
	; 
}

void FBSurface::setDirty() 
{
	;
}

void FBSurface::basePtr(uint32_t*& pixels, uint32_t& pitch)
{
	pixels = pixels;
	pitch = pitch;
}

void FBSurface::setSrcSize(uint32_t w, uint32_t h)
{
}

void FBSurface::render()
{
	// render to screen
}
