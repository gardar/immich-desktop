# Security policy

## Supported versions

Security fixes are applied to the current `main` branch and the latest stable
release of Immich Desktop. Older releases are generally not patched separately;
please upgrade when a fixed release is available.

| Version | Supported |
| --- | --- |
| Latest stable release | Yes |
| Rolling [beta](https://github.com/hdmain/immich-desktop/releases/tag/beta) | Best effort |
| Older releases | No |

## Reporting a vulnerability

Please **do not** open a public GitHub issue for security vulnerabilities.

Prefer one of these private channels:

1. **GitHub Security Advisories** - [Report a vulnerability](https://github.com/hdmain/immich-desktop/security/advisories/new)
2. Contact the maintainer privately: [@hdmain](https://github.com/hdmain)

Include as much detail as you can:

- Affected version or commit
- Impact (e.g. credential leak, unexpected network behavior, local file access)
- Steps to reproduce or a proof of concept
- Whether you plan to disclose publicly and on what timeline

## What to expect

- We aim to acknowledge reports within a few days.
- We will discuss severity, a fix plan, and coordinated disclosure when appropriate.
- Credit is welcome unless you ask to remain anonymous.

## Scope notes

Immich Desktop talks to **your** Immich server and related URLs you configure.
Reports about the official Immich server itself should go to the
[Immich project](https://github.com/immich-app/immich/security), not this repo.
Client bugs that mishandle credentials, certificates, custom headers, or local
caches in this app are in scope here.
