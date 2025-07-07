#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
using namespace std;


// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // debug( "unimplemented sequence_numbers_in_flight() called" );
  return sequences_in_flight.size();
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  debug( "unimplemented consecutive_retransmissions() called" );
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  std::vector<TCPSenderMessage> messages = std::vector<TCPSenderMessage>();
  if(next_seqno_ == 0){
    TCPSenderMessage message_syn = {isn_,true,"",false,false};
    messages.push_back(message_syn);
    sequences_in_flight.push(message_syn);
    next_seqno_ += message_syn.sequence_length();
    RTO_ms_ = initial_RTO_ms_;
  }
  while(next_seqno_ < first_index_ + window_size_ && !reader().is_finished()){
    uint64_t length = std::min(first_index_ + window_size_ - next_seqno_,reader().bytes_buffered());
    length = std::min(length,(uint64_t)TCPConfig::MAX_PAYLOAD_SIZE);
    if(length == 0) break;
    if(!fin_ && reader().is_finished() && next_seqno_ + length < first_index_ + window_size_){
      fin_ = true;
    }
    string payload;
    read(reader(),length,payload);
    TCPSenderMessage message = {isn_.wrap(next_seqno_,isn_),false,payload,fin_,false};
    messages.push_back(message);
    sequences_in_flight.push(message);
    next_seqno_ += message.sequence_length();
  }
  if(!fin_ && reader().is_finished() && next_seqno_ < first_index_ + window_size_){
    fin_ = true;
    TCPSenderMessage message_fin = {isn_.wrap(next_seqno_,isn_),false,"",fin_,false};
    next_seqno_ += message_fin.sequence_length();
    sequences_in_flight.push(message_fin);
    messages.push_back(message_fin);
  }
  if(!messages.empty())
  timer_.start(RTO_ms_);
  for(auto& msg: messages){
    transmit(msg);
  }
  // debug( "unimplemented push() called" );
  // (void)transmit;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  debug( "unimplemented make_empty_message() called" );
  return {isn_.wrap(next_seqno_,isn_),false,"",false,false};;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  uint64_t ackno = msg.ackno.value().unwrap(isn_,first_index_);
  bool new_data_acked = false;
  while(!sequences_in_flight.empty()){
    uint64_t l = sequences_in_flight.front().seqno.unwrap(isn_,first_index_);
    if(l + sequences_in_flight.front().sequence_length() <= ackno){
      first_index_ += sequences_in_flight.front().sequence_length();
      sequences_in_flight.pop();
      continue;
    }
    break;
  }

  if (new_data_acked) {
    consecutive_retransmissions_ = 0;
    RTO_ms_ = initial_RTO_ms_;
  }
  window_size_ = msg.window_size;
  if (sequences_in_flight.empty()) {
    timer_.stop();
  } else if (new_data_acked) {
    timer_.start(RTO_ms_);
  }
  // debug( "unimplemented receive() called" );
  // (void)msg;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{

  timer_.tick(ms_since_last_tick);
  if(timer_.expired() && !sequences_in_flight.empty()){
    //重传
    transmit(sequences_in_flight.front());
    if(window_size_ > 0){
      RTO_ms_ *= 2;
      consecutive_retransmissions_++;
    }
    timer_.start(RTO_ms_);
  }
  // debug( "unimplemented tick({}, ...) called", ms_since_last_tick );
  // (void)transmit;
}
