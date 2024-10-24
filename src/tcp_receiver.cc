#include "tcp_receiver.hh"
#include <iostream>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  //(void)message;
    if (message.RST) {
        reassembler_.reader().set_error();
        return;
    }

    // handle initial SYN 
    if (!syn_recive_) {
        if (message.SYN) {
            syn_recive_ = true;
            isn = message.seqno; 
        } else {
            return;
        }
    } 

    uint64_t checkpoint = reassembler_.writer().bytes_pushed() + 1; // +1 for SYN
    uint64_t abs_seqno = message.seqno.unwrap(isn, checkpoint);
    uint64_t stream_index = abs_seqno - !message.SYN;

    cerr << checkpoint << ' ' << abs_seqno << ' ' << stream_index << ' ' <<endl;

    reassembler_.insert(stream_index, message.payload, message.FIN);

    if (message.FIN) {
        fin_recive_ = true;
    }
}

TCPReceiverMessage TCPReceiver::send() const
{
    // Your code here.
    TCPReceiverMessage msg;

    // set the RST flag
    if (reassembler_.reader().has_error()) {
        msg.RST = true;
        return msg;
    }

    // calculate the window size 
    size_t window_size = reassembler_.writer().available_capacity();
    if (window_size > UINT16_MAX)
        window_size = UINT16_MAX;
    msg.window_size = static_cast<uint16_t>(window_size);

    // if SYN has been received, compute ackno
    if (syn_recive_) {
        uint64_t bytes_written = reassembler_.writer().bytes_pushed();
        uint64_t abs_ackno = bytes_written + 1; // +1 accounts for SYN

        // if FIN has been received and the input is ended
        if (fin_recive_ && reassembler_.writer().is_closed()) {
            abs_ackno += 1; // +1 for FIN
        }

        msg.ackno = Wrap32::wrap(abs_ackno, isn);
    }

    return msg;
}
