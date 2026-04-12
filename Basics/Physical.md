## The Foundation: The "Clock"
Every network card has an internal oscillator (a clock) that ticks at a specific frequency (e.g., 10 million times per second for 10 Mbps Ethernet).
* The Sender: Uses its clock to decide when to put a bit on the wire.
* The Receiver: Uses its own clock to decide when to "look" at the wire to read that bit.
The problem is that these two physical clocks are never perfectly identical. They are manufactured by different companies and operate in different temperatures, leading to two specific failures: Clock Phase issues and Clock Slip.

## Clock Phase (The "Where" Problem)
Clock Phase refers to the alignment of the "ticks" between the sender and the receiver.
![phase](./res/clock_phase_rising_edge_good_vs_bad.svg)
- Digital signals don't change from 0 to 1 instantly; they follow a curve.
- Setup & Hold Times: To read a "1" reliably, the voltage must be stable for a tiny window of time.
- The Result: If the receiver samples during that vertical climb (the transition), the voltage might be at 0.5V. The receiver’s hardware won't know if that's a high 0 or a low 1, leading to metastability or bit errors.

Even if both clocks are ticking at the exact same speed, they might start at different times. If the receiver "looks" at the wire exactly when the sender is in the middle of changing a bit, the receiver gets a blurry or transitional signal instead of a clean 0 or 1.

The Goal: The receiver needs to align its "look" (its phase) to the middle of the sender’s bit, where the signal is most stable.

## Clock Slip (The "Speed" Problem)
Clock Slip occurs when one clock is slightly faster or slower than the other. This is cumulative, meaning the error grows over time.

Imagine the sender transmits a long string of 0s.
* The sender's clock is exactly 10.0 MHz.
* The receiver's clock is 10.1 MHz (slightly faster).
* After 100 bits, the receiver’s faster clock will have "ticked" 101 times.

The receiver will think it saw 101 bits of data when the sender only sent 100. This "extra" or "missing" bit is a Clock Slip. It corrupts the entire data stream because every bit after the slip is now in the wrong position.

## Manchester Encoding is the Solution

###  Root Problem (Why This Topic Exists)
#### Why
- When a sender transmits data, the receiver must know when to read each bit. The receiver uses its own internal clock to decide the timing.
- The receiver clock and sender clock are two separate hardware oscillators. They never run at exactly the same speed forever — they drift apart over time.

#### Problem 
- When the receiver clock drifts, the rising edge (latch trigger) no longer falls at the center of the bit. It starts falling closer and closer to the transition zone — the dangerous boundary between two bits.
- Once the rising edge lands on a transition, the latched value is wrong or random.
- The receiver has no way to know it has drifted. There is no feedback. It just silently reads wrong bits.

#### Need
The receiver needs a way to re-synchronize its clock continuously — not just once at the start, but on every single bit — so drift never accumulates to a dangerous level.

### What Synchronization Actually Requires
#### What
- To re-synchronize, the receiver needs a reference point — a moment in time where the signal does something predictable. The receiver can use that moment to reset or nudge its clock back to the correct phase.
- A transition on the wire (the voltage changing from high $\rightarrow$ low or low $\rightarrow$ high) is a detectable event. It has a precise moment in time.
#### Problem
In standard binary encoding (NRZ — Non-Return-to-Zero), a 1 is high voltage and a 0 is low voltage for the entire bit period. If the data has several consecutive identical bits — e.g. `1 1 1 1 1` — the wire stays flat with no transitions at all. No transitions = no reference points = no way to re-synchronize.
#### Conclusion
Any encoding that can produce long runs of the same voltage level is fundamentally unreliable for clock recovery over time.

### Manchester Encoding: The Idea
#### What
Manchester Encoding is a method of encoding binary data onto a wire such that every single bit period is guaranteed to contain exactly one transition (high $\rightarrow$ low or low $\rightarrow$ high) — regardless of what the bit value is.

#### Key Rule

The two primary types of Manchester encoding are *Standard Manchester Encoding* and *Differential Manchester Encoding*

**Standard Manchester Encoding/IEEE 802.3 Ethernet/ G.E.Thomas Method**
- Low  $\rightarrow$ High  means 1
- High  $\rightarrow$ Low  means 0

**Differential Manchester Encoding/IEEE 802.5 Token Ring**
- Low  $\rightarrow$ High  means 0
- High  $\rightarrow$ Low  means 1

For discussion let's take *IEEE 802.3 Ethernet*

Each bit period is split into two equal halves. The transition that happens at the midpoint of the bit period encodes the bit value:
- Bit 0 = transition from HIGH  $\rightarrow$ high LOW at the midpoint
- Bit 1 = transition from LOW $\rightarrow$ HIGH at the midpoint
Because Manchester encoding requires a transition in the middle of every bit, the signal must change states twice as fast as the actual data rate.

![Manchester](./res/manchester_vs_nrz_encoding.svg)

Manchester uses 2 $\times$ the frequency (bandwidth). To send `N` bits per second, `NRZ` needs `N Hz` of bandwidth. Manchester needs `2*N Hz` — because every bit has at least one mid-period transition, and consecutive identical bits also add a boundary transition (like the 1 $\rightarrow$ 1 boundary at position 3$\rightarrow$4 in the diagram, marked in orange). That boundary transition carries no data — it just repositions the line for the next bit's mid-transition.

| | NRZ | Manchester |
|---|---|---|
| Bandwidth needed | 1× | 2× |
| Self-clocking | No | Yes |
| Sync loss risk | High (long runs) | None |
| Used in | Serial, USB | Ethernet (10BASE-T), IR remotes |

### Why This Solves Clock Recovery

#### HOW IT SOLVES IT
- Because every bit period has a guaranteed midpoint transition, the receiver always has a fresh reference point every single bit. The receiver detects the transition and uses it to reset its clock phase precisely.
- Even if the receiver clock drifted slightly during the previous bit, the next midpoint transition corrects it immediately. Drift never accumulates across multiple bits.

#### KEY INSIGHT
The clock information is embedded inside the data signal itself. There is no separate clock wire needed. The receiver extracts timing and data from the same signal simultaneously.

### The Trade-off
#### TRADE-OFF
Manchester Encoding uses twice the bandwidth of NRZ. To send 1 bit of data, the signal must make 2 half-period transitions. The wire must switch at twice the rate. This means the physical medium (wire, fiber, radio) must handle twice the signal frequency to carry the same amount of data.
#### WHERE IT IS USED
Manchester Encoding is used in systems where reliable clock recovery is more important than bandwidth efficiency:
- Ethernet (10BASE-T, classic Ethernet)
- RFID communication
- Industrial sensors and serial protocols

### Complete Mental Model
Step 1. Sender encodes each bit as a transition direction at the midpoint of its time slot. Bit 1 = rising. Bit 0 = falling.

Step 2. The sender may add a boundary transition at the start of a new bit period purely to set the signal to the correct starting level for the next bit's midpoint.

Step 3. Receiver watches the wire. Every midpoint transition detected = one bit read. Rising = 1. Falling = 0.

Step 4. Receiver uses each midpoint transition to re-lock its clock. Phase error from the previous bit is corrected immediately.

Step 5. There is no clock drift accumulation. The receiver never loses synchronization as long as data is being transmitted.