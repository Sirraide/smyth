#include <UI/SmythPlainTextEdit.hh>
#include <UI/SmythRichTextEdit.hh>

import smyth.persistent;

using namespace smyth;
using namespace smyth::ui;

void SmythPlainTextEdit::persist(void* store, std::string_view key) {
    Persist<&This::toPlainText, &This::setPlainText>(
        *static_cast<PersistentStore*>(store),
        std::format("{}.text", key),
        this
    );
}

void SmythRichTextEdit::persist(void* store, std::string_view key, bool include_text) {
    if (include_text) Persist<&This::toHtml, &This::setHtml>(
        *static_cast<PersistentStore*>(store),
        std::format("{}.text", key),
        this
    );
}
