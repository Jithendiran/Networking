# Diffie-Hellman Key Exchange — Complete Guide

> **The main idea:** Two people need to end up with the same secret number. They can only talk over a network where an attacker can read everything. They must never send the secret number itself. This sounds impossible. It is not.

---

## Modular Arithmetic

Modular arithmetic is just division — but instead of keeping the answer, only the **remainder** is kept.

The number being divided by is called the **modulus**. The operation is written as `a mod n`.

### The clock picture

A 12-hour clock is the simplest example. If it is 10 o'clock and 5 hours pass, the clock shows **3**, not 15. The count wraps around at 12.

```
(10 + 5) mod 12 = 3
```

### How to compute it by hand

To find `a mod n`:

1. Divide `a` by `n`. Keep only the whole number part (ignore any decimal).
2. Multiply that whole number by `n`.
3. Subtract the result from `a`. What is left is the answer.

**Example: 17 mod 5**

```
Step 1:  17 ÷ 5 = 3.4  ->  whole part = 3
Step 2:  3 × 5 = 15
Step 3:  17 − 15 = 2

Answer: 17 mod 5 = 2
```

**Example: 15 mod 12**

```
Step 1:  15 ÷ 12 = 1.25  ->  whole part = 1
Step 2:  1 × 12 = 12
Step 3:  15 − 12 = 3

Answer: 15 mod 12 = 3
```

### Alternative method (decimal trick)

1. Divide `a` by `n`. Get the decimal number.
2. Remove the whole part. Keep only the decimal.
3. Multiply that decimal by `n`.

**Example: 17 mod 5 using the decimal trick**

```
Step 1:  17 ÷ 5 = 3.4
Step 2:  Keep only 0.4
Step 3:  0.4 × 5 = 2

Answer: 2   (same answer)
```

### Works with non-whole numbers too

**Example: 7.5 mod 2.1**

```
How many times does 2.1 fit into 7.5?
  2.1 × 3 = 6.3  
  2.1 × 4 = 8.4   (too big)

So 3 complete groups fit.

7.5 − 6.3 = 1.2

Answer: 7.5 mod 2.1 = 1.2
```

> **Key rule:** The result of `a mod n` is always between `0` and `n − 1`. It can never equal or be greater than `n`.

---

## Prime Numbers

A **prime number** is a whole number greater than 1 that can only be divided evenly by **1 and itself**.

- `7` is prime. Only `1 × 7` works.
- `13` is prime. Only `1 × 13` works.
- `9` is **not** prime. `3 × 3 = 9`, so 9 has more than two divisors.

The first primes: `2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37 …`

There are infinitely many primes. They go on forever.

> **Why primes matter in crypto:** Large primes are extremely hard to work backwards from. A prime with 600 digits cannot be factored by any computer in any practical amount of time.

---

## Composite Numbers

A **composite number** is any whole number greater than 1 that is **not** prime. It can be broken into smaller pieces.

- `6 = 2 × 3`
- `12 = 2 × 2 × 3`
- `100 = 2 × 2 × 5 × 5`

Every composite number has at least three divisors: 1, itself, and at least one more in between.

---

## Prime Factors

Every whole number greater than 1 is either:
- a prime itself, or
- a product of primes.

Those primes are called its **prime factors**.

This fact has a name: the **Fundamental Theorem of Arithmetic**. Every number has exactly one set of prime factors (It means every number has a unique "DNA" recipe of prime numbers that cannot be found in any other number).

### How to find prime factors

Keep dividing until every piece is prime and cannot be divided further.

**Example: 12**

```
12 = 2 × 6
 6 = 2 × 3

Prime factors of 12:  2 × 2 × 3  (check: 2 × 2 × 3 = 12 ) This is the unique DNA recipe, you cannot form 12 by 2x2x2, 5x2x3,..
```

**Example: 30**

```
30 = 2 × 15
15 = 3 × 5

Prime factors of 30:  2 × 3 × 5  (check: 2 × 3 × 5 = 30 )
```

> **Why this matters for Diffie-Hellman:** To test if a number is a valid generator, the prime factors of `p − 1` are used as a shortcut. This is covered in Section 6.

---

## Cycle Length and Uniformity

When computing repeated powers modulo a number, the results always form a repeating pattern. The number of steps before the pattern repeats is called the **cycle length**.

The maximum possible cycle length for a modulus `n` is `n − 1`. When the sequence reaches that maximum and hits every number from `1` to `n − 1` exactly once, it is called a **full cycle**.

### Why a prime modulus is good

When the modulus is prime and the right base number is chosen, a full cycle is guaranteed. Every number from `1` to `p − 1` appears exactly once. Nothing is skipped.

