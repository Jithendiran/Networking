## Session termination — the two ways a session ends

A session can end in one of two ways: orderly release or abrupt release.

* Orderly release is the cooperative, planned end of a session. Both sides agree that the session is complete, confirm that all data has been exchanged and acknowledged, and then close the session in a negotiated sequence. In orderly release, no data is lost. Both sides flush their buffers, exchange finish signals, and release resources. This is the normal case.

* Abrupt release is unilateral. One side closes the session immediately without waiting for the other side to acknowledge or finish. Any data that was in transit is lost. Abrupt release is used when an error condition makes orderly release impossible, or when one side detects a security violation and must terminate immediately.

There is an important nuance here: an orderly release at the session layer does not automatically mean an orderly release at the transport layer. The session layer may close its session while the underlying TCP connection stays open (for reuse by a future session), or the TCP connection may close first (causing the session layer to detect an abrupt release). These two layers are independent.
