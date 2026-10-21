/*
 * Scenery Editor X manual: theme switch, contents drawer, search, "on this page" highlighting, copy buttons.
 * No dependencies; the search index is a script (assets/search-index.js) so the site also works from file://.
 */
(function () {
  "use strict";

  var root = document.documentElement;

  // ---- Light / dark -------------------------------------------------------------------------------------------------

  function effectiveTheme() {
    var chosen = root.getAttribute("data-theme");
    if (chosen) return chosen;
    return window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
  }

  document.querySelectorAll("[data-theme-toggle]").forEach(function (button) {
    button.addEventListener("click", function () {
      var next = effectiveTheme() === "dark" ? "light" : "dark";
      root.setAttribute("data-theme", next);
      try { localStorage.setItem("sedx-manual-theme", next); } catch (e) { /* Private mode: the choice lasts this page */ }
    });
  });

  // ---- Contents drawer (narrow screens) ---------------------------------------------------------------------------

  var body = document.body;
  document.querySelectorAll("[data-menu-toggle]").forEach(function (button) {
    button.addEventListener("click", function () { body.classList.toggle("nav-open"); });
  });
  var scrim = document.querySelector("[data-scrim]");
  if (scrim) scrim.addEventListener("click", function () { body.classList.remove("nav-open"); });
  document.querySelectorAll(".sidebar a").forEach(function (link) {
    link.addEventListener("click", function () { body.classList.remove("nav-open"); });
  });

  // Keep the current chapter in view in a long contents list
  var current = document.querySelector(".nav-part li.current");
  var sidebar = document.querySelector(".sidebar");
  if (current && sidebar) {
    var top = current.getBoundingClientRect().top - sidebar.getBoundingClientRect().top;
    if (top + 120 > sidebar.clientHeight) sidebar.scrollTop += top - 96;
  }

  // ---- Copy buttons on code blocks -----------------------------------------------------------------------------------

  document.querySelectorAll("pre.code").forEach(function (pre) {
    var button = document.createElement("button");
    button.type = "button";
    button.className = "copy-button";
    button.textContent = "Copy";
    button.addEventListener("click", function () {
      var text = pre.querySelector("code").innerText;
      var done = function () {
        button.textContent = "Copied";
        setTimeout(function () { button.textContent = "Copy"; }, 1400);
      };
      if (navigator.clipboard && navigator.clipboard.writeText) navigator.clipboard.writeText(text).then(done, function () {});
    });
    pre.appendChild(button);
  });

  // ---- Figures open full size on click -------------------------------------------------------------------------------

  document.querySelectorAll(".chapter-body figure img").forEach(function (img) {
    img.addEventListener("click", function () {
      var overlay = document.createElement("div");
      overlay.className = "lightbox";
      var large = document.createElement("img");
      large.src = img.src;
      large.alt = img.alt;
      overlay.appendChild(large);
      var close = function () { overlay.remove(); document.removeEventListener("keydown", onKey); };
      var onKey = function (event) { if (event.key === "Escape") close(); };
      overlay.addEventListener("click", close);
      document.addEventListener("keydown", onKey);
      document.body.appendChild(overlay);
    });
  });

  // ---- On this page: highlight the section being read ---------------------------------------------------------------

  var tocLinks = Array.prototype.slice.call(document.querySelectorAll("[data-toc-link], .nav-sections a"));
  if (tocLinks.length && "IntersectionObserver" in window) {
    var headings = Array.prototype.slice.call(document.querySelectorAll(".chapter-body h2[id], .chapter-body h3[id]"));
    var visible = {};
    var setActive = function (id) {
      tocLinks.forEach(function (link) { link.classList.toggle("active", link.getAttribute("href") === "#" + id); });
    };
    var observer = new IntersectionObserver(function (entries) {
      entries.forEach(function (entry) { visible[entry.target.id] = entry.isIntersecting; });
      for (var i = 0; i < headings.length; i++) {
        if (visible[headings[i].id]) { setActive(headings[i].id); return; }
      }
    }, { rootMargin: "-70px 0px -65% 0px" });
    headings.forEach(function (h) { observer.observe(h); });
  }

  // ---- Search ----------------------------------------------------------------------------------------------------

  var input = document.querySelector("[data-search]");
  var panel = document.querySelector("[data-search-results]");
  var index = window.SEDX_SEARCH || [];
  if (!input || !panel) return;

  var results = [];
  var selected = -1;

  function escapeHtml(text) {
    return text.replace(/[&<>"]/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]; });
  }

  function escapeRegExp(text) {
    return text.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  }

  function highlight(text, terms) {
    var html = escapeHtml(text);
    terms.forEach(function (term) {
      html = html.replace(new RegExp("(" + escapeRegExp(escapeHtml(term)) + ")", "ig"), "<mark>$1</mark>");
    });
    return html;
  }

  function snippet(text, terms) {
    var lower = text.toLowerCase();
    var at = -1;
    for (var i = 0; i < terms.length && at < 0; i++) at = lower.indexOf(terms[i]);
    if (at < 0) return text.slice(0, 150) + (text.length > 150 ? "…" : "");
    var start = Math.max(0, at - 60);
    var cut = text.slice(start, start + 170);
    return (start > 0 ? "…" : "") + cut + (start + 170 < text.length ? "…" : "");
  }

  function search(query) {
    var terms = query.toLowerCase().split(/\s+/).filter(function (t) { return t.length > 1 || /\d/.test(t); });
    if (!terms.length) return [];
    var scored = [];
    index.forEach(function (entry) {
      var heading = (entry.h || "").toLowerCase();
      var chapter = entry.c.toLowerCase();
      var text = entry.t.toLowerCase();
      var score = 0;
      for (var i = 0; i < terms.length; i++) {
        var term = terms[i];
        var inHeading = heading.indexOf(term) >= 0;
        var inChapter = chapter.indexOf(term) >= 0;
        var position = text.indexOf(term);
        if (!inHeading && !inChapter && position < 0) return;
        if (inHeading) score += heading.indexOf(term) === 0 ? 14 : 10;
        if (inChapter) score += 5;
        if (position >= 0) score += 2 + Math.max(0, 3 - position / 400) + Math.min(4, text.split(term).length - 1) * 0.5;
      }
      scored.push({ entry: entry, score: score });
    });
    scored.sort(function (a, b) { return b.score - a.score; });
    return scored.slice(0, 14).map(function (s) { return { entry: s.entry, terms: terms }; });
  }

  function render() {
    if (!input.value.trim()) { panel.hidden = true; return; }
    results = search(input.value.trim());
    selected = results.length ? 0 : -1;
    if (!results.length) {
      panel.innerHTML = '<div class="search-empty">Nothing found for “' + escapeHtml(input.value.trim()) + '”.</div>';
    } else {
      panel.innerHTML = results.map(function (r, i) {
        var e = r.entry;
        return '<a class="search-result' + (i === selected ? " active" : "") + '" href="' + e.u + '">' +
          "<small>" + escapeHtml(e.c) + "</small>" +
          "<strong>" + highlight(e.h || e.c.replace(/^[^ ]+ /, ""), r.terms) + "</strong>" +
          "<span>" + highlight(snippet(e.t, r.terms), r.terms) + "</span></a>";
      }).join("");
    }
    panel.hidden = false;
  }

  function move(delta) {
    var links = panel.querySelectorAll(".search-result");
    if (!links.length) return;
    selected = (selected + delta + links.length) % links.length;
    links.forEach(function (link, i) { link.classList.toggle("active", i === selected); });
    links[selected].scrollIntoView({ block: "nearest" });
  }

  var timer = null;
  input.addEventListener("input", function () {
    clearTimeout(timer);
    timer = setTimeout(render, 80);
  });
  input.addEventListener("focus", function () { if (input.value.trim()) render(); });
  input.addEventListener("keydown", function (event) {
    if (event.key === "ArrowDown") { event.preventDefault(); move(1); }
    else if (event.key === "ArrowUp") { event.preventDefault(); move(-1); }
    else if (event.key === "Enter") {
      var link = panel.querySelectorAll(".search-result")[selected];
      if (link) window.location.href = link.getAttribute("href");
    } else if (event.key === "Escape") { input.value = ""; panel.hidden = true; input.blur(); }
  });
  document.addEventListener("click", function (event) {
    if (!event.target.closest(".search")) panel.hidden = true;
  });
  document.addEventListener("keydown", function (event) {
    var typing = /^(INPUT|TEXTAREA|SELECT)$/.test(document.activeElement.tagName);
    if (event.key === "/" && !typing) { event.preventDefault(); input.focus(); input.select(); }
  });
})();
