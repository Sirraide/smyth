#include <Smyth/JSON.hh>
#include <UI/App.hh>
#include <UI/PersistObjects.hh>
#include <UI/Utils.hh>

using namespace smyth;
using namespace smyth::json_utils;
using namespace smyth::ui;
using namespace smyth::detail;

/// ====================================================================
///  Deserialiser
/// ====================================================================
auto Serialiser<QByteArray>::Deserialise(const json& j) -> Result<QByteArray> {
    using Status = QByteArray::Base64DecodingStatus;
    auto FormatError = [](Status status)  -> std::string_view {
        switch (status) {
            default: return "Unknown error";
            case Status::IllegalCharacter: return "Illegal character";
            case Status::IllegalPadding: return "Illegal padding";
            case Status::IllegalInputLength: return "Illegal input length";
        }
    };

    auto str = Try(Get<std::string>(j));
    auto res = QByteArray::fromBase64Encoding(
        QByteArray::fromStdString(str),
        QByteArray::AbortOnBase64DecodingErrors
    );

    if (res.decodingStatus != Status::Ok) return Error(
        "Failed to decode base64 QByteArray: {}",
        FormatError(res.decodingStatus)
    );

    return std::move(res.decoded);
}

auto Serialiser<QFont>::Deserialise(const json& tn) -> Result<QFont> {
    auto description = Try(Serialiser<QString>::Deserialise(tn));
    QFont f;
    if (not f.fromString(description)) return Error(
        "Invalid font record '{}' in save file",
        description
    );
    return f;
}

auto Serialiser<QSize>::Deserialise(const json& tn) -> Result<QSize> {
    const json::array_t& arr = Try(Get<json::array_t>(tn));
    if (arr.size() != 2) return Error("Expected array of size 2 when reading QSize");
    auto first = Try(Get<i64>(arr.at(0)));
    auto second = Try(Get<i64>(arr.at(1)));
    return QSize{int(first), int(second)};
}

auto Serialiser<QString>::Deserialise(const json& j) -> Result<QString> {
    auto text = Try(Serialiser<std::string>::Deserialise(j));
    return QString::fromUtf8(text.data(), qsizetype(text.size()));
}

/// ====================================================================
///  Serialiser
/// ====================================================================
auto Serialiser<QByteArray>::Serialise(const QByteArray& val) -> json {
    auto arr = val.toBase64();
    return Serialiser<std::string>::Serialise(arr.toStdString());
}

auto Serialiser<QFont>::Serialise(const QFont& val) -> json {
    return Serialiser<QString>::Serialise(val.toString());
}

auto Serialiser<QString>::Serialise(const QString& val) -> json {
    return Serialiser<std::string>::Serialise(val.toStdString());
}

auto Serialiser<QSize>::Serialise(QSize val) -> json {
    return {val.width(), val.height()};
}

/// ====================================================================
///  Persistence
/// ====================================================================
void ui::PersistCBox(PersistentStore& store, std::string key, QComboBox* cbox) {
    Persist<&QComboBox::currentIndex, &QComboBox::setCurrentIndex>(
        store,
        std::move(key),
        cbox
    );
}

void ui::PersistChBox(PersistentStore& store, std::string key, QCheckBox* cbox) {
    Persist<&QCheckBox::checkState, &QCheckBox::setCheckState>(
        store,
        std::move(key),
        cbox
    );
}

/// Like PersistCBox, but the entries are generated dynamically.
void ui::PersistDynCBox(PersistentStore& store, std::string key, QComboBox* cbox) {
    Persist<&QComboBox::currentText, [](QComboBox* cbox, QString str) {
        // If the item does not exists yet, add it.
        if (cbox->findText(str) == -1) cbox->addItem(str);
        cbox->setCurrentText(str);
    }>(store, std::move(key), cbox);
}
