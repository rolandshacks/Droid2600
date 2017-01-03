#pragma once

#include "bspf.hxx"

class FBSurface
{
    public:
		struct Attributes 
		{
			bool smoothing;    // Scaling is smoothed or blocky
			bool blending;     // Blending is enabled
			uint32_t blendalpha; // Alpha to use in blending mode (0-100%)
		};

    protected:
		static const uInt32* myPalette;
		uint32_t* pixels;
		uint32_t pitch;
        size_t sz;
        int w;
        int h;
		Attributes myAttributes;

    public:
		FBSurface();
        virtual ~FBSurface();
		
	public:
		void alloc(int w, int h, const uint32_t* data);
        void free();
		void basePtr(uint32_t*& pixels, uint32_t& pitch);
        int getWidth() const;
        int getHeight() const;
        size_t getSize() const;
        uint32_t* getPixels() const;
        int getPitch() const;

    public:
		Attributes& attributes();
		void applyAttributes();
		void setDirty();
		void setSrcSize(uint32_t w, uint32_t h);
		void render();

};
