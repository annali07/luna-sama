/*

Items:

“Change Mode ▸” → submenu of modes (radio-checked)

Separator

“Close”

(future) “Settings…”, “Auto-start on login”, etc.

On “Close”: QCoreApplication::quit().

Attach it by reimplementing CharacterView::mousePressEvent(QMouseEvent*), detect Qt::RightButton, menu->exec(QCursor::pos()).

*/