#include "byte_stream.hh"
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
  bytes_pushed_ += bytes;
  buffer_.insert( buffer_.end(), data.begin(), data.begin() + bytes ); // 插入数据
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
  return bytes_pushed_;
}

std::string_view Reader::peek() const
{
  if ( buffer_.empty() ) {
    return {};
  }
  return { &buffer_.front(), 1 }; // 返回视图
}

void Reader::pop( uint64_t len )
{
  uint64_t bytes = std::min( len, buffer_.size() );
  buffer_.erase( buffer_.begin(), buffer_.begin() + bytes ); // 移除数据
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