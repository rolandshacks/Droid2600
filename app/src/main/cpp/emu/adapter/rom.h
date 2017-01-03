#pragma once

#include "bspf.hxx"

class Rom
{
    private:
        void* image;
        size_t imageSize;
        std::string filename;

    public:
        Rom();
        virtual ~Rom();
        Rom(const Rom& rom);
        Rom& operator=(const Rom& rom);

    public:
        void create(const void* data, int data_size, const std::string& filename);
        void free();

    public:
        size_t getImageSize() const;
        const void* getImage() const;

    public:
        std::string getNameWithExt(const std::string& s) const;
        std::string getShortPath();
        bool equals(const Rom& r) const;

};
