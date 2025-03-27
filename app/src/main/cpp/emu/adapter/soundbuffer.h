
#pragma once

class OSystem;

#include "bspf.hxx"
#include "TIASnd.hxx"
#include "Sound.hxx"

class SoundBuffer : public Sound
{
    public:
          /**
            Create a new sound object.  The init method must be invoked before
            using the object.
          */
        SoundBuffer(OSystem& osystem);

        /**
          Destructor
        */
        virtual ~SoundBuffer();

        public:
          /**
            Enables/disables the sound subsystem.

            @param state  True or false, to enable or disable the sound system
          */
        void setEnabled(bool state) override;

        /**
          The system cycle counter is being adjusting by the specified amount. Any
          members using the system cycle counter should be adjusted as needed.

          @param amount  The amount the cycle counter is being adjusted by
        */
        void adjustCycleCounter(Int32 amount) override;

        /**
          Sets the number of channels (mono or stereo sound).  Note that this
          determines how the emulation should 'mix' the channels of the TIA sound
          system (of which there are always two).  It does not specify the actual
          number of hardware channels that SDL should use; it will always attempt
          to use two channels in hardware.

          @param channels  The number of channels
        */
        void setChannels(uInt32 channels) override;

        /**
          Sets the display framerate.  Sound generation for NTSC and PAL games
          depends on the framerate, so we need to set it here.

          @param framerate The base framerate depending on NTSC or PAL ROM
        */
        void setFrameRate(float framerate) override;

        /**
          Initializes the sound device.  This must be called before any
          calls are made to derived methods.
        */
        void open() override;

        /**
          Should be called to close the sound device.  Once called the sound
          device can be started again using the open method.
        */
        void close() override;

        /**
          Set the mute state of the sound object.  While muted no sound is played.

          @param state  Mutes sound if true, unmute if false
        */
        void mute(bool state) override;

        /**
          Reset the sound device.
        */
        void reset() override;

        /**
          Sets the sound register to a given value.

          @param addr   The register address
          @param value  The value to save into the register
          @param cycle  The system cycle at which the register is being updated
        */
        void set(uInt16 addr, uInt8 value, Int32 cycle) override;

        /**
          Sets the volume of the sound device to the specified level.  The
          volume is given as a percentage from 0 to 100.  Values outside
          this range indicate that the volume shouldn't be changed at all.

          @param percent  The new volume percentage level for the sound device
        */
        void setVolume(Int32 percent) override;

        /**
          Adjusts the volume of the sound device based on the given direction.

          @param direction  Increase or decrease the current volume by a predefined
                            amount based on the direction (1 = increase, -1 = decrease)
        */
        void adjustVolume(Int8 direction) override;

    public:
          /**
            Saves the current state of this device to the given Serializer.

            @param out  The serializer device to save to.
            @return  The result of the save.  True on success, false on failure.
          */
        bool save(Serializer& out) const override;

        /**
          Loads the current state of this device from the given Serializer.

          @param in  The Serializer device to load from.
          @return  The result of the load.  True on success, false on failure.
        */
        bool load(Serializer& in) override;

        /**
          Get a descriptor for this console class (used in error checking).

          @return  The name of the object
        */
        string name() const override { return "TIASound"; }

    protected:
          /**
            Invoked by the sound callback to process the next sound fragment.
            The stream is 16-bits (even though the callback is 8-bits), since
            the TIASnd class always generates signed 16-bit stereo samples.

            @param stream  Pointer to the start of the fragment
            @param length  Length of the fragment
          */
        void processFragment(Int16* stream, uInt32 length);

    protected:
          // Struct to hold information regarding a TIA sound register write
        struct RegWrite
        {
            uInt16 addr;
            uInt8 value;
            double delta;
        };

    /**
      A queue class used to hold TIA sound register writes before being
      processed while creating a sound fragment.
    */
    class RegWriteQueue
    {
        public:
              /**
                Create a new queue instance with the specified initial
                capacity.  If the queue ever reaches its capacity then it will
                automatically increase its size.
              */
            RegWriteQueue(uInt32 capacity = 512);

        public:
              /**
                Clear any items stored in the queue.
              */
            void clear();

            /**
              Dequeue the first object in the queue.
            */
            void dequeue();

            /**
              Return the duration of all the items in the queue.
            */
            double duration() const;

            /**
              Enqueue the specified object.
            */
            void enqueue(const RegWrite& info);

            /**
              Return the item at the front on the queue.

              @return  The item at the front of the queue.
            */
            RegWrite& front() const;

            /**
              Answers the number of items currently in the queue.

              @return  The number of items in the queue.
            */
            uInt32 size() const;

            private:
              // Increase the size of the queue
            void grow();

            private:
            unique_ptr<RegWrite[]> myBuffer;
            uInt32 myCapacity;
            uInt32 mySize;
            uInt32 myHead;
            uInt32 myTail;

            private:
              // Following constructors and assignment operators not supported
            RegWriteQueue(const RegWriteQueue&) = delete;
            RegWriteQueue(RegWriteQueue&&) = delete;
            RegWriteQueue& operator=(const RegWriteQueue&) = delete;
            RegWriteQueue& operator=(RegWriteQueue&&) = delete;
        };

    private:
        int myFragmentSize;
        int myNumBits;
        int myNumSamplesPerSecond;
        float myFramerate;
        bool fragmentParamsDirty;

          // TIASound emulation object
        TIASound myTIASound;

        // Indicates if the sound subsystem is to be initialized
        bool myIsEnabled;

        // Indicates if the sound device was successfully initialized
        bool myIsInitializedFlag;

        // Indicates the cycle when a sound register was last set
        Int32 myLastRegisterSetCycle;

        // Indicates the number of channels (mono or stereo)
        uInt32 myNumChannels;

        // Log base 2 of the selected fragment size
        double myFragmentSizeLogBase2;

        // The myFragmentSizeLogBase2 variable is used in only two places,
        // both of which involve an expensive division in the sound
        // processing callback
        // These are pre-computed to speed up the callback as much as possible
        double myFragmentSizeLogDiv1, myFragmentSizeLogDiv2;

        // Indicates if the sound is currently muted
        bool myIsMuted;

        // Current volume as a percentage (0 - 100)
        uInt32 myVolume;

        // Audio specification structure
        //SDL_AudioSpec myHardwareSpec;

        // Queue of TIA register writes
        RegWriteQueue myRegWriteQueue;

    public:
        void update(uint8_t* buffer, int bufferLen);

    private:
          // Callback function invoked by the SDL Audio library when it needs data
        static void callback(void* userData, uInt8* buffer, int bufferLen);

        // Following constructors and assignment operators not supported
        SoundBuffer() = delete;
        SoundBuffer(const SoundBuffer&) = delete;
        SoundBuffer(SoundBuffer&&) = delete;
        SoundBuffer& operator=(const SoundBuffer&) = delete;
        SoundBuffer& operator=(SoundBuffer&&) = delete;

    private:
        void setPaused(bool enable);
        void setLock(bool lock);
            
};
