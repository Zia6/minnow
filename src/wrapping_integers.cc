#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // 直接相加，uint32_t 的自动截断会处理溢出
  return Wrap32 { static_cast<uint32_t>( n + zero_point.raw_value_ ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // debug( "unwrap( {}, {} ) called", zero_point.raw_value_, checkpoint );

  const uint64_t num_32_bit_range = 1ULL << 32; // 2^32，即模数

  // 1. 计算我们期望的最终结果 'n' 的低 32 位。
  // 这就是 (this->raw_value_ - zero_point.raw_value_) 模 2^32 的结果。
  // 无符号减法 `uint32_t - uint32_t` 本身就实现了对 2^32 的模运算，结果会自动回卷。
  uint32_t expected_n_low_32_bits = this->raw_value_ - zero_point.raw_value_;

  // 2. 构造一个初始候选值 `candidate_base`：
  //    它由 `checkpoint` 所在 2^32 周期的高位部分
  //    结合我们期望的 `expected_n_low_32_bits` 作为低位构成。
  uint64_t candidate_base = ( checkpoint & ~( num_32_bit_range - 1ULL ) ) | (uint64_t)expected_n_low_32_bits;
  // 注意：~(num_32_bit_range - 1ULL) 是正确清除低32位的掩码。
  // 例如，如果 num_32_bit_range = 0x100000000 (2^32)，
  // 那么 num_32_bit_range - 1ULL = 0x0FFFFFFFF (UINT32_MAX)，
  // ~ (num_32_bit_range - 1ULL) = 0xFFFFFFFF00000000 (高32位全1，低32位全0)。
  // 这正是我们需要的掩码，用于保留 checkpoint 的高 32 位。

  // 3. 定义三个最可能的候选值：
  //    a) `candidate_base` 本身
  //    b) `candidate_base` 减去一个周期
  //    c) `candidate_base` 加上一个周期
  // 使用 std::array 或原始数组皆可
  uint64_t candidates[] = { candidate_base, candidate_base - num_32_bit_range, candidate_base + num_32_bit_range };

  // 4. 找到距离 `checkpoint` 最近的那个候选值
  uint64_t best_n = candidates[0];
  // 使用 int64_t 来正确计算差值的绝对值，避免无符号数回卷问题
  uint64_t min_diff
    = ( candidates[0] >= checkpoint ) ? ( candidates[0] - checkpoint ) : ( checkpoint - candidates[0] );
  for ( int i = 1; i < 3; ++i ) {
    uint64_t current_diff
      = ( candidates[i] >= checkpoint ) ? ( candidates[i] - checkpoint ) : ( checkpoint - candidates[i] );
    if ( current_diff < min_diff ) {
      min_diff = current_diff;
      best_n = candidates[i];
    }
  }

  return best_n;
}
