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
// $Id: Console.hxx 3310 2016-08-18 18:44:57Z stephena $
//============================================================================

#ifndef CONSOLE_HXX
#define CONSOLE_HXX

class Event;
class Switches;
class System;
class TIA;
class M6502;
class M6532;
class Cartridge;
class CompuMate;
////class Debugger;

#include "bspf.hxx"
#include "Control.hxx"
#include "Props.hxx"
#include "TIATables.hxx"
////#include "FrameBuffer.hxx"
#include "Serializable.hxx"
////#include "NTSCFilter.hxx"
#include "EventHandler.hxx"

/**
  Contains detailed info about a console.
*/
struct ConsoleInfo
{
    string BankSwitch;
    string CartName;
    string CartMD5;
    string Control0;
    string Control1;
    string DisplayFormat;
    string InitialFrameRate;
};

/**
  This class represents the entire game console.

  @author  Bradford W. Mott
  @version $Id: Console.hxx 3310 2016-08-18 18:44:57Z stephena $
*/
class Console : public Serializable
{
    public:
      /**
        Create a new console for emulating the specified game using the
        given game image and operating system.

        @param osystem  The OSystem object to use
        @param cart     The cartridge to use with this console
        @param props    The properties for the cartridge
      */
    Console(OSystem& osystem, unique_ptr<Cartridge>& cart,
        const Properties& props);

/**
  Destructor
*/
    virtual ~Console();

    public:
      /**
        Get the controller plugged into the specified jack

        @return The specified controller
      */
    Controller& leftController() const { return *myLeftControl; }
    Controller& rightController() const { return *myRightControl; }

    /**
      Get the TIA for this console

      @return The TIA
    */
    TIA& tia() const { return *myTIA; }

    /**
      Get the properties being used by the game

      @return The properties being used by the game
    */
    const Properties& properties() const { return myProperties; }

    /**
      Get the console switches

      @return The console switches
    */
    Switches& switches() const { return *mySwitches; }

    /**
      Get the 6502 based system used by the console to emulate the game

      @return The 6502 based system
    */
    System& system() const { return *mySystem; }

    /**
      Get the cartridge used by the console which contains the ROM code

      @return The cartridge for this console
    */
    Cartridge& cartridge() const { return *myCart; }

    /**
      Get the 6532 used by the console

      @return The 6532 for this console
    */
    M6532& riot() const { return *myRiot; }

    /**
      Saves the current state of this console class to the given Serializer.

      @param out The serializer device to save to.
      @return The result of the save.  True on success, false on failure.
    */
    bool save(Serializer& out) const override;

    /**
      Loads the current state of this console class from the given Serializer.

      @param in The Serializer device to load from.
      @return The result of the load.  True on success, false on failure.
    */
    bool load(Serializer& in) override;

    /**
      Get a descriptor for this console class (used in error checking).

      @return The name of the object
    */
    string name() const override { return "Console"; }

    /**
      Set the properties to those given

      @param props The properties to use for the current game
    */
    void setProperties(const Properties& props);

    /**
      Query detailed information about this console.
    */
    const ConsoleInfo& about() const { return myConsoleInfo; }

    /**
      Set up the console to use the debugger.
    */
    ////void attachDebugger(Debugger& dbg);

    /**
      Informs the Console of a change in EventHandler state.
    */
    void stateChanged(EventHandler::State state);

    public:
      /**
        Toggle between NTSC/PAL/SECAM (and variants) display format.

        @param direction +1 indicates increase, -1 indicates decrease.
      */
    void toggleFormat(int direction = 1);

    /**
      Toggle between the available palettes.
    */
    void togglePalette();

    /**
      Sets the palette according to the given palette name.

      @param palette  The palette to switch to.
    */
    void setPalette(const string& palette);

    /**
      Toggles phosphor effect.
    */
    void togglePhosphor();

    /**
      Toggles the PAL color-loss effect.
    */
    void toggleColorLoss();
    void toggleColorLoss(bool state);

    /**
      Initialize the video subsystem wrt this class.
      This is required for changing window size, title, etc.

      @param full  Whether we want a full initialization,
                   or only reset certain attributes.

      @return  The results from FrameBuffer::initialize()
    */
    void initializeVideo(bool full = true);

    /**
      Initialize the audio subsystem wrt this class.
      This is required any time the sound settings change.
    */
    void initializeAudio();

    /**
      "Fry" the Atari (mangle memory/TIA contents)
    */
    void fry() const;

    /**
      Change the "Display.YStart" variable.

      @param direction +1 indicates increase, -1 indicates decrease.
    */
    void changeYStart(int direction);

    /**
      Change the "Display.Height" variable.

      @param direction +1 indicates increase, -1 indicates decrease.
    */
    void changeHeight(int direction);

    /**
      Sets the framerate of the console, which in turn communicates
      this to all applicable subsystems.
    */
    void setFramerate(float framerate);

