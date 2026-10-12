# Codex backend setup

Daywater keeps Inspect as the default agent backend. The additional Codex
backend currently runs **static research with manual review**. It reuses the
task's prompt and exposes the task's permitted Daywater tools through a
task-scoped stdio MCP server. The Codex SDK runs in the Daywater container;
Inspect and its viewer remain available.

## Authenticate and start

1. On the Docker host, run `codex login` and `codex login status`. Use a
   ChatGPT login if you want to avoid an OpenAI API key. The file-based login
   must be present at `~/.codex/auth.json`. A keyring-only login cannot be
   mounted as a file.
2. Create the container's private Codex state directory and point Compose at
   the **absolute** host auth-file path:

   ```bash
   mkdir -p sessions/.codex
   chmod 700 sessions/.codex
   export DAYWATER_CODEX_AUTH_FILE="$HOME/.codex/auth.json"
   docker compose -f docker-compose.yml -f docker-compose.codex.yml up --build -d
   ```

   The optional overlay mounts only `auth.json`, read-only, at
   `/app/sessions/.codex/auth.json` and sets `CODEX_HOME` to that directory.
   Do not add the auth file to this repository. `sessions/` is Git-ignored.
3. In the setup wizard, choose **Codex / existing ChatGPT login** and test the
   connection. For an existing install, use **Settings → Add Codex (existing
   login)**, then activate that configuration. No API key is stored in the
   Codex model configuration. An empty model uses the SDK default.
4. Create a **Static research** task for a project with an uploaded ISO. For a
   trivial check, ask it to call `list_research()` and report what exists.
   The run succeeds only after a real Daywater MCP tool call and a final
   response. The task result shows that response, while the task event stream
   records tool names and Codex progress.

## Boundary and limits

Codex runs inside the same container as Ghidra and Dolphin, so its MCP
adapter can reuse Daywater's existing capability-gated tools and project
storage. A small dictionary-backed store replaces Inspect's per-sample store
for those tools in the MCP process. Codex thread state lives under
`sessions/.codex`; Daywater records the thread ID and final response in
the task config. The MCP process and Codex thread are scoped to one Daywater
run. There is no Daywater cancellation endpoint, so cancellation is not
exposed yet.

The mounted auth file is read-only. If Codex needs to refresh credentials on
disk, refresh the host login and restart the container; the read-only mount
cannot save a refreshed token. The host-side Codex process option would avoid
mounting auth but would need a networked Daywater tool bridge and a new
process lifecycle. A standalone network MCP server adds similar complexity.
The SDK plus local stdio MCP avoids a raw `codex exec` subprocess per
prompt and keeps tools inside the existing container.

Codex code-patch and runtime tasks are rejected until their scoring and
session lifecycles are verified on a live environment. A completed static
research run confirms agent/tool plumbing; it is not a verification of any
game finding. Existing Inspect configurations continue to use their
provider/model strings and API keys.
