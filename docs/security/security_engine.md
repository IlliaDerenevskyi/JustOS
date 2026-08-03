# Just — Security Engine

> This document defines the architecture of the Just Security Engine.
>
> It describes how security responsibilities are organized, how trust is established and maintained, and how security decisions are produced throughout the engineering system.

---

# Purpose

The Security Engine preserves the integrity, trustworthiness and reliability of the Just engineering system.

Unlike traditional security models, security is not implemented as an isolated subsystem.

Instead, security is distributed across multiple independent engineering layers that cooperate to continuously evaluate and protect the system.

The Security Engine transforms the principles defined in the Security Philosophy into concrete engineering mechanisms.

---

# Design Goals

The Security Engine is designed to be:

- deterministic;
- explainable;
- continuously verifiable;
- modular;
- backend-independent;
- documentation-driven;
- transparent to legitimate users;
- fail-safe.

---

# Security Architecture

```text
Layer 0  Boot Security
Layer 1  Identity Engine
Layer 2  Cryptographic Engine
Layer 3  Authentication
Layer 4  Authorization
Layer 5  Trust Engine
Layer 6  Runtime Protection
Layer 7  Response Engine
Layer 8  Audit Engine
Layer 9  Recovery Engine
```

Every layer has a clearly defined responsibility.

Layers cooperate through explicit interfaces and never duplicate responsibilities.

---

# Layer 0 — Boot Security

## Purpose

Protect the engineering system before kernel execution begins.

### Responsibilities

- Boot Manager integrity
- Kernel loading
- BootInfo validation
- Secure Boot (optional)
- Boot chain verification

---

# Layer 1 — Identity Engine

## Purpose

Provide deterministic engineering identities for every critical engineering component.

The Identity Engine is the authoritative source of engineering identities.

Every other security layer consumes identities from this layer rather than creating its own.

## Engineering Identities

| Identity | Purpose | Lifetime |
|----------|----------|----------|
| os_id | Installation identity | Until reinstall |
| machine_id | Hardware identity | Hardware lifetime |
| session_id | Authenticated session | Login → Logout |
| user_id | Logical user | Permanent |
| package_id | Installed package | Package lifetime |
| binary_id | Executable identity | Binary lifetime |
| process_id | Runtime process | Process lifetime |
| ipc_id | IPC endpoint | IPC lifetime |
| module_id | Internal Just subsystem | Permanent |

---

# Layer 2 — Cryptographic Engine

## Purpose

Provide cryptographic primitives used by higher security layers.

This layer performs no trust evaluation and makes no security decisions.

## Hashing

**SHA-256**

Used for:

- binaries;
- packages;
- configuration;
- plugins;
- caches.

---

## Authenticated Encryption

**ChaCha20-Poly1305**

Used for:

- IPC packets;
- AI communication;
- encrypted configuration;
- secrets;
- tokens.

---

## Message Authentication

**HMAC-SHA256**

Used for:

- IPC authentication;
- session tokens;
- replay protection;
- message authentication.

---

## Digital Signatures

**Ed25519**

Used for:

- packages;
- JSL libraries;
- themes;
- plugins;
- updates;
- marketplace assets.

---

## Random Generation

**getrandom()**

**ChaCha20 DRBG**

Used for:

- session identifiers;
- IPC identifiers;
- cryptographic keys;
- nonces.

---

# Layer 3 — Authentication

## Purpose

Determine who is requesting an engineering operation.

### Responsibilities

- login;
- password verification;
- sudo authentication;
- session authentication;
- IPC authentication;
- AI authentication.

---

# Layer 4 — Authorization

## Purpose

Determine what an authenticated engineering component is allowed to perform.

### Responsibilities

- permissions;
- capabilities;
- policies;
- security rules;
- service permissions;
- JSL permissions.

---

# Layer 5 — Trust Engine

## Purpose

Continuously evaluate engineering trust.

The Trust Engine correlates information provided by multiple independent security layers.

Trust evaluation considers:

- Binary Identity;
- Package Identity;
- Session Identity;
- IPC Identity;
- Permission Context;
- Process Lineage;
- Runtime Behaviour;
- Integrity State.

Trust is continuously updated throughout execution.

Trust is never permanently granted.

It is continuously established, adjusted and, when necessary, revoked.

---

## Trust Levels

| Score | State |
|--------|-------|
| 80–100 | Trusted |
| 60–79 | Observe |
| 40–59 | Monitor |
| 20–39 | Restricted |
| 1–19 | Freeze |
| 0 | Kill |

The trust scoring algorithm is specified separately.

---

# Layer 6 — Runtime Protection

## Purpose

Observe and protect the engineering system during execution.

### Responsibilities

- runtime monitoring;
- behaviour analysis;
- privilege escalation detection;
- IPC monitoring;
- identity consistency verification;
- suspicious process detection.

---

## Verification Strategy

### Static Verification

Performed only when engineering state changes.

Includes:

- machine_id;
- os_id;
- package_id;
- binary_id.

---

### Continuous Verification

Performed continuously during runtime.

Includes:

- session_id;
- process_id;
- ipc_id;
- process lineage;
- permission context;
- trust score;
- runtime behaviour.

This strategy minimizes unnecessary verification while preserving continuous trust evaluation.

---

# Layer 7 — Response Engine

## Purpose

Transform trust evaluation into engineering actions.

Possible security responses:

- Allow;
- Log;
- Monitor;
- Restrict;
- Freeze;
- Kill;
- Quarantine.

Responses always remain proportional to engineering risk.

---

## Example Response Matrix

| Event | Security Decision |
|--------|-------------------|
| Invalid IPC HMAC | Drop packet |
| Invalid nonce | Ignore |
| Unknown executable | Reduce Trust |
| Modified binary | Freeze process |
| Invalid package signature | Reject |
| Trust below threshold | Restrict |
| Trust equals zero | Kill |

---

# Layer 8 — Audit Engine

## Purpose

Maintain complete engineering accountability.

Typical audited events include:

- login;
- logout;
- sudo;
- package installation;
- package removal;
- binary modification;
- IPC anomalies;
- trust transitions;
- freeze;
- kill;
- quarantine.

Audit information exists to support engineering diagnostics rather than surveillance.

---

# Layer 9 — Recovery Engine

## Purpose

Restore engineering integrity after security incidents.

### Responsibilities

- snapshots;
- rollback;
- recovery mode;
- integrity repair;
- package restoration;
- session recovery.

---

# Security Flow

Every security-sensitive operation follows the same engineering pipeline.

```text
Request
    │
    ▼
Authentication
    │
    ▼
Identity Verification
    │
    ▼
Authorization
    │
    ▼
Trust Engine
    │
    ▼
Risk Evaluation
    │
    ▼
Response Engine
    │
    ▼
Audit Engine
    │
    ▼
Recovery Engine (optional)
```

---

# Relationship to Other Documents

The Security Engine translates the principles defined by the Security Philosophy into a concrete engineering architecture.

It defines the responsibilities of every security layer while delegating implementation details to dedicated technical specifications.

```text
Security Philosophy
        │
        ▼
   Security Engine
        │
        ├──────────────┐
        ▼              ▼
Security Layers   Technical Specifications
        │              │
        └──────┬───────┘
               ▼
       Implementation
```

Every security layer may be further specified by its own technical specification document.

No lower-level specification or implementation may contradict the architecture established by this document.