### Why a composite modulus is weak

Composite numbers have internal factors. Those factors cause the sequence to get trapped in a small loop early — long before a full cycle is reached.

**Prime modulus example — p = 11, base = 2:**

| Power | Calculation | Result |
|-------|-------------|--------|
| $2^{1}$ mod 11 | 2 | **2** |
| $2^{2}$ mod 11 | 4 | **4** |
| $2^{3}$ mod 11 | 8 | **8** |
| $2^{4}$ mod 11 | 16 -> 5 | **5** |
| $2^{5}$ mod 11 | 32 -> 10 | **10** |
| $2^{6}$ mod 11 | 64 -> 9 | **9** |
| $2^{7}$ mod 11 | 128 -> 7 | **7** |
| $2^{8}$ mod 11 | 256 -> 3 | **3** |
| $2^{9}$ mod 11 | 512 -> 6 | **6** |
| $2^{10}$ mod 11 | 1024 -> 1 | **1** |

All 10 numbers from 1 to 10 appeared exactly once. Full cycle.

**Composite modulus example — n = 12, base = 2:**

| Power | Calculation | Result |
|-------|-------------|--------|
| $2^{1}$ mod 12 | 2 | **2** |
| $2^{2}$ mod 12 | 4 | **4** |
| $2^{3}$ mod 12 | 8 | **8** |
| $2^{4}$ mod 12 | 16 -> **4** | repeats! |
| $2^{5}$ mod 12 | 32 -> **8** | repeats! |

Stuck after only 3 distinct values. An attacker has only 3 possible guesses instead of 11.

> **Security consequence:** A short cycle collapses the number of possible secret keys. With a 2048-bit prime modulus, the cycle is so long that guessing is completely impossible in practice.

---

## Exponent
**Why use of exponent instead of additio, subtraction,..?**
In a linear system, such as addition ($g \cdot x \pmod p$), the gap between each consecutive value is constant. If the generator is $3$ and the modulus is $7$, the sequence is:
* $3, 6, 2, 5, 1, 4...$
* The difference between steps is always $3$ (or $-4$). This creates a clear, predictable pattern that is easy to map.

In an exponential system ($g^x \pmod p$), the relationship between $x$ and the result is non-linear. Using the same generator $3$ and modulus $7$:
* $3^1 \equiv 3 \pmod 7$
* $3^2 \equiv 2 \pmod 7$
* $3^3 \equiv 6 \pmod 7$
* $3^4 \equiv 4 \pmod 7$
* $3^5 \equiv 5 \pmod 7$
* $3^6 \equiv 1 \pmod 7$

**If addition**

| Step | Calculation | Result |
| :--- | :--- | :--- |
| $(3) \pmod 7$ | 3 | **3** |
| $(3+3) \pmod 7$ | 6 | **6** |
| $(3+3+3) \pmod 7$ | 9 $\rightarrow$ 2 | **2** |
| $(3+3+3+3) \pmod 7$ | 12 $\rightarrow$ 5 | **5** |
| $(3+3+3+3+3) \pmod 7$ | 15 $\rightarrow$ 1 | **1** |
| $(3+3+3+3+3+3) \pmod 7$ | 18 $\rightarrow$ 4 | **4** |
| $(3+3+3+3+3+3+3) \pmod 7$ | 21 $\rightarrow$ 0 | **0** |

The sequence ($3, 2, 6, 4, 5, 1$) does not follow a visible trend. It jumps across the range of available numbers. This lack of a visible pattern is the reason exponentiation is preferred for security.

The sequence ($3,6,2,5,1,4,0$) follows a linear trend of $+3$ or $-4$, $3+3 = 6$, $2+3=5$, $5-4 = 1$, $4-4 = 0$

The perception of linearity in exponentiation is a common misconception, often arising from how numbers behave in standard arithmetic versus modular arithmetic. In a standard number system, exponents create a smooth, predictable curve. However, when a modulus is applied, that predictability vanishes.

### The Diffusion of Information
Exponents are chosen because they provide diffusion. In a linear sequence, a small change in the input $x$ leads to a small, predictable change in the output. In modular exponentiation, changing the exponent by $1$ causes the result to jump to a completely different, seemingly unrelated part of the number set.

### Asymmetry of effort
* Forward Direction: Calculating $g^x \pmod p$ is computationally efficient even for massive numbers.
* Reverse Direction: Deducing $x$ from the result is computationally "hard." No shortcut exists to skip the steps; one must essentially test every possibility or use extremely complex algorithms.

This "hardness" is absent in addition or multiplication. If addition were used, the "secret" exponent could be found instantly using simple division. Exponentiation ensures that while the generator hits every number, it does so in a way that hides the path taken.

