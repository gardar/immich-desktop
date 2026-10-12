# Bundled icon packs

Lucide is the default. Appearance settings can switch to Tabler or Bootstrap Icons.
All SVGs are bundled in the application; switching packs does not require a network connection.

- Lucide: https://github.com/lucide-icons/lucide — license in `LICENSE.txt`.
- Tabler: https://github.com/tabler/tabler-icons — MIT license in `tabler/LICENSE.txt`.
- Bootstrap Icons: https://github.com/twbs/icons — MIT license in `bootstrap/LICENSE.txt`.

Alternative packs use the same application resource names as Lucide. When upstream
names differ, the downloaded SVG is renamed to the application's semantic name.
For example, Tabler's `player-play` maps to `play.svg` and Bootstrap's `gear` maps
to `settings.svg`. Bootstrap's circle and triangle status icons represent the
cloud success and error states. Missing icons fall back to Lucide.

SVG assets were retrieved from these upstream repositories on 2026-10-13.
