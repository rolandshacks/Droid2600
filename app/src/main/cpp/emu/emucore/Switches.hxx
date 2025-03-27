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
// $Id: Switches.hxx 3258 2016-01-23 22:56:16Z stephena $
//============================================================================

#ifndef SWITCHES_HXX
#define SWITCHES_HXX

class Event;
class Properties;

#include "Serializable.hxx"
#include "bspf.hxx"

/**
  This class represents the console switches of the game console.

  @author  Bradford W. Mott
  @version $Id: Switches.hxx 3258 2016-01-23 22:56:16Z stephena $
*/
class Switches : public Serializable
{
  /**
    Riot debug class needs special access to the underlying controller state
  */
    friend class RiotDebug;

    public:
      /**
        Create a new set of switches using the specified events and
        properties

        @param event The event object to use for events
      */
    Switches(const Event& event, const Properties& properties);
    virtual ~Switches() = default;

    public:
      /**
        Get the value of the console switches

        @return The 8 bits which represent the state of the console switches
      */
    uInt8 read() const { return mySwitches; }

    /**
      Update the switches variable
    */
    void update();

    /**
      Save the current state of the switches to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const override;

    /**
      Load the current state of the switches from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in) override;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "Switches"; }

    /**
      Query the 'Console_TelevisionType' switches bit.

      @return  True if 'Color', false if 'BlackWhite'
    */
    bool tvColor() const { return (mySwitches & 0x08) != 0; }

    /**
      Query the 'Console_LeftDifficulty' switches bit.

      @return  True if 'A', false if 'B'
    */
    bool leftDifficultyA() const { return (mySwitches & 0x40) != 0; }

    /**
      Query the 'Console_RightDifficulty' switches bit.

      @return  True if 'A', false if 'B'
    */
    bool rightDifficultyA() const { return (mySwitches & 0x80) != 0; }

    private:
      // Reference to the event object to use
    const Event& myEvent;

    // State of the console switches
    uInt8 mySwitches;

    private:
      // Following constructors and assignment operators not supported
    Switches() = delete;
    Switches(const Switches&) = delete;
    Switches(Switches&&) = delete;
    Switches& operator=(const Switches&) = delete;
    Switches& operator=(Switches&&) = delete;
};

#endif
