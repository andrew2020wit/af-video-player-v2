#include "KeyMapping.h"

#include <QChar>
#include <QHash>
#include <Qt>

namespace afvp {

namespace {

QHash<char16_t, int> buildCyrillicKeys() {
    // Lower-case letters of the Russian and Ukrainian layouts -> Latin key at the same position.
    const QHash<char16_t, int> keys = {
        {u'й', Qt::Key_Q},          {u'ц', Qt::Key_W},          {u'у', Qt::Key_E},
        {u'к', Qt::Key_R},          {u'е', Qt::Key_T},          {u'н', Qt::Key_Y},
        {u'г', Qt::Key_U},          {u'ш', Qt::Key_I},          {u'щ', Qt::Key_O},
        {u'з', Qt::Key_P},          {u'х', Qt::Key_BracketLeft}, {u'ъ', Qt::Key_BracketRight},
        {u'ї', Qt::Key_BracketRight}, {u'ф', Qt::Key_A},        {u'ы', Qt::Key_S},
        {u'і', Qt::Key_S},          {u'в', Qt::Key_D},          {u'а', Qt::Key_F},
        {u'п', Qt::Key_G},          {u'р', Qt::Key_H},          {u'о', Qt::Key_J},
        {u'л', Qt::Key_K},          {u'д', Qt::Key_L},          {u'ж', Qt::Key_Semicolon},
        {u'э', Qt::Key_Apostrophe}, {u'є', Qt::Key_Apostrophe}, {u'я', Qt::Key_Z},
        {u'ч', Qt::Key_X},          {u'с', Qt::Key_C},          {u'м', Qt::Key_V},
        {u'и', Qt::Key_B},          {u'т', Qt::Key_N},          {u'ь', Qt::Key_M},
        {u'б', Qt::Key_Comma},      {u'ю', Qt::Key_Period},     {u'ё', Qt::Key_QuoteLeft},
    };

    return keys;
}

} // namespace

int physicalKey(int key) {
    static const QHash<char16_t, int> cyrillicKeys = buildCyrillicKeys();

    if (key < 0x0400 || key > 0x04FF) {
        return key;
    }

    const char16_t letter = QChar(char16_t(key)).toLower().unicode();

    return cyrillicKeys.value(letter, key);
}

} // namespace afvp
