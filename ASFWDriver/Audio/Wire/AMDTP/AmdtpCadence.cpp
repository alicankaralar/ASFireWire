#include "AmdtpCadence.hpp"

namespace ASFW::Protocols::Audio::AMDTP {

// Blocking mode at 48 kHz (IEC 61883-6): 6 audio frames arrive per 125 µs bus
// cycle (48000 / 8000), a data packet always carries SYT_INTERVAL = 8 frames.
// Accumulate 6 frames per cycle, emit 8 whenever ≥ 8 are pending:
//
//   pending: 0 → no-data → 6 → data → 4 → data → 2 → data → 0 → ...
//
// yielding the N,D,D,D pattern: exactly 6000 data packets per 8000 cycles.
// phase_ holds the pending (not yet emitted) frame count {0, 6, 4, 2}.
//
// Note: this is cadence only. Alignment of the no-data slot relative to SYT
// is owned by the timing model (Milestone 2), not the cadence.

namespace {
constexpr uint8_t kFramesPerCycle48k = 6;
} // namespace

void BlockingCadence::Configure(uint32_t sampleRateHz,
                                uint8_t sytIntervalFrames) noexcept {
    framesPerCycleNum_ = sampleRateHz; // rate/8000 frames per cycle, scaled by 8000
    sytIntervalFrames_ = sytIntervalFrames;
    sytThresholdNum_ =
        static_cast<uint32_t>(sytIntervalFrames) * kCyclesPerSecond;
    Reset();
}

void BlockingCadence::Reset() noexcept {
    readyNum_ = 0;
    totalCycles_ = 0;
}

bool BlockingCadence::CurrentCycleIsData() const noexcept {
    // FFADO cip.c: data iff floor(ready_samples + samples_per_cycle) >=
    // syt_interval, i.e. (readyNum_ + rate) >= syt_interval * 8000.
    return (readyNum_ + framesPerCycleNum_) >= sytThresholdNum_;
}

uint8_t BlockingCadence::CurrentCycleDataFrames() const noexcept {
    return CurrentCycleIsData() ? sytIntervalFrames_ : 0;
}

uint64_t BlockingCadence::TotalCycles() const noexcept {
    return totalCycles_;
}

void BlockingCadence::AdvanceCycle() noexcept {
    readyNum_ = readyNum_ + framesPerCycleNum_ -
                (CurrentCycleIsData() ? sytThresholdNum_ : 0u);
    ++totalCycles_;
}

// Non-blocking mode at 48 kHz: the per-cycle frame count is integral (6), so
// every cycle is a data packet carrying exactly 6 frames; no no-data packets.

void NonBlocking48kCadence::Reset() noexcept {
    totalCycles_ = 0;
}

bool NonBlocking48kCadence::CurrentCycleIsData() const noexcept {
    return true;
}

uint8_t NonBlocking48kCadence::CurrentCycleDataFrames() const noexcept {
    return kFramesPerCycle48k;
}

uint64_t NonBlocking48kCadence::TotalCycles() const noexcept {
    return totalCycles_;
}

void NonBlocking48kCadence::AdvanceCycle() noexcept {
    ++totalCycles_;
}

} // namespace ASFW::Protocols::Audio::AMDTP
