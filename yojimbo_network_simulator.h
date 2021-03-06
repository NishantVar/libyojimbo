/*
    Yojimbo Client/Server Network Protocol Library.
    
    Copyright © 2016, The Network Protocol Company, Inc.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef YOJIMBO_NETWORK_SIMULATOR_H
#define YOJIMBO_NETWORK_SIMULATOR_H

#include "yojimbo_config.h"
#include "yojimbo_common.h"
#include "yojimbo_address.h"
#include "yojimbo_allocator.h"
#include "yojimbo_transport.h"

/** @file */

namespace yojimbo
{
    /**
        Simulates packet loss, latency, jitter and duplicate packets.

        One of these is created for each network transport created, allowing you to simulate bad network conditions on top of your transport.

        This is useful during development, so your game is tested and played under real world conditions, instead of ideal LAN conditions.

        This simulator works on packet send. This means that if you want 125ms of latency (round trip), you must to add 125/2 = 62.5ms of latency to each side.

        @see NetworkTransport
     */

    class NetworkSimulator
    {
    public:

        /**
            Create a network simulator.

            Initial network conditions are set to:

                Latency: 0ms
                Jitter: 0ms
                Packet Loss: 0%
                Duplicates: 0%

            This means the simulator should not in theory have any affect on packets sent through it. See NetworkSimulator::IsActive.

            @param allocator The allocator to use.
            @param numPackets The maximum number of packets that can be stored in the simulator at any time.
         */

        NetworkSimulator( Allocator & allocator, int numPackets = 4096 );

        /**
            Network simulator destructor.

            Any packet data still in the network simulator is destroyed.
         */

        ~NetworkSimulator();

        /**
            Set the latency in milliseconds.

            This latency is added on packet send. To simulate a round trip time of 100ms, add 50ms of latency to both sides of the connection.

            @param milliseconds The latency to add in milliseconds.
         */

        void SetLatency( float milliseconds );

        /**
            Set the packet jitter in milliseconds.

            Jitter is applied +/- this amount in milliseconds. To be truly effective, jitter must be applied together with some latency.

            @param milliseconds The amount of jitter to add in milliseconds (+/-).
         */

        void SetJitter( float milliseconds );

        /**
            Set the amount of packet loss to apply on send.

            @param percent The packet loss percentage. 0% = no packet loss. 100% = all packets are dropped.
         */

        void SetPacketLoss( float percent );

        /**
            Set percentage chance of packet duplicates.

            If the duplicate chance succeeds, a duplicate packet is added to the queue with a random delay of up to 1 second.

            @param percent The percentage chance of a packet duplicate being sent. 0% = no duplicate packets. 100% = all packets have a duplicate sent.
         */

        void SetDuplicate( float percent );

        /**
            Is the network simulator active?

            The network simulator is active when packet loss, latency, duplicates or jitter are non-zero values.

            This is used by the transport to know whether it should shunt packets through the simulator, or send them directly to the network. This is a minor optimization.
         */

        bool IsActive() const;

        /**
            Queue a packet up for send. 

            IMPORTANT: Ownership of the packet data pointer is *not* transferred to the network simulator. It makes a copy of the data instead.

            @param from The address the packet is sent from.
            @param to The address the packet is sent to.
            @param packetData The packet data.
            @param packetSize The packet size (bytes).
         */
        
        void SendPacket( const Address & from, const Address & to, uint8_t * packetData, int packetSize );

        /**
            Receive packets sent to any address.

            IMPORTANT: You take ownership of the packet data you receive and are responsible for freeing it. See NetworkSimulator::GetAllocator.

            @param maxPackets The maximum number of packets to receive.
            @param packetData Array of packet data pointers to be filled [out].
            @param packetSize Array of packet sizes to be filled [out].
            @param from Array of from addresses to be filled [out].
            @param to Array of to addresses to be filled [out].

            @returns The number of packets received.
         */

        int ReceivePackets( int maxPackets, uint8_t * packetData[], int packetSize[], Address from[], Address to[] );

        /** 
            Receive packets sent to a specified address.

            IMPORTANT: You take ownership of the packet data you receive and are responsible for freeing it. See NetworkSimulator::GetAllocator.

            @param maxPackets The maximum number of packets to receive.
            @param to Only packets sent to this address will be received.
            @param packetData Array of packet data pointers to be filled [out].
            @param packetSize Array of packet sizes to be filled [out].
            @param from Array of from addresses to be filled [out].

            @returns The number of packets received.
         */

        int ReceivePacketsSentToAddress( int maxPackets, const Address & to, uint8_t * packetData[], int packetSize[], Address from[] );

        /**
            Discard all packets in the network simulator.

            This is useful if the simulator needs to be reset and used for another purpose.
         */

        void DiscardPackets();

        /**
            Discard all packets sent from an address.
            
            @param address Packets sent from this address will be discarded.
         */

        void DiscardPacketsFromAddress( const Address & address );

        /**
            Advance network simulator time.

            You must pump this regularly otherwise the network simulator won't work.

            @param time The current time value. Please make sure you use double values for time so you retain sufficient precision as time increases.
         */

        void AdvanceTime( double time );

        /**
            Get the allocator to use to free packet data.

            @returns The allocator that packet data is allocated with.
         */

        Allocator & GetAllocator() { assert( m_allocator ); return *m_allocator; }

    protected:

        /**
            Helper function to update the active flag whenever network settings are changed.

            Active is set to true if any of the network conditions are non-zero. This allows you to quickly check if the network simulator is active and would actually do something.
         */

        void UpdateActive();

        /**
            Packets ready to be received are removed from the main simulator buffer and put into pending receive packet arrays by this function.

            This is an optimization that reduces the amount of work that needs to be done when walking the simulator to dequeue packets sent to a particular address.
         */

        void UpdatePendingReceivePackets();

    private:

        Allocator * m_allocator;                        ///< The allocator passed in to the constructor. It's used to allocate and free packet data.

        float m_latency;                                ///< Latency in milliseconds

        float m_jitter;                                 ///< Jitter in milliseconds +/-

        float m_packetLoss;                             ///< Packet loss percentage.

        float m_duplicate;                              ///< Duplicate packet percentage

        bool m_active;                                  ///< True if network simulator is active, eg. if any of the network settings above are enabled.

        /// A packet buffered in the network simulator.

        struct PacketEntry
        {
            PacketEntry()
            {
                deliveryTime = 0.0;
                packetData = NULL;
                packetSize = 0;
            }

            Address from;                               ///< Address the packet was sent from.
            Address to;                                 ///< Address the packet should be sent to.
            double deliveryTime;                        ///< Delivery time for this packet (seconds).
            uint8_t * packetData;                       ///< Packet data (owns this pointer).
            int packetSize;                             ///< Size of packet in bytes.
        };

        double m_time;                                  ///< Current time from last call to advance time.

        int m_currentIndex;                             ///< Current index in the packet entry array. New packets are inserted here.

        int m_numPacketEntries;                         ///< Number of elements in the packet entry array.

        PacketEntry * m_packetEntries;                  ///< Pointer to dynamically allocated packet entries. This is where buffered packets are stored.

        int m_numPendingReceivePackets;                 ///< Number of pending receive packets. This is an optimization to make receiving packets sent to an address faster.

        PacketEntry * m_pendingReceivePackets;          ///< List of packets pending receive. Updated each time you call NetworkSimulator::AdvanceTime.

        double m_lastPendingReceiveTime;                ///< Time of last pending receive. Work-around multiple simulator updates when the simulator is set on multiple local transports.
    };
}

#endif // #ifndef YOJIMBO_NETWORK_SIMULATOR_H
