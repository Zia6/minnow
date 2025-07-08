#include "reassembler.hh"
#include "debug.hh"

using namespace std;

Reassembler::message merge( Reassembler::message m1, Reassembler::message m2 )
{
  if ( m1.index > m2.index ) {
    std::swap( m1, m2 );
  }

  uint64_t m1_end_exclusive = m1.index + m1.data.size();

  uint64_t m2_start_idx_to_append = 0;
  if ( m1_end_exclusive > m2.index ) {
    m2_start_idx_to_append = m1_end_exclusive - m2.index;
  }

  if ( m2_start_idx_to_append < m2.data.size() ) {
    m1.data += m2.data.substr( m2_start_idx_to_append );
  }

  // 最终的 is_last_substring 状态：只要合并的两个片段中有一个是最后一个，结果就是最后一个
  m1.is_last_substring = m1.is_last_substring || m2.is_last_substring;

  return m1;
}

bool intersect( Reassembler::message m1, Reassembler::message m2 )
{
  if ( m1.index > m2.index ) {
    std::swap( m1, m2 );
  }
  return m1.index + m1.data.size() > m2.index;
}

void Reassembler::transport()
{
  // 将传来的string 插入到缓冲区

  auto it = bytes_accept.begin();
  while ( it != bytes_accept.end() && it->index == last && output_.writer().available_capacity() ) {
    if ( output_.writer().is_closed() ) {
      return;
    }
    uint64_t cnt = std::min( it->data.size(), output_.writer().available_capacity() );
    last += cnt;
    output_.writer().push( it->data );
    debug( "writer push {}", it->data );
    if ( it->is_last_substring && cnt == it->data.size() ) {
      output_.writer().close();
    }
    it++;
  }
  bytes_accept.erase( bytes_accept.begin(), it );
}
void Reassembler::insert_to_buffer( uint64_t first_index, string data, bool is_last_substring )
{
  message now = { first_index, data, is_last_substring };
  if ( first_index + data.size() < last ) {
    return;
  }
  if ( first_index < last ) {
    now.data = data.substr( last - first_index, data.size() - last + first_index );
    now.index = last;
  }
  if ( bytes_accept.empty() ) {
    bytes_accept.insert( now );
    return;
  }
  auto s = bytes_accept.lower_bound( { first_index, "", false } ),
       e = bytes_accept.upper_bound( { first_index + data.size(), "", false } );
  if ( !bytes_accept.empty() && s != bytes_accept.begin() && intersect( *std::prev( s ), now ) ) {
    s = std::prev(s);
  }
  for ( auto it = s; it != e; it++ ) {
    now = merge( *it, now );
  }
  if(s != bytes_accept.end())
  bytes_accept.erase( s, e );
  bytes_accept.insert( now );
}

void Reassembler::clean()
{
  auto s = bytes_accept.lower_bound( { last + output_.writer().available_capacity(), "", false } );
  if ( s != bytes_accept.begin() ) {
    s--;
    auto temp = *s;
    if ( s->index + s->data.size() > last + output_.writer().available_capacity() ) {
      // first_index + n - 1 <= last + output_.writer().available_capacity() - 1
      temp.data = temp.data.substr( 0, last + output_.writer().available_capacity() - temp.index );
      temp.is_last_substring = false;
    }
    bytes_accept.erase( s, bytes_accept.end() );
    bytes_accept.insert( temp );
  } else
    bytes_accept.erase( s, bytes_accept.end() );
}
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  insert_to_buffer( first_index, data, is_last_substring );
  transport();
  clean();
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t total_pending_bytes = 0;
  for ( const auto& msg : bytes_accept ) {
    total_pending_bytes += msg.data.size();
  }
  return total_pending_bytes;
}