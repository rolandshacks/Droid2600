//============================================================================
//
// MM     MM  6666  555555  0000   2222
// MMMM MMMM 66  66 55     00  00 22  22
// MM MMM MM 66     55     00  00     22
// MM  M  MM 66666  55555  00  00  22222  --  "A 6502 Microprocessor Emulator"
// MM     MM 66  66     55 00  00 22
// MM     MM 66  66 55  55 00  00 22
// MM     MM  6666   5555   0000  222222
//
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: System.cxx 3240 2015-12-29 21:28:10Z stephena $
//============================================================================

#include <cassert>
#include <iostream>

#include "Device.hxx"
#include "M6502.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "Cart.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
System::System(const OSystem& osystem, M6502& m6502, M6532& m6532,
    TIA& mTIA, Cartridge& mCart)
    : myOSystem(osystem),
    myM6502(m6502),
    myM6532(m6532),
    myTIA(mTIA),
    myCart(mCart),
    myCycles(0),
    myDataBusState(0),
    myDataBusLocked(false),
    mySystemInAutodetect(false)
{
  // Re-initialize random generator
    randGenerator().initSeed();

    // Initialize page access table
    PageAccess access(&myNullDevice, System::PA_READ);
    for (int page = 0; page < System::S_NUM_PAGES; ++page)
    {
        setPageAccess(page, access);
        myPageIsDirtyTable[page] = false;
    }

    // Bus starts out unlocked (in other words, peek() changes myDataBusState)
    myDataBusLocked = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::initialize()
{
  // Install all devices
    myM6532.install(*this);
    myTIA.install(*this);
    myCart.install(*this);
    myM6502.install(*this);  // Must always be installed last
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::reset(bool autodetect)
{
  // Provide hint to devices that autodetection is active (or not)
    mySystemInAutodetect = autodetect;

    // Reset system cycle counter
    resetCycles();

    // Reset all devices
    myM6532.reset();
    myTIA.reset();
    myCart.reset();
    myM6502.reset();  // Must always be reset last

    // There are no dirty pages upon startup
    clearDirtyPages();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::resetCycles()
{
  // First we let all of the device attached to me know about the reset
    myM6532.systemCyclesReset();
    myTIA.systemCyclesReset();
    myCart.systemCyclesReset();

    // Now, we reset cycle count to zero
    myCycles = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool System::isPageDirty(uInt16 start_addr, uInt16 end_addr) const
{
    uInt16 start_page = (start_addr & System::S_ADDRESS_MASK) >> System::S_PAGE_SHIFT;
    uInt16 end_page = (end_addr & System::S_ADDRESS_MASK) >> System::S_PAGE_SHIFT;

    for (uInt16 page = start_page; page <= end_page; ++page)
        if (myPageIsDirtyTable[page])
            return true;

    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::clearDirtyPages()
{
    for (uInt32 i = 0; i < System::S_NUM_PAGES; ++i)
        myPageIsDirtyTable[i] = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 System::peek(uInt16 addr, uInt8 flags)
{
    PageAccess& access = myPageAccessTable[(addr & System::S_ADDRESS_MASK) >> System::S_PAGE_SHIFT];

#ifdef DEBUGGER_SUPPORT
  // Set access type
    if (access.codeAccessBase)
        *(access.codeAccessBase + (addr & System::S_PAGE_MASK)) |= flags;
    else
        access.device->setAccessFlags(addr, flags);
#endif

  // See if this page uses direct accessing or not 
    uInt8 result;
    if (access.directPeekBase)
        result = *(access.directPeekBase + (addr & System::S_PAGE_MASK));
    else
        result = access.device->peek(addr);

#ifdef DEBUGGER_SUPPORT
    if (!myDataBusLocked)
#endif
        myDataBusState = result;

    return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::poke(uInt16 addr, uInt8 value)
{
    uInt16 page = (addr & System::S_ADDRESS_MASK) >> System::S_PAGE_SHIFT;
    PageAccess& access = myPageAccessTable[page];

    // See if this page uses direct accessing or not 
    if (access.directPokeBase)
    {
      // Since we have direct access to this poke, we can dirty its page
        *(access.directPokeBase + (addr & System::S_PAGE_MASK)) = value;
        myPageIsDirtyTable[page] = true;
    }
    else
    {
      // The specific device informs us if the poke succeeded
        myPageIsDirtyTable[page] = access.device->poke(addr, value);
    }

#ifdef DEBUGGER_SUPPORT
    if (!myDataBusLocked)
#endif
        myDataBusState = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 System::getAccessFlags(uInt16 addr) const
{
#ifdef DEBUGGER_SUPPORT
    const PageAccess& access = myPageAccessTable[(addr & ADDRESS_MASK) >> PAGE_SHIFT];

    if (access.codeAccessBase)
        return *(access.codeAccessBase + (addr & System::S_PAGE_MASK));
    else
        return access.device->getAccessFlags(addr);
#else
    return 0;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::setAccessFlags(uInt16 addr, uInt8 flags)
{
#ifdef DEBUGGER_SUPPORT
    PageAccess& access = myPageAccessTable[(addr & ADDRESS_MASK) >> PAGE_SHIFT];

    if (access.codeAccessBase)
        *(access.codeAccessBase + (addr & System::S_PAGE_MASK)) |= flags;
    else
        access.device->setAccessFlags(addr, flags);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool System::save(Serializer& out) const
{
    try
    {
        out.putString(name());
        out.putInt(myCycles);
        out.putByte(myDataBusState);

        // Save the state of each device
        if (!myM6502.save(out))
            return false;
        if (!myM6532.save(out))
            return false;
        if (!myTIA.save(out))
            return false;
        if (!myCart.save(out))
            return false;
    }
    catch (...)
    {
        cerr << "ERROR: System::save" << endl;
        return false;
    }

    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool System::load(Serializer& in)
{
    try
    {
        if (in.getString() != name())
            return false;

        myCycles = in.getInt();
        myDataBusState = in.getByte();

        // Load the state of each device
        if (!myM6502.load(in))
            return false;
        if (!myM6532.load(in))
            return false;
        if (!myTIA.load(in))
            return false;
        if (!myCart.load(in))
            return false;
    }
    catch (...)
    {
        cerr << "ERROR: System::load" << endl;
        return false;
    }

    return true;
}
