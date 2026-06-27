#pragma once

#include "AmdtpTypes.hpp"

#include <cstdint>

namespace ASFW::Protocols::Audio::AMDTP {

class IAmdtpCadence {
public:
    virtual ~IAmdtpCadence() = default;

    virtual void Reset() noexcept = 0;

    virtual bool CurrentCycleIsData() const noexcept = 0;
    virtual uint8_t CurrentCycleDataFrames() const noexcept = 0;
    virtual uint64_t TotalCycles() const noexcept = 0;

    virtual void AdvanceCycle() noexcept = 0;
};

// Blocking-mode AMDTP cadence for any 1x/2x/4x rate. Defaults to 48 kHz so
// existing default-constructed callers are byte-identical to the old
// Blocking48kCadence. Configure() switches the rate.
//
// Cross-validated with FFADO iec61883_cip_fill_header (libffado-2.4.9
// src/libstreaming/util/cip.c:130-160): the blocking cadence is a rational
// accumulator of rate/8000 frames per 125 us bus cycle that emits a
// syt_interval-frame data block whenever at least syt_interval frames are
// pending; otherwise a NO-DATA packet. The previous 48 kHz integer form
// (6 frames/cycle, emit 8) is exactly the rate==48000 special case.
class BlockingCadence final : public IAmdtpCadence {
public:
    // sytIntervalFrames is the blocking frames-per-data-packet: 8 @1x
    // (32/44.1/48 k), 16 @2x, 32 @4x (FFADO getSytInterval()).
    void Configure(uint32_t sampleRateHz, uint8_t sytIntervalFrames) noexcept;

    void Reset() noexcept override;

    bool CurrentCycleIsData() const noexcept override;
    uint8_t CurrentCycleDataFrames() const noexcept override;
    uint64_t TotalCycles() const noexcept override;

    void AdvanceCycle() noexcept override;

private:
    // Rational accumulator over an 8000 cycles/s denominator: readyNum_ counts
    // pending frames * 8000. Each cycle adds framesPerCycleNum_ (== sampleRateHz,
    // i.e. rate/8000 frames scaled by 8000) and emits a block of
    // sytIntervalFrames_ when readyNum_ + framesPerCycleNum_ reaches
    // sytThresholdNum_ (== sytIntervalFrames_ * 8000). Exact, drift-free for
    // fractional rates such as 44.1 kHz (5.5125 frames/cycle).
    static constexpr uint32_t kCyclesPerSecond = 8000;
    uint32_t framesPerCycleNum_{48000};
    uint32_t sytThresholdNum_{8u * kCyclesPerSecond};
    uint8_t sytIntervalFrames_{8};
    uint32_t readyNum_{0};
    uint64_t totalCycles_{0};
};

// Transitional alias: existing callers default-construct this for 48 kHz.
using Blocking48kCadence = BlockingCadence;

class NonBlocking48kCadence final : public IAmdtpCadence {
public:
    void Reset() noexcept override;

    bool CurrentCycleIsData() const noexcept override;
    uint8_t CurrentCycleDataFrames() const noexcept override;
    uint64_t TotalCycles() const noexcept override;

    void AdvanceCycle() noexcept override;

private:
    uint64_t totalCycles_{0};
};

} // namespace ASFW::Protocols::Audio::AMDTP
