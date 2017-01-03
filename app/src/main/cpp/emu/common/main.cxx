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
// $Id: main.cxx 3308 2016-05-24 16:55:45Z stephena $
//============================================================================

#include <cstdlib>

#include "bspf.hxx"
////#include "MediaFactory.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "EventHandler.hxx"
////#include "FrameBuffer.hxx"
#include "PropsSet.hxx"
#include "Sound.hxx"
#include "Settings.hxx"
////#include "FSNode.hxx"
#include "OSystem.hxx"
#include "System.hxx"

#ifdef DEBUGGER_SUPPORT
#include "Debugger.hxx"
#endif

#ifdef CHEATCODE_SUPPORT
#include "CheatManager.hxx"
#endif

#include "../../emu_bindings.h"

// Pointer to the main parent osystem object or the null pointer
unique_ptr<OSystem> theOSystem;
unique_ptr<Rom> theRom;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Does general Cleanup in case any operation failed (or at end of program)
int Cleanup()
{
    theOSystem->logMessage("Cleanup from main", 2);
    theOSystem->saveConfig();

    return 0;
}

extern "C" {

    int DLLBINDING emu_init(const char* prefs, int flags)
    {
        std::ios_base::sync_with_stdio(false);

        // Create the parent OSystem object
        theOSystem = make_ptr<OSystem>();
        theOSystem->loadConfig();
        theOSystem->logMessage("Loading config options ...", 2);

        // Take care of commandline arguments
        theOSystem->logMessage("Loading commandline arguments ...", 2);
        ////string romfile = "game.bin"; // theOSystem->settings().loadCommandLine(argc, argv);

        // Finally, make sure the settings are valid
        // We do it once here, so the rest of the program can assume valid settings
        theOSystem->logMessage("Validating config options ...", 2);
        theOSystem->settings().validate();

        // Create the full OSystem after the settings, since settings are
        // probably needed for defaults
        theOSystem->logMessage("Creating the OSystem ...", 2);
        if (!theOSystem->create())
        {
            theOSystem->logMessage("ERROR: Couldn't create OSystem", 0);
            Cleanup();
            return -1;
        }

        theRom = make_ptr<Rom>();

        return 0;
    }

    int DLLBINDING emu_input(int keyCode, int state)
    {
        return 0;
    }

    int DLLBINDING emu_load(int data_type, const void* data, int data_size, const char* filename)
    {
        theOSystem->eventHandler().enterMenuMode(EventHandler::S_MENU);

        theRom->create(data, data_size, filename);

        const string& result = theOSystem->createConsole(*theRom);
        if (result != EmptyString)
        {
            Cleanup();
            return -1;
        }

        theOSystem->eventHandler().leaveMenuMode();

        return 0;
    }

    int DLLBINDING emu_store(int data_type, void* buffer, int buffer_size)
    {
        return 0;
    }

    int DLLBINDING emu_command(int command, int param)
    {
        return theOSystem->execCommand(command, param);
    }

    int DLLBINDING emu_update_input(int joystickInput, int flags)
    {
        return theOSystem->updateInput(joystickInput, flags);
    }

    int DLLBINDING emu_update_audio(void* buffer, int bufferLen, int flags)
    {
        return theOSystem->updateAudio(buffer, bufferLen, flags);
    }

    int DLLBINDING emu_update_video(emu_update_info_t* update_info, int flags)
    {
        return theOSystem->updateVideo(update_info, flags);
    }

    int DLLBINDING emu_get(const char* key, char* buffer, int buffer_size)
    {
        if (NULL == key || NULL == buffer) return 0;
        *buffer = '\0';

        std::string cstrKey = key;
        std::string cstrValue = theOSystem->getAttribute(cstrKey);

        if (cstrValue.length() > 0 && cstrValue.length() < buffer_size)
        {
            strcpy(buffer, cstrValue.c_str());
        }

        return strlen(buffer);
    }

    int DLLBINDING emu_shutdown()
    {
        Cleanup();
        theOSystem->quit();
        theOSystem = nullptr;

        theRom->free();
        theRom = nullptr;

        return 0;
    }

} // extern "C"
