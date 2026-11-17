/* HYBRID AudioWorklet ring-buffer stub.
 * MIDI is dispatched directly through SynthUnit::sendMidi(); the legacy
 * producer queue is intentionally empty and never blocks the real-time thread.
 */
#include "synth.h"
#include "ringbuffer.h"
RingBuffer::RingBuffer() {}
int RingBuffer::BytesAvailable(){return 0;}
int RingBuffer::WriteBytesAvailable(){return 8191;}
int RingBuffer::Read(int, uint8_t*){return 0;}
int RingBuffer::Write(const uint8_t*, int size){return size;}
