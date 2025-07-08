#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // debug( "unimplemented sequence_numbers_in_flight() called" );
  return sequences_in_flight_count_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  debug( "unimplemented consecutive_retransmissions() called" );
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  uint64_t effective_window_size = ( window_size_ == 0 ) ? 1 : window_size_;
  debug( "window_size is {}", window_size_ );
  debug( "reader is {}", reader().is_finished() );
  debug( "next_seqno = {},first_index = {}", next_seqno_, first_index_ );
  std::vector<TCPSenderMessage> messages = std::vector<TCPSenderMessage>();
  while ( next_seqno_ < first_index_ + effective_window_size && !reader().is_finished() ) {
    bool send_syn = false;
    if ( !syn_ ) {
      syn_ = true;
      send_syn = true;
    }
    uint64_t length = std::min( first_index_ + effective_window_size - next_seqno_ - send_syn, reader().bytes_buffered() );
    length = std::min( length, (uint64_t)TCPConfig::MAX_PAYLOAD_SIZE );
    debug( "length = {}", length );

    string payload;
    read( reader(), length, payload );
    if ( !fin_ && reader().is_finished() && next_seqno_ + length < first_index_ + effective_window_size ) {
      fin_ = true;
    }
    debug( "payload = {}", payload );
    TCPSenderMessage message = { isn_.wrap( next_seqno_, isn_ ), send_syn, payload, fin_, reader().has_error() };
    if ( message.sequence_length() == 0 ) {
      break;
    }
    messages.push_back( message );
    sequences_in_flight.push( message );
    next_seqno_ += message.sequence_length();
    sequences_in_flight_count_ += message.sequence_length();
  }
  if ( !fin_ && reader().is_finished() && next_seqno_ < first_index_ + effective_window_size ) {
    fin_ = true;
    bool send_syn = false;
    if ( !syn_ ) {
      syn_ = true;
      send_syn = true;
    }
    TCPSenderMessage message_fin = { isn_.wrap( next_seqno_, isn_ ), send_syn, "", fin_, reader().has_error() };
    next_seqno_ += message_fin.sequence_length();
    sequences_in_flight.push( message_fin );
    messages.push_back( message_fin );
    sequences_in_flight_count_ += message_fin.sequence_length();
  }
  if ( !messages.empty() && !timer_.is_active() ) {
    RTO_ms_ = initial_RTO_ms_;
    timer_.start( RTO_ms_ );
  }
  for ( auto& msg : messages ) {
    transmit( msg );
  }
  // debug( "unimplemented push() called" );
  // (void)transmit;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  debug( "unimplemented make_empty_message() called" );
  return { isn_.wrap( next_seqno_, isn_ ), false, "", false, reader().has_error() };
  ;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST ) {
    reader().set_error();
  }
  if ( !msg.ackno.has_value() ) {
    window_size_ = msg.window_size; // 仍然更新窗口大小
    return;
  }

  uint64_t ackno = msg.ackno.value().unwrap( isn_, first_index_ );
  if ( ackno > next_seqno_ ) {
    // ackno 确认了还没发送的数据，忽略这个 ACK
    //  但仍然更新窗口大小
    window_size_ = msg.window_size;
    return;
  }
  debug( "ackno = {}", ackno );
  debug( "active first_index = {}", first_index_ );
  // 添加重复 ACK 检查
  if ( ackno <= first_index_ ) {
    window_size_ = msg.window_size;
    return; // 重复或过时的 ACK
  }
  bool new_data_acked = false;
  while ( !sequences_in_flight.empty() ) {
    uint64_t l = sequences_in_flight.front().seqno.unwrap( isn_, first_index_ );
    if ( l + sequences_in_flight.front().sequence_length() <= ackno ) {
      first_index_ += sequences_in_flight.front().sequence_length();
      sequences_in_flight_count_ -= sequences_in_flight.front().sequence_length();
      sequences_in_flight.pop();
      new_data_acked = true;
      continue;
    }
    break;
  }
  window_size_ = msg.window_size;
  if ( new_data_acked ) {
    timer_.stop();
    consecutive_retransmissions_ = 0;
    RTO_ms_ = initial_RTO_ms_;
    if ( !sequences_in_flight.empty() ) {
      timer_.start( RTO_ms_ );
    }
  }
  // debug( "unimplemented receive() called" );
  // (void)msg;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{

  timer_.tick( ms_since_last_tick );
  if ( timer_.expired() && !sequences_in_flight.empty() ) {
    // 重传
    transmit( sequences_in_flight.front() );
    consecutive_retransmissions_++;
    if ( window_size_ > 0 || sequences_in_flight.front().SYN )
      RTO_ms_ *= 2;
    timer_.start( RTO_ms_ );
  }
  // debug( "unimplemented tick({}, ...) called", ms_since_last_tick );
  // (void)transmit;
}
