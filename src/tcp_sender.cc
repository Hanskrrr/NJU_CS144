#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <algorithm>
#include <optional>

using namespace std;

Timer::Timer( uint64_t init_RTO ) : timer( init_RTO ), initial_RTO( init_RTO ), RTO( init_RTO ), running( false ) {}

void Timer::elapse( uint64_t time_elapsed )
{
  if ( running ) {
    timer = ( time_elapsed >= timer ) ? 0 : timer - time_elapsed;
  }
}

void Timer::double_RTO()
{
  RTO = min( RTO * 2, static_cast<uint64_t>( TCPConfig::TIMEOUT_DFLT ) );
}

void Timer::reset()
{
  timer = RTO;
}

void Timer::start()
{
  if ( !running ) {
    timer = RTO;
    running = true;
  }
}

void Timer::stop()
{
  running = false;
}

bool Timer::expired() const
{
  return running && timer == 0;
}

bool Timer::is_stopped() const
{
  return !running;
}

void Timer::restore_RTO()
{
  RTO = initial_RTO;
}

void Timer::restart()
{
  reset();
  start();
}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return next_seqno_ - ackno_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  fill_window( transmit );
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap( next_seqno_, isn_ );
  msg.SYN = false;
  msg.FIN = false;
  msg.RST = input_.has_error();
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  // Update window size
  window_size_ = msg.window_size;

  if ( msg.ackno.has_value() ) {
    uint64_t ack = msg.ackno->unwrap( isn_, next_seqno_ );

    // Ignore duplicate or old ACKs
    if ( ack > ackno_ && ack <= next_seqno_ ) {
      ackno_ = ack;

      // Remove fully acknowledged segments from outstanding_messages
      while ( !outstanding_messages.empty() ) {
        const TCPSenderMessage& segment = outstanding_messages.front();
        uint64_t seq_end = segment.seqno.unwrap( isn_, next_seqno_ ) + segment.sequence_length();
        if ( seq_end <= ackno_ ) {
          outstanding_messages.pop_front();
        } else {
          break;
        }
      }

      // Reset retransmission counter and RTO to initial value
      retransmissions_ = 0;
      timer.restore_RTO();

      // Restart the timer if there are outstanding segments
      if ( !outstanding_messages.empty() ) {
        timer.restart();
      } else {
        // Stop the timer if there's no outstanding data
        timer.stop();
      }
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  timer.elapse( ms_since_last_tick );
  if ( timer.expired() ) {
    if ( !outstanding_messages.empty() ) {
      const TCPSenderMessage& segment = outstanding_messages.front();
      transmit( segment );
      if ( window_size_ > 0 ) {
        retransmissions_++;
        timer.double_RTO();
      }
      timer.reset();
      timer.start();
      timer_running_ = true;
    }
  }
}

void TCPSender::fill_window( const TransmitFunction& transmit )
{
  uint64_t window_size = window_size_ ? window_size_ : 1; // Pretend window size is 1 if zero
  uint64_t inflight = sequence_numbers_in_flight();
  uint64_t window_remaining = window_size > inflight ? window_size - inflight : 0;

  if ( !syn_sent_ && window_remaining > 0 ) {
    TCPSenderMessage syn_segment;
    syn_segment.seqno = Wrap32::wrap( next_seqno_, isn_ );
    syn_segment.SYN = true;

    transmit( syn_segment );

    outstanding_messages.push_back( syn_segment );

    syn_sent_ = true;
    next_seqno_ += syn_segment.sequence_length();
    window_remaining -= syn_segment.sequence_length();

    if ( timer.is_stopped() ) {
      timer.start();
    }

    if ( window_remaining == 0 ) {
      return;
    }
  }

  // Fill the window with data segments
  while ( window_remaining > 0 ) {
    size_t payload_size = min( { window_remaining,
                                 static_cast<uint64_t>( TCPConfig::MAX_PAYLOAD_SIZE ),
                                 input_.reader().bytes_buffered() } );

    if ( payload_size == 0 && ( !input_.reader().is_finished() || fin_sent_ ) ) {
      break;
    }

    TCPSenderMessage segment;
    segment.seqno = Wrap32::wrap( next_seqno_, isn_ );
    segment.payload = input_.reader().read( payload_size );

    // If the outbound stream is at EOF and we haven't sent FIN yet, and there is space in the window
    if ( input_.reader().is_finished() && !fin_sent_ && window_remaining > segment.sequence_length() ) {
      segment.FIN = true;
      fin_sent_ = true;
    }

    if ( segment.sequence_length() == 0 ) {
      break;
    }

    transmit( segment );

    outstanding_messages.push_back( segment );

    next_seqno_ += segment.sequence_length();
    window_remaining -= segment.sequence_length();

    if ( timer.is_stopped() ) {
      timer.start();
    }

    if ( window_size_ == 0 ) {
      break;
    }
  }
}
