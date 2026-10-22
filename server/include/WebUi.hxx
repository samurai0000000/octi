/*
 * WebUi.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_SERVER_WEB_UI_HXX
#define OCTI_SERVER_WEB_UI_HXX

namespace octi {

/*
 * Self-contained single page (HTML + CSS + JavaScript) served at GET /: Web
 * Archive, Leaderboard and Live Spectator. It talks only to the same
 * origin's /api/v1 endpoints, loads no external resource and sends no
 * telemetry. All dynamic text is inserted with textContent.
 */
const char *webUiHtml();

} // namespace octi

#endif // OCTI_SERVER_WEB_UI_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
