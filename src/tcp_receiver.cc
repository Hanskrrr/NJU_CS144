#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  //(void)message;
    if (message.RST) {
        reassembler_.reader().set_error();
        return;
    }

    // Handle initial SYN 
    if (!syn_recive_) {
        if (message.SYN) {
            syn_recive_ = true;
            isn = message.seqno; 
        } else {
            return;
        }
    }

    uint64_t checkpoint = reassembler_.writer().bytes_pushed() + 1; // +1 accounts for SYN

    uint64_t abs_seqno = message.seqno.unwrap(isn, checkpoint);

    uint64_t stream_index = abs_seqno - 1;

    reassembler_.insert(stream_index, message.payload, message.FIN);
    if (message.FIN) {
        fin_recive_ = true;
    }
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage msg;

    // Set the RST flag if an error has occurred
    if (reassembler_.reader().has_error()) {
        msg.RST = true;
        return msg;
    }

    // Calculate the window size (available capacity), capped at UINT16_MAX
    size_t window_size = reassembler_.writer().available_capacity();
    if (window_size > UINT16_MAX)
        window_size = UINT16_MAX;
    msg.window_size = static_cast<uint16_t>(window_size);

    // If SYN has been received, compute the acknowledgment number (ackno)
    if (syn_recive_) {
        uint64_t bytes_written = reassembler_.writer().bytes_pushed();
        uint64_t abs_ackno = bytes_written + 1; // +1 accounts for SYN

        // If FIN has been received and the input is ended, increment ackno for FIN
        if (fin_recive_ && reassembler_.writer().is_closed()) {
            abs_ackno += 1; // +1 accounts for FIN
        }

        // Wrap the absolute ackno to a 32-bit sequence number
        msg.ackno = Wrap32::wrap(abs_ackno, isn);
    }

    return msg;
}
