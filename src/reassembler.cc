#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  while(bytes_accept.top().first <= last){
    std::pair<uint64_t,std::string> p = bytes_accept.top();
    bytes_accept.pop();
    if(p.first == last){
      last += p.second.size();
      count -= p.second.size();
      output_.writer().push(data);
    } 
  }
  if(last == first_index){
    if(output_.writer().available_capacity() >= data.size()){
      output_.writer().push(data);
    }
  }else {
    if(last > first_index) return;
    else if(last + output_.writer().available_capacity() >= first_index + data.size()){
      bytes_accept.push({first_index,data});
      count += data.size();
    }
  }
  // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return count;
}
