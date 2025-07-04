#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }
  if ( !zero_point_.has_value() && !message.SYN )
    return;
  if ( !zero_point_.has_value() ) {
    init_zero_point( message.seqno );
  }
  uint64_t first_index = message.seqno.unwrap( zero_point_.value(), writer().bytes_pushed() );
  reassembler_.insert( message.SYN ? first_index : first_index - 1, message.payload, message.FIN );

  // // Your code here.
  debug( "reassmbler insert {}", message.payload );
  // debug( "unimplemented receive() called" );
}

TCPReceiverMessage TCPReceiver::send() const
{
  std::optional<Wrap32> ankno;
  if ( zero_point_.has_value() ) {
    ankno = Wrap32 { zero_point_->wrap( writer().bytes_pushed(), zero_point_.value() ) + 1 + writer().is_closed() };
  } else
    ankno = std::nullopt;
  uint16_t window_size = writer().available_capacity() > UINT16_MAX ? UINT16_MAX : writer().available_capacity();
  return { ankno, window_size, reassembler_.reader().has_error() };
}
