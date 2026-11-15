# Just — Security Architecture

> This document defines the architecture of the Just Security System.
>
> It specifies the engineering components responsible for establishing trust,
> protecting engineering objects and preserving system integrity.
>
> This document intentionally avoids implementation details.
> Every component is specified separately in its own engineering document.

---

# Purpose

The Security Architecture defines how security is organized throughout the Just engineering system.

Rather than relying on a single protection mechanism, security emerges from multiple independent engineering components cooperating through explicit interfaces.

Every component owns exactly one engineering responsibility.

Together they establish, evaluate and preserve trust across the entire engineering system.

---

# Design Goals

The architecture is engineered to be:

- deterministic;
- modular;
- backend-independent;
- documentation-driven;
- continuously verifiable;
- explainable;
- transparent to legitimate users;
- fail-safe.

---

# Architectural Principles

The Security Architecture follows several engineering principles.

- Every security component owns exactly one responsibility.
- Components communicate only through explicit interfaces.
- Security decisions are deterministic.
- Security state is derived rather than duplicated.
- Trust is continuously evaluated.
- Engineering Objects remain the Single Source of Truth.
- Every engineering responsibility has an authoritative owner.

---

# Security Stack

```text
                Boot Security
                      │
                      ▼
               Identity Engine
                      │
                      ▼
            Cryptographic Engine
                      │
                      ▼
               Authentication
                      │
                      ▼
                Authorization
                      │
                      ▼
             Protection Manager
                      │
                      ▼
                 Index Engine
                      │
                      ▼
               Security Engine
                      │
                      ▼
                 Trust Engine
                      │
                      ▼
             Runtime Protection
                      │
                      ▼
               Response Engine
                      │
                      ▼
                 Audit Engine
                      │
                      ▼
               Recovery Engine
```

Each layer contributes one engineering capability.

No individual layer secures the engineering system.

System security emerges from the cooperation of all layers.

---

# Engineering Objects

Security operates exclusively on Engineering Objects.

An Engineering Object represents any entity managed by the engineering system.

Examples include:

- files;
- binaries;
- packages;
- applications;
- plugins;
- themes;
- wallpapers;
- widgets;
- workspaces;
- configuration artifacts;
- documentation;
- services;
- users;
- sessions;
- JSL libraries.

Every Engineering Object is represented through a unified Object Profile.

---

# Object Profile

```text
Engineering Object
        │
        ▼
Object Profile
 ├── Identity
 ├── Metadata
 ├── Relations
 ├── State
 ├── Capabilities
 ├── Security Profile
 └── Extensions
```

The Object Profile is the canonical engineering description of an object.

Subsystems consume the same profile rather than maintaining independent copies.

Consumers include:

- Index Engine;
- Search;
- Launcher;
- AI;
- IDE;
- Marketplace;
- Settings;
- Security Engine.

---

# Engineering Ownership Model

Engineering information always has a single authoritative owner.

Every security-related component consumes authoritative engineering data rather than maintaining independent copies.

```text
Engineering Object
        │
        ▼
Object Profile
        │
        ├───────────────┐
        ▼               ▼
Identity Engine   Index Engine
        │               │
        └──────┬────────┘
               ▼
      Protection Manager
               │
               ▼
       Security Engine
               │
               ▼
        Trust Engine
```

This ownership model guarantees consistency while preventing duplicated engineering state.

---

# Security Profile

Every Engineering Object owns exactly one Security Profile.

```text
Security Profile
 ├── Protection Label
 ├── Protection Policies
 ├── Ownership
 ├── Security Metadata
 └── Audit Metadata
```

The Security Profile represents security-related engineering metadata.

It is stored by the Index Engine and maintained through the Protection Manager.

Runtime trust is intentionally excluded.

Trust is evaluated dynamically by the Trust Engine rather than stored as persistent metadata.

---

# Protection Labels

Protection Labels describe the engineering importance of an object.

They are independent from permissions.

Supported labels include:

- Normal
- UserProtected
- Sensitive
- System
- Critical
- Immutable

Protection Labels determine how aggressively the engineering system protects an object.

They never determine access permissions.

---

# Security Components

## Boot Security

### Responsibility

Establish engineering integrity before kernel execution.

### Provides

- Boot validation
- Boot chain integrity
- BootInfo generation
- Kernel loading

---

## Identity Engine

### Responsibility

Generate and maintain authoritative engineering identities.

### Provides

- System Identity
- Machine Identity
- User Identity
- Session Identity
- Package Identity
- Binary Identity
- Process Identity
- IPC Identity
- Module Identity

