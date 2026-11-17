/* Standard-tuning interface adapted from MSFA; Apache-2.0. */
#ifndef HYBRID_DX7_TUNING_H
#define HYBRID_DX7_TUNING_H
#include <stdint.h>
#include <memory>
class TuningState {
 public:
  virtual ~TuningState() {}
  virtual int32_t midinote_to_logfreq(int midinote) = 0;
  virtual bool is_standard_tuning() { return true; }
};
std::shared_ptr<TuningState> createStandardTuning();
#endif
