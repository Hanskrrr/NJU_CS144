#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return _next_seqno.unwrap(isn_, _next_seqno.unwrap(isn_, 0));
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return _consecutive_retransmissions;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  while (!_send_queue.empty() && sequence_numbers_in_flight() < _window_size) {
        TCPSenderMessage msg = _send_queue.front();
        transmit(msg); // Use the transmit function to send the message

        _last_sent_segment = msg; // Keep track of the last sent segment
        _send_queue.pop(); // Remove it from the queue

        // Update the next sequence number
        _next_seqno = _next_seqno + msg.sequence_length(); // Update to the next sequence number after sending
        _consecutive_retransmissions = 0; // Reset the retransmission count after a successful send
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg;
    msg.seqno = _next_seqno; 
    msg.payload = ""; 
    msg.SYN = false;
    msg.FIN = false; 
    msg.RST = false; 
    return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  if (msg.ackno.has_value() && *msg.ackno > _ackno) {
        _ackno = *msg.ackno; 
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  _elapsed_time += ms_since_last_tick;

  if (_elapsed_time >= _current_RTO) {
      if (_last_sent_segment) {
          transmit(_last_sent_segment.value()); 
          _consecutive_retransmissions++; 
      }
      _elapsed_time = 0; 
  }
}
