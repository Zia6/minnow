#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <functional>
#include <queue>

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms )
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // For testing: how many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
  const Writer& writer() const { return input_.writer(); }
  const Reader& reader() const { return input_.reader(); }
  Writer& writer() { return input_.writer(); }

private:
  Reader& reader() { return input_.reader(); }
  std::queue<TCPSenderMessage> sequences_in_flight = std::queue<TCPSenderMessage>();
  uint64_t first_index_ = 0;
  uint64_t window_size_ = 0;
  uint64_t next_seqno_ = 0;
  bool fin_ = false;
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t RTO_ms_;
  uint64_t consecutive_retransmissions_ = 0;
  class Timer
  {
  private:
    uint64_t RTO_ms_ = 0;
    uint64_t elapsed_ms_ = 0;
    bool active_ = false;
  public:
    void start( uint64_t RTO_ms )
    {
      active_ = true;
      RTO_ms_ = RTO_ms;
      elapsed_ms_ = 0;
    }
    void stop() { active_ = false; }
    void tick( uint64_t ms )
    {
      if ( active_ )
        elapsed_ms_ += ms;
    }
    bool expired() const { return active_ && elapsed_ms_ >= RTO_ms_; }
  };
  Timer timer_;
};
