#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class Timer
{
public:
    Timer(uint64_t init_RTO);

    void elapse(uint64_t time_elapsed);   // Decrease the remaining time by elapsed time
    void double_RTO();                    // Double the RTO value (for exponential backoff)
    void reset();                         // Reset the timer to the current RTO value
    void start();                         // Start the timer
    void stop();                          // Stop the timer
    bool expired() const;                 // Check if the timer has expired
    bool is_stopped() const;              // Check if the timer is stopped or expired
    void restore_RTO();                   // Restore RTO to the initial value
    void restart();                       // Restart the timer

private:
    uint64_t timer;       
    uint64_t initial_RTO; 
    uint64_t RTO;         
    bool running;         
};


class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), 
    timer(initial_RTO_ms), outstanding_messages()
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  uint64_t next_seqno_ = 0;          
  uint64_t ackno_ = 0;               
  uint64_t window_size_ = 1;         
  uint64_t retransmissions_ = 0;    
  bool syn_sent_ = false;            
  bool fin_sent_ = false; 

  /*Wrap32 _next_seqno;
  Wrap32 _ackno;
  uint64_t _consecutive_retransmissions;
  uint64_t _current_RTO;
  uint64_t _elapsed_time;*/

  /*std::queue<TCPSenderMessage> _send_queue;
  uint64_t _window_size;
  bool _is_connected;
  std::optional<TCPSenderMessage> _last_sent_segment;*/
  
  Timer timer;                    
  std::deque<TCPSenderMessage> outstanding_messages;

  bool timer_running_ = false;

  void fill_window(const TransmitFunction& transmit);
};
