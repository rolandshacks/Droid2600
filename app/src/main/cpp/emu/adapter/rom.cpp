
#include "./rom.h"

Rom::Rom()
{
    image = NULL;
    imageSize = 0;
}

Rom::~Rom()
{
    free();
}

Rom::Rom(const Rom& rom)
{
    image = NULL;
    imageSize = 0;

    this->filename = rom.filename;

    if (NULL != rom.image && rom.imageSize > 0)
    {
        this->create(rom.image, rom.imageSize, rom.filename);
    }
}

Rom& Rom::operator=(const Rom& rom)
{
    free();

    this->filename = rom.filename;

    if (NULL != rom.image && rom.imageSize > 0)
    {
        this->create(rom.image, rom.imageSize, rom.filename);
    }

    return *this;
}

void Rom::create(const void* data, int data_size, const std::string& filename)
{
    free();

    this->filename = filename;
    image = new uint8_t[data_size];
    imageSize = data_size;
    memcpy(image, data, imageSize);


    /*
    // DUMP

    LOG("-ROM--------------------------------------------------\n");

    const uint8_t* src = (const uint8_t*) image;

    LOG("%02x %02x %02x %02x %02x %02x %02x %02x\n",
           src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7]);

    src += data_size - 8;

    LOG("%02x %02x %02x %02x %02x %02x %02x %02x\n",
           src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7]);

    LOG("------------------------------------------------------\n");
    */

}

void Rom::free()
{
    if (NULL != image)
    {
        delete [] (uint8_t*) image;
        image = NULL;
    }

    imageSize = 0;
    filename = "";
}

std::string Rom::getNameWithExt(const std::string& s) const
{
	return "";
}

std::string Rom::getShortPath()
{
	return "";
}

bool Rom::equals(const Rom& r) const
{
	return false;
}

size_t Rom::getImageSize() const
{
	return imageSize;
}

const void* Rom::getImage() const
{
	return image;
}
