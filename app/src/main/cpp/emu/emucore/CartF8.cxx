//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CartF8.cxx 3316 2016-08-24 23:57:07Z stephena $
//============================================================================

#include "System.hxx"
#include "CartF8.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF8::CartridgeF8(const uInt8* image, uInt32 size, const string& md5,
    const Settings& settings)
    : Cartridge(settings),
    myCurrentBank(0)
{
  // Copy the ROM image into my buffer
    memcpy(myImage, image, std::min(8192u, size));
    createCodeAccessBase(8192);

    // Normally bank 1 is the reset bank, unless we're dealing with ROMs
    // that have been incorrectly created with banks in the opposite order
    myStartBank =
        (md5 == "bc24440b59092559a1ec26055fd1270e" ||  // Private Eye [a]
            md5 == "75ea60884c05ba496473c23a58edf12f" ||  // 8-in-1 Yars Revenge
            md5 == "75ee371ccfc4f43e7d9b8f24e1266b55" ||  // Snow White
            md5 == "74c8a6f20f8adaa7e05183f796eda796" ||  // Tricade Demo
            md5 == "9905f9f4706223dadee84f6867ede8e3" ||  // Challenge
            md5 == "3c7a7b3a0a7e6319b2fa0f923ef6c9af")    // Racer Prototype
        ? 0 : 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF8::reset()
{
  // Upon reset we switch to the reset bank
    bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF8::install(System& system)
{
    mySystem = &system;

    // Install pages for the startup bank
    bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeF8::peek(uInt16 address)
{
    address &= 0x0FFF;

    // Switch banks if necessary
    switch (address)
    {
        case 0x0FF8:
          // Set the current bank to the lower 4k bank
            bank(0);
            break;

        case 0x0FF9:
          // Set the current bank to the upper 4k bank
            bank(1);
            break;

        default:
            break;
    }

    return myImage[(myCurrentBank << 12) + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8::poke(uInt16 address, uInt8)
{
    address &= 0x0FFF;

    // Switch banks if necessary
    switch (address)
    {
        case 0x0FF8:
          // Set the current bank to the lower 4k bank
            bank(0);
            break;

        case 0x0FF9:
          // Set the current bank to the upper 4k bank
            bank(1);
            break;

        default:
            break;
    }
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8::bank(uInt16 bank)
{
    if (bankLocked()) return false;

    // Remember what bank we're in
    myCurrentBank = bank;
    uInt16 offset = myCurrentBank << 12;

    System::PageAccess access(this, System::PA_READ);

    // Set the page accessing methods for the hot spots
    for (uInt32 i = (0x1FF8 & ~System::S_PAGE_MASK); i < 0x2000;
        i += (1 << System::S_PAGE_SHIFT))
    {
        access.codeAccessBase = &myCodeAccessBase[offset + (i & 0x0FFF)];
        mySystem->setPageAccess(i >> System::S_PAGE_SHIFT, access);
    }

    // Setup the page access methods for the current bank
    for (uInt32 address = 0x1000; address < (0x1FF8U & ~System::S_PAGE_MASK);
        address += (1 << System::S_PAGE_SHIFT))
    {
        access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
        access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
        mySystem->setPageAccess(address >> System::S_PAGE_SHIFT, access);
    }
    return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeF8::getBank() const
{
    return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeF8::bankCount() const
{
    return 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8::patch(uInt16 address, uInt8 value)
{
    myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
    return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeF8::getImage(int& size) const
{
    size = 8192;
    return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8::save(Serializer& out) const
{
    try
    {
        out.putString(name());
        out.putShort(myCurrentBank);
    }
    catch (...)
    {
        cerr << "ERROR: CartridgeF8::save" << endl;
        return false;
    }

    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF8::load(Serializer& in)
{
    try
    {
        if (in.getString() != name())
            return false;

        myCurrentBank = in.getShort();
    }
    catch (...)
    {
        cerr << "ERROR: CartridgeF8SC::load" << endl;
        return false;
    }

    // Remember what bank we were in
    bank(myCurrentBank);

    return true;
}