## Generators

A **generator** (also called a *primitive root*) is a number `g` such that, when raised to the powers `1, 2, 3 …` modulo a prime `p`, it produces every value from `1` to `p − 1` exactly once.

In math notation: for every possible target value `y`, there is some exponent `x` where `g^x mod p = y`.

### Example with p = 7

**g = 3 is a generator:**

| Power | Calculation | Result |
|-------|-------------|--------|
| $3^{1}$ mod 7 | 3 | **3** |
| $3^{2}$ mod 7 | 9 -> 2 | **2** |
| $3^{3}$ mod 7 | 27 -> 6 | **6** |
| $3^{4}$ mod 7 | 81 -> 4 | **4** |
| $3^{5}$ mod 7 | 243 -> 5 | **5** |
| $3^{6}$ mod 7 | 729 -> 1 | **1** |

Hit all of {1, 2, 3, 4, 5, 6}. That is a full cycle. 3 is a generator. 

**g = 2 is NOT a generator:**

| Power | Calculation | Result |
|-------|-------------|--------|
| $2^{1}$ mod 7 | 2 | **2** |
| $2^{2}$ mod 7 | 4 | **4** |
| $2^{3}$ mod 7 | 8 -> 1 | **1** |
| $2^{4}$ mod 7 | -> **2** | repeats! |

Only hit {1, 2, 4}. Missed {3, 5, 6}. Not a generator. 

### How to test if a number is a generator

For small primes, listing every power works. For large primes used in real cryptography, a shortcut test is used instead — based on the prime factors of `p − 1`.

**The test:**

1. Find all prime factors of `p − 1`. Call them `q_1, q_2, q_3 …`
2. For each prime factor `q_i`, compute: `g^((p−1)/q_i) mod p`
3. If **any** result equals `1`, then `g` is **not** a generator.
4. If **none** of the results equal `1`, then `g` **is** a generator.

**Example: Is g = 3 a generator for p = 7?**

```
p − 1 = 6 = 2 × 3   ->   prime factors: 2 and 3

Test factor 2:  3^(6/2) = 3^3 = 27 mod 7 = 6   (not 1) 
Test factor 3:  3^(6/3) = 3^2 = 9 mod 7  = 2   (not 1) 

None equal 1  ->  3 IS a generator. 
```

**Example: Is g = 2 a generator for p = 7?**

```
p − 1 = 6 = 2 × 3   ->   prime factors: 2 and 3

Test factor 2:  2^(6/2) = 2^3 = 8 mod 7 = 1   <- equals 1! 

Fails here  ->  2 is NOT a generator. 
```

> **Why generators matter:** In Diffie-Hellman, the generator is public. Security comes from the secret exponent. When `g` is a true generator, the secret exponent could be any of `p − 2` possible values. A non-generator shrinks that number dramatically, making the secret much easier to guess.

---

## The Discrete Logarithm Problem

The security of Diffie-Hellman comes from one important asymmetry:

| Direction | Task | Speed |
|-----------|------|-------|
| **Forward (easy)** | Given `g`, `x`, `p` — compute `g^x mod p` | Very fast |
| **Reverse (hard)** | Given `g`, `y`, `p` — find `x` where `g^x mod p = y` | Practically impossible |

The reverse direction is the **Discrete Logarithm Problem (DLP)**.

### Why the forward direction is fast

A technique called **fast exponentiation** (also called square-and-multiply) computes `g^x mod p` in roughly `log_2(x)` steps. For a 2048-bit exponent, that is about 2048 steps — a computer does this instantly.

### Why the reverse direction is hard

There is no known shortcut. The best known methods still require an enormous amount of computation. For a 2048-bit prime, solving the DLP would take longer than the age of the universe on any computer that exists today.

> **Simple way to think about it:** Modular exponentiation is a one-way street. Easy to drive forward. Essentially impossible to drive backward. That one-way property is the entire lock that secures Diffie-Hellman.

---

## The Diffie-Hellman Protocol, Step by Step

Two people — **Alice** and **Bob** — want to agree on a shared secret. An attacker can read every single message they send.

### Step 1 — Agree on public parameters

Alice and Bob publicly agree on two values. These are **not secret**. Anyone can see them.

```
p = a large prime number
g = a generator for p  (a primitive root)
```

These are usually taken from pre-approved published standards (like RFC 3526). There is nothing special about knowing them.

### Step 2 — Each picks a private key

Alice picks a random number `a`. Bob picks a random number `b`. Neither tells the other. These are their **private keys** and are never sent over the network.

```
Alice: private key = a  (kept secret)
Bob:   private key = b  (kept secret)
```

### Step 3 — Each computes and sends a public value

