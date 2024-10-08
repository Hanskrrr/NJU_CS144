#include "byte_stream.hh"
#include <string>
#include <algorithm>
#include <deque>

using namespace std;

ByteStream::ByteStream( uint64_t capacity )  
: capacity_( capacity ),
  error_(false),
  buffer_(),
  is_closed_(false),
  tot_bytes_pushed_(0),
  tot_bytes_popped_(0) {}



bool Writer::is_closed() const
{
  // Your code here.
  return is_closed_;
}

void Writer::push( string data )
{
  // Your code here.
  if(is_closed_){
    return;
  }

  uint64_t avail = capacity_ - buffer_.size();
  uint64_t to_push = min(avail, static_cast<uint64_t>(data.size()));
  //buffer_.append(data.substr(0, to_push));
  for(uint64_t i = 0; i < to_push; ++i){
    buffer_.push_back(data[i]);
  }
  tot_bytes_pushed_ += to_push;
  return;
}

void Writer::close()
{
  // Your code here.
  is_closed_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return tot_bytes_pushed_;
}





bool Reader::is_finished() const
{
  // Your code here.
  return is_closed_ && buffer_.empty();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return tot_bytes_popped_;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view(buffer_);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t to_pop = min(len, static_cast<uint64_t>(buffer_.size()));
  buffer_.erase(0, to_pop);
  /*for(uint64_t i; i < to_pop; ++i){
    buffer_.pop_front();
  }*/
  tot_bytes_popped_ += to_pop;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buffer_.size();
}
