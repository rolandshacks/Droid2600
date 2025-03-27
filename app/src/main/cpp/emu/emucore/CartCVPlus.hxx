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
// $Id: CartCVPlus.hxx 3311 2016-08-21 21:37:06Z stephena $
//============================================================================

#ifndef CARTRIDGECVPlus_HXX
#define CARTRIDGECVPlus_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"
#ifdef DEBUGGER_SUPPORT
#include "CartCVPlusWidget.hxx"
#endif

/**
  Cartridge class based on both Commavid and 3F/3E schemes:

  Commavid (RAM):
    $F000-$F3FF read from RAM
    $F400-$F7FF write to RAM

  3F/3E (ROM):
    $F800-$FFFF ROM

  In this bankswitching scheme the 2600's 4K cartridge
  address space is broken into two 2K segments.  The lower 2K
  is RAM, as decribed above (same as CV/Commavid scheme).
  To map ROM, the desired bank number of the upper 2K segment is
  selected by storing its value into $3D.

  @author  Stephen Anthony, LS_Dracon
  @version $Id: CartCVPlus.hxx 3311 2016-08-21 21:37:06Z stephena $
*/

class CartridgeCVPlus : public Cartridge
{
    friend class CartridgeCVPlusWidget;

    public:
      /**
        Create a new cartridge using the specified image and size

        @param image     Pointer to the ROM image
        @param size      The size of the ROM image
        @param settings  A reference to the various settings (read-only)
      */
    CartridgeCVPlus(const uInt8* image, uInt32 size, const Settings& settings);
    virtual ~CartridgeCVPlus() = default;

    public:
      /**
        Reset device to its power-on state
      */
    void reset() override;

    /**
      Install cartridge in the specified system.  Invoked by the system
      when the cartridge is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system) override;

    /**
      Install pages for the specified bank in the system.

      @param bank The bank that should be installed in the system
    */
    bool bank(uInt16 bank) override;

    /**
      Get the current bank.
    */
    uInt16 getBank() const override;

    /**
      Query the number of banks supported by the cartridge.
    */
    uInt16 bankCount() const override;

    /**
      Patch the cartridge ROM.

      @param address  The ROM address to patch
      @param value    The value to place into the address
      @return    Success or failure of the patch operation
    */
    bool patch(uInt16 address, uInt8 value) override;

    /**
      Access the internal ROM image for this cartridge.

      @param size  Set to the size of the internal ROM image data
      @return  A pointer to the internal ROM image data
    */
    const uInt8* getImage(int& size) const override;

    /**
      Save the current state of this cart to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const override;

    /**
      Load the current state of this cart from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in) override;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "CartridgeCV+"; }

#ifdef DEBUGGER_SUPPORT
  /**
    Get debugger widget responsible for accessing the inner workings
    of the cart.
  */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
        return new CartridgeCVPlusWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
#endif

    public:
      /**
        Get the byte at the specified address

        @return The byte at the specified address
      */
    uInt8 peek(uInt16 address) override;

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
      @return  True if the poke changed the device address space, else false
    */
    bool poke(uInt16 address, uInt8 value) override;

    private:
      // Pointer to a dynamically allocated ROM image of the cartridge
    BytePtr myImage;

    // The 1024 bytes of RAM
    uInt8 myRAM[1024];

    // Size of the ROM image
    uInt32 mySize;

    // Indicates which bank is currently active for the first segment
    uInt16 myCurrentBank;

    private:
      // Following constructors and assignment operators not supported
    CartridgeCVPlus() = delete;
    CartridgeCVPlus(const CartridgeCVPlus&) = delete;
    CartridgeCVPlus(CartridgeCVPlus&&) = delete;
    CartridgeCVPlus& operator=(const CartridgeCVPlus&) = delete;
    CartridgeCVPlus& operator=(CartridgeCVPlus&&) = delete;
};

#endif