Each person raises `g` to their private key, modulo `p`. The result is their **public value**. This is sent over the network. The attacker can see it.

```
Alice computes:  A = g^a mod p  ->  sends A to Bob
Bob computes:    B = g^b mod p  ->  sends B to Alice
```

### Step 4 — Each computes the shared secret

Alice takes Bob's public value `B` and raises it to her private key `a`.
Bob takes Alice's public value `A` and raises it to his private key `b`.

```
Alice: S = B^a mod p  =  (g^b)^a mod p  =  g^{ab} mod p
Bob:   S = A^b mod p  =  (g^a)^b mod p  =  g^{ab} mod p
```

Both arrive at `g^{ab} mod p`. This is their shared secret. **The attacker never saw `a`, `b`, or `g^{ab} mod p`.**

> **Why they get the same result:** `(gᵃ)ᵇ = g^{ab} = (g^b)^a`. Exponentiation is commutative — the order does not matter. Both paths lead to the same number.

---

### Worked numeric example (small numbers for clarity)

**Public parameters:**
```
p = 23   (prime)
g = 5    (a generator for 23)
```

**Private keys:**
```
Alice picks:  a = 6    (kept secret)
Bob picks:    b = 15   (kept secret)
```

**Public values (sent over network):**
```
A = 5^6 mod 23  =  15625 mod 23  =  8    <- Alice sends this
B = 5^{15} mod 23  =  30517578125 mod 23  =  19   <- Bob sends this
```

**Shared secret:**
```
Alice:  19^6 mod 23  =  2
Bob:    8^{15} mod 23  =  2

Shared secret = 2  
```

The attacker saw `p = 23`, `g = 5`, `A = 8`, `B = 19`. To get the secret, they must find `a` from `5ᵃ mod 23 = 8`. That is the Discrete Logarithm Problem.

---

## Why It Is Secure

### What the attacker knows

The attacker can see everything sent publicly:
- The prime `p`
- The generator `g`
- Alice's public value `A = g^a mod p`
- Bob's public value `B = g^b mod p`

### What the attacker needs

To compute the shared secret `g^{ab} mod p`, the attacker must find either `a` or `b`. To find `a`, the attacker must solve: *given `g`, `A`, and `p`, find `a` where `g^a mod p = A`*. That is the Discrete Logarithm Problem. For large primes, this is computationally infeasible.

### Parameter sizes used in practice

| Security level | Prime size | Status |
|----------------|-----------|--------|
| Legacy | 1024 bits |  Not recommended |
| Standard | 2048 bits |  Acceptable |
| Strong | 3072 bits |  Recommended |
| Long-term | 4096 bits |  High security |

### Known weaknesses to be aware of

**Man-in-the-middle attack:**
Basic DH does not verify *who* is communicating. An attacker sitting between Alice and Bob can intercept both exchanges and set up two separate "shared secrets" — one with Alice and one with Bob — without either knowing. This is why DH is almost always used together with digital signatures or certificates in real protocols (like TLS/HTTPS).

**Small subgroup attack:**
If an attacker can force a weak public value to be used (one that only cycles through a tiny subgroup), the secret space collapses. The fix: always check that received public values fall in the expected valid range.

**Logjam attack (2015):**
Researchers showed that 512-bit "export-grade" DH parameters could be broken in real time. This led to permanently removing all parameters smaller than 1024 bits from use.

**Quantum computers (future risk):**
Shor's algorithm, running on a sufficiently powerful quantum computer, can solve the DLP efficiently. Post-quantum cryptographic standards are being developed to address this. Current DH is safe against classical computers.

---

## Complete Summary

### The building blocks, in order

| # | Concept | One-line meaning |
|---|---------|-----------------|
| 1 | Modular arithmetic | Division that keeps only the remainder; numbers wrap around |
| 2 | Prime numbers | Numbers divisible only by 1 and themselves |
| 3 | Composite numbers | Numbers with more than two divisors; weak as moduli |
| 4 | Prime factors | Every number breaks down uniquely into primes |
| 5 | Cycle length | How long a power sequence runs before repeating |
| 6 | Generators | A base that reaches every value in the group; tested via prime factors of `p−1` |
| 7 | Discrete Logarithm Problem | Computing `g^x mod p` is easy; reversing it is hard |
| 8 | DH protocol | Agree on `(p, g)` publicly; keep private keys secret; exchange public values; both arrive at `g^{ab} mod p` |

### The one sentence to remember

> Two people can each compute the same value `g^{ab} mod p` without either one ever transmitting `a`, `b`, or `g^{ab} mod p` — because computing `g^x mod p` is easy, but finding `x` from `g^x mod p` is not.
