#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  uint32_t wrapped_val = zero_point.raw_value_ + static_cast<uint32_t>(n);
  return Wrap32(wrapped_val);

}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  //(void)zero_point;
  //(void)checkpoint;
  //return {};
  uint32_t seqno_diff = this->raw_value_ - zero_point.raw_value_;

  uint64_t candidate = (checkpoint & 0xFFFFFFFF00000000) + seqno_diff;

  if (candidate > checkpoint && candidate - checkpoint > (1ull << 31)) {
      candidate -= (1ull << 32);
  } else if (candidate < checkpoint && checkpoint - candidate > (1ull << 31)) {
      candidate += (1ull << 32);
  }

  return candidate;
}
