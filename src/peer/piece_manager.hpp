#pragma once

#include <vector>
#include <mutex>
#include <cstdint>
#include <optional>

class PieceManager {
private:
    uint32_t total_pieces_;
    std::vector<bool> completed_pieces_;
    std::vector<bool> in_progress_pieces_;
    std::vector<std::vector<uint8_t>> piece_data_;
    mutable std::mutex mutex_;
    uint32_t next_piece_index_;

public:
    PieceManager(uint32_t total_pieces);

    // Get the next piece that needs to be downloaded
    // Returns std::nullopt if all pieces are complete or in progress
    std::optional<uint32_t> get_next_piece();

    // Mark a piece as completed and store its data
    void mark_complete(uint32_t piece_index, const std::vector<uint8_t>& data);

    // Mark a piece as failed (so it can be retried)
    void mark_failed(uint32_t piece_index);

    // Check if all pieces are complete
    bool is_complete() const;

    // Get the completed piece data in order
    std::vector<uint8_t> assemble_file() const;

    // Get progress statistics
    uint32_t get_completed_count() const;
    uint32_t get_total_pieces() const;
};
