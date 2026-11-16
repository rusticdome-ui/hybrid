/* Standard equal-temperament table from the upstream MSFA formula.
 * Copyright 2012 Google Inc.; Apache-2.0.
 */
#include "tuning.h"
class StandardTuning : public TuningState {
 public:
  StandardTuning() {
    const int32_t base=50857777;
    const int32_t step=(1<<24)/12;
    for(int n=0;n<128;++n)table_[n]=base+step*n;
  }
  int32_t midinote_to_logfreq(int n) override {
    if(n<0)n=0;
    if(n>127)n=127;
    return table_[n];
  }
 private:
  int32_t table_[128];
};
std::shared_ptr<TuningState> createStandardTuning(){return std::make_shared<StandardTuning>();}
