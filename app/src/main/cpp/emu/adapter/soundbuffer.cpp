
#include <sstream>
#include <cassert>
#include <cmath>

#include "TIASnd.hxx"
#include "TIATables.hxx"
#include "Settings.hxx"
#include "System.hxx"
#include "OSystem.hxx"
#include "Console.hxx"
#include "soundbuffer.h"

#define LOG2 0.30102999566; // log2

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundBuffer::SoundBuffer(OSystem& osystem)
    : Sound(osystem),
    myIsEnabled(false),
    myIsInitializedFlag(false),
    myLastRegisterSetCycle(0),
    myNumChannels(0),
    myFragmentSizeLogBase2(0),
    myFragmentSizeLogDiv1(0),
    myFragmentSizeLogDiv2(0),
    myIsMuted(true),
    myVolume(100)
{
    myOSystem.logMessage("SoundBuffer::SoundBuffer started ...", 2);

    myFragmentSize = 0; // passed as argument for update
    myNumBits = 16;
    myNumSamplesPerSecond = 44100;
    myNumChannels = 1;
    myFramerate = 60.0f;
    fragmentParamsDirty = true;

    myIsInitializedFlag = true;

    myOSystem.logMessage("SoundBuffer::SoundBuffer initialized", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundBuffer::~SoundBuffer()
{
  // Close the SDL audio system if it's initialized
    if (myIsInitializedFlag)
    {
        myIsEnabled = myIsInitializedFlag = false;
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::setEnabled(bool state)
{
    myOSystem.settings().setValue("sound", state);

    myOSystem.logMessage(state ? "SoundBuffer::setEnabled(true)" :
        "SoundBuffer::setEnabled(false)", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::open()
{
    myOSystem.logMessage("SoundBuffer::open started ...", 2);
    myIsEnabled = false;
    mute(true);
    if (!myIsInitializedFlag || !myOSystem.settings().getBool("sound"))
    {
        myOSystem.logMessage("Sound disabled\n", 1);
        return;
    }

    // Now initialize the TIASound object which will actually generate sound
    myTIASound.outputFrequency(myNumSamplesPerSecond);
    const string& chanResult =
        myTIASound.channels(myNumChannels, myNumChannels == 2);

    // Adjust volume to that defined in settings
    myVolume = myOSystem.settings().getInt("volume");
    setVolume(myVolume);

    // And start the SDL sound subsystem ...
    myIsEnabled = true;
    mute(false);

    fragmentParamsDirty = true;

    myOSystem.logMessage("SoundBuffer::open finished", 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::close()
{
    if (myIsInitializedFlag)
    {
        myIsEnabled = false;
        setPaused(true);
        myLastRegisterSetCycle = 0;
        myTIASound.reset();
        myRegWriteQueue.clear();
        myOSystem.logMessage("SoundBuffer::close", 2);
    }
}

void SoundBuffer::setPaused(bool enabled)
{
    ;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::mute(bool state)
{
    if (myIsInitializedFlag)
    {
        myIsMuted = state;
        setPaused(myIsMuted);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::reset()
{
    if (myIsInitializedFlag)
    {
        setPaused(true);
        myLastRegisterSetCycle = 0;
        myTIASound.reset();
        myRegWriteQueue.clear();
        mute(myIsMuted);
    }
}

void SoundBuffer::setLock(bool lock)
{
    ;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::setVolume(Int32 percent)
{
    if (myIsInitializedFlag && (percent >= 0) && (percent <= 100))
    {
        myOSystem.settings().setValue("volume", percent);
        setLock(true);
        myVolume = percent;
        myTIASound.volume(percent);
        setLock(false);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::adjustVolume(Int8 direction)
{
    ostringstream strval;
    string message;

    Int32 percent = myVolume;

    if (direction == -1)
        percent -= 2;
    else if (direction == 1)
        percent += 2;

    if ((percent < 0) || (percent > 100))
        return;

    setVolume(percent);

    // Now show an onscreen message
    strval << percent;
    message = "Volume set to ";
    message += strval.str();

    myOSystem.showMessage(message);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::adjustCycleCounter(Int32 amount)
{
    myLastRegisterSetCycle += amount;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::setChannels(uInt32 channels)
{
    if (channels == 1 || channels == 2)
        myNumChannels = channels;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::setFrameRate(float framerate)
{
    myFramerate = framerate;

    // Recalculate since frame rate has changed
    fragmentParamsDirty = true;

    // FIXME - should we clear out the queue or adjust the values in it?
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::set(uInt16 addr, uInt8 value, Int32 cycle)
{
    setLock(true);

    // First, calculate how many seconds would have past since the last
    // register write on a real 2600
    double delta = double(cycle - myLastRegisterSetCycle) / 1193191.66666667;

    // Now, adjust the time based on the frame rate the user has selected. For
    // the sound to "scale" correctly, we have to know the games real frame 
    // rate (e.g., 50 or 60) and the currently emulated frame rate. We use these
    // values to "scale" the time before the register change occurs.
    RegWrite info;
    info.addr = addr;
    info.value = value;
    info.delta = delta;
    myRegWriteQueue.enqueue(info);

    // Update last cycle counter to the current cycle
    myLastRegisterSetCycle = cycle;

    setLock(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::processFragment(Int16* stream, uInt32 length)
{
    uInt32 channels = myNumChannels;
    length = length / channels;

    // If there are excessive items on the queue then we'll remove some
    if (myRegWriteQueue.duration() > myFragmentSizeLogDiv1)
    {
        double removed = 0.0;
        while (removed < myFragmentSizeLogDiv2)
        {
            RegWrite& info = myRegWriteQueue.front();
            removed += info.delta;
            myTIASound.set(info.addr, info.value);
            myRegWriteQueue.dequeue();
        }
    }

    double position = 0.0;
    double remaining = length;

    while (remaining > 0.0)
    {
        if (myRegWriteQueue.size() == 0)
        {
          // There are no more pending TIA sound register updates so we'll
          // use the current settings to finish filling the sound fragment
            myTIASound.process(stream + (uInt32(position) * channels),
                length - uInt32(position));

            // Since we had to fill the fragment we'll reset the cycle counter
            // to zero.  NOTE: This isn't 100% correct, however, it'll do for
            // now.  We should really remember the overrun and remove it from
            // the delta of the next write.
            myLastRegisterSetCycle = 0;
            break;
        }
        else
        {
          // There are pending TIA sound register updates so we need to
          // update the sound buffer to the point of the next register update
            RegWrite& info = myRegWriteQueue.front();

            // How long will the remaining samples in the fragment take to play
            double duration = remaining / this->myNumSamplesPerSecond;

            // Does the register update occur before the end of the fragment?
            if (info.delta <= duration)
            {
              // If the register update time hasn't already passed then
              // process samples upto the point where it should occur
                if (info.delta > 0.0)
                {
                  // Process the fragment upto the next TIA register write.  We
                  // round the count passed to process up if needed.
                    double samples = (this->myNumSamplesPerSecond * info.delta);
                    myTIASound.process(stream + (uInt32(position) * channels),
                        uInt32(samples) + uInt32(position + samples) -
                        (uInt32(position) + uInt32(samples)));

                    position += samples;
                    remaining -= samples;
                }
                myTIASound.set(info.addr, info.value);
                myRegWriteQueue.dequeue();
            }
            else
            {
              // The next register update occurs in the next fragment so finish
              // this fragment with the current TIA settings and reduce the register
              // update delay by the corresponding amount of time
                myTIASound.process(stream + (uInt32(position) * channels),
                    length - uInt32(position));
                info.delta -= duration;
                break;
            }
        }
    }
}

void SoundBuffer::update(uint8_t* buffer, int bufferLen)
{
    int numSamples = bufferLen / (myNumBits*myNumChannels/8); // 16bit

    if (numSamples != myFragmentSize || fragmentParamsDirty) {
        myFragmentSize = numSamples;
        fragmentParamsDirty = false;

        // pre-compute fragment-related variables
        myFragmentSizeLogBase2 = log(numSamples) / LOG2;
        myFragmentSizeLogDiv1 = myFragmentSizeLogBase2 / myFramerate;
        myFragmentSizeLogDiv2 = (myFragmentSizeLogBase2 - 1) / myFramerate;
    }

    if (myIsEnabled)
    {
        // The callback is requesting 8-bit (unsigned) data, but the TIA sound
        // emulator deals in 16-bit (signed) data
        // So, we need to convert the pointer and half the length
        processFragment(reinterpret_cast<Int16*>(buffer), uInt32(bufferLen) >> 1);
    }
    else
    {
        memset(buffer, 0, bufferLen);  // Write 'silence'
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::callback(void* userData, uInt8* buffer, int bufferLen)
{
    SoundBuffer* sound = static_cast<SoundBuffer*>(userData);
    sound->update(buffer, bufferLen);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundBuffer::save(Serializer& out) const
{
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SoundBuffer::load(Serializer& in)
{
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundBuffer::RegWriteQueue::RegWriteQueue(uInt32 capacity)
    : myBuffer(make_ptr<RegWrite[]>(capacity)),
    myCapacity(capacity),
    mySize(0),
    myHead(0),
    myTail(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::RegWriteQueue::clear()
{
    myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::RegWriteQueue::dequeue()
{
    if (mySize > 0)
    {
        myHead = (myHead + 1) % myCapacity;
        --mySize;
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SoundBuffer::RegWriteQueue::duration() const
{
    double duration = 0.0;
    for (uInt32 i = 0; i < mySize; ++i)
    {
        duration += myBuffer[(myHead + i) % myCapacity].delta;
    }
    return duration;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::RegWriteQueue::enqueue(const RegWrite& info)
{
  // If an attempt is made to enqueue more than the queue can hold then
  // we'll enlarge the queue's capacity.
    if (mySize == myCapacity)
        grow();

    myBuffer[myTail] = info;
    myTail = (myTail + 1) % myCapacity;
    ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundBuffer::RegWrite& SoundBuffer::RegWriteQueue::front() const
{
    assert(mySize != 0);
    return myBuffer[myHead];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundBuffer::RegWriteQueue::size() const
{
    return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundBuffer::RegWriteQueue::grow()
{
    unique_ptr<RegWrite[]> buffer = make_ptr<RegWrite[]>(myCapacity * 2);
    for (uInt32 i = 0; i < mySize; ++i)
        buffer[i] = myBuffer[(myHead + i) % myCapacity];

    myHead = 0;
    myTail = mySize;
    myCapacity *= 2;

    myBuffer = std::move(buffer);
}
