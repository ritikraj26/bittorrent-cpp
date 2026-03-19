#include "peer/piece_manager.hpp"
#include <algorithm>
#include <iostream>

PieceManager::PieceManager(uint32_t total_pieces)
    : total_pieces_(total_pieces),
      completed_pieces_(total_pieces, false),
      in_progress_pieces_(total_pieces, false),
      piece_data_(total_pieces),
      next_piece_index_(0) {
}

std::optional<uint32_t> PieceManager::get_next_piece() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Find the first piece that is not completed and not in progress
    for (uint32_t i = 0; i < total_pieces_; i++) {
        uint32_t piece_index = (next_piece_index_ + i) % total_pieces_;
        
        if (!completed_pieces_[piece_index] && !in_progress_pieces_[piece_index]) {
            in_progress_pieces_[piece_index] = true;
            next_piece_index_ = (piece_index + 1) % total_pieces_;
            
            std::cout << "Assigned piece " << piece_index << " for download" << std::endl;
            return piece_index;
        }
    }

    return std::nullopt;  // All pieces are either completed or in progress
}

void PieceManager::mark_complete(uint32_t piece_index, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (piece_index >= total_pieces_) {
        throw std::runtime_error("Invalid piece index");
    }

    piece_data_[piece_index] = data;
    completed_pieces_[piece_index] = true;
    in_progress_pieces_[piece_index] = false;

    std::cout << "Piece " << piece_index << " completed ("
              << get_completed_count() << "/" << total_pieces_ << ")" << std::endl;
}

void PieceManager::mark_failed(uint32_t piece_index) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (piece_index >= total_pieces_) {
        throw std::runtime_error("Invalid piece index");
    }

    in_progress_pieces_[piece_index] = false;
    std::cout << "Piece " << piece_index << " marked as failed, will retry" << std::endl;
}

bool PieceManager::is_complete() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return std::all_of(completed_pieces_.begin(), completed_pieces_.end(),
                      [](bool completed) { return completed; });
}

std::vector<uint8_t> PieceManager::assemble_file() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<uint8_t> file_data;
    
    for (uint32_t i = 0; i < total_pieces_; i++) {
        if (!completed_pieces_[i]) {
            throw std::runtime_error("Cannot assemble incomplete file");
        }
        
        file_data.insert(file_data.end(),
                        piece_data_[i].begin(),
                        piece_data_[i].end());
    }

    return file_data;
}

uint32_t PieceManager::get_completed_count() const {
    // Caller must hold mutex
    return std::count(completed_pieces_.begin(), completed_pieces_.end(), true);
}

uint32_t PieceManager::get_total_pieces() const {
    return total_pieces_;
}
