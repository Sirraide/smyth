#ifndef SMYTH_UI_DICTIONARY_HH
#define SMYTH_UI_DICTIONARY_HH

// General concept:
//
// PREREQUISITES:
// User settings: global per-user settings that are *not* stored in the project
// file, e.g. the default font to use (so multiple people can use the same project
// file with different settings). Add an option to the settings to override the
// settings with the per-user settings.
//
// USE JSON INSTEAD.
//
// FEATURES:
//
// List view that presents entries.
//
// Separate saving logic so only rows that have changed are saved on every
// operation that changes a row.
//
// CTRL+C when an entry is selected to copy the word.
//
// Backspace/Delete to delete an entry. Always ask the user for confirmation
// before an entry is deleted. (TODO: Also support undoing deletions)
//
// Double-click an entry to *open* it. This shows the entry in a dialog
//     that contains a more detailed view of it (including e.g. all senses,
//     examples, etc.)
//
// Right-click an entry to open a menu that lets you:
//     - Delete an entry
//     - Edit an entry (this brings up another dialog)
//
// Button to add a new entry. This opens a dialog that, unlike the other
//     dialogs described above, is *NOT* modal, so people can still use
//     Smyth while adding an entry.
//
// Search bar at the top that lets you filter entries (by any column; use
//     a select menu for that); search uses NFKD on the search input; the
//     search also applies NFKD to the field being searched.
//
// Sort entries by NFKD of the headword.
//
// Export to LaTeX.

#endif // SMYTH_UI_DICTIONARY_HH
