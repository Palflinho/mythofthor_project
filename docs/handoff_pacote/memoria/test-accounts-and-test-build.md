---
name: test-accounts-and-test-build
description: which game account Claude may use in automated tests, and how the user's separate test build works
metadata:
  type: feedback
---

Never run automated tests (MythLogin etc.) on the user's `mythtest` account: on 2026-10-10 the tests changed ThorTester's
class (@job 3), HP and look, and it had to be restored by SQL. Use `mythclaude` / `mythclaude123` (char ClaudeTester,
`server/init_claude_account.sql`): `MythDelayed 3 MythLogin+mythclaude+mythclaude123`.

**Why:** the user plays and tests on mythtest; my @commands and deaths leave it in a bad state, and two logins of the
same account kick each other.

**How to apply:** the user also asked (2026-10-10) for a frozen test version outside the dev project:
`tools/make_test_build.ps1` copies the compiled game to `E:\Dev\MythTeste`; refresh it after each batch of finished
features and update `docs/TESTE_JOGADOR.md`. `build_watch.ps1` must not kill that window. See [[mythofthor-dev-environment]].