    /**
      Returns the framerate based on a number of factors
      (whether 'framerate' is set, what display format is in use, etc)
    */
    float getFramerate() const { return myFramerate; }

    /**
      Toggles the TIA bit specified in the method name.
    */
    void toggleP0Bit() const { toggleTIABit(P0Bit, "P0"); }
    void toggleP1Bit() const { toggleTIABit(P1Bit, "P1"); }
    void toggleM0Bit() const { toggleTIABit(M0Bit, "M0"); }
    void toggleM1Bit() const { toggleTIABit(M1Bit, "M1"); }
    void toggleBLBit() const { toggleTIABit(BLBit, "BL"); }
    void togglePFBit() const { toggleTIABit(PFBit, "PF"); }
    void toggleHMOVE() const;
    void toggleBits() const;

    /**
      Toggles the TIA collisions specified in the method name.
    */
    void toggleP0Collision() const { toggleTIACollision(P0Bit, "P0"); }
    void toggleP1Collision() const { toggleTIACollision(P1Bit, "P1"); }
    void toggleM0Collision() const { toggleTIACollision(M0Bit, "M0"); }
    void toggleM1Collision() const { toggleTIACollision(M1Bit, "M1"); }
    void toggleBLCollision() const { toggleTIACollision(BLBit, "BL"); }
    void togglePFCollision() const { toggleTIACollision(PFBit, "PF"); }
    void toggleCollisions() const;

    /**
      Toggles the TIA 'fixed debug colors' mode.
    */
    void toggleFixedColors() const;

    /**
      Toggles the TIA 'scanline jitter' mode.
    */
    void toggleJitter() const;

    private:
      /**
        Sets various properties of the TIA (YStart, Height, etc) based on
        the current display format.
      */
    void setTIAProperties();

    /**
      Adds the left and right controllers to the console.
    */
    void setControllers(const string& rommd5);

    /**
      Loads a user-defined palette file (from OSystem::paletteFile), filling the
      appropriate user-defined palette arrays.
    */
    void loadUserPalette();

    /**
      Loads all defined palettes with PAL color-loss data, even those that
      normally can't have it enabled (NTSC), since it's also used for
      'greying out' the frame in the debugger.
    */
    void generateColorLossPalette();

    void toggleTIABit(TIABit bit, const string& bitname, bool show = true) const;
    void toggleTIACollision(TIABit bit, const string& bitname, bool show = true) const;

    private:
      // Reference to the osystem object
    OSystem& myOSystem;

    // Reference to the event object to use
    const Event& myEvent;

    // Properties for the game
    Properties myProperties;

    // Pointer to the 6502 based system being emulated 
    unique_ptr<System> mySystem;

    // Pointer to the M6502 CPU
    unique_ptr<M6502> my6502;

    // Pointer to the 6532 (aka RIOT) (the debugger needs it)
    // A RIOT of my own! (...with apologies to The Clash...)
    unique_ptr<M6532> myRiot;

    // Pointer to the TIA object 
    unique_ptr<TIA> myTIA;

    // Pointer to the Cartridge (the debugger needs it)
    unique_ptr<Cartridge> myCart;

    // Pointer to the switches on the front of the console
    unique_ptr<Switches> mySwitches;

    // Pointers to the left and right controllers
    unique_ptr<Controller> myLeftControl, myRightControl;

    // Pointer to CompuMate handler (only used in CompuMate ROMs)
    shared_ptr<CompuMate> myCMHandler;

    // The currently defined display format (NTSC/PAL/SECAM)
    string myDisplayFormat;

    // The currently defined display framerate
    float myFramerate;

    // Display format currently in use
    uInt32 myCurrentFormat;

    // Indicates whether an external palette was found and
    // successfully loaded
    bool myUserPaletteDefined;

    // Contains detailed info about this console
    ConsoleInfo myConsoleInfo;

    // Table of RGB values for NTSC, PAL and SECAM
    static uInt32 ourNTSCPalette[256];
    static uInt32 ourPALPalette[256];
    static uInt32 ourSECAMPalette[256];

    // Table of RGB values for NTSC, PAL and SECAM - Z26 version
    static uInt32 ourNTSCPaletteZ26[256];
    static uInt32 ourPALPaletteZ26[256];
    static uInt32 ourSECAMPaletteZ26[256];

    // Table of RGB values for NTSC, PAL and SECAM - user-defined
    static uInt32 ourUserNTSCPalette[256];
    static uInt32 ourUserPALPalette[256];
    static uInt32 ourUserSECAMPalette[256];

    private:
      // Following constructors and assignment operators not supported
    Console() = delete;
    Console(const Console&) = delete;
    Console(Console&&) = delete;
    Console& operator=(const Console&) = delete;
    Console& operator=(Console&&) = delete;
};

#endif
