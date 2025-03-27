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
// $Id: CartDPCPlus.hxx 3311 2016-08-21 21:37:06Z stephena $
//============================================================================

#ifndef CARTRIDGE_DPC_PLUS_HXX
#define CARTRIDGE_DPC_PLUS_HXX

class System;
#ifdef THUMB_SUPPORT
#include "Thumbulator.hxx"
#endif
#ifdef DEBUGGER_SUPPORT
#include "CartDPCPlusWidget.hxx"
#endif

#include "bspf.hxx"
#include "Cart.hxx"

/**
  Cartridge class used for DPC+, derived from Pitfall II.  There are six 4K
  program banks, a 4K display bank, 1K frequency table and the DPC chip.
  DPC chip access is mapped to $1000 - $1080 ($1000 - $103F is read port,
  $1040 - $107F is write port).

  For complete details on the DPC chip see David P. Crane's United States
  Patent Number 4,644,495.

  @author  Darrell Spice Jr, Fred Quimby, Stephen Anthony, Bradford W. Mott
  @version $Id: CartDPCPlus.hxx 3311 2016-08-21 21:37:06Z stephena $
*/
class CartridgeDPCPlus : public Cartridge
{
    friend class CartridgeDPCPlusWidget;
    friend class CartridgeRamDPCPlusWidget;

    public:
      /**
        Create a new cartridge using the specified image

        @param image     Pointer to the ROM image
        @param size      The size of the ROM image
        @param settings  A reference to the various settings (read-only)
      */
    CartridgeDPCPlus(const uInt8* image, uInt32 size, const Settings& settings);
    virtual ~CartridgeDPCPlus() = default;

    public:
      /**
        Reset device to its power-on state
      */
    void reset() override;

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary
      to override this method for devices that remember cycle counts.
    */
    void systemCyclesReset() override;

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
    string name() const override { return "CartridgeDPC+"; }

#ifdef DEBUGGER_SUPPORT
  /**
    Get debugger widget responsible for accessing the inner workings
    of the cart.
  */
    CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
        const GUI::Font& nfont, int x, int y, int w, int h) override
    {
        return new CartridgeDPCPlusWidget(boss, lfont, nfont, x, y, w, h, *this);
    }
#endif

    public:
      /**
        Get the byte at the specified address.

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
      /**
        Sets the initial state of the DPC pointers and RAM
      */
    void setInitialState();

    /**
      Clocks the random number generator to move it to its next state
    */
    void clockRandomNumberGenerator();

    /**
      Clocks the random number generator to move it to its prior state
    */
    void priorClockRandomNumberGenerator();

    /**
      Updates any data fetchers in music mode based on the number of
      CPU cycles which have passed since the last update.
    */
    void updateMusicModeDataFetchers();

    /**
      Call Special Functions
    */
    void callFunction(uInt8 value);

    private:
      // The ROM image and size
    BytePtr myImage;
    uInt32 mySize;

    // Pointer to the 24K program ROM image of the cartridge
    uInt8* myProgramImage;

    // Pointer to the 4K display ROM image of the cartridge
    uInt8* myDisplayImage;

    // The DPC 8k RAM image, used as:
    //   3K DPC+ driver
    //   4K Display Data
    //   1K Frequency Data
    uInt8 myDPCRAM[8192];

#ifdef THUMB_SUPPORT
    // Pointer to the Thumb ARM emulator object
    unique_ptr<Thumbulator> myThumbEmulator;
#endif

    // Pointer to the 1K frequency table
    uInt8* myFrequencyImage;

    // The top registers for the data fetchers
    uInt8 myTops[8];

    // The bottom registers for the data fetchers
    uInt8 myBottoms[8];

    // The counter registers for the data fetchers
    uInt16 myCounters[8];

    // The counter registers for the fractional data fetchers
    uInt32 myFractionalCounters[8];

    // The fractional increments for the data fetchers
    uInt8 myFractionalIncrements[8];

    // The Fast Fetcher Enabled flag
    bool myFastFetch;

    // Flags that last byte peeked was A9 (LDA #)
    bool myLDAimmediate;

    // Parameter for special functions
    uInt8 myParameter[8];

    // Parameter pointer for special functions
    uInt8 myParameterPointer;

    // The music mode counters
    uInt32 myMusicCounters[3];

    // The music frequency
    uInt32 myMusicFrequencies[3];

    // The music waveforms
    uInt16 myMusicWaveforms[3];

    // The random number generator register
    uInt32 myRandomNumber;

    // System cycle count when the last update to music data fetchers occurred
    Int32 mySystemCycles;

    // Fractional DPC music OSC clocks unused during the last update
    double myFractionalClocks;

    // Indicates which bank is currently active
    uInt16 myCurrentBank;

    private:
      // Following constructors and assignment operators not supported
    CartridgeDPCPlus() = delete;
    CartridgeDPCPlus(const CartridgeDPCPlus&) = delete;
    CartridgeDPCPlus(CartridgeDPCPlus&&) = delete;
    CartridgeDPCPlus& operator=(const CartridgeDPCPlus&) = delete;
    CartridgeDPCPlus& operator=(CartridgeDPCPlus&&) = delete;
};

#endif
