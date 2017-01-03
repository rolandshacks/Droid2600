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
// $Id: OSystem.cxx 3308 2016-05-24 16:55:45Z stephena $
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>

#include <ctime>
#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#endif

#include "bspf.hxx"

////#include "MediaFactory.hxx"
#include "Sound.hxx"
#include "soundbuffer.h"

#define SWAP_RGB

#ifdef DEBUGGER_SUPPORT
#include "Debugger.hxx"
#endif

#ifdef CHEATCODE_SUPPORT
#include "CheatManager.hxx"
#endif

////#include "FSNode.hxx"
#include "MD5.hxx"
#include "Cart.hxx"
#include "Settings.hxx"
#include "PropsSet.hxx"
#include "EventHandler.hxx"
////#include "Menu.hxx"
////#include "CommandMenu.hxx"
////#include "Launcher.hxx"
////#include "Widget.hxx"
#include "Console.hxx"
#include "Random.hxx"
////#include "SerialPort.hxx"
#include "StateManager.hxx"
#include "Version.hxx"

#include "TIA.hxx"
#include "OSystem.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::OSystem()
    : myConsole(nullptr),
      ////myLauncherUsed(false),
    myQuitLoop(false)
{
  // Calculate startup time
    myMillisAtStart = uInt32(time(NULL) * 1000);

    // Get built-in features
#ifdef SOUND_SUPPORT
    myFeatures += "Sound ";
#endif
#ifdef JOYSTICK_SUPPORT
    myFeatures += "Joystick ";
#endif
#ifdef DEBUGGER_SUPPORT
    myFeatures += "Debugger ";
#endif
#ifdef CHEATCODE_SUPPORT
    myFeatures += "Cheats";
#endif

    // Get build info
    ostringstream info;

    info << "Build " << STELLA_BUILD << " [" << BSPF::ARCH << "]";
    myBuildInfo = info.str();

    mySettings = make_ptr<Settings>(*this); ////MediaFactory::createSettings(*this);
    myRandom = make_ptr<Random>(*this);

    myPalette = NULL;
    lastJoystickInput = 0x0;
    lastSoundCycle = 0;

    selectTriggered = selectReleased = false;
    resetTriggered = resetReleased = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::~OSystem()
{
    freePalette();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::create()
{
  // Get updated paths for all configuration files
    setConfigPaths();
    ostringstream buf;
    buf << "Stella " << STELLA_VERSION << endl
        << "  Features: " << myFeatures << endl
        << "  " << myBuildInfo << endl;
        //<< "Base directory:       '"
        //<< FilesystemNode(myBaseDir).getShortPath() << "'" << endl
        //<< "Configuration file:   '"
        //<< FilesystemNode(myConfigFile).getShortPath() << "'" << endl
        //<< "User game properties: '"
        //<< FilesystemNode(myPropertiesFile).getShortPath() << "'" << endl;
    logMessage(buf.str(), 1);

    // NOTE: The framebuffer MUST be created before any other object!!!
    // Get relevant information about the video hardware
    // This must be done before any graphics context is created, since
    // it may be needed to initialize the size of graphical objects
    /*
    try        { myFrameBuffer = MediaFactory::createVideo(*this); }
    catch(...) { return false; }
    if(!myFrameBuffer->initialize())
      return false;
    */

    // Create the event handler for the system
    myEventHandler = make_ptr<EventHandler>(*this); //// MediaFactory::createEventHandler(*this);
    myEventHandler->initialize();

    // Create a properties set for us to use and set it up
    myPropSet = make_ptr<PropertiesSet>(propertiesFile());

#ifdef CHEATCODE_SUPPORT
    myCheatManager = make_ptr<CheatManager>(*this);
    myCheatManager->loadCheatDatabase();
#endif

  // Create menu and launcher GUI objects
  //myMenu = make_ptr<Menu>(*this);
  //myCommandMenu = make_ptr<CommandMenu>(*this);
  //myLauncher = make_ptr<Launcher>(*this);
    myStateManager = make_ptr<StateManager>(*this);

    // Create the sound object; the sound subsystem isn't actually
    // opened until needed, so this is non-blocking (on those systems
    // that only have a single sound device (no hardware mixing)
    createSound();

    // Create the serial port object
    // This is used by any controller that wants to directly access
    // a real serial port on the system
    ////mySerialPort = MediaFactory::createSerialPort();

    // Re-initialize random seed
    myRandom->initSeed();

    // Create PNG handler
    ////myPNGLib = make_ptr<PNGLibrary>(*myFrameBuffer);

    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::loadConfig()
{
    mySettings->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::saveConfig()
{
  // Ask all subsystems to save their settings
  ////if(myFrameBuffer)
  ////  myFrameBuffer->tiaSurface().ntsc().saveConfig(*mySettings);

    if (mySettings)
        mySettings->saveConfig();

    if (myPropSet)
        myPropSet->save(myPropertiesFile);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setConfigPaths()
{
    /*
  // Paths are saved with special characters preserved ('~' or '.')
  // We do some error checking here, so the rest of the codebase doesn't
  // have to worry about it
  FilesystemNode node;
  string s;

  validatePath(myStateDir, "statedir", myBaseDir + "state");
  validatePath(mySnapshotSaveDir, "snapsavedir", defaultSnapSaveDir());
  validatePath(mySnapshotLoadDir, "snaploaddir", defaultSnapLoadDir());
  validatePath(myNVRamDir, "nvramdir", myBaseDir + "nvram");
  validatePath(myCfgDir, "cfgdir", myBaseDir + "cfg");

  s = mySettings->getString("cheatfile");
  if(s == "") s = myBaseDir + "stella.cht";
  node = FilesystemNode(s);
  myCheatFile = node.getPath();
  mySettings->setValue("cheatfile", node.getShortPath());

  s = mySettings->getString("palettefile");
  if(s == "") s = myBaseDir + "stella.pal";
  node = FilesystemNode(s);
  myPaletteFile = node.getPath();
  mySettings->setValue("palettefile", node.getShortPath());

  s = mySettings->getString("propsfile");
  if(s == "") s = myBaseDir + "stella.pro";
  node = FilesystemNode(s);
  myPropertiesFile = node.getPath();
  mySettings->setValue("propsfile", node.getShortPath());
  */
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setBaseDir(const string& basedir)
{
    /*
  FilesystemNode node(basedir);
  if(!node.isDirectory())
    node.makeDir();

  myBaseDir = node.getPath();
    */
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setConfigFile(const string& file)
{
    /*
  FilesystemNode node(file);
  myConfigFile = node.getPath();
  */
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setFramerate(float framerate)
{
    if (framerate > 0.0)
    {
        myDisplayFrameRate = framerate;
        myTimePerFrame = uInt32(1000000.0 / myDisplayFrameRate);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::createFrameBuffer()
{
    myConsole->initializeVideo();

    /*
  // Re-initialize the framebuffer to current settings
  FBInitStatus fbstatus = kFailComplete;
  switch(myEventHandler->state())
  {
    case EventHandler::S_EMULATE:
    case EventHandler::S_PAUSE:
    case EventHandler::S_MENU:
    case EventHandler::S_CMDMENU:
      if((fbstatus = myConsole->initializeVideo()) != kSuccess)
        return fbstatus;
      break;  // S_EMULATE, S_PAUSE, S_MENU, S_CMDMENU

    case EventHandler::S_LAUNCHER:
      if((fbstatus = myLauncher->initializeVideo()) != kSuccess)
        return fbstatus;
      break;  // S_LAUNCHER

#ifdef DEBUGGER_SUPPORT
    case EventHandler::S_DEBUGGER:
      if((fbstatus = myDebugger->initializeVideo()) != kSuccess)
        return fbstatus;
      break;  // S_DEBUGGER
#endif

    default:  // Should never happen
      logMessage("ERROR: Unknown emulation state in createFrameBuffer()", 0);
      break;
  }
  return fbstatus;
  */
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::createSound()
{
    if (!mySound)
        mySound = make_ptr<SoundBuffer>(*this);
#ifndef SOUND_SUPPORT
    mySettings->setValue("sound", false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::createConsole(const Rom& rom, const string& md5sum,
    bool newrom)
{
    bool showmessage = false;

    // If same ROM has been given, we reload the current one (assuming one exists)
    if (!newrom && myRomFile.equals(rom))
    {
        showmessage = true;  // we show a message if a ROM is being reloaded
    }
    else
    {
        myRomFile = rom;
        myRomMD5 = md5sum;

        // Each time a new console is loaded, we simulate a cart removal
        // Some carts need knowledge of this, as they behave differently
        // based on how many power-cycles they've been through since plugged in
        mySettings->setValue("romloadcount", 0);
    }

    // Create an instance of the 2600 game console
    ostringstream buf;
    string type, id;
    try
    {
        closeConsole();
        myConsole = openConsole(myRomFile, myRomMD5, type, id);
    }
    catch (const runtime_error& e)
    {
        buf << "ERROR: Couldn't create console (" << e.what() << ")";
        logMessage(buf.str(), 0);
        return buf.str();
    }

    if (myConsole)
    {
#ifdef DEBUGGER_SUPPORT
        myDebugger = make_ptr<Debugger>(*this, *myConsole);
        myDebugger->initialize();
        myConsole->attachDebugger(*myDebugger);
#endif
#ifdef CHEATCODE_SUPPORT
        myCheatManager->loadCheats(myRomMD5);
#endif
        myEventHandler->reset(EventHandler::S_EMULATE);
        myEventHandler->setMouseControllerMode(mySettings->getString("usemouse"));
        createFrameBuffer();
        myConsole->initializeAudio();

        if (showmessage)
        {
            if (id == "")
                showMessage("New console created");
            else
                showMessage("Multicart " + type + ", loading ROM" + id);
        }
        buf << "Game console created:" << endl
            << "  ROM file: " << myRomFile.getShortPath() << endl << endl
            << getROMInfo(*myConsole) << endl;
        logMessage(buf.str(), 1);

        // Update the timing info for a new console run
        resetLoopTiming();

        ////myFrameBuffer->setCursorState();

        // Also check if certain virtual buttons should be held down
        // These must be checked each time a new console is being created
        if (mySettings->getBool("holdreset"))
            myEventHandler->handleEvent(Event::ConsoleReset, 1);
        if (mySettings->getBool("holdselect"))
            myEventHandler->handleEvent(Event::ConsoleSelect, 1);

        const string& holdjoy0 = mySettings->getString("holdjoy0");
        if (BSPF::containsIgnoreCase(holdjoy0, "U"))
            myEventHandler->handleEvent(Event::JoystickZeroUp, 1);
        if (BSPF::containsIgnoreCase(holdjoy0, "D"))
            myEventHandler->handleEvent(Event::JoystickZeroDown, 1);
        if (BSPF::containsIgnoreCase(holdjoy0, "L"))
            myEventHandler->handleEvent(Event::JoystickZeroLeft, 1);
        if (BSPF::containsIgnoreCase(holdjoy0, "R"))
            myEventHandler->handleEvent(Event::JoystickZeroRight, 1);
        if (BSPF::containsIgnoreCase(holdjoy0, "F"))
            myEventHandler->handleEvent(Event::JoystickZeroFire, 1);

        const string& holdjoy1 = mySettings->getString("holdjoy1");
        if (BSPF::containsIgnoreCase(holdjoy1, "U"))
            myEventHandler->handleEvent(Event::JoystickOneUp, 1);
        if (BSPF::containsIgnoreCase(holdjoy1, "D"))
            myEventHandler->handleEvent(Event::JoystickOneDown, 1);
        if (BSPF::containsIgnoreCase(holdjoy1, "L"))
            myEventHandler->handleEvent(Event::JoystickOneLeft, 1);
        if (BSPF::containsIgnoreCase(holdjoy1, "R"))
            myEventHandler->handleEvent(Event::JoystickOneRight, 1);
        if (BSPF::containsIgnoreCase(holdjoy1, "F"))
            myEventHandler->handleEvent(Event::JoystickOneFire, 1);
#ifdef DEBUGGER_SUPPORT
        if (mySettings->getBool("debug"))
            myEventHandler->enterDebugMode();
#endif
    }
    return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::reloadConsole()
{
    return createConsole(myRomFile, myRomMD5, false) == EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::hasConsole() const
{
    return myConsole != nullptr &&
        myEventHandler->state() != EventHandler::S_LAUNCHER;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::createLauncher(const string& startdir)
{
    closeConsole();

    if (mySound)
        mySound->close();

    mySettings->setValue("tmpromdir", startdir);
    bool status = false;

    myEventHandler->reset(EventHandler::S_LAUNCHER);
    createFrameBuffer();
    ////myLauncher->reStack();
    ////myFrameBuffer->setCursorState();

    setFramerate(60);
    resetLoopTiming();
    status = true;

    ////myLauncherUsed = myLauncherUsed || status;
    return status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::getROMInfo(const Rom& romfile)
{
    string md5, type, id, result = "";
    unique_ptr<Console> console;
    try
    {
        console = openConsole(romfile, md5, type, id);
    }
    catch (const runtime_error& e)
    {
        ostringstream buf;
        buf << "ERROR: Couldn't get ROM info (" << e.what() << ")";
        return buf.str();
    }

    result = getROMInfo(*console);
    return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::logMessage(const string& message, uInt8 level)
{
    if (level == 0)
    {
        cout << message << endl << std::flush;
        myLogMessages += message + "\n";
    }
    else if (level <= uInt8(mySettings->getInt("loglevel")))
    {
        if (mySettings->getBool("logtoconsole"))
            cout << message << endl << std::flush;
        myLogMessages += message + "\n";
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<Console> OSystem::openConsole(const Rom& romfile,
    string& md5, string& type, string& id)
{
    unique_ptr<Console> console;

    // Open the cartridge image and read it in
    const uInt8* image = NULL;
    uInt32 size = 0;

    if ((image = openROM(romfile, md5, size)) != nullptr)
    {
      // Get a valid set of properties, including any entered on the commandline
      // For initial creation of the Cart, we're only concerned with the BS type
        Properties props;
        myPropSet->getMD5(md5, props);

        auto CMDLINE_PROPS_UPDATE = [&](const string& name, PropertyType prop)
        {
            const string& s = mySettings->getString(name);
            if (s != "") props.set(prop, s);
        };

        CMDLINE_PROPS_UPDATE("bs", Cartridge_Type);
        CMDLINE_PROPS_UPDATE("type", Cartridge_Type);

        // Now create the cartridge
        string cartmd5 = md5;
        type = props.get(Cartridge_Type);
        unique_ptr<Cartridge> cart =
            Cartridge::create(image, size, cartmd5, type, id, *this, *mySettings);

          // It's possible that the cart created was from a piece of the image,
          // and that the md5 (and hence the cart) has changed
        if (props.get(Cartridge_MD5) != cartmd5)
        {
            if (!myPropSet->getMD5(cartmd5, props))
            {
              // Cart md5 wasn't found, so we create a new props for it
                props.set(Cartridge_MD5, cartmd5);
                props.set(Cartridge_Name, props.get(Cartridge_Name) + id);
                myPropSet->insert(props, false);
            }
        }

        CMDLINE_PROPS_UPDATE("channels", Cartridge_Sound);
        CMDLINE_PROPS_UPDATE("ld", Console_LeftDifficulty);
        CMDLINE_PROPS_UPDATE("rd", Console_RightDifficulty);
        CMDLINE_PROPS_UPDATE("tv", Console_TelevisionType);
        CMDLINE_PROPS_UPDATE("sp", Console_SwapPorts);
        CMDLINE_PROPS_UPDATE("lc", Controller_Left);
        CMDLINE_PROPS_UPDATE("rc", Controller_Right);
        const string& s = mySettings->getString("bc");
        if (s != "") { props.set(Controller_Left, s); props.set(Controller_Right, s); }
        CMDLINE_PROPS_UPDATE("cp", Controller_SwapPaddles);
        CMDLINE_PROPS_UPDATE("ma", Controller_MouseAxis);
        CMDLINE_PROPS_UPDATE("format", Display_Format);
        CMDLINE_PROPS_UPDATE("ystart", Display_YStart);
        CMDLINE_PROPS_UPDATE("height", Display_Height);
        CMDLINE_PROPS_UPDATE("pp", Display_Phosphor);
        CMDLINE_PROPS_UPDATE("ppblend", Display_PPBlend);

        // Finally, create the cart with the correct properties
        if (cart)
            console = make_ptr<Console>(*this, cart, props);
    }

    return console;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::closeConsole()
{
    if (myConsole)
    {
#ifdef CHEATCODE_SUPPORT
  // If a previous console existed, save cheats before creating a new one
        myCheatManager->saveCheats(myConsole->properties().get(Cartridge_MD5));
#endif
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt8* OSystem::openROM(const Rom& rom, string& md5, uInt32& size)
{
  // This method has a documented side-effect:
  // It not only loads a ROM and creates an array with its contents,
  // but also adds a properties entry if the one for the ROM doesn't
  // contain a valid name

    size = rom.getImageSize();
    const uInt8* image = (const uInt8*) rom.getImage();

    if (NULL == image) return NULL;

    // If we get to this point, we know we have a valid file to open
    // Now we make sure that the file has a valid properties entry
    // To save time, only generate an MD5 if we really need one
    if (md5 == "")
        md5 = MD5::hash(image, size);

      // Some games may not have a name, since there may not
      // be an entry in stella.pro.  In that case, we use the rom name
      // and reinsert the properties object
    Properties props;
    myPropSet->getMD5WithInsert(rom, md5, props);

    return image;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::getROMInfo(const Console& console)
{
    const ConsoleInfo& info = console.about();
    ostringstream buf;

    buf << "  Cart Name:       " << info.CartName << endl
        << "  Cart MD5:        " << info.CartMD5 << endl
        << "  Controller 0:    " << info.Control0 << endl
        << "  Controller 1:    " << info.Control1 << endl
        << "  Display Format:  " << info.DisplayFormat << endl
        << "  Bankswitch Type: " << info.BankSwitch << endl;

    return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::resetLoopTiming()
{
    myTimingInfo.start = myTimingInfo.virt = getTicks();
    myTimingInfo.current = 0;
    myTimingInfo.totalTime = 0;
    myTimingInfo.totalFrames = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::validatePath(string& path, const string& setting,
    const string& defaultpath)
{
    /*
  const string& s = mySettings->getString(setting) == "" ? defaultpath :
                    mySettings->getString(setting);
  FilesystemNode node(s);
  if(!node.isDirectory())
    node.makeDir();

  path = node.getPath();
  mySettings->setValue(setting, node.getShortPath());
  */
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 OSystem::getTicks() const
{
#ifdef HAVE_GETTIMEOFDAY
  // Gettimeofday natively refers to the UNIX epoch (a set time in the past)
    timeval now;
    gettimeofday(&now, 0);

    return uInt64(now.tv_sec) * 1000000 + now.tv_usec;
#else
    return (uInt64) Core::System::Clock::getMicros();
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::mainLoop()
{
    ////updateFramebuffer();

/*
  if(mySettings->getString("timing") == "sleep")
  {
    // Sleep-based wait: good for CPU, bad for graphical sync
    for(;;)
    {
      myTimingInfo.start = getTicks();
      myEventHandler->poll(myTimingInfo.start);

      if(myQuitLoop) break;  // Exit if the user wants to quit

      updateFramebuffer();
      myTimingInfo.current = getTicks();
      myTimingInfo.virt += myTimePerFrame;

      // Timestamps may periodically go out of sync, particularly on systems
      // that can have 'negative time' (ie, when the time seems to go backwards)
      // This normally results in having a very large delay time, so we check
      // for that and reset the timers when appropriate
      if((myTimingInfo.virt - myTimingInfo.current) > (myTimePerFrame << 1))
      {
        myTimingInfo.start = myTimingInfo.current = myTimingInfo.virt = getTicks();
      }

      if(myTimingInfo.current < myTimingInfo.virt)
        SDL_Delay(uInt32(myTimingInfo.virt - myTimingInfo.current) / 1000);

      myTimingInfo.totalTime += (getTicks() - myTimingInfo.start);
      myTimingInfo.totalFrames++;
    }
  }
  else
  {
    // Busy-wait: bad for CPU, good for graphical sync
    for(;;)
    {
      myTimingInfo.start = getTicks();
      myEventHandler->poll(myTimingInfo.start);

      if(myQuitLoop) break;  // Exit if the user wants to quit
      updateFramebuffer();
      myTimingInfo.virt += myTimePerFrame;

      while(getTicks() < myTimingInfo.virt)
        ;  // busy-wait

      myTimingInfo.totalTime += (getTicks() - myTimingInfo.start);
      myTimingInfo.totalFrames++;
    }
  }

  // Cleanup time
#ifdef CHEATCODE_SUPPORT
  if(myConsole)
    myCheatManager->saveCheats(myConsole->properties().get(Cartridge_MD5));

  myCheatManager->saveCheatDatabase();
#endif
*/
}

void OSystem::showMessage(const std::string& message, int positionInfo, bool force)
{
    /// TODO
    printf("LOG: %s\n", message.c_str());
}

void OSystem::setPalette(const uInt32* palette)
{
    freePalette();

    myPalette = new uInt32[256];

    #ifdef SWAP_RGB

    for (int i=0; i<256; i++)
    {
        uint32_t p = palette[i];
        uint8_t r = (p & 0x00ff0000) >> 16;
        uint8_t g = (p & 0x0000ff00) >> 8;
        uint8_t b = (p & 0x000000ff) >> 0;
        uint32_t c = (r >> 0) + (g << 8) + (b << 16);

        myPalette[i] = c;
    }

    #else

        memcpy(myPalette, palette, 256*sizeof(uInt32));

    #endif
}

void OSystem::freePalette()
{
    if (NULL != myPalette)
    {
        delete [] myPalette;
        myPalette = NULL;
    }
}

void OSystem::enablePhosphor(bool enable, int blend)
{
}

void OSystem::setSound(uInt16 addr, uInt8 value, Int32 cycle)
{
    double delta = double(cycle - lastSoundCycle) / 1193191.66666667;
    lastSoundCycle = cycle;
}

void OSystem::processInputs()
{
    if (resetTriggered) {
        resetTriggered = false;
        myEventHandler->handleEvent(Event::ConsoleReset, 1);
        resetReleased = true;
    } else if (resetReleased) {
        resetReleased = false;
        myEventHandler->handleEvent(Event::ConsoleReset, 0);
    } else if (selectTriggered) {
        selectTriggered = false;
        myEventHandler->handleEvent(Event::ConsoleSelect, 1);
        selectReleased = true;
    } else if (selectReleased) {
        selectReleased = false;
        myEventHandler->handleEvent(Event::ConsoleSelect, 0);
    }
}

int OSystem::updateInput(int joystickInput, int flags)
{
    processInputs();

    if (joystickInput != lastJoystickInput)
    {
        bool joystickSwap = ((flags&0x4) != 0);

        if (!joystickSwap)
        {
            if ((joystickInput & 0x01) != (lastJoystickInput & 0x01))
                myEventHandler->handleEvent(Event::JoystickZeroFire,
                                            (joystickInput & 0x01) != 0 ? 1 : 0);
            if ((joystickInput & 0x02) != (lastJoystickInput & 0x02))
                myEventHandler->handleEvent(Event::JoystickZeroLeft,
                                            (joystickInput & 0x02) != 0 ? 1 : 0);
            if ((joystickInput & 0x04) != (lastJoystickInput & 0x04))
                myEventHandler->handleEvent(Event::JoystickZeroRight,
                                            (joystickInput & 0x04) != 0 ? 1 : 0);
            if ((joystickInput & 0x08) != (lastJoystickInput & 0x08))
                myEventHandler->handleEvent(Event::JoystickZeroUp,
                                            (joystickInput & 0x08) != 0 ? 1 : 0);
            if ((joystickInput & 0x10) != (lastJoystickInput & 0x10))
                myEventHandler->handleEvent(Event::JoystickZeroDown,
                                            (joystickInput & 0x10) != 0 ? 1 : 0);
        }
        else
        {
            if ((joystickInput & 0x01) != (lastJoystickInput & 0x01))
                myEventHandler->handleEvent(Event::JoystickOneFire,
                                            (joystickInput & 0x01) != 0 ? 1 : 0);
            if ((joystickInput & 0x02) != (lastJoystickInput & 0x02))
                myEventHandler->handleEvent(Event::JoystickOneLeft,
                                            (joystickInput & 0x02) != 0 ? 1 : 0);
            if ((joystickInput & 0x04) != (lastJoystickInput & 0x04))
                myEventHandler->handleEvent(Event::JoystickOneRight,
                                            (joystickInput & 0x04) != 0 ? 1 : 0);
            if ((joystickInput & 0x08) != (lastJoystickInput & 0x08))
                myEventHandler->handleEvent(Event::JoystickOneUp,
                                            (joystickInput & 0x08) != 0 ? 1 : 0);
            if ((joystickInput & 0x10) != (lastJoystickInput & 0x10))
                myEventHandler->handleEvent(Event::JoystickOneDown,
                                            (joystickInput & 0x10) != 0 ? 1 : 0);
        }

        lastJoystickInput = joystickInput;
    }

    eventHandler().poll(getTicks());

    return 1;
}

int OSystem::updateAudio(void* buffer, int bufferSize, int flags)
{
    EventHandler::State state = eventHandler().state();
    if (EventHandler::S_EMULATE != state)
    {
        return 0;
    }

    mySound->update((uint8_t*) buffer, bufferSize);

    return 1;
}

int OSystem::updateVideo(emu_update_info_t* updateInfo, int flags)
{
    EventHandler::State state = eventHandler().state();
    if (EventHandler::S_EMULATE != state)
    {
        return 0;
    }

    TIA& tia = console().tia();

    tia.update();

    if (eventHandler().frying())
    {
        console().fry();
    }

    if (NULL != updateInfo)
    {
        updateInfo->video_buffer = tia.currentFrameBuffer();
        updateInfo->video_width = (int) tia.width();
        updateInfo->video_height = (int) tia.height();
        updateInfo->video_ystart = (int) tia.ystart();
        updateInfo->palette = myPalette;
    }

    return 1; // 1 := updated
}

shared_ptr<FBSurface> OSystem::allocateSurface(int w, int h, const uInt32* data)
{
    shared_ptr<FBSurface> surface = make_ptr<FBSurface>();

    surface->alloc(w, h, data);

    return surface;
}


uint32_t OSystem::mapRGB(uint8_t r, uint8_t g, uint8_t b) const
{
    return 0x00ff8800;
}

std::string OSystem::getAttribute(const std::string& key)
{
    std::string value;

    if (0 == key.compare("image.info"))
    {
        const ConsoleInfo& info = myConsole->about();
        value = info.CartName;
    }

    return value;
}

void OSystem::input(int keyCode, int state)
{
    ;
}

int OSystem::execCommand(int command, int param)
{
    switch (command)
    {
        case 3: // COMMAND_RESET
            resetReleased = false;
            resetTriggered = true;
            break;
        case 8: // COMMAND_SELECT
            selectReleased = false;
            selectTriggered = true;
            break;
        default: {
            return 0;
        }
    }

    return 1;
}
