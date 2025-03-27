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
// $Id: Props.cxx 3302 2016-04-02 23:47:46Z stephena $
//============================================================================

#include <cctype>
#include <algorithm>
#include <sstream>

#include "bspf.hxx"
#include "Props.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::Properties()
{
    setDefaults();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties::Properties(const Properties& properties)
{
    copy(properties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::set(PropertyType key, const string& value)
{
    if (key != LastPropType)
    {
        myProperties[key] = value;
        if (BSPF::equalsIgnoreCase(myProperties[key], "AUTO-DETECT"))
            myProperties[key] = "AUTO";

        switch (key)
        {
            case Cartridge_Type:
            case Display_Format:
            case Cartridge_Sound:
            case Console_LeftDifficulty:
            case Console_RightDifficulty:
            case Console_TelevisionType:
            case Console_SwapPorts:
            case Controller_Left:
            case Controller_Right:
            case Controller_SwapPaddles:
            case Controller_MouseAxis:
            case Display_Phosphor:
            {
                transform(myProperties[key].begin(), myProperties[key].end(),
                    myProperties[key].begin(), ::toupper);
                break;
            }

            case Display_PPBlend:
            {
                int blend = atoi(myProperties[key].c_str());
                if (blend < 0 || blend > 100) blend = 77;
                ostringstream buf;
                buf << blend;
                myProperties[key] = buf.str();
                break;
            }

            default:
                break;
        }
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
istream& operator >> (istream& is, Properties& p)
{
    p.setDefaults();

    // Loop reading properties
    string key, value;
    for (;;)
    {
      // Get the key associated with this property
        key = p.readQuotedString(is);

        // Make sure the stream is still okay
        if (!is)
            return is;

          // A null key signifies the end of the property list
        if (key == "")
            break;

          // Get the value associated with this property
        value = p.readQuotedString(is);

        // Make sure the stream is still okay
        if (!is)
            return is;

          // Set the property 
        PropertyType type = Properties::getPropertyType(key);
        p.set(type, value);
    }

    return is;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ostream& operator<<(ostream& os, const Properties& p)
{
  // Write out each of the key and value pairs
    bool changed = false;
    for (int i = 0; i < LastPropType; ++i)
    {
      // Try to save some space by only saving the items that differ from default
        if (p.myProperties[i] != Properties::ourDefaultProperties[i])
        {
            p.writeQuotedString(os, Properties::ourPropertyNames[i]);
            os.put(' ');
            p.writeQuotedString(os, p.myProperties[i]);
            os.put('\n');
            changed = true;
        }
    }

    if (changed)
    {
      // Put a trailing null string so we know when to stop reading
        p.writeQuotedString(os, "");
        os.put('\n');
        os.put('\n');
    }

    return os;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Properties::readQuotedString(istream& in)
{
  // Read characters until we see a quote
    char c;
    while (in.get(c))
        if (c == '"')
            break;

        // Read characters until we see the close quote
    string s;
    while (in.get(c))
    {
        if ((c == '\\') && (in.peek() == '"'))
            in.get(c);
        else if ((c == '\\') && (in.peek() == '\\'))
            in.get(c);
        else if (c == '"')
            break;
        else if (c == '\r')
            continue;

        s += c;
    }

    return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::writeQuotedString(ostream& out, const string& s)
{
    out.put('"');
    for (uInt32 i = 0; i < s.length(); ++i)
    {
        if (s[i] == '\\')
        {
            out.put('\\');
            out.put('\\');
        }
        else if (s[i] == '\"')
        {
            out.put('\\');
            out.put('"');
        }
        else
            out.put(s[i]);
    }
    out.put('"');
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Properties::operator == (const Properties& properties) const
{
    for (int i = 0; i < LastPropType; ++i)
        if (myProperties[i] != properties.myProperties[i])
            return false;

    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Properties::operator != (const Properties& properties) const
{
    return !(*this == properties);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Properties& Properties::operator = (const Properties& properties)
{
  // Do the assignment only if this isn't a self assignment
    if (this != &properties)
    {
      // Now, make myself a copy of the given object
        copy(properties);
    }

    return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::copy(const Properties& properties)
{
  // Now, copy each property from properties
    for (int i = 0; i < LastPropType; ++i)
        myProperties[i] = properties.myProperties[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::print() const
{
    cout << get(Cartridge_MD5) << "|"
        << get(Cartridge_Name) << "|"
        << get(Cartridge_Manufacturer) << "|"
        << get(Cartridge_ModelNo) << "|"
        << get(Cartridge_Note) << "|"
        << get(Cartridge_Rarity) << "|"
        << get(Cartridge_Sound) << "|"
        << get(Cartridge_Type) << "|"
        << get(Console_LeftDifficulty) << "|"
        << get(Console_RightDifficulty) << "|"
        << get(Console_TelevisionType) << "|"
        << get(Console_SwapPorts) << "|"
        << get(Controller_Left) << "|"
        << get(Controller_Right) << "|"
        << get(Controller_SwapPaddles) << "|"
        << get(Controller_MouseAxis) << "|"
        << get(Display_Format) << "|"
        << get(Display_YStart) << "|"
        << get(Display_Height) << "|"
        << get(Display_Phosphor) << "|"
        << get(Display_PPBlend)
        << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::setDefaults()
{
    for (int i = 0; i < LastPropType; ++i)
        myProperties[i] = ourDefaultProperties[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PropertyType Properties::getPropertyType(const string& name)
{
    for (int i = 0; i < LastPropType; ++i)
        if (ourPropertyNames[i] == name)
            return PropertyType(i);

        // Otherwise, indicate that the item wasn't found
    return LastPropType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Properties::printHeader()
{
    cout << "Cartridge_MD5|"
        << "Cartridge_Name|"
        << "Cartridge_Manufacturer|"
        << "Cartridge_ModelNo|"
        << "Cartridge_Note|"
        << "Cartridge_Rarity|"
        << "Cartridge_Sound|"
        << "Cartridge_Type|"
        << "Console_LeftDifficulty|"
        << "Console_RightDifficulty|"
        << "Console_TelevisionType|"
        << "Console_SwapPorts|"
        << "Controller_Left|"
        << "Controller_Right|"
        << "Controller_SwapPaddles|"
        << "Controller_MouseAxis|"
        << "Display_Format|"
        << "Display_YStart|"
        << "Display_Height|"
        << "Display_Phosphor|"
        << "Display_PPBlend"
        << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* Properties::ourDefaultProperties[LastPropType] = {
  "",          // Cartridge.MD5
  "",          // Cartridge.Manufacturer
  "",          // Cartridge.ModelNo
  "Untitled",  // Cartridge.Name
  "",          // Cartridge.Note
  "",          // Cartridge.Rarity
  "MONO",      // Cartridge.Sound
  "AUTO",      // Cartridge.Type
  "B",         // Console.LeftDifficulty
  "B",         // Console.RightDifficulty
  "COLOR",     // Console.TelevisionType
  "NO",        // Console.SwapPorts
  "JOYSTICK",  // Controller.Left
  "JOYSTICK",  // Controller.Right
  "NO",        // Controller.SwapPaddles
  "AUTO",      // Controller.MouseAxis
  "AUTO",      // Display.Format
  "34",        // Display.YStart
  "210",       // Display.Height
  "NO",        // Display.Phosphor
  "77"         // Display.PPBlend
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* Properties::ourPropertyNames[LastPropType] = {
  "Cartridge.MD5",
  "Cartridge.Manufacturer",
  "Cartridge.ModelNo",
  "Cartridge.Name",
  "Cartridge.Note",
  "Cartridge.Rarity",
  "Cartridge.Sound",
  "Cartridge.Type",
  "Console.LeftDifficulty",
  "Console.RightDifficulty",
  "Console.TelevisionType",
  "Console.SwapPorts",
  "Controller.Left",
  "Controller.Right",
  "Controller.SwapPaddles",
  "Controller.MouseAxis",
  "Display.Format",
  "Display.YStart",
  "Display.Height",
  "Display.Phosphor",
  "Display.PPBlend"
};
