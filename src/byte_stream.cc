#include "byte_stream.hh"
#include "debug.hh"
#include "exception.hh"
#include <deque>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  if ( closed_ ) {
    return;
  }
  uint64_t bytes = std::min( data.size(), capacity_ - buffer_.size() );
  // debug("bytes {}\n",bytes);
  bytes_pushed_ += bytes;
  buffer_.append( data.substr( 0, bytes ) );
}

void Writer::close()
{
  closed_ = true;
}

bool Writer::is_closed() const
{
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // debug("bytes_pushed_ {}",bytes_pushed_);
  return bytes_pushed_;
}

std::string_view Reader::peek() const
{
  if ( buffer_.empty() ) {
    return {};
  }
  return { buffer_.data(), buffer_.size() }; // 返回视图
}

void Reader::pop( uint64_t len )
{
  uint64_t bytes = std::min( len, buffer_.size() );
  buffer_ = buffer_.substr( bytes, buffer_.size() - bytes );
}

bool Reader::is_finished() const
{
  return closed_ && buffer_.empty();
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}

uint64_t Reader::bytes_popped() const
{
  return bytes_pushed_ - buffer_.size();
}