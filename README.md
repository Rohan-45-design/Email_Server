# Email Server (SMTP + IMAP + Web UI)

This repository contains a complete single-node email server implementation,
including:

- C++ Backend (SMTP, IMAP, Queue, HA Controller)
- Node.js API Server (Auth, Status, Metrics)
- Web Frontend (Login, Dashboard)

## Architecture
- Backend: C++17, multithreaded, file-based persistence
- API Server: Node.js + Express
- Frontend: Vite + JS
- HA: Leader-based single-node HA simulation

## Status
- Deployable for demo and controlled environments

## How to Run
See individual README files inside each folder.
