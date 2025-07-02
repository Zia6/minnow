#include "reassembler.hh"
#include "debug.hh"

using namespace std;

Reassembler::message merge(Reassembler::message m1, Reassembler::message m2) {
    if (m1.index > m2.index) {
        std::swap(m1, m2);
    }

    uint64_t m1_end_exclusive = m1.index + m1.data.size();

    uint64_t m2_start_idx_to_append = 0;
    if (m1_end_exclusive > m2.index) {
        m2_start_idx_to_append = m1_end_exclusive - m2.index;
    }

    if (m2_start_idx_to_append < m2.data.size()) {
        m1.data += m2.data.substr(m2_start_idx_to_append);
    }

    // 最终的 is_last_substring 状态：只要合并的两个片段中有一个是最后一个，结果就是最后一个
    m1.is_last_substring = m1.is_last_substring || m2.is_last_substring;

    return m1;
}

bool intersect(Reassembler::message m1,Reassembler::message m2){
  if(m1.index > m2.index){
    std::swap(m1,m2);
  }
  return m1.index + m1.data.size() - 1 >= m2.index;
}


void Reassembler::transport(){
  //将传来的string 插入到缓冲区
  Writer writer = output_.writer();
  // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
  auto it = bytes_accept.begin();
  while(it != bytes_accept.end() && it -> index == last && it -> data.size() <= writer.available_capacity()){
    writer.push(it -> data);
    count += it-> data.size();
    last += it -> data.size();
  }
  bytes_accept.erase(bytes_accept.begin(),it);

}
void Reassembler::insert_to_buffer(uint64_t first_index, string& data, bool is_last_substring){
  message now = {first_index,data,is_last_substring};
  if(first_index + data.size() <= last){
    return;
  }
  if(first_index <= last){
    data = data.substr(last - first_index,data.size() - last + first_index);
    first_index = last;
  }

  auto s = bytes_accept.lower_bound({first_index,"",false}),e = bytes_accept.upper_bound({first_index + data.size(),"",false});
  if(s != bytes_accept.begin()){
    s--;
  }
  if(!intersect(*s,now)){
    s++;
  }
  for(auto it = s; it != e; it++){
    now = merge(*it,now);
  }
  bytes_accept.erase(s,e);
  bytes_accept.insert(now);
}


void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  insert_to_buffer(first_index,data,is_last_substring);
  transport();
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const {
    uint64_t total_pending_bytes = 0;
    for (const auto& msg : bytes_accept) {
        total_pending_bytes += msg.data.size();
    }
    return total_pending_bytes;
}