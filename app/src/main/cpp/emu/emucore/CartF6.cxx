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
// $Id: CartF6.cxx 3316 2016-08-24 23:57:07Z stephena $
//============================================================================

#include "System.hxx"
#include "CartF6.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeF6::CartridgeF6(const uInt8* image, uInt32 size, const Settings& settings)
    : Cartridge(settings),
    myCurrentBank(0)
{
  // Copy the ROM image into my buffer
    memcpy(myImage, image, std::min(16384u, size));
    createCodeAccessBase(16384);

    // Remember startup bank
    myStartBank = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF6::reset()
{
  // Upon reset we switch to the startup bank
    bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeF6::install(System& system)
{
    mySystem = &system;

    // Upon install we'll setup the startup bank
    bank(myStartBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeF6::peek(uInt16 address)
{
    address &= 0x0FFF;

    // Switch banks if necessary
    switch (address)
    {
        case 0x0FF6:
          // Set the current bank to the first 4k bank
            bank(0);
            break;

        case 0x0FF7:
          // Set the current bank to the second 4k bank
            bank(1);
            break;

        case 0x0FF8:
          // Set the current bank to the third 4k bank
            bank(2);
            break;

        case 0x0FF9:
          // Set the current bank to the forth 4k bank
            bank(3);
            break;

        default:
            break;
    }

    return myImage[(myCurrentBank << 12) + address];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF6::poke(uInt16 address, uInt8)
{
    address &= 0x0FFF;

    // Switch banks if necessary
    switch (address)
    {
        case 0x0FF6:
          // Set the current bank to the first 4k bank
            bank(0);
            break;

        case 0x0FF7:
          // Set the current bank to the second 4k bank
            bank(1);
            break;

        case 0x0FF8:
          // Set the current bank to the third 4k bank
            bank(2);
            break;

        case 0x0FF9:
          // Set the current bank to the forth 4k bank
            bank(3);
            break;

        default:
            break;
    }
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF6::bank(uInt16 bank)
{
    if (bankLocked()) return false;

    // Remember what bank we're in
    myCurrentBank = bank;
    uInt16 offset = myCurrentBank << 12;

    System::PageAccess access(this, System::PA_READ);

    // Set the page accessing methods for the hot spots
    for (uInt32 i = (0x1FF6 & ~System::S_PAGE_MASK); i < 0x2000;
        i += (1 << System::S_PAGE_SHIFT))
    {
        access.codeAccessBase = &myCodeAccessBase[offset + (i & 0x0FFF)];
        mySystem->setPageAccess(i >> System::S_PAGE_SHIFT, access);
    }

    // Setup the page access methods for the current bank
    for (uInt32 address = 0x1000; address < (0x1FF6U & ~System::S_PAGE_MASK);
        address += (1 << System::S_PAGE_SHIFT))
    {
        access.directPeekBase = &myImage[offset + (address & 0x0FFF)];
        access.codeAccessBase = &myCodeAccessBase[offset + (address & 0x0FFF)];
        mySystem->setPageAccess(address >> System::S_PAGE_SHIFT, access);
    }
    return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeF6::getBank() const
{
    return myCurrentBank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeF6::bankCount() const
{
    return 4;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF6::patch(uInt16 address, uInt8 value)
{
    myImage[(myCurrentBank << 12) + (address & 0x0FFF)] = value;
    return myBankChanged = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* CartridgeF6::getImage(int& size) const
{
    size = 16384;
    return myImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF6::save(Serializer& out) const
{
    try
    {
        out.putString(name());
        out.putShort(myCurrentBank);
    }
    catch (...)
    {
        cerr << "ERROR: CartridgeF6::save" << endl;
        return false;
    }

    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeF6::load(Serializer& in)
{
    try
    {
        if (in.getString() != name())
            return false;

        myCurrentBank = in.getShort();
    }
    catch (...)
    {
        cerr << "ERROR: CartridgeF6::load" << endl;
        return false;
    }

    // Remember what bank we were in
    bank(myCurrentBank);

    return true;
}
