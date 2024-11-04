#include "reassembler.hh"
#include <vector>
#include <algorithm>
#include <cstdint>

#include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  //(void)first_index;
  //(void)data;
  //(void)is_last_substring;


  // _popped_already_||||||||***ext_segments***|_____the_window_____||||||||||||
  // _______________________________________nxt_idx______________acp_end_____

    /*if(is_last_substring && data.empty()){
        if(!output_.writer().is_closed()){
            output_.writer().close();
        }
        return;
    }*/
    

  // determine the true start & end of the buffer
    size_t capacity_remain = output_.writer().available_capacity();
    uint64_t acp_end = nxt_idx_ + capacity_remain;

    uint64_t data_start = first_index;
    uint64_t data_end = first_index + data.size();

    if(is_last_substring){
        if(data_end == nxt_idx_){
            output_.writer().close();
            return;
        }  
    }

    if (is_last_substring) {
        end_byte_idx_ = data_end;
    }

  // if the data is in the left side of the window
    if (data_end <= nxt_idx_) {
        return;
    }

  // if the data is in the right side of the window
    if (data_start >= acp_end) {
        return;
    }

  // take the acceptable fragments of the data
    if (data_start < nxt_idx_) {
        data.erase(0, nxt_idx_ - data_start);
        data_start = nxt_idx_;
    }
    if (data_end > acp_end) {
        data.resize(acp_end - data_start);
        data_end = acp_end;
    }

    if(data.empty()){
        return;
    }

    Segment new_seg{ data_start, data_end, data };
    uint64_t new_seg_length = new_seg.length();

    // choose a right start
    auto it = unasmb_segments_.lower_bound(new_seg.sta_idx);
    if (it != unasmb_segments_.begin()) {
        --it;
    }

    // segments to be deleted
    std::vector<std::map<uint64_t, Segment>::iterator> to_erase;

    // merge the segments
    while (it != unasmb_segments_.end() && it->second.sta_idx <= new_seg.end_idx) {
        Segment& ext_segments = it->second;

        // if they are not overlaped
        if (ext_segments.end_idx < new_seg.sta_idx) {
            ++it;
            continue;
        }

        // merge two segments
        uint64_t merged_start = std::min(new_seg.sta_idx, ext_segments.sta_idx);
        uint64_t merged_end = std::max(new_seg.end_idx, ext_segments.end_idx);
        size_t merged_length = merged_end - merged_start;
        std::string merged_data(merged_length, '\0');

        // copy the data
        size_t ext_offset = ext_segments.sta_idx - merged_start;
        std::copy(ext_segments.data.begin(), ext_segments.data.end(),
                  merged_data.begin() + ext_offset);

        size_t new_offset = new_seg.sta_idx - merged_start;
        std::copy(new_seg.data.begin(), new_seg.data.end(),
                  merged_data.begin() + new_offset);

        // update new_seg
        new_seg.sta_idx = merged_start;
        new_seg.end_idx = merged_end;
        new_seg_length = merged_end - merged_start;
        new_seg.data = merged_data;
        
        if(unasmb_bytes_ >= ext_segments.length())
            unasmb_bytes_ -= ext_segments.length();
        else{
            unasmb_bytes_ = 0;
            //assert(0);
        }

        to_erase.push_back(it);

        ++it;
    }

    for (auto& iter : to_erase) {
        unasmb_segments_.erase(iter);
    }

    // load new_seg
    unasmb_segments_[new_seg.sta_idx] = new_seg;
    unasmb_bytes_ += new_seg_length;

    // test
    cerr << new_seg.data << new_seg.length()<< endl;
    // test

    // push the data
    it = unasmb_segments_.find(nxt_idx_);
    while (it != unasmb_segments_.end() && it->first == nxt_idx_) {
        Segment& seg = it->second;
        output_.writer().push(seg.data);
        nxt_idx_ += seg.length();
        if (unasmb_bytes_ >= seg.length()) {
            unasmb_bytes_ -= seg.length();
        }
        else {
            unasmb_bytes_ = 0;
        }
        it = unasmb_segments_.erase(it);
    }

    if (nxt_idx_ == end_byte_idx_ && unasmb_bytes_ == 0 && !output_.writer().is_closed()) {// unasmb_bytes_ == 0 
        output_.writer().close();
    }

}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return unasmb_bytes_;
}