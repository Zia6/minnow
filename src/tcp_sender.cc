#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"
using namespace std;


class TCPSender::Timer{
private:
  uint64_t RTO_ms_ = 0;
  uint64_t elapsed_ms_ = 0;
  bool active_ = false;
public:
  void start(uint64_t RTO_ms){
    active_ = true;
    RTO_ms_ = RTO_ms;
    elapsed_ms_ = 0;
  }
  void stop(){
    active_ = false;
  }
  void tick(uint64_t ms){
    if(active_) elapsed_ms_ += ms;
  }
  bool expired() const {
    return active_ && elapsed_ms_ >= RTO_ms_;
  }
};

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
  return sequences_in_flight.front().count;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  std::vector<TCPSenderMessage> messages = std::vector<TCPSenderMessage>();
  if(next_seqno_ == 0){
    TCPSenderMessage message_syn = {isn_,true,"",false,false};
    messages.push_back(message_syn);
    sequences_in_flight.push({message_syn,0});
    next_seqno_ += message_syn.sequence_length();
    RTO_ms_ = initial_RTO_ms_;
  }
  while(next_seqno_ < first_index_ + window_size_ && !reader().is_finished()){
    uint64_t length = std::min(first_index_ + window_size_ - next_seqno_,reader().bytes_buffered());
    length = std::min(length,(uint64_t)TCPConfig::MAX_PAYLOAD_SIZE);
    if(!fin_ && reader().is_finished() && next_seqno_ + length < first_index_ + window_size_){
      fin_ = true;
    }
    string payload;
    read(reader(),length,payload);
    TCPSenderMessage message = {isn_.wrap(next_seqno_,isn_),false,payload,fin_,false};
    messages.push_back(message);
    sequences_in_flight.push({message,0});
    next_seqno_ += message.sequence_length();
  }
  if(!fin_ && reader().is_finished()){
    fin_ = true;
    TCPSenderMessage message_fin = {isn_.wrap(next_seqno_,isn_),false,"",fin_,false};
    next_seqno_ += message_fin.sequence_length();
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
  RTO_ms_ = initial_RTO_ms_;
  while(!sequences_in_flight.empty()){
    uint64_t l = sequences_in_flight.front().msg.seqno.unwrap(isn_,first_index_);
    if(l + sequences_in_flight.front().msg.sequence_length() <= ackno){
      first_index_ += sequences_in_flight.front().msg.sequence_length();
      sequences_in_flight.pop();
      continue;
    }
    break;
  }
  // debug( "unimplemented receive() called" );
  // (void)msg;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{

  timer_.tick(ms_since_last_tick);
  if(timer_.expired() && !sequences_in_flight.empty()){
    //重传
    transmit(sequences_in_flight.front().msg);
    if(window_size_ > 0){
      sequences_in_flight.front().count++;
      RTO_ms_ *= 2;
    }
    timer_.start(RTO_ms_);
  }
  // debug( "unimplemented tick({}, ...) called", ms_since_last_tick );
  // (void)transmit;
}