The Identity Engine is the authoritative source of engineering identities.

---

## Cryptographic Engine

### Responsibility

Provide cryptographic primitives.

### Provides

- Hashing
- Encryption
- Message Authentication
- Digital Signatures
- Random Generation
- Key Management

This layer provides cryptographic guarantees for higher security layers.

It performs no trust evaluation and makes no security decisions.

---

## Authentication

### Responsibility

Verify engineering identities.

### Provides

- Login verification
- Password authentication
- Session authentication
- IPC authentication
- Service authentication

---

## Authorization

### Responsibility

Evaluate engineering permissions.

### Provides

- Permission evaluation
- Capability evaluation
- Security policies
- Access rules

---

## Protection Manager

### Responsibility

Define engineering protection requirements.

### Provides

- Protection Labels
- Protection Policies
- Protected Objects
- Trusted Applications
- Security Exceptions
- Quarantine Policies

The Protection Manager defines what requires protection.

It performs no runtime verification.

---

## Index Engine

### Responsibility

Maintain the engineering object registry.

### Provides

- Object Discovery
- Metadata Extraction
- Object Relations
- Search Index
- Security Profile Storage

The Index Engine is the authoritative registry of Engineering Objects.

It stores engineering metadata and relationships.

It performs no security evaluation.

---

## Security Engine

### Responsibility

Coordinate security evaluation.

### Provides

- Identity verification
- Metadata retrieval
- Security orchestration
- Risk evaluation
- Decision coordination

The Security Engine never stores persistent metadata.

It evaluates engineering context using information provided by other components.

---

## Trust Engine

### Responsibility

Evaluate engineering trust.

### Provides

- Trust Score
- Behaviour Analysis
- Identity Consistency
- Runtime Trust Updates
- Trust Recalculation

Trust is continuously established, adjusted and, when necessary, revoked.

---

## Runtime Protection

### Responsibility

Observe engineering system execution.

### Provides

- Runtime monitoring
- Behaviour analysis
- IPC monitoring
- Process verification
- Privilege escalation detection
- Identity consistency verification

Runtime Protection continuously observes system execution independently of individual security requests.

---

## Response Engine

### Responsibility

Execute security responses.

### Possible Actions

- Allow
- Log
- Monitor
- Restrict
- Freeze
- Kill
- Quarantine

Responses always remain proportional to engineering risk.

---

## Audit Engine

### Responsibility

Maintain engineering accountability.

### Records

- Security events
- Trust transitions
- IPC activity
- Process activity
- Package activity
- Recovery history

---

## Recovery Engine

### Responsibility

Restore engineering integrity.

### Provides

- Rollback
- Recovery Mode
- Snapshots
- Integrity Repair
- Package Restore
- Session Recovery

---

# Security Decision Flow

Every security-sensitive operation follows the same engineering decision pipeline.

Runtime Protection continuously monitors the engineering system in parallel.

```text
                Runtime Protection
                        │
                        ▼
Request → Authentication → Authorization
                        │
                        ▼
                Identity Engine
                        │
                        ▼
              Protection Manager
                        │
                        ▼
                  Index Engine
                        │
                        ▼
                Security Engine
                        │
                        ▼
                  Trust Engine
                        │
                        ▼
                Response Engine
                        │
                        ▼
                  Audit Engine
                        │
                        ▼
                Recovery Engine
```

---

# Authoritative Responsibilities

| Component | Authoritative Responsibility |
|------------|------------------------------|
| Boot Security | Boot integrity |
| Identity Engine | Engineering identities |
| Cryptographic Engine | Cryptographic primitives |
| Authentication | Identity verification |
| Authorization | Permissions and capabilities |
| Protection Manager | Protection Labels and protection policies |
| Index Engine | Engineering Object registry, metadata and Security Profiles |
| Security Engine | Security orchestration |
| Trust Engine | Runtime trust evaluation |
| Runtime Protection | Runtime observation |
| Response Engine | Security responses |
| Audit Engine | Engineering accountability |
| Recovery Engine | System recovery |

Every engineering responsibility has exactly one authoritative owner.

No responsibility is duplicated.

---

# Relationship to Other Documents

```text
Project Vision
        │
        ▼
Security Philosophy
        │
        ▼
Security Architecture
        │
        ├──────────────┐
        ▼              ▼
Security Components   Technical Specifications
        │              │
        └──────┬───────┘
               ▼
        Implementation
               ▼
          Source Code
```

The Security Architecture defines **what security components exist**, their responsibilities and their relationships.

Individual component documents define **how each component operates internally**.

No lower-level document may contradict the architecture established by this document.