# Web roadmap

Open `/roadmap.html` in the Daywater web UI, or use **Roadmap** in the shared
toolbar. The page describes the route from the current emulator-dependent
prototype to an independent Windows game.

The reusable presentation is in `frontend/shared/roadmap.js` and
`frontend/shared/roadmap.css`. NFL Street facts are isolated in
`frontend/roadmaps/nfl-street.json`; no game addresses or gameplay policy are
added to Daywater's core. Static hosting means this page needs no new API,
dependency, or server restart with the development frontend mount.

`STATUS.md`, `PORTING_PLAN.md`, and the linked reconstruction notes remain the
detailed evidence sources. Update the JSON review date, evidence, remaining
work, and milestone status when a material milestone changes. Do not mark a
whole subsystem complete based on a bounded replay. Do not use a percentage
estimate, automatically advance milestones, or imply a worker is running
from this summary. Source paths are references to repository notes, not
links to private captures. Keep private data and build artifacts on W:.

The roadmap is intentionally a dated summary rather than a live activity
monitor. It gives the reader a stable view of what works, what does not yet
work, and the acceptance criteria for the next useful checkpoint.